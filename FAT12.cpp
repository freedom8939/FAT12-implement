#include "FAT12.h"
#include "dir.h"

Disk disk;

/**
 * ��ȡ�����FAT�����ش���
 */
void read_fat_from_vfd(char *vfd_file) {
    fseek(diskFile, 512, SEEK_SET);
    fread(&disk.FAT1, sizeof(FATEntry), 1536, diskFile);
}

/**
 * ��ȡ�����mbr�����ش���
 */
void read_mbr_from_vfd(char *vfd_file) {
    //һ��MBRҲ����512�ֽ� ����512�ֽڿռ�
    MBRHeader *mbr = (MBRHeader *) malloc(sizeof(MBRHeader));
    fseek(diskFile, 0, SEEK_SET);
    uint32_t readed_size = fread(mbr, 1, SECTOR_SIZE, diskFile);
    if (readed_size != SECTOR_SIZE) {
        perror("�ļ���ȡ����,�����Ƿ�·����ȷ");
        free(mbr);
        return;
    }
    disk.MBR = *mbr;
    uint32_t rootDirStartSec = 1 + (disk.MBR.BPB_NumFATs * disk.MBR.BPB_FATSz16); // 1 MBR��2 �� FAT ���������
    fseek(diskFile, rootDirStartSec * SECTOR_SIZE, SEEK_SET); //�ƶ�����Ŀ¼����
    uint16_t read_size = fread(disk.rootDirectory, sizeof(RootEntry), disk.MBR.BPB_RootEntCnt, diskFile);
    if (read_size != 224) {  //����д�� ��ʦ�ṩ�Ĵ���Ϊ112
        perror("��ȡ��Ŀ¼ʧ�ܣ������ļ�");
        fclose(diskFile);
        return;
    }
    free(mbr);
}

/**
 * �����ļ�Entry��
 */
RootEntry *findFile(string &fileName) {
    if (fileName.empty()) {
        cout << "������Ϊ��";
        return NULL;
    }
    uint16_t dotPosition = fileName.find('.');  //��.�ָ�ǰ��
    string baseName = (dotPosition == string::npos) ? fileName : fileName.substr(0, dotPosition);
    string extension = (dotPosition == string::npos) ? "" : fileName.substr(dotPosition + 1);

    for (int i = 0; i < disk.MBR.BPB_RootEntCnt; i++) {
        // ��������Ŀ
        if (disk.rootDirectory[i].DIR_Name[0] == 0) continue;

        // ��Ŀ¼��Ŀ����ȡ�ļ�������չ��
        char _name[9] = "";
        char _ext[4] = "";

        memcpy(_name, disk.rootDirectory[i].DIR_Name, 8);
        memcpy(_ext, disk.rootDirectory[i].DIR_Name + 8, 3);

        // ������׼�����ļ�������չ��
        string origin_file_name = string(_name);
        string origin_file_ext = string(_ext);
//        cout << "---" << origin_file_name << "-" <<origin_file_ext <<endl;
        //ȥ���ո�
        origin_file_name = origin_file_name.substr(0, origin_file_name.find_last_not_of(' ') + 1);
        origin_file_ext = origin_file_ext.substr(0, origin_file_name.find_last_not_of(' ') + 3);

//        cout << "---" << origin_file_name << "-" <<origin_file_ext <<endl;


        //�Ƚ��ļ�������չ�������Դ�Сд
        bool isNameMatch = strcasecmp(origin_file_name.c_str(), baseName.c_str()) == 0;
        bool isExtMatch = strcasecmp(origin_file_ext.c_str(), extension.c_str()) == 0;

        if (isNameMatch && isExtMatch) {
            return &disk.rootDirectory[i]; // �����ҵ����ļ���Ŀ
        }
    }
    return nullptr;
}

/**
 * �Ӹ����Ļ�������ȡ��һ���غ�
 * @param buffer ָ��洢 FAT ���ݵĻ�����
 * @param flag ָʾλ�����ھ�����ζ�ȡ�غ�
 * @return ������һ���غ�
 */
unsigned short getClus(unsigned char *buffer, char flag) {
    uint16_t ans = 0;

    if (!flag) {
        ans += buffer[0];
        ans += (buffer[1] << 8) & 0xFFF; // ֻȡ��12λ
    } else {
        ans += buffer[1] << 4;
        ans += buffer[0] >> 4; // ȡ��4λ
    }
    return ans;
}

/**
 * ��ȡ�ļ�����
 * @param firstCluster ��һ���غ�
 * @param fileSize �ļ���С
 */
void readFileData(uint16_t firstCluster, uint32_t fileSize) {
    uint8_t sectorsPerCluster = disk.MBR.BPB_SecPerClus; // ÿ���ص�������
    uint32_t fatSize = disk.MBR.BPB_FATSz16; // FAT �Ĵ�С
    uint32_t fatStartSector = 1; // FAT ��ʼ����
    uint32_t dataStartSector = fatStartSector + (disk.MBR.BPB_NumFATs * fatSize) +
                               (disk.MBR.BPB_RootEntCnt * sizeof(RootEntry) / disk.MBR.BPB_BytesPerSec);

    uint16_t currentCluster = firstCluster;
    uint32_t bytesRead = 0;


    while (bytesRead < fileSize) {
        // ���㵱ǰ�ض�Ӧ����ʼ����
        uint32_t clusterSector = dataStartSector + (currentCluster - 2) * sectorsPerCluster;
        // ���ڴ洢��ȡ���ֽ�
        uint8_t buffer[SECTOR_SIZE];
        // ��������ȡ����
        for (uint8_t sector = 0; sector < sectorsPerCluster; ++sector) {
            if (bytesRead >= fileSize) break; // ����Ѷ�ȡ��ϣ��˳�ѭ��

            FILE *boot = fopen(PATH, "rb+");
            // ��ȡ��ǰ����
            fseek(boot, (clusterSector + sector) * SECTOR_SIZE, SEEK_SET);
            size_t readSize = fread(buffer, 1, SECTOR_SIZE, boot);
            fclose(boot);

            // �����ȡ������
            for (size_t i = 0; i < readSize && bytesRead < fileSize; i++, bytesRead++) {
                putchar(buffer[i]); // ����ַ�������̨
            }
        }

        unsigned short nextClusNum = getClus(&disk.FAT1[currentCluster / 2].data[currentCluster % 2],
                                             currentCluster % 2);
        currentCluster = nextClusNum + 1;
        cout << "��ȡ�Ĵ���" << currentCluster << endl;
        // �����ȡ����һ������ ff8 ~ fff��������ȡ
        if (currentCluster >= 0xFF8) { // ���� FAT12��0x0FF8 �����ϵĴر�ʾ EOF
            break;
        }
    }

    // �����Խ������
    cout << endl;
}

/**
 * �����ļ���
 * @param _name
 */
void cd(string &_name) {
    string fileFullName = _name.substr(3); // �� "cd " ��ʼ
    toLowerCase(fileFullName);

    if (fileFullName == ".") {
        cout << "���ڵ�ǰĿ¼" << endl;
        return;
    }
    if (fileFullName == "/") {
        go_back_to_directory_position();
        //���·��ջ
        while (!clusterStack.empty() && clusterStack.top() != 2) {
            clusterStack.pop();
        }
        while (!pathStack.empty() && pathStack.top() != "/") {
            pathStack.pop();
        }
        return;
    }
    if (fileFullName == "..") {

        //�����ʱջ��Ԫ��ֻ��һ��Ԫ��2�����Ѿ��ڶ���Ŀ¼ �޷�����
        if (clusterStack.top() == 2 || pathStack.top() == "/") {     //ֱ�ӻص���Ŀ¼
            go_back_to_directory_position();

            return;
        }
        //ջ�ղ�������
        if (clusterStack.empty() || pathStack.empty()) {
            return;
        }


        //ջ���ж��ָ�Ԫ�ص������

        /**
         *
               9��ջ Ȼ�󷵻ش�Ϊ8 ���� ���� now_clus =8
              8��ջ  Ȼ�󷵻ش�Ϊ6 ����now_clus = 6
             6��ջ ����2 now_clus =2
           2 == 2 ����
         */

        string tempPathName = pathStack.top(); //ջ��·��
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
        if (disk.rootDirectory[i].DIR_Name[0] == 0) continue; //������Ŀ¼��

        //�ļ��������Ա�Ƚ�
        char file_name[9];
        //��ǰ8λ��file_name
        memcpy(file_name, disk.rootDirectory[i].DIR_Name, 8);
        //תΪ�ַ����������
        string actual_name = file_name;
        actual_name = actual_name.substr(0, actual_name.find_last_not_of(' ') + 1);
        toLowerCase(actual_name);

        if (fileFullName == actual_name && (disk.rootDirectory[i].DIR_Attr & 0x10)) { //�����Ŀ¼
            uint16_t cluster = disk.rootDirectory[i].DIR_FstClus;
            //�������ջ
            clusterStack.push(cluster);
            now_clu = cluster;
            if (pathStack.top() != "/") {
                pathStack.push("/" + actual_name);
            } else {
                pathStack.push(actual_name);
            }

            //�滻��ǰdisk.rootDirctoryĿ¼Ϊָ��
            navigator_to_specific_directory_position(cluster);
            cout << "��ǰĿ¼��" << actual_name << endl;
            return;
        }
    }
    cout << "δ�ҵ�Ŀ¼: " << fileFullName << endl;
}

/**
 * ʵ���������
 * @param command
 */
void executeCommand(string &command) {
    if (command == "dir") {
        dir(); // ��ʾ��Ŀ¼�Ľṹ
    } else if (command == "exit") {
        cout << "�˳�����..." << endl;
        exit(0);
    } else if (command.substr(0, 4) == "cat ") { // ������ cat�ո� ���ͷ
        cat(command); // ���� cat ����
    } else if (command == "pwd") {
        pwd();
    } else if (command.substr(0, 6) == "mkdir ") {
        mkdir(command);
    } else if (command.substr(0, 3) == "cd ") { // ������ cd�ո� ���ͷ
        cd(command); // ���� cd ����
    } else if (command == "te") { //���̲��ԣ�Ŀ¼���ԣ�
        te(clusterStack);
    } else if (command.substr(0, 6) == "touch ") {
        touch(command);
    } else if (command == "gc") { //��ǰ�����Ĵغ�
        cout << (uint32_t) getNowClu() << endl;
    } else {
        cout << "δ֪����: " << command << endl;
    }
}

/**
 * �鿴�ļ�����
 * @param _name
 */
void cat(string &_name) {
    // ��ȡ�ļ���
    string fileFullName = _name.substr(4); // �� "cat " ��ʼ
    // �����ļ���Ŀ
    RootEntry *fileEntry = findFile(fileFullName);
    if (fileEntry) {
        // ����ҵ��ļ���Ŀ������ҵ����ļ���Ϣ
//        cout << "��ʼ�غ�" << fileEntry->DIR_FstClus << "�Ŵ�" << endl;
//        cout << "�ļ���С: " << fileEntry->DIR_FileSize << " �ֽ�" << endl;
        uint16_t firstCluster = fileEntry->DIR_FstClus;
        readFileData(firstCluster, fileEntry->DIR_FileSize);
    } else {
        cout << "�ļ�δ�ҵ�: " << fileFullName << endl;
    }
}

/**
 * ����Entru��ʱ��
 * @param entry
 */
void setTime(RootEntry &entry) {
    time_t currentTime = time(NULL);
    tm *localTime = localtime(&currentTime);

    // ��ȡ���������һ��д�����ڣ�16λ��
    uint16_t lastWriteDate = ((localTime->tm_year - 80) << 9) |
                             ((localTime->tm_mon + 1) << 5) |
                             localTime->tm_mday;

    // ��ȡ���������һ��д��ʱ�䣨16λ��
    uint16_t lastWriteTime = (localTime->tm_hour << 11) |
                             (localTime->tm_min << 5) |
                             0b00000; // ����������Ϊ0������ֵ

    // �������ʱ������ڴ洢���ṹ����
    entry.DIR_WrtDate[0] = lastWriteDate & 0xFF;           // ���ֽ�
    entry.DIR_WrtDate[1] = (lastWriteDate >> 8) & 0xFF;    // ���ֽ�
    entry.DIR_WrtTime[0] = lastWriteTime & 0xFF;           // ���ֽ�
    entry.DIR_WrtTime[1] = (lastWriteTime >> 8) & 0xFF;    // ���ֽ�
//    parseTime(entry);
}

/*
 * �����ļ���
 * @param dirName
 */
void mkdir(string &dirName) {
    RootEntry rootEntry;
    // 1.�����ļ���
    string fileFullName = dirName.substr(6);
    char _name[12] = "";
    memset(rootEntry.DIR_Name, 0x20, sizeof(rootEntry.DIR_Name));
    strncpy(_name, fileFullName.c_str(), 11);
    _name[11] = '\0';
    memcpy(rootEntry.DIR_Name, _name, strlen(_name)); // ������ memcpy �����ַ�

    //2.����ļ�����
    rootEntry.DIR_Attr = DIRECTORY_CODE; // �ļ�����
    //3.�ļ�10������λ
    memset(rootEntry.DIR_reserve, 0, sizeof(rootEntry.DIR_reserve));
    //4.����ʱ��
    setTime(rootEntry);
    //5.�ļ���С����Ϊ0
    rootEntry.DIR_FileSize = 0x00; // ��ʼ��С

    //6.����غ�
    uint16_t clus_num = getFreeClusNum();
    //7.������ر��Ϊ�Ѿ�ʹ��


    if (clus_num == 0xFFF) {
        cout << "û�пɷ���Ĵغ�" << endl;
    }

    usedClus(clus_num);

    cout << "��ʼλ����" << (1 + 9 + 9 + 14 - 2 + clus_num) * 512 << endl;
    cout << "����Ĵغ���" << clus_num << endl;
    rootEntry.DIR_FstClus = clus_num;


    //7.����.Ŀ¼��..Ŀ¼
    RootEntry dot, dotdot;
    createDotDirectory(&dot, clus_num);
    createDotDotDirectory(&dotdot);

    //��.��..д��vfd����
    writeRootEntry(clus_num, dot, 0);
    writeRootEntry(clus_num, dotdot, 1);


    //�������ӵ���Ŀ¼��!�������������    rootEntry.DIR_FileSize = SECTOR_SIZE;�ԣ�
//    writeRootEntry(clus_num,rootEntry,2);

    //write_to_directory_root
    write_to_directory_root(rootEntry);
    cout << "д��ɹ�" << endl;

    read_fat_from_vfd(PATH);
    read_mbr_from_vfd(PATH);
    fflush(diskFile);
}

int main() {
    showCommandList();
    Init();
 //������
/*uint16_t i = getFreeClusNum();
    cout << "���еĴ���" << i  << endl;
    cout << "����FAT������" <<( i / 2 )<< endl;*/
    int i = 25;
    uint32_t f = getClus(&disk.FAT1[ i /2 ].data[i%2],i%2);
    cout << "��һ���غ�" << f << endl;

    while (true) {
        cout << "A>:";
        getline(cin, command);
        executeCommand(command);
    }
}