/**
 * Created by ����԰ on 2024/10/21.
 */
#include <cstdio>
#include <iostream>
#include <string>
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <stack>
#include <ctime>

using namespace std;

#define PATH "./wjyImg.vfd"
FILE *diskFile = fopen(PATH, "rb+");
uint8_t now_clu = 2;
/**
* ������̵���Ϣ����δ�����ʦ�Ĵ���)
*/
#define SECTOR_SIZE 512        //������СĬ��512
#define DATA_SECTOR_NUM 2847   //����������
#define ROOT_DICT_NUM 224      //��Ŀ¼��
#define DIRECTORY_CODE 0x10
#define FAT_SECTOR_NUM 9        //FAT������
#define DIRECTORY_POS  19  //Ŀ¼ȥ��ʼ��λ��
#define DATA_POS 33   //��������ʼ��λ��
#define FAT_SIZE 4608   //FAT�������ֽڴ�С

/**
* ������������
*/
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef char Sector[SECTOR_SIZE];   //��������С
#pragma pack(1)
/**
 * FAT����ṹ
 */
typedef struct FATEntry {
    uint8_t data[3]; //3�ֽ� 24λ  �����غ�
} FATEntry;

/**
 * MBR
 */
typedef struct MBRHeader {
    char BS_jmpBOOT[3];
    char BS_OEMName[8];
    // ÿ�������ֽ��� 512
    uint16_t BPB_BytesPerSec;
    // ÿ�������� 1
    uint8_t BPB_SecPerClus;
    // boot����ռ������ 1
    uint16_t BPB_ResvdSecCnt;
    //һ���м���FAT�� 2
    uint8_t BPB_NumFATs;
    //��Ŀ¼�ļ������  0xe0 = 224
    uint16_t BPB_RootEntCnt;
    //��������  0xb40 = 2880
    uint16_t BPB_TotSec16;
    //��������  0xf0
    uint8_t BPB_Media;
    //ÿ��FAT��ռ������ 9
    uint16_t BPB_FATSz16;
    // ÿ���ŵ�ռ������ 0x12
    uint16_t BPB_SecPerTrk;
    // ��ͷ��   0x2
    uint16_t BPB_NumHeads;
    // ���������� 0
    uint32_t BPB_HiddSec;
    // ���BPB_TotSec16=0,����������������� 0
    uint32_t BPB_TotSec32;
    // INT 13H�������� 0
    uint8_t BS_DrvNum;
    //������δ��    0
    uint8_t BS_Reserved1;
    //��չ�������  0x29
    uint8_t BS_BootSig;
    // �����к� 0
    uint32_t BS_VollD;
    // ��� 'yxr620'
    uint8_t BS_VolLab[11];
    // �ļ�ϵͳ���� 'FAT12'
    uint8_t BS_FileSysType[8];
    //��������
    char code[448];
    //������־
    char end_point[2];
} MBRHeader;

/**
 *  ��Ŀ¼����ʽ
 */
typedef struct RootEntry {
    uint8_t DIR_Name[11];//�ļ�������չ��
    uint8_t DIR_Attr;//�ļ�����
    uint8_t DIR_reserve[10];//����λ
    uint8_t DIR_WrtTime[2];//���һ��д��ʱ��
    uint8_t DIR_WrtDate[2];//���һ��д������
    uint16_t DIR_FstClus;//�ļ���ʼ�Ĵغ�
    uint32_t DIR_FileSize;//�ļ���С
} RootEntry;

/**
 * �������
 */
struct Disk {
    MBRHeader MBR;  //1������
    FATEntry FAT1[1536];     // 9������ ȫ����ʾ������
    FATEntry FAT2[1536];     // copy fat1
    RootEntry rootDirectory[ROOT_DICT_NUM];   //��Ŀ¼��
    Sector dataSector[DATA_SECTOR_NUM]; //2880������
};
extern Disk disk;
#pragma pack()

//��ʼ��MBR
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

//��ӡMBR
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

//��ʼ��FAT��
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

//��VFD�ж���mbr
void read_mbr_from_vfd(char *vfd_file);

//�����ļ�������
void cd(string &_name);

//�鿴�ļ�
void cat(string &_name);

//ת�ַ���ΪСд
void toLowerCase(string &str) {
    for (char &c: str) {
        c = tolower(c); // ��ÿ���ַ�ת��ΪСд
    }
}

//��ȡ����
void readFileData(uint16_t firstCluster, uint32_t fileSize);

//��Ŀ¼�²����ļ�FATEntry
RootEntry *findFile(string &fileName);

//ִ������
void executeCommand(string &command);

//��vfd��ȡfat�����ش���
void read_fat_from_vfd(char *vfd_file);

//�鿴����Ŀ¼
void dir();

//��ȡ�غ�
unsigned short getClus(unsigned char *buffer, char flag);

// �����б�
string command;

void showCommandList() {
    cout << "��ʹ������:" << endl;
    cout << "(1):dir" << endl;
    cout << "(2):cat �ļ���" << endl;
    cout << "(3):cd �ļ���" << endl;
    cout << "(4):pwd" << endl;
    cout << "(5):touch" << endl;
    cout << "(6):rm �ļ���" << endl;
    cout << "(7):mkdir �ļ�����" << endl;
    cout << "(0):exit" << endl;
}

//��ǰ��Ŀ¼�Ƿ������ļ���(���ų�.��..)
bool hasSubdirectories();

//������ջ�����ӽ�ȥһ��Entry
//Ŀ¼ջ(�غ�)
stack<uint16_t> clusterStack; // �����洢ÿһ��Ŀ¼�Ĵغ�
//Ŀ¼ջ��·������
stack<string> pathStack;

//��������
void te(const stack<uint16_t> &clusterStack) {
    // ����һ����ʱ�������洢ջ�е�Ԫ��
    vector<uint16_t> tempStack;
    stack<uint16_t> temp = clusterStack; // ����ջ

    // ��ջԪ��ת�Ƶ���ʱ����
    while (!temp.empty()) {
        tempStack.push_back(temp.top());
        temp.pop();
    }

    // �����ʱ�����е�Ԫ��
    cout << "��ǰĿ¼�غ�ջ��";
    for (size_t i = 0; i < tempStack.size(); ++i) {
        cout << tempStack[i];
        if (i < tempStack.size() - 1) {
            cout << " -> "; // ֻ��Ԫ��֮����ӷָ���
        }
    }
    bool subdirectories = hasSubdirectories();
    cout << endl;
//    if (subdirectories) {
//        cout << "����Ŀ¼" << endl;
//    } else {
//        cout << "û����Ŀ¼" << endl;
//    }

}

//��ʼ��FAT��MBR
void Init() {
    read_mbr_from_vfd(PATH);
    read_fat_from_vfd(PATH);
    clusterStack.push(2); //��ʼ��
    pathStack.push("/");

}

//��ȡ��ǰ���ڴغ�
uint32_t getNowClu() {
    return now_clu;
}

//�Ƿ�����Ŀ¼
bool hasSubdirectories() {
    for (uint16_t i = 0; i < disk.MBR.BPB_RootEntCnt; i++) {
        // ���Ŀ¼���Ƿ���Ч
        if (disk.rootDirectory[i].DIR_Name[0] == 0) continue;

        // ����Ƿ���Ŀ¼
        if (disk.rootDirectory[i].DIR_Attr & DIRECTORY_CODE) {
            // �ų������ . �� .. Ŀ¼
            uint8_t file_name[9];
            memcpy(file_name, disk.rootDirectory[i].DIR_Name, 8);
            file_name[8] = '\0';
            string dirName(reinterpret_cast<char *>(file_name));
            dirName = dirName.substr(0, dirName.find_last_not_of(' ') + 1);

            if (dirName != "." && dirName != "..") {
                return true; // �ҵ���Ŀ¼
            }
        }
    }
    return false; // û���ҵ���Ŀ¼
}

//�����ļ���
void mkdir(string &dirName);

//����wrTimeʱ���
void setTime(RootEntry &entry);

//д��FAT�� ,
void setClus(uint16_t clusNum) {
    uint8_t *entry = &disk.FAT1[clusNum / 2].data[clusNum % 2];  // ��ӦFAT������ֽڵ�ַ

    if (!(clusNum % 2)) {
        // ż���غţ���һ���صĵ�12λ
        entry[0] = clusNum & 0xFF;                // ��8λ
        entry[1] = (entry[1] & 0xF0) | ((clusNum >> 8) & 0x0F);  // ��4λ
    } else {
        // �����غţ��ڶ����صĸ�12λ
        entry[0] = (entry[0] & 0x0F) | ((clusNum << 4) & 0xF0);  // ��4λ
        entry[1] = (clusNum >> 4) & 0xFF;         // ��8λ
    }

    fseek(diskFile, 512, SEEK_SET); // �ƶ����ļ���ͷ
    fwrite(disk.FAT1, sizeof(disk.FAT1), 1, diskFile); // д�� FAT1

    // ���� FAT2 ������ FAT1 ����
    fseek(diskFile, 10 * 512, SEEK_SET); // �ƶ��� FAT2 ��λ��
    fwrite(disk.FAT2, sizeof(disk.FAT2), 1, diskFile); // д�� FAT2

}

//д��FAT������ֵ
void setFATEntry(uint16_t clusterNum, uint16_t value) {
    // ��λ��FAT����
    FATEntry &entry = disk.FAT1[clusterNum / 2];

    if (clusterNum % 2 == 0) {
        // ż���غţ����� `data[0]` �� `data[1]`
        entry.data[0] = value & 0xFF;          // ��8λ
        entry.data[1] = (entry.data[1] & 0xF0) | ((value >> 8) & 0x0F); // ��4λ
    } else {
        // �����غţ����� `data[1]` �� `data[2]`
        entry.data[1] = (entry.data[1] & 0x0F) | ((value << 4) & 0xF0); // ��4λ
        entry.data[2] = (value >> 4) & 0xFF;  // ��8λ
    }
}



uint16_t getClusValue(uint16_t clusNum) {
    FATEntry& entry = disk.FAT1[clusNum / 2];
    uint16_t value;

    if (clusNum % 2 == 0) {
        value = entry.data[0] | ((entry.data[1] & 0x0F) << 8);
    } else {
        value = (entry.data[0] >> 4) | (entry.data[1] << 4);
    }
    return value;
}

//��ȡ���еĴغ�
uint16_t getFreeClusNum() {
    for (uint16_t i = 2; i < sizeof(disk.FAT1) * 2 / 3; ++i) {
        uint16_t entryValue = getClusValue(i);
        if (entryValue == 0x000) {  // �ҵ����д�
            return i;
        }
    }
    return 0xFFF;  // ��ʾû�п��д�
}

//����dotĿ¼��
void createDotDirectory(RootEntry *dotEntry, uint8_t cur_clu_num) {
    memcpy(dotEntry, &disk.rootDirectory, sizeof(RootEntry));

    memset(dotEntry->DIR_Name, 0x20, sizeof(dotEntry->DIR_Name));
    memset(dotEntry->DIR_Name, 0x2E, 1);

    dotEntry->DIR_FstClus = cur_clu_num;
    dotEntry->DIR_Attr = DIRECTORY_CODE;

    memset(dotEntry->DIR_reserve, 0x00, 10);
    dotEntry->DIR_FileSize = 0;
}

//����..Ŀ¼��
void createDotDotDirectory(RootEntry *dotdotENtry) {

    memset(dotdotENtry->DIR_Name, 0x20, sizeof(dotdotENtry->DIR_Name));
    dotdotENtry->DIR_Name[0] = '.';
    dotdotENtry->DIR_Name[1] = '.';
    dotdotENtry->DIR_Name[2] = 0x20;


    dotdotENtry->DIR_Attr = DIRECTORY_CODE;
    memset(dotdotENtry->DIR_reserve, 0x00, 10);
    setTime(*dotdotENtry);
    dotdotENtry->DIR_FileSize = 0;
    dotdotENtry->DIR_FstClus = 0; // ddot��FstClus�ֶ�Ϊ��Ŀ¼FstClus��
    // ����Ŀ¼Ϊ��Ŀ¼��������Ϊ0
}

//mkdirʱд��������
void writeRootEntry(uint16_t clus_num, RootEntry &entry, uint8_t flag) {
    uint8_t ENTRY_SIZE = 32;
    uint32_t entryOffset = (31 + clus_num) * 512;
    if (flag == 0) {
        entryOffset = (31 + clus_num) * 512;
    } else if (flag == 1) {
        entryOffset = (31 + clus_num) * 512 + 32;
    } else {
        entryOffset = (31 + clus_num) * 512 + 64;
    }
    fseek(diskFile, entryOffset, SEEK_SET);
    // �� rootEntry д�� VFD �ļ�
    if (fwrite(&entry, ENTRY_SIZE, 1, diskFile) != 1) {
        cerr << "д��ʧ�ܣ���������ļ�״̬��Ȩ��rb+" << endl;
    }
}

//д����Ŀ¼
void write_to_directory_root(RootEntry &rootEntry) {
    const uint32_t ROOT_DIRECTORY_OFFSET = 19 * 512; // ��Ŀ¼��ʼƫ����
    const uint16_t ROOT_ENTRIES = 224;               // ��Ŀ¼��Ŀ��
    const uint16_t ENTRY_SIZE = 32;                  // ÿ��RootEntry��Ŀ��С

    for (uint16_t i = 0; i < ROOT_ENTRIES; i++) {
        uint32_t entryOffset = ROOT_DIRECTORY_OFFSET + i * ENTRY_SIZE;
        // ��λ����ǰRootEntry��Ŀ
        fseek(diskFile, entryOffset, SEEK_SET);
        RootEntry tempEntry;
        fread(&tempEntry, ENTRY_SIZE, 1, diskFile);
        // �ҵ�������Ŀ
        if (tempEntry.DIR_Name[0] == 0x00) {
            // �ص�������Ŀλ��
            fseek(diskFile, entryOffset, SEEK_SET);
            fwrite(&rootEntry, ENTRY_SIZE, 1, diskFile);
            return;
        }
    }
    cout << "��Ŀ¼�������޷�д���µ���Ŀ" << endl;
}

//����ǰdisk.rootDirectory�滻Ϊ��Ŀ¼��Ŀ¼����
void go_back_to_directory_position() {
    now_clu = 2;
    uint16_t offset = DIRECTORY_POS * SECTOR_SIZE;
    fseek(diskFile, offset, SEEK_SET);
    fread(disk.rootDirectory, 1, disk.MBR.BPB_RootEntCnt, diskFile);
    cout << "�ѻص���Ŀ¼" << endl;
}

//����ǰdisk.rootDIrctory�滻Ϊָ��Ŀ¼(������)
void navigator_to_specific_directory_position(uint8_t cluster_num) {
    uint16_t offset = (DATA_POS + (cluster_num - 2)) * SECTOR_SIZE;
    fseek(diskFile, offset, SEEK_SET);
    fread(disk.rootDirectory, 1, disk.MBR.BPB_RootEntCnt, diskFile);
    now_clu = cluster_num;
}

//pwd
void pwd() {
    stack<string> tempStack = pathStack;
    string path;
    // ��ջ�׵�ջ��ƴ��·��
    stack<string> reverseStack;
    while (!tempStack.empty()) {
        reverseStack.push(tempStack.top());
        tempStack.pop();
    }
    // ֱ��ƴ��·��������ӷָ���
    while (!reverseStack.empty()) {
        path += reverseStack.top();
        reverseStack.pop();
    }
    cout << path << endl;
}

//��Ǵ˴��Ѿ�ʹ��
void usedClus(uint16_t clusNum){
    uint8_t *entry = &disk.FAT1[clusNum / 2].data[clusNum % 2];  // ��ӦFAT������ֽڵ�ַ

    if (!(clusNum % 2)) {
        entry[0] = 0xFF;                  // ��0�ֽ�ȫ��ΪF
        entry[1] = entry[1] | 0x0F ;
    } else {
        entry[0] = entry[0] | 0xF0;
        entry[1] = 0xFF;             // ��1�ֽ�ȫ��ΪF
    }

    fseek(diskFile, 512, SEEK_SET); // �ƶ����ļ���ͷ
    fwrite(disk.FAT1, sizeof(disk.FAT1), 1, diskFile); // д�� FAT1

    // ���� FAT2 ������ FAT1 ����
    fseek(diskFile, 10 * 512, SEEK_SET); // �ƶ��� FAT2 ��λ��
    fwrite(disk.FAT2, sizeof(disk.FAT2), 1, diskFile); // д�� FAT2
}

//����ʱ��
void parseTime(RootEntry entry) {
    //����
    uint16_t lastWriteTime = (entry.DIR_WrtTime[1] << 8) | entry.DIR_WrtTime[0];
    uint16_t lastWriteDate = (entry.DIR_WrtDate[1] << 8) | entry.DIR_WrtDate[0];
    // �������һ��д��ʱ��
    int hours = (lastWriteTime >> 11) & 0x1F; // ��ȡСʱ
    int minutes = (lastWriteTime >> 5) & 0x3F; // ��ȡ����
    // �������һ��д������
    int year = ((lastWriteDate >> 9) & 0x7F) + 1980; // ��ȡ���
    int month = (lastWriteDate >> 5) & 0x0F; // ��ȡ�·�
    int day = lastWriteDate & 0x1F; // ��ȡ����
    cout << year << "/" << month << "/" << day << "," << hours << ":" << minutes << endl;
}

//touch�����е���������11λ���ֳ��뵽����
void setFileName(RootEntry &rootEntry, string str) {
    //1.�����ļ���
    string fileName = str.substr(6);
    uint8_t len = fileName.length();
    if (len > 11 || len < 2) {   // ����11�ַ���С��2�ַ�
        cout << "�ļ���������̫��" << endl;
        return;
    }
    // �ҵ��ļ�������չ���ķָ���
    uint16_t dotPosition = fileName.find('.');

    memset(rootEntry.DIR_Name, 0x20, sizeof(rootEntry.DIR_Name));  // ��DIR_Name��ʼ��Ϊ�ո����
    // �����ļ�������չ��
    string baseName = (dotPosition == string::npos) ? fileName : fileName.substr(0, dotPosition);
    string extension = (dotPosition == string::npos) ? "" : fileName.substr(dotPosition + 1);
    // ����ļ�������չ���ĳ�������
    if (baseName.length() > 8) {
        cerr << "�ļ������������8�ַ�" << endl;
        return;
    }
    if (extension.length() > 3) {
        cerr << "��չ�����������3�ַ�" << endl;
        return;
    }
    // ���ļ�������չ�������� rootEntry.DIR_Name �У��հ�λ���Զ��ÿո����
    memcpy(rootEntry.DIR_Name, baseName.c_str(), baseName.length());
    memcpy(rootEntry.DIR_Name + 8, extension.c_str(), extension.length());

}
void read_rootDir_from_vfd(char *vfd_file);
//touch����
void touch(string str) {
    RootEntry rootEntry;
    // 1. �����ļ�����
    setFileName(rootEntry, str);
    // 1.1 �ж��ظ�
    string fileName = str.substr(6);
    RootEntry *pEntry = findFile(fileName);
    if (pEntry != nullptr) {
        cout << "�ļ��Ѵ���" << endl;
        return;
    }
    // 2. �����ļ�����
    rootEntry.DIR_Attr = 0x00;
    // 3. 10������λ
    memset(rootEntry.DIR_reserve, 0, sizeof(rootEntry.DIR_reserve));
    // 4. ����ʱ��
    setTime(rootEntry);
    // 5. ����غŲ������ʹ��
    uint16_t a = getFreeClusNum();
    cout << "����Ĵغ���: " << a << endl;
    if (a == 0xFFF) {
        cout << "û�пɷ���Ĵغ�" << endl;
        return;
    }
    rootEntry.DIR_FstClus = a;
    usedClus(a);    //�������Ѿ���ʹ�ù���
    // ��������
    cout << "����������:";
    string content;
    getline(cin, content);

    // �����ļ���С
    rootEntry.DIR_FileSize = content.size();

    uint32_t remain_size = content.size();
    uint32_t written_size = 0;
    if (remain_size <= 512) {
        // ֱ��д�벢����
        uint32_t offset = (31 + a) * 512;
        fseek(diskFile, offset, SEEK_SET);
        fwrite(content.c_str(), sizeof(char), remain_size, diskFile);
        usedClus(a);  // ������һ����Ϊ����
    } else {
        // ������Ҫ��������
        uint16_t need_sector_num = (rootEntry.DIR_FileSize + 511) / 512;
        cout << "��Ҫ��������Ϊ��" << (int)need_sector_num << endl;

        //
        for (uint8_t i = 0; i < need_sector_num; i++) {
            //д��
            uint32_t offset = (31 + a) * 512;
            fseek(diskFile, offset, SEEK_SET);
            uint32_t bytesToWrite = (remain_size < 512) ? remain_size : 512;
            fwrite(content.c_str() + written_size, sizeof(char), bytesToWrite, diskFile);
            written_size += bytesToWrite;
            remain_size -= bytesToWrite;

            if (remain_size > 0) {
                // ��ȡ���д�,���Ұ��¸��ر��Ϊ��ʹ��
                uint16_t next_free_clus = getFreeClusNum();
                usedClus(next_free_clus);
                setFATEntry(a,next_free_clus); //�ɹ����ӵ���һ��
                a = next_free_clus;
            } else {
                // ���һ����ֱ�ӱ��Ϊ����
                usedClus(a);
            }
        }
    }
    // 6. ���¸�Ŀ¼��Ŀ
    write_to_directory_root(rootEntry);

    // 7. ˢ���ļ�������FAT��
    fflush(diskFile);
    read_fat_from_vfd(PATH);
    read_mbr_from_vfd(PATH);
}

void clearDataArea(uint16_t startClus) {
    uint16_t currentClus = startClus;

    while (currentClus != 0xFFF) { // ֱ�����һ����
        cout << "�����" <<currentClus <<"�Ŵ�" << endl;
        // �����������е�ǰ�ص�ƫ����
        uint32_t offset = (31 + currentClus) * 512; // ������������ƫ��
        // ����ǰ�ص����������
        fseek(diskFile, offset, SEEK_SET);
        char emptyBuffer[512] = {0}; // ����һ��ȫ��Ļ�����
        fwrite(emptyBuffer, 1, 512, diskFile);

        // ��ȡ��һ���غ�
        currentClus = getClus(&disk.FAT1[currentClus /2 ].data[currentClus %2],currentClus % 2);

    }
    //���һ������FF ������
}

void deleteFile(string filename) {
    // 1. �ڸ�Ŀ¼�в����ļ�
    string fileFullName = filename.substr(3); // �� "cat " ��ʼ
    RootEntry *pEntry = findFile(fileFullName);
    if (pEntry == nullptr) {
        cout << "�ļ�������" << endl;
        return;
    }
    cout << "��ɾ�����ļ���Ϣ" << pEntry->DIR_FstClus<<endl;

    // 2. ��ȡ�ļ�����ʼ�غ�
    uint16_t clus_num = pEntry->DIR_FstClus;
    uint16_t firstClus = clus_num;

    //3.ѭ���������������
    clearDataArea(clus_num);

    // 4. ���FAT�����е����д�
    while (clus_num != 0xFFF) {
        uint16_t next_clus = getClus(&disk.FAT1[clus_num / 2].data[clus_num % 2],clus_num %2);
        setFATEntry(clus_num, 0);  // ��Ǵ�Ϊ����
        clus_num = next_clus;
    }
    setFATEntry(firstClus, 0);  // ��Ǵ�Ϊ����


//    setFATEntry(firstClus, 0x000);  // ��Ǵ�Ϊ����

    //4.1 �ҵ����ĸ�Ŀ¼���Ǹ�32�ֽ�ɾ��
    uint32_t offset = (firstClus * sizeof(RootEntry)) + (31 * 512); // ��Ŀ¼�ӵ�31��������ʼ
    fseek(diskFile, offset, SEEK_SET); // ��λ��Ŀ¼��λ��
    memset(pEntry, 0, sizeof(RootEntry)); // ��Ŀ¼�����


    // 5. д�ظ��º��FAT��͸�Ŀ¼
    fseek(diskFile, 512, SEEK_SET); // �ƶ����ļ���ͷ
    fwrite(disk.FAT1, sizeof(disk.FAT1), 1, diskFile); // д�� FAT1

    // ���� FAT2 ������ FAT1 ����
    fseek(diskFile, 10 * 512, SEEK_SET);
    fwrite(disk.FAT2, sizeof(disk.FAT2), 1, diskFile);

    fseek(diskFile,(1+9+9)*512,SEEK_SET);
    fwrite(disk.rootDirectory, 7168, 1, diskFile);


    // 6. ����FAT��
    read_fat_from_vfd(PATH);
    read_mbr_from_vfd(PATH);
    read_rootDir_from_vfd(PATH);
    cout << "�ļ�ɾ���ɹ�" << endl;
}
