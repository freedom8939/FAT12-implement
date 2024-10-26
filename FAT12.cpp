#include "FAT12.h"
#include "dir.h"

Disk disk;

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
    unsigned short ans = 0;
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
    string fileFullName = _name.substr(3); // �� "cat " ��ʼ
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
            cout << "�ѷ��ظ�Ŀ¼" << endl;
            return;
        }

        if (fileFullName == ".") { // ��ǰĿ¼
            cout << "�������ڵ�ǰĿ¼" << endl;
            return;
        }

        if (fileFullName == "..") { // ������һ��Ŀ¼
            cout << "TODO�ѷ�����һ��Ŀ¼" << endl;
            return;
        }


        if (fileFullName == actual_name && disk.rootDirectory[i].DIR_Attr & 0x10) {
//            cout << "�ҵ�����ʼ�غ���" << disk.rootDirectory[i].DIR_FstClus << endl;
            uint16_t offset = (33 + (disk.rootDirectory[i].DIR_FstClus - 2)) * 512;
//            cout << "offset��" << offset << endl;
            fseek(diskFile, offset, SEEK_SET);
            fread(disk.rootDirectory, 1, SECTOR_SIZE, diskFile);
            return;
        }
    }
    cout << "δ�ҵ�" << endl;
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
    } else if (command.substr(0, 3) == "cd ") { // ������ cd�ո� ���ͷ
        cd(command); // ���� cd ����
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