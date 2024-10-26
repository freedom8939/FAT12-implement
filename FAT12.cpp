#include "FAT12.h"
using namespace std;
#define PATH "G:\\OS-homework\\grp07\\wjyImg.vfd"
Disk disk;

//�����������ٲ��֣�
RootEntry *findFile(const string &fileName);

void cat(string &_name);

void executeCommand(string &command);
/**
 * ��ʼ��MBR
 * @return
 */
//��ʼ��MBR.
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
 * ��ʼ������FAT����
 */
void InitFAT() {
    memset(disk.FAT1, 0x00, sizeof(disk.FAT1));
    memset(disk.FAT2, 0x00, sizeof(disk.FAT2));

    // ��ʼ����һ�� FAT ����Ŀ
//    disk.FAT1[0].firstEntry = 0xFF0;  // ��һ����Ŀ��ʾ�������ͣ�0xF0��ʾ���̣�
//    disk.FAT1[0].secondEntry = 0xFFF; // �ڶ�����Ŀ��ʾ�������

    // �� FAT1 ���Ƶ� FAT2
    memcpy(disk.FAT2, disk.FAT1, sizeof(disk.FAT1));

    // ��ʼ������ FAT ����Ŀ��������Ҫ���ã�
    for (int i = 1; i < sizeof(disk.FAT1) / sizeof(FATEntry); i++) {
//        disk.FAT1[i].firstEntry = 0x000;  // δʹ�õĴ�
//        disk.FAT1[i].secondEntry = 0x000; // δʹ�õĴ�
    }

    // ���Ƴ�ʼ����� FAT1 �� FAT2
    memcpy(disk.FAT2, disk.FAT1, sizeof(disk.FAT1));
}

/**
 * ��ӡ����MBR������
 * @param temp
 */
void print_MBR(MBRHeader *temp) {
    printf("Disk Name - ��������: %s\n", temp->BS_OEMName);
    printf("Sector Size - ������С: %d\n", temp->BPB_BytesPerSec);
    printf("Sectors per cluster - ÿ��������: %d\n", temp->BPB_SecPerClus);
    printf("Number of sectors occupied by boot - ����ռ�õ�������: %d\n", temp->BPB_ResvdSecCnt);
    printf("Number of FATs - FAT ������: %d\n", temp->BPB_NumFATs);
    printf("The max capacity of root entry - ��Ŀ¼�����Ŀ��: %d\n", temp->BPB_RootEntCnt);
    printf("The number of all Sectors - ��������: %d\n", temp->BPB_TotSec16);
// printf("Media Descriptor - ý��������: %c\n", temp->BPB_Media);
    printf("The number of Sectors of each Fat table - ÿ�� FAT ��ռ�õ�������: %d\n", temp->BPB_FATSz16);
    printf("Number of sectors per track - ÿ���ŵ���������: %d\n", temp->BPB_SecPerTrk);
    printf("The number of magnetic read head - ��ͷ����: %d\n", temp->BPB_NumHeads);
    printf("Number of hidden sectors - ����������: %d\n", temp->BPB_HiddSec);
    printf("Volume serial number - �����к�: %d\n", temp->BS_VollD);
    printf("Volume label - ���: ");
    for (int i = 0; i < 11; i++) {
        printf("%c", temp->BS_VolLab[i]);
    }
    printf("\n");
    printf("File system type - �ļ�ϵͳ����: ");
    for (int i = 0; i < 8; i++) {
        printf("%c", temp->BS_FileSysType[i]);
    }
    printf("\n");

}

void read_fat_from_vfd(char *vfd_file){
    FILE *file = fopen(vfd_file, "rb");
    if (!file) {
        perror("�޷��� VFD �ļ�");
        return;
    }
    fseek(file,512,SEEK_SET);
    fread(&disk.FAT1, sizeof(FATEntry), 192, file);  // ��ȡ192��FATEntry


    fclose(file);
}

/**
 * ��ȡ�����mbr�ļ��б���չʾ�ڿ���̨
 */
void read_mbr_from_vfd(char *vfd_file) {
    //��ȡvfd
    FILE *boot = fopen(vfd_file, "rb");

    //һ��MBRҲ����512�ֽ� ����512�ֽڿռ�
    //��ȡmbr
    MBRHeader *mbr = (MBRHeader *) malloc(sizeof(MBRHeader));
    size_t readed_size = fread(mbr, 1, SECTOR_SIZE, boot);
    if (readed_size != SECTOR_SIZE) {
        perror("�ļ���ȡ����,�����Ƿ�·����ȷ");
        free(mbr);
        return;
    }
    disk.MBR = *mbr;

//    print_MBR(&disk.MBR);
    uint32_t rootDirStartSec = 1 + (disk.MBR.BPB_NumFATs * disk.MBR.BPB_FATSz16); // 1 MBR��2 �� FAT ���������
//    printf("ÿ��FAT��ռ%d������", disk.MBR.BPB_FATSz16);
    fseek(boot, rootDirStartSec * SECTOR_SIZE, SEEK_SET); //�ƶ�����Ŀ¼����
    uint16_t read_size = fread(disk.rootDirectory, sizeof(RootEntry), disk.MBR.BPB_RootEntCnt, boot);

    if (read_size != 224) {  //����д�� ��ʦ�ṩ�Ĵ���Ϊ112
        perror("��ȡ��Ŀ¼ʧ�ܣ�������11111��");
        fclose(boot);
        return;
    }

    free(mbr);
    fclose(boot);
}

/**
 * dir����
 */
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
        extension = extension.substr(0, fileName.find_last_not_of(' ') + 1);
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
 * ��ȡ FAT12 ����һ���غ�
 * @param currentCluster ��ǰ�غ�
 * @return ��һ���غ�
 */

/**
 * �����ļ�Entry��
 * @param command
 */
RootEntry *findFile(const string &fileName) {
    if (fileName == "") {
        cout << "������Ϊ��";
        return NULL;
    }
    //
    size_t dotPosition = fileName.find('.');
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
        // �Ƚ��ļ�������չ�������Դ�Сд
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
 * ��ȡ��һ���غ�
 * @param cluster ��ǰ�غ�
 * @return ������һ���غ�
 */
uint16_t getNextCluster(uint16_t currentCluster) {
    uint32_t fatOffset = currentCluster * 3 / 2;
    uint8_t fatSector[SECTOR_SIZE];

    FILE *vfdFile = fopen(PATH, "rb");
    if (!vfdFile) {
        perror("Unable to open VFD file");
        return 0;
    }

    fseek(vfdFile, fatOffset / SECTOR_SIZE * SECTOR_SIZE, SEEK_SET);
    fread(fatSector, 1, SECTOR_SIZE, vfdFile);
    fclose(vfdFile);

    // ��ȡ��һ���ص�ֵ
    if (currentCluster % 2 == 0) {
        return (fatSector[fatOffset % SECTOR_SIZE] | (fatSector[fatOffset % SECTOR_SIZE + 1] << 8)) & 0x0FFF;
    } else {
        return (fatSector[fatOffset % SECTOR_SIZE] >> 4 | (fatSector[fatOffset % SECTOR_SIZE + 1] << 4)) & 0x0FFF;
    }
}


/**
 * �ļ��ĵ�һ���غ�
 * @param firstCluster  ��һ���غ�
 * @param size   ��ȡ�Ĵ�С ��bug��
 */
/**
 * ��ȡ�ļ�����
 * @param firstCluster ��һ���غ�
 * @param fileSize �ļ���С
 */
/**
 * ��ȡ�ļ�����
 * @param firstCluster ��һ���غ�
 * @param fileSize �ļ���С
 */

void readFileData(uint16_t firstCluster, uint32_t fileSize) {
    uint8_t sectorsPerCluster = disk.MBR.BPB_SecPerClus; // ÿ���ص�������
    uint32_t fatSize = disk.MBR.BPB_FATSz16; // FAT �Ĵ�С
    uint32_t fatStartSector = 1; // FAT ��ʼ����
    uint32_t dataStartSector = fatStartSector + (disk.MBR.BPB_NumFATs * fatSize) + (disk.MBR.BPB_RootEntCnt * sizeof(RootEntry) / disk.MBR.BPB_BytesPerSec);

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

            FILE *vfdFile = fopen(PATH, "rb");
            if (!vfdFile) {
                perror("Unable to open VFD file");
                return;
            }

            // ��ȡ��ǰ����
            fseek(vfdFile, (clusterSector + sector) * SECTOR_SIZE, SEEK_SET);
            size_t readSize = fread(buffer, 1, SECTOR_SIZE, vfdFile);
            fclose(vfdFile);

            // �����ȡ������
            for (size_t i = 0; i < readSize && bytesRead < fileSize; ++i, ++bytesRead) {
                putchar(buffer[i]); // ����ַ�������̨
            }
        }

        unsigned short nextClusNum = getClus(&disk.FAT1[currentCluster / 2].data[currentCluster % 2], currentCluster % 2);
        currentCluster = nextClusNum;
        // �� FAT ���л�ȡ��һ���ص�ֵ
//        currentCluster = getNextCluster(currentCluster);
       
        // �����ȡ����һ������ EOF��������ȡ
        if (currentCluster >= 0x0FF8) { // ���� FAT12��0x0FF8 �����ϵĴر�ʾ EOF
            break;
        }
    }

    // �����Խ������
    putchar('\n');
}

void executeCommand(string &command) {

    if (command == "dir") {
        dir(); // ��ʾ��Ŀ¼�Ľṹ
    } else if (command == "exit") {
        cout << "�˳�����..." << endl;
        exit(0);
    } else if (command.substr(0, 4) == "cat ") { // ������ cat�ո� ���ͷ
        cat(command); // ���� cat ����
    } else {
        cout << "δ֪����: " << command << endl;
    }
}



void cat(string &_name) {
    // ��ȡ�ļ���
    string fileFullName = _name.substr(4); // �� "cat " ��ʼ
    // �����ļ���Ŀ
    RootEntry *fileEntry = findFile(fileFullName);
    if (fileEntry) {
        // ����ҵ��ļ���Ŀ������ҵ����ļ���Ϣ
//        cout << "��ʼ�غ�" << fileEntry->DIR_FstClus << "�Ŵ�" << endl;
//        cout << "�ļ���С: " << fileEntry->DIR_FileSize << " �ֽ�" << endl;
        // ��Ŀ¼���л�ȡ��ʼ�غ�
        uint16_t firstCluster = fileEntry->DIR_FstClus;
        //һ�ζ�һ������
        readFileData(firstCluster,fileEntry->DIR_FileSize);
    } else {
        cout << "�ļ�δ�ҵ�: " << fileFullName << endl;
    }
}
int main() {
    string command;
    cout << "��ʹ������:" << endl;
    cout << "(1):dir" << endl;
    cout << "(2):cat �ļ���" << endl;
    cout << "(3):exit" << endl;
//    InitMBR(&disk);  //��ʼ������MBR��Ϣ mock�ı�������
    read_mbr_from_vfd(PATH);
    read_fat_from_vfd(PATH);
//    InitFAT();  //��ʼ������fat mock��������ʹ��
//    unsigned short i = getClus(&disk.FAT1[clus /2].data[clus %2],clus %2);
//    cout << i;
//    int clus  = 7;
//    cout << getClus(&disk.FAT1[clus /2].data[clus %2],clus %2);


    while (1) {
        cout << ">:";
        getline(cin, command);
        executeCommand(command);
    }
}