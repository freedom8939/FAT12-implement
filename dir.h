
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
        bool isDirectory = (disk.rootDirectory[i].DIR_Attr & DIRECTORY_CODE) != 0;
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
