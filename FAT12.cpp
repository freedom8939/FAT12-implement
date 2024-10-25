#include "FAT12.h"
using namespace std;
#define PATH "G:\\OS-homework\\grp07\\wjyImg.vfd"
Disk disk;

//函数声明（少部分）
RootEntry *findFile(const string &fileName);

void cat(string &_name);

void executeCommand(string &command);
/**
 * 初始化MBR
 * @return
 */
//初始化MBR.
void InitMBR(Disk *disk) {
    memset(disk->MBR.BS_jmpBOOT, 3, 0);
    memcpy(disk->MBR.BS_OEMName, "LNNU WJY", 8);
    disk->MBR.BPB_BytesPerSec = 512;
    disk->MBR.BPB_SecPerClus = 1;
    disk->MBR.BPB_ResvdSecCnt = 1;
    disk->MBR.BPB_NumFATs = 2;
    disk->MBR.BPB_RootEntCnt = ROOT_DICT_NUM;
    disk->MBR.BPB_TotSec16 = 2880;
    disk->MBR.BPB_Media = 0xf0;
    disk->MBR.BPB_FATSz16 = FAT_SECTOR_NUM;
    disk->MBR.BPB_SecPerTrk = 0x12;
    disk->MBR.BPB_NumHeads = 0x2;
    disk->MBR.BPB_HiddSec = 0;
    disk->MBR.BPB_TotSec32 = 0;
    disk->MBR.BS_DrvNum = 0;
    disk->MBR.BS_Reserved1 = 0;
    disk->MBR.BS_BootSig = 0x29;
    disk->MBR.BS_VollD = 0;
    memcpy(disk->MBR.BS_VolLab, "C", 1);
    memcpy(disk->MBR.BS_FileSysType, "FAT12", 5);
    memset(disk->MBR.code, 0, 448);
    disk->MBR.end_point[0] = 0x55;
    disk->MBR.end_point[1] = 0xAA;
}

/**
 * 初始化所有FAT表项
 */
void InitFAT() {
    memset(disk.FAT1, 0x00, sizeof(disk.FAT1));
    memset(disk.FAT2, 0x00, sizeof(disk.FAT2));

    // 初始化第一个 FAT 表条目
    disk.FAT1[0].firstEntry = 0xFF0;  // 第一个条目表示介质类型（0xF0表示软盘）
    disk.FAT1[0].secondEntry = 0xFFF; // 第二个条目表示结束标记

    // 将 FAT1 复制到 FAT2
    memcpy(disk.FAT2, disk.FAT1, sizeof(disk.FAT1));

    // 初始化其他 FAT 表条目（根据需要设置）
    for (int i = 1; i < sizeof(disk.FAT1) / sizeof(FATEntry); i++) {
        disk.FAT1[i].firstEntry = 0x000;  // 未使用的簇
        disk.FAT1[i].secondEntry = 0x000; // 未使用的簇
    }

    // 复制初始化后的 FAT1 到 FAT2
    memcpy(disk.FAT2, disk.FAT1, sizeof(disk.FAT1));
}

/**
 * 打印所有MBR的内容
 * @param temp
 */
void print_MBR(MBRHeader *temp) {
    printf("Disk Name - 磁盘名称: %s\n", temp->BS_OEMName);
    printf("Sector Size - 扇区大小: %d\n", temp->BPB_BytesPerSec);
    printf("Sectors per cluster - 每簇扇区数: %d\n", temp->BPB_SecPerClus);
    printf("Number of sectors occupied by boot - 引导占用的扇区数: %d\n", temp->BPB_ResvdSecCnt);
    printf("Number of FATs - FAT 表数量: %d\n", temp->BPB_NumFATs);
    printf("The max capacity of root entry - 根目录最大条目数: %d\n", temp->BPB_RootEntCnt);
    printf("The number of all Sectors - 扇区总数: %d\n", temp->BPB_TotSec16);
// printf("Media Descriptor - 媒体描述符: %c\n", temp->BPB_Media);
    printf("The number of Sectors of each Fat table - 每个 FAT 表占用的扇区数: %d\n", temp->BPB_FATSz16);
    printf("Number of sectors per track - 每个磁道的扇区数: %d\n", temp->BPB_SecPerTrk);
    printf("The number of magnetic read head - 磁头数量: %d\n", temp->BPB_NumHeads);
    printf("Number of hidden sectors - 隐藏扇区数: %d\n", temp->BPB_HiddSec);
    printf("Volume serial number - 卷序列号: %d\n", temp->BS_VollD);
    printf("Volume label - 卷标: ");
    for (int i = 0; i < 11; i++) {
        printf("%c", temp->BS_VolLab[i]);
    }
    printf("\n");
    printf("File system type - 文件系统类型: ");
    for (int i = 0; i < 8; i++) {
        printf("%c", temp->BS_FileSysType[i]);
    }
    printf("\n");

}

/**
 * 读取镜像的mbr文件列表并且展示在控制台
 */
void read_mbr_from_vfd(char *vfd_file) {
    //读取vfd
    FILE *boot = fopen(vfd_file, "rb");

    //一个MBR也就是512字节 申请512字节空间
    //读取mbr
    MBRHeader *mbr = (MBRHeader *) malloc(sizeof(MBRHeader));
    size_t readed_size = fread(mbr, 1, SECTOR_SIZE, boot);
    if (readed_size != SECTOR_SIZE) {
        perror("文件读取错误,请检查是否路径正确");
        free(mbr);
        return;
    }
    disk.MBR = *mbr;

//    print_MBR(&disk.MBR);
    uint32_t rootDirStartSec = 1 + (disk.MBR.BPB_NumFATs * disk.MBR.BPB_FATSz16); // 1 MBR，2 是 FAT 表的扇区数
//    printf("每个FAT表占%d个扇区", disk.MBR.BPB_FATSz16);
    fseek(boot, rootDirStartSec * SECTOR_SIZE, SEEK_SET); //移动到根目录区了
    uint16_t read_size = fread(disk.rootDirectory, sizeof(RootEntry), disk.MBR.BPB_RootEntCnt, boot);

    if (read_size != 224) {  //本地写死 老师提供的磁盘为112
        perror("读取根目录失败，请检查文件");
        fclose(boot);
        return;
    }

    free(mbr);
    fclose(boot);
}

/**
 * dir命令
 */
void dir() {
    for (int i = 0; i < disk.MBR.BPB_RootEntCnt; i++) {
        if (disk.rootDirectory[i].DIR_Name[0] == 0) {
            continue; // 空条目，跳过
        }

        // 获取文件大小
        uint32_t fileSize = disk.rootDirectory[i].DIR_FileSize;

        char name[9] = "";
        char ext[4] = "";

        memcpy(name, disk.rootDirectory[i].DIR_Name, 8);
        memcpy(ext, disk.rootDirectory[i].DIR_Name + 8, 3);

        string fileName = name;
        string extension = ext;
        //去掉空格
        fileName = fileName.substr(0, fileName.find_last_not_of(' ') + 1);
        extension = extension.substr(0, fileName.find_last_not_of(' ') + 1);
        //进行与操作
        /*
         * hex   0001 0000               0010 0000
         *     & 0001 0000             & 0001 0000
         *       0001 0000 所以是目录      0000 0000 等于0说明肯定不是目录 应该是普通文件所以不做操作
         */
        // 获取最后一次写入时间和日期 (组成16位)
        uint16_t lastWriteTime = (disk.rootDirectory[i].DIR_WrtTime[1] << 8) | disk.rootDirectory[i].DIR_WrtTime[0];
        uint16_t lastWriteDate = (disk.rootDirectory[i].DIR_WrtDate[1] << 8) | disk.rootDirectory[i].DIR_WrtDate[0];
//
//        // 解析最后一次写入时间
        int hours = (lastWriteTime >> 11) & 0x1F; // 获取小时
        int minutes = (lastWriteTime >> 5) & 0x3F; // 获取分钟
//        // 解析最后一次写入日期
        int year = ((lastWriteDate >> 9) & 0x7F) + 1980; // 获取年份
        int month = (lastWriteDate >> 5) & 0x0F; // 获取月份
        int day = lastWriteDate & 0x1F; // 获取日期
        /*
         *  dir格式
         * 2023/07/26  18:30    <DIR>          IdeaSnapshots
            2024/06/30  18:02           258,682 java_error_in_clion64_2720.log
         */
        bool isDirectory = (disk.rootDirectory[i].DIR_Attr & 0x10) != 0;
        if (isDirectory) {
            cout << year << "/" << month << "/" << day << setw(7) << hours << ":" << minutes << setw(10) << "<dir>"
                 << setw(20) << fileName << endl;
        } else {
            cout << year << "/" << month << "/" << day << setw(7) << hours << ":" << minutes << setw(11) << "<file>"
                 << setw(6) << fileSize << " B" << setw(10) <<
                 fileName << "." << extension << endl;
        }
    }
}
/**
 * 获取 FAT12 中下一个簇号
 * @param currentCluster 当前簇号
 * @return 下一个簇号
 */
uint16_t getNextCluster(uint16_t currentCluster) {
    uint32_t fatOffset = (currentCluster * 3) / 2;
    uint16_t nextCluster;

    if (currentCluster % 2 == 0) {
        // 偶数簇号：从FAT表中获取低 12 位
        nextCluster = (*(uint16_t*)&disk.FAT1[fatOffset]) & 0x0FFF;
    } else {
        // 奇数簇号：从FAT表中获取高 12 位
        nextCluster = (*(uint16_t*)&disk.FAT1[fatOffset] >> 4) & 0x0FFF;
    }

    return nextCluster;
}

/**
 * 查找文件Entry块
 * @param command
 */
RootEntry *findFile(const string &fileName) {
    if (fileName == "") {
        cout << "待查找为空";
        return NULL;
    }
    //
    size_t dotPosition = fileName.find('.');
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
        // 比较文件名和扩展名，忽略大小写
        bool isNameMatch = strcasecmp(origin_file_name.c_str(), baseName.c_str()) == 0;
        bool isExtMatch = strcasecmp(origin_file_ext.c_str(), extension.c_str()) == 0;

        if (isNameMatch && isExtMatch) {
            return &disk.rootDirectory[i]; // 返回找到的文件条目
        }
    }
    return NULL;
}

/**
 * 文件的第一个簇号
 * @param firstCluster  第一个簇号
 * @param size   读取的大小 （bug）
 */
void readFileData(uint16_t firstCluster, uint32_t fileSize) {
    uint32_t clusterSize = disk.MBR.BPB_SecPerClus * disk.MBR.BPB_BytesPerSec;
    uint32_t firstDataSector = 1 + (disk.MBR.BPB_NumFATs * disk.MBR.BPB_FATSz16) +
                               (disk.MBR.BPB_RootEntCnt * sizeof(RootEntry) / disk.MBR.BPB_BytesPerSec);

    uint16_t currentCluster = firstCluster;
    uint32_t bytesReadTotal = 0;

    FILE *diskImage = fopen(PATH, "rb");
    if (!diskImage) {
        printf("无法打开磁盘");
        exit(0);
    }

    char buffer[512] = {0};

    while (bytesReadTotal < fileSize && currentCluster < 0xFF8) { // 0xFF8表示结束
        // 计算当前簇的偏移
        uint32_t clusterOffset = (firstDataSector + (currentCluster - 2) * disk.MBR.BPB_SecPerClus) * disk.MBR.BPB_BytesPerSec;

        // 打印调试信息
        printf("读取簇: %u, 偏移: %u\n", currentCluster, clusterOffset);

        // 设置文件指针
        if (fseek(diskImage, clusterOffset, SEEK_SET) != 0) {
            perror("设置文件指针时出错");
            break;
        }

        // 计算要读取的字节数
        uint32_t bytesToRead = (fileSize - bytesReadTotal < clusterSize) ? fileSize - bytesReadTotal : clusterSize;
        size_t bytesRead = fread(buffer, sizeof(char), bytesToRead, diskImage);

        if (bytesRead == 0) {
            perror("读取数据时出错");
            break;
        }

        // 打印读取的数据
        for (uint32_t i = 0; i < bytesRead; i++) {
            if (isprint(buffer[i])) {
                printf("%c", buffer[i]);
            } else {
                printf(".");
            }
            if ((i + 1) % 64 == 0) {
                printf("\n");
            }
        }

        bytesReadTotal += bytesRead;

        // 获取下一个簇号
        currentCluster = getNextCluster(currentCluster);

        if (currentCluster >= 0xFF8) {
            break; // 结束
        }
    }

    printf("\n读取完成，共读取 %u 字节\n", bytesReadTotal);
    fclose(diskImage);
}


void cat(string &_name) {
    // 提取文件名
    string fileFullName = _name.substr(4); // 从 "cat " 后开始
    // 查找文件条目
    RootEntry *fileEntry = findFile(fileFullName);
    if (fileEntry) {

        // 如果找到文件条目，输出找到的文件信息
//        cout << "起始簇号" << fileEntry->DIR_FstClus << "号簇" << endl;
//        cout << "文件大小: " << fileEntry->DIR_FileSize << " 字节" << endl;
        // 从目录项中获取起始簇号
        uint16_t firstCluster = fileEntry->DIR_FstClus;
        //一次读一个扇区
//        readFileData(firstCluster,fileEntry->DIR_FileSize-2);
    } else {
        cout << "文件未找到: " << fileFullName << endl;
    }
}

void executeCommand(string &command) {

    if (command == "dir") {
        dir(); // 显示根目录的结构
    } else if (command == "exit") {
        cout << "退出程序..." << endl;
        exit(0);
    } else if (command.substr(0, 4) == "cat ") { // 必须是 cat空格 命令开头
        cat(command); // 调用 cat 函数
    } else {
        cout << "未知命令: " << command << endl;
    }
}



int main() {
    string command;
    cout << "可使用命令:" << endl;
    cout << "(1):dir" << endl;
    cout << "(2):cat 文件名" << endl;
    cout << "(3):exit" << endl;
//    InitMBR(&disk);  //初始化磁盘MBR信息 mock的本地数据
    read_mbr_from_vfd(PATH);
//    InitFAT();  //初始化磁盘fat mock本地数据使用
    while (1) {
        cout << ">:";
        getline(cin, command);
        executeCommand(command);
    }
}