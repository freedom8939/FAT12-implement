#include "FAT12.h"
#include "dir.h"

Disk disk;

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
//        cout << "---" << origin_file_name << "-" <<origin_file_ext <<endl;
        //去除空格
        origin_file_name = origin_file_name.substr(0, origin_file_name.find_last_not_of(' ') + 1);
        origin_file_ext = origin_file_ext.substr(0, origin_file_name.find_last_not_of(' ') + 3);

//        cout << "---" << origin_file_name << "-" <<origin_file_ext <<endl;


        //比较文件名和扩展名，忽略大小写
        bool isNameMatch = strcasecmp(origin_file_name.c_str(), baseName.c_str()) == 0;
        bool isExtMatch = strcasecmp(origin_file_ext.c_str(), extension.c_str()) == 0;

        if (isNameMatch && isExtMatch) {
            return &disk.rootDirectory[i]; // 返回找到的文件条目
        }
    }
    return nullptr;
}

/**
 * 从给定的缓冲区获取下一个簇号
 * @param buffer 指向存储 FAT 数据的缓冲区
 * @param flag 指示位，用于决定如何读取簇号
 * @return 返回下一个簇号
 */
unsigned short getClus(unsigned char *buffer, char flag) {
    uint16_t ans = 0;

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

            FILE *boot = fopen(PATH, "rb+");
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
        currentCluster = nextClusNum + 1;
        cout << "读取的簇是" << currentCluster << endl;
        // 如果获取的下一个簇是 ff8 ~ fff，结束读取
        if (currentCluster >= 0xFF8) { // 对于 FAT12，0x0FF8 及以上的簇表示 EOF
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
    string fileFullName = _name.substr(3); // 从 "cd " 后开始
    toLowerCase(fileFullName);

    if (fileFullName == ".") {
        cout << "已在当前目录" << endl;
        return;
    }
    if (fileFullName == "/") {
        go_back_to_directory_position();
        //清空路径栈
        while (!clusterStack.empty() && clusterStack.top() != 2) {
            clusterStack.pop();
        }
        while (!pathStack.empty() && pathStack.top() != "/") {
            pathStack.pop();
        }
        return;
    }
    if (fileFullName == "..") {

        //如果此时栈内元素只有一个元素2代表已经在顶级目录 无法返回
        if (clusterStack.top() == 2 || pathStack.top() == "/") {     //直接回到主目录
            go_back_to_directory_position();

            return;
        }
        //栈空不允许弹出
        if (clusterStack.empty() || pathStack.empty()) {
            return;
        }


        //栈内有多种个元素的情况：

        /**
         *
               9出栈 然后返回簇为8 并且 更新 now_clus =8
              8出栈  然后返回簇为6 更新now_clus = 6
             6出栈 返回2 now_clus =2
           2 == 2 拦截
         */

        string tempPathName = pathStack.top(); //栈顶路径
        clusterStack.pop();
        pathStack.pop();
        now_clu = clusterStack.top();
        if (now_clu == 2) {
            go_back_to_directory_position();
            return;
        }
        navigator_to_specific_directory_position(now_clu);
        return;
    }

    for (uint16_t i = 0; i < disk.MBR.BPB_RootEntCnt; i++) {
        if (disk.rootDirectory[i].DIR_Name[0] == 0) continue; //跳过空目录项

        //文件名整理以便比较
        char file_name[9];
        //把前8位给file_name
        memcpy(file_name, disk.rootDirectory[i].DIR_Name, 8);
        //转为字符串方便操作
        string actual_name = file_name;
        actual_name = actual_name.substr(0, actual_name.find_last_not_of(' ') + 1);
        toLowerCase(actual_name);

        if (fileFullName == actual_name && (disk.rootDirectory[i].DIR_Attr & 0x10)) { //如果是目录
            uint16_t cluster = disk.rootDirectory[i].DIR_FstClus;
            //把其簇入栈
            clusterStack.push(cluster);
            now_clu = cluster;
            if (pathStack.top() != "/") {
                pathStack.push("/" + actual_name);
            } else {
                pathStack.push(actual_name);
            }

            //替换当前disk.rootDirctory目录为指定
            navigator_to_specific_directory_position(cluster);
            cout << "当前目录：" << actual_name << endl;
            return;
        }
    }
    cout << "未找到目录: " << fileFullName << endl;
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
    } else if (command == "pwd") {
        pwd();
    } else if (command.substr(0, 6) == "mkdir ") {
        mkdir(command);
    } else if (command.substr(0, 3) == "cd ") { // 必须是 cd空格 命令开头
        cd(command); // 调用 cd 函数
    } else if (command == "te") { //过程测试（目录测试）
        te(clusterStack);
    } else if (command.substr(0, 6) == "touch ") {
        touch(command);
    } else if (command == "gc") { //当前所处的簇号
        cout << (uint32_t) getNowClu() << endl;
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

/**
 * 设置Entru的时间
 * @param entry
 */
void setTime(RootEntry &entry) {
    time_t currentTime = time(NULL);
    tm *localTime = localtime(&currentTime);

    // 获取并构造最后一次写入日期（16位）
    uint16_t lastWriteDate = ((localTime->tm_year - 80) << 9) |
                             ((localTime->tm_mon + 1) << 5) |
                             localTime->tm_mday;

    // 获取并构造最后一次写入时间（16位）
    uint16_t lastWriteTime = (localTime->tm_hour << 11) |
                             (localTime->tm_min << 5) |
                             0b00000; // 秒数可以设为0或其他值

    // 将构造的时间和日期存储到结构体中
    entry.DIR_WrtDate[0] = lastWriteDate & 0xFF;           // 低字节
    entry.DIR_WrtDate[1] = (lastWriteDate >> 8) & 0xFF;    // 高字节
    entry.DIR_WrtTime[0] = lastWriteTime & 0xFF;           // 低字节
    entry.DIR_WrtTime[1] = (lastWriteTime >> 8) & 0xFF;    // 高字节
//    parseTime(entry);
}

/*
 * 创建文件夹
 * @param dirName
 */
void mkdir(string &dirName) {
    RootEntry rootEntry;
    // 1.处理文件名
    string fileFullName = dirName.substr(6);
    char _name[12] = "";
    memset(rootEntry.DIR_Name, 0x20, sizeof(rootEntry.DIR_Name));
    strncpy(_name, fileFullName.c_str(), 11);
    _name[11] = '\0';
    memcpy(rootEntry.DIR_Name, _name, strlen(_name)); // 这里用 memcpy 复制字符

    //2.填充文件属性
    rootEntry.DIR_Attr = DIRECTORY_CODE; // 文件属性
    //3.文件10个保留位
    memset(rootEntry.DIR_reserve, 0, sizeof(rootEntry.DIR_reserve));
    //4.设置时间
    setTime(rootEntry);
    //5.文件大小设置为0
    rootEntry.DIR_FileSize = 0x00; // 初始大小

    //6.分配簇号
    uint16_t clus_num = getFreeClusNum();
    //7.把这个簇标记为已经使用


    if (clus_num == 0xFFF) {
        cout << "没有可分配的簇号" << endl;
    }

    usedClus(clus_num);

    cout << "起始位置是" << (1 + 9 + 9 + 14 - 2 + clus_num) * 512 << endl;
    cout << "分配的簇号是" << clus_num << endl;
    rootEntry.DIR_FstClus = clus_num;


    //7.创建.目录和..目录
    RootEntry dot, dotdot;
    createDotDirectory(&dot, clus_num);
    createDotDotDirectory(&dotdot);

    //把.和..写进vfd磁盘
    writeRootEntry(clus_num, dot, 0);
    writeRootEntry(clus_num, dotdot, 1);


    //这个是添加到根目录的!不出现在这里调    rootEntry.DIR_FileSize = SECTOR_SIZE;试！
//    writeRootEntry(clus_num,rootEntry,2);

    //write_to_directory_root
    write_to_directory_root(rootEntry);
    cout << "写入成功" << endl;

    read_fat_from_vfd(PATH);
    read_mbr_from_vfd(PATH);
    fflush(diskFile);
}

int main() {
    showCommandList();
    Init();
 //测试用
/*uint16_t i = getFreeClusNum();
    cout << "空闲的簇是" << i  << endl;
    cout << "他的FAT表项是" <<( i / 2 )<< endl;*/
    int i = 25;
    uint32_t f = getClus(&disk.FAT1[ i /2 ].data[i%2],i%2);
    cout << "下一个簇号" << f << endl;

    while (true) {
        cout << "A>:";
        getline(cin, command);
        executeCommand(command);
    }
}