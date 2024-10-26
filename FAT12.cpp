#include "FAT12.h"
#include "dir.h"

Disk disk;

FILE *diskFile = fopen(PATH, "rb");

/**
 * 读取镜像的FAT到本地磁盘
 */
void read_fat_from_vfd(char *vfd_file) {
    fseek(diskFile, 512, SEEK_SET);
    fread(&disk.FAT1, sizeof(FATEntry), 1536, diskFile);
}

/**
 * 读取镜像的mbr到本地磁盘
 */
void read_mbr_from_vfd(char *vfd_file) {
    //一个MBR也就是512字节 申请512字节空间
    MBRHeader *mbr = (MBRHeader *) malloc(sizeof(MBRHeader));
    fseek(diskFile, 0, SEEK_SET);
    uint32_t readed_size = fread(mbr, 1, SECTOR_SIZE, diskFile);
    if (readed_size != SECTOR_SIZE) {
        perror("文件读取错误,请检查是否路径正确");
        free(mbr);
        return;
    }
    disk.MBR = *mbr;
    uint32_t rootDirStartSec = 1 + (disk.MBR.BPB_NumFATs * disk.MBR.BPB_FATSz16); // 1 MBR，2 是 FAT 表的扇区数
    fseek(diskFile, rootDirStartSec * SECTOR_SIZE, SEEK_SET); //移动到根目录区了
    uint16_t read_size = fread(disk.rootDirectory, sizeof(RootEntry), disk.MBR.BPB_RootEntCnt, diskFile);
    if (read_size != 224) {  //本地写死 老师提供的磁盘为112
        perror("读取根目录失败，请检查文件");
        fclose(diskFile);
        return;
    }
    free(mbr);
}

/**
 * 查找文件Entry块
 */
RootEntry *findFile(string &fileName) {
    if (fileName.empty()) {
        cout << "待查找为空";
        return NULL;
    }
    uint16_t dotPosition = fileName.find('.');  //用.分割前后
    string baseName = (dotPosition == string::npos) ? fileName : fileName.substr(0, dotPosition);
    string extension = (dotPosition == string::npos) ? "" : fileName.substr(dotPosition + 1);

    for (int i = 0; i < disk.MBR.BPB_RootEntCnt; i++) {
        // 跳过空条目
        if (disk.rootDirectory[i].DIR_Name[0] == 0) continue;

        // 从目录条目中提取文件名和扩展名
        char _name[9] = "";
        char _ext[4] = "";

        memcpy(_name, disk.rootDirectory[i].DIR_Name, 8);
        memcpy(_ext, disk.rootDirectory[i].DIR_Name + 8, 3);

        // 创建标准化的文件名和扩展名
        string origin_file_name = string(_name);
        string origin_file_ext = string(_ext);

        //去除空格
        origin_file_name = origin_file_name.substr(0, origin_file_name.find_last_not_of(' ') + 1);
        origin_file_ext = origin_file_ext.substr(0, origin_file_name.find_last_not_of(' ') + 1);

        //比较文件名和扩展名，忽略大小写
        bool isNameMatch = strcasecmp(origin_file_name.c_str(), baseName.c_str()) == 0;
        bool isExtMatch = strcasecmp(origin_file_ext.c_str(), extension.c_str()) == 0;

        if (isNameMatch && isExtMatch) {
            return &disk.rootDirectory[i]; // 返回找到的文件条目
        }
    }
    return NULL;
}

/**
 * 从给定的缓冲区获取下一个簇号
 * @param buffer 指向存储 FAT 数据的缓冲区
 * @param flag 指示位，用于决定如何读取簇号
 * @return 返回下一个簇号
 */
unsigned short getClus(unsigned char *buffer, char flag) {
    unsigned short ans = 0;
    if (!flag) {
        ans += buffer[0];
        ans += (buffer[1] << 8) & 0xFFF; // 只取低12位
    } else {
        ans += buffer[1] << 4;
        ans += buffer[0] >> 4; // 取高4位
    }
    return ans;
}

/**
 * 读取文件数据
 * @param firstCluster 第一个簇号
 * @param fileSize 文件大小
 */
void readFileData(uint16_t firstCluster, uint32_t fileSize) {
    uint8_t sectorsPerCluster = disk.MBR.BPB_SecPerClus; // 每个簇的扇区数
    uint32_t fatSize = disk.MBR.BPB_FATSz16; // FAT 的大小
    uint32_t fatStartSector = 1; // FAT 起始扇区
    uint32_t dataStartSector = fatStartSector + (disk.MBR.BPB_NumFATs * fatSize) +
                               (disk.MBR.BPB_RootEntCnt * sizeof(RootEntry) / disk.MBR.BPB_BytesPerSec);

    uint16_t currentCluster = firstCluster;
    uint32_t bytesRead = 0;

    while (bytesRead < fileSize) {
        // 计算当前簇对应的起始扇区
        uint32_t clusterSector = dataStartSector + (currentCluster - 2) * sectorsPerCluster;
        // 用于存储读取的字节
        uint8_t buffer[SECTOR_SIZE];
        // 逐扇区读取数据
        for (uint8_t sector = 0; sector < sectorsPerCluster; ++sector) {
            if (bytesRead >= fileSize) break; // 如果已读取完毕，退出循环

            FILE *boot = fopen(PATH, "rb");
            // 读取当前扇区
            fseek(boot, (clusterSector + sector) * SECTOR_SIZE, SEEK_SET);
            size_t readSize = fread(buffer, 1, SECTOR_SIZE, boot);
            fclose(boot);

            // 输出读取的数据
            for (size_t i = 0; i < readSize && bytesRead < fileSize; i++, bytesRead++) {
                putchar(buffer[i]); // 输出字符到控制台
            }
        }

        unsigned short nextClusNum = getClus(&disk.FAT1[currentCluster / 2].data[currentCluster % 2],
                                             currentCluster % 2);
        currentCluster = nextClusNum;

        // 如果获取的下一个簇是 ff8 ~ fff，结束读取
        if (currentCluster >= 0x0FF8) { // 对于 FAT12，0x0FF8 及以上的簇表示 EOF
            break;
        }
    }

    // 换行以结束输出
    cout << endl;
}

/**
 * 进入文件夹
 * @param _name
 */
void cd(string &_name) {
    string fileFullName = _name.substr(3); // 从 "cat " 后开始
    toLowerCase(fileFullName);
    for (uint16_t i = 0; i < disk.MBR.BPB_RootEntCnt; i++) {
        if (disk.rootDirectory[i].DIR_Name[0] == 0) continue;
        uint8_t file_name[9];
        memcpy(file_name, disk.rootDirectory[i].DIR_Name, 8);
        file_name[8] = '\0';
        std::string actual_name(reinterpret_cast<char *>(file_name));
        actual_name = actual_name.substr(0, actual_name.find_last_not_of(' ') + 1);
        toLowerCase(actual_name);

        if (fileFullName == "/") {
            fseek(diskFile, 19 * 512, SEEK_SET);
            fread(disk.rootDirectory, 1, SECTOR_SIZE, diskFile);
            cout << "已返回根目录" << endl;
            return;
        }

        if (fileFullName == ".") { // 当前目录
            cout << "您正处于当前目录" << endl;
            return;
        }

        if (fileFullName == "..") { // 返回上一级目录
            cout << "TODO已返回上一级目录" << endl;
            return;
        }


        if (fileFullName == actual_name && disk.rootDirectory[i].DIR_Attr & 0x10) {
//            cout << "找到，起始簇号是" << disk.rootDirectory[i].DIR_FstClus << endl;
            uint16_t offset = (33 + (disk.rootDirectory[i].DIR_FstClus - 2)) * 512;
//            cout << "offset是" << offset << endl;
            fseek(diskFile, offset, SEEK_SET);
            fread(disk.rootDirectory, 1, SECTOR_SIZE, diskFile);
            return;
        }
    }
    cout << "未找到" << endl;
}

/**
 * 实现命令汇总
 * @param command
 */
void executeCommand(string &command) {
    if (command == "dir") {
        dir(); // 显示根目录的结构
    } else if (command == "exit") {
        cout << "退出程序..." << endl;
        exit(0);
    } else if (command.substr(0, 4) == "cat ") { // 必须是 cat空格 命令开头
        cat(command); // 调用 cat 函数
    } else if (command.substr(0, 3) == "cd ") { // 必须是 cd空格 命令开头
        cd(command); // 调用 cd 函数
    } else {
        cout << "未知命令: " << command << endl;
    }
}

/**
 * 查看文件内容
 * @param _name
 */
void cat(string &_name) {
    // 提取文件名
    string fileFullName = _name.substr(4); // 从 "cat " 后开始
    // 查找文件条目
    RootEntry *fileEntry = findFile(fileFullName);
    if (fileEntry) {
        // 如果找到文件条目，输出找到的文件信息
//        cout << "起始簇号" << fileEntry->DIR_FstClus << "号簇" << endl;
//        cout << "文件大小: " << fileEntry->DIR_FileSize << " 字节" << endl;
        uint16_t firstCluster = fileEntry->DIR_FstClus;
        readFileData(firstCluster, fileEntry->DIR_FileSize);
    } else {
        cout << "文件未找到: " << fileFullName << endl;
    }
}

int main() {
    showCommandList();
    read_mbr_from_vfd(PATH);
    read_fat_from_vfd(PATH);
    while (true) {
        cout << "A>:";
        getline(cin, command);
        executeCommand(command);
    }
}