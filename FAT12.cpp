#include "FAT12.h"
#include "dir.h"

Disk disk;
RootEntry root;

FILE *diskFile = fopen(PATH, "rb");

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

        //ȥ���ո�
        origin_file_name = origin_file_name.substr(0, origin_file_name.find_last_not_of(' ') + 1);
        origin_file_ext = origin_file_ext.substr(0, origin_file_name.find_last_not_of(' ') + 1);

        //�Ƚ��ļ�������չ�������Դ�Сд
        bool isNameMatch = strcasecmp(origin_file_name.c_str(), baseName.c_str()) == 0;
        bool isExtMatch = strcasecmp(origin_file_ext.c_str(), extension.c_str()) == 0;

        if (isNameMatch && isExtMatch) {
            return &disk.rootDirectory[i]; // �����ҵ����ļ���Ŀ
        }
    }
    return NULL;
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

            FILE *boot = fopen(PATH, "rb");
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
        currentCluster = nextClusNum;

        // �����ȡ����һ������ ff8 ~ fff��������ȡ
        if (currentCluster >= 0x0FF8) { // ���� FAT12��0x0FF8 �����ϵĴر�ʾ EOF
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
        uint16_t offset = (19) * SECTOR_SIZE;
        fseek(diskFile, offset, SEEK_SET);
        fread(disk.rootDirectory, 1, disk.MBR.BPB_RootEntCnt, diskFile);
        cout << "�ѻص���Ŀ¼" << endl;
        return;
    }

    if (fileFullName == "..") {
        if (clusterStack.empty()) {
            return;
        }
        if (!clusterStack.empty()) {
//            if(!hasSubdirectories()){ //�������Ŀ¼ ����ջ
//            }
            //��һ��Ĵغ�
            uint8_t tempClusterNum = clusterStack.top();
            clusterStack.pop(); // ������ǰĿ¼�Ĵغ�
            //����Ļص���Ŀ¼���tempClusterNum =2
            if (tempClusterNum == 2) {
                uint16_t offset = (19 + (tempClusterNum - 2)) * SECTOR_SIZE;
                fseek(diskFile, offset, SEEK_SET);
                fread(disk.rootDirectory, 1, disk.MBR.BPB_RootEntCnt, diskFile);
                cout << "�ѷ�����һ��Ŀ¼" << endl;
                return;
            }
            uint16_t offset = (33 + (tempClusterNum - 2)) * SECTOR_SIZE;
            fseek(diskFile, offset, SEEK_SET);
            fread(disk.rootDirectory, 1, disk.MBR.BPB_RootEntCnt, diskFile);
            return;
        } else {
            cout << "��ǰ���ڸ�Ŀ¼" << endl;
        }
        return;
    }

    for (uint16_t i = 0; i < disk.MBR.BPB_RootEntCnt; i++) {
        if (disk.rootDirectory[i].DIR_Name[0] == 0) continue; //������Ŀ¼��
        //�ļ��������Ա�Ƚ�
        uint8_t file_name[9];
        memcpy(file_name, disk.rootDirectory[i].DIR_Name, 8);
        file_name[8] = '\0';
        string actual_name(reinterpret_cast<char *>(file_name));
        actual_name = actual_name.substr(0, actual_name.find_last_not_of(' ') + 1);
        toLowerCase(actual_name);

        if (fileFullName == actual_name && (disk.rootDirectory[i].DIR_Attr & 0x10)) { //�����Ŀ¼
            uint16_t cluster = disk.rootDirectory[i].DIR_FstClus;

            if (clusterStack.empty()) clusterStack.push(2);
            clusterStack.push(cluster);

            uint16_t offset = (33 + (cluster - 2)) * SECTOR_SIZE;
            fseek(diskFile, offset, SEEK_SET);
            fread(disk.rootDirectory, 1, disk.MBR.BPB_RootEntCnt, diskFile);

            cout << "����Ŀ¼��" << actual_name << endl;
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
    } else if (command.substr(0, 6) == "mkdir ") {
        mkdir(command);
    } else if (command.substr(0, 3) == "cd ") { // ������ cd�ո� ���ͷ
        cd(command); // ���� cd ����
    } else if (command == "te") { //���̲���
        te(clusterStack);
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
}

void printRootEntry(const RootEntry &entry) {
    // ��ӡ�ļ���
    cout << "�ļ���: ";
    for (int i = 0; i < 11; ++i) {
        cout << entry.DIR_Name[i];
    }
    cout << endl;
    // ��ӡ�ļ�����
    cout << "�ļ�����: 0x" << hex << setw(2) << setfill('0') << static_cast<int>(entry.DIR_Attr) << endl;

    // ��ӡ����λ
    cout << "����λ: ";
    for (int i = 0; i < 10; ++i) {
        cout << "0x" << hex << setw(2) << setfill('0') << static_cast<int>(entry.DIR_reserve[i]) << " ";
    }
    cout << endl;

    // ��ӡ���һ��д��ʱ��
    cout << "���һ��д��ʱ��: "
         << static_cast<int>(entry.DIR_WrtTime[0]) << ":"
         << static_cast<int>(entry.DIR_WrtTime[1]) << endl;

    // ��ӡ���һ��д������
    cout << "���һ��д������: "
         << static_cast<int>(entry.DIR_WrtDate[0]) << "-"
         << static_cast<int>(entry.DIR_WrtDate[1]) << endl;

    // ��ӡ�ļ���ʼ�Ĵغ�
    cout << "�ļ���ʼ�Ĵغ�: " << entry.DIR_FstClus << endl;

    // ��ӡ�ļ���С
    cout << "�ļ���С: " << entry.DIR_FileSize << " �ֽ�" << endl;
}

//�ļ�������
void mkdir(string &dirName) {
    RootEntry rootEntry;
    string fileFullName = dirName.substr(5);

    // �����ļ���
    char _name[11] = "";
    strncpy(_name, fileFullName.c_str(), 11);
    _name[11] = '\0';

    // ���Ŀ¼��
    memset(rootEntry.DIR_Name, 0x20, sizeof(rootEntry.DIR_Name)); // �ո����
    strncpy(reinterpret_cast<char*>(rootEntry.DIR_Name), _name, 11);
    rootEntry.DIR_Attr = 0x10; // Ŀ¼����
    memset(rootEntry.DIR_reserve, 0, sizeof(rootEntry.DIR_reserve));

    // ����ʱ��
    setTime(rootEntry);
    rootEntry.DIR_FileSize = 512; // ��ʼ��С

    uint16_t cluster_num_allocated = allocateFATCluster();
    if (cluster_num_allocated == 0xFFFF) {
        cout << "���дض���������" << endl;
        return;
    }
    rootEntry.DIR_FstClus = cluster_num_allocated;

    // ���¸�Ŀ¼
    for (int i = 0; i < 1536; ++i) {
        if (disk.rootDirectory[i].DIR_Name[0] == 0x00) { // �ҵ�����Ŀ¼��
            disk.rootDirectory[i] = rootEntry; // �����ڴ��еĸ�Ŀ¼
            cout << "Ŀ¼�����ɹ�, �غ���: " << cluster_num_allocated << endl;

            // д����µ�����
            // ȷ��ָ����Ч

            // �����ļ�ָ�뵽��Ŀ¼��ʼλ��
            fseek(diskFile, 19*512, SEEK_SET); // ROOT_DIR_START �Ǹ�Ŀ¼�ڴ����ϵ���ʼƫ����
            // д���Ŀ¼���ݵ�����
            fwrite(&rootEntry, sizeof(RootEntry), 1, diskFile);

            return;
        }
    }

    cout << "��Ŀ¼�������޷������Ŀ¼��" << endl;
}


int main() {
    showCommandList();
    Init();
    while (true) {
        cout << "A>:";
        getline(cin, command);
        executeCommand(command);
    }
}