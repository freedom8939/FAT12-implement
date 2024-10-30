
void dir() {
    for (int i = 0; i < disk.MBR.BPB_RootEntCnt; i++) {
        if (disk.rootDirectory[i].DIR_Name[0] == 0) {
            continue; // ����Ŀ������
        }

        // ��ȡ�ļ���С
        uint32_t fileSize = disk.rootDirectory[i].DIR_FileSize;

        char name[9] = "";
        char ext[4] = "";

        memcpy(name, disk.rootDirectory[i].DIR_Name, 8);
        memcpy(ext, disk.rootDirectory[i].DIR_Name + 8, 3);

        string fileName = name;
        string extension = ext;
        //ȥ���ո�
        fileName = fileName.substr(0, fileName.find_last_not_of(' ') + 1);
        extension = extension.substr(0, fileName.find_last_not_of(' ') + 2);

        //���������
        /*
         * hex   0001 0000               0010 0000
         *     & 0001 0000             & 0001 0000
         *       0001 0000 ������Ŀ¼      0000 0000 ����0˵���϶�����Ŀ¼ Ӧ������ͨ�ļ����Բ�������
         */
        // ��ȡ���һ��д��ʱ������� (���16λ)
        uint16_t lastWriteTime = (disk.rootDirectory[i].DIR_WrtTime[1] << 8) | disk.rootDirectory[i].DIR_WrtTime[0];
        uint16_t lastWriteDate = (disk.rootDirectory[i].DIR_WrtDate[1] << 8) | disk.rootDirectory[i].DIR_WrtDate[0];
//
//        // �������һ��д��ʱ��
        int hours = (lastWriteTime >> 11) & 0x1F; // ��ȡСʱ
        int minutes = (lastWriteTime >> 5) & 0x3F; // ��ȡ����
//        // �������һ��д������
        int year = ((lastWriteDate >> 9) & 0x7F) + 1980; // ��ȡ���
        int month = (lastWriteDate >> 5) & 0x0F; // ��ȡ�·�
        int day = lastWriteDate & 0x1F; // ��ȡ����
        /*
         *  dir��ʽ
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
