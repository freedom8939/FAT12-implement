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
RootEntry *findFile(const string &fileName);

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
    fseek(diskFile, 10 * 512 , SEEK_SET); // �ƶ��� FAT2 ��λ��
    fwrite(disk.FAT2, sizeof(disk.FAT2), 1, diskFile); // д�� FAT2

}

//��ȡ���еĴغ�
uint16_t getFreeClusNum() {
    for (uint8_t i = 2; i < 1536; i++) {
        uint16_t freeClusNum = getClus(&disk.FAT1[i / 2].data[i % 2], i % 2);
        if (freeClusNum == 0) { // freeClusNum ���д�

            return i; // ���ؿ��дغ�
        }
    }
    return 0xFFF; // û�п��д�
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
void setDirName(RootEntry &rootEntry,string str){
    //1.�����ļ���
    string fileName = str.substr(6);
    uint8_t len = fileName.length();
    if (len > 11 || len < 2) {   // ����11�ַ���С��2�ַ�
        cerr << "�ļ���������̫��" << endl;
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

//touch����
void touch(string str){
    RootEntry rootEntry;
    //1.�����ļ�����
    setDirName(rootEntry,str);

    //2.�����ļ�����
    rootEntry.DIR_Attr = 0x00;

    //3.10������λ
    memset(rootEntry.DIR_reserve, 0, sizeof(rootEntry.DIR_reserve));

    //4.����ʱ��
    setTime(rootEntry);

    //5.����غ�
    uint16_t clus_num = getFreeClusNum();
    if (clus_num == 0xFFF) {
        cout << "û�пɷ���Ĵغ�" << endl;
    }
    setClus(clus_num);
    rootEntry.DIR_FstClus = clus_num;

    //6.�ļ���С���� TODO �Ժ��趨  ��Ҫ���ݶ�ȡ����Ĵ�С�����伸������
    //6.1���ٲ�����ջ�����
    char buffer[512];  // ����512�ֽڵĻ�����
    memset(buffer, 0, sizeof(buffer));
    //6.2��������
    cout << "��������Ҫ���������:" ;
    cin.getline(buffer,sizeof(buffer));
    //6.3���ú��ļ��Ĵ�С
    rootEntry.DIR_FileSize = strlen(buffer);
    cout << "������ļ���СΪ��" << rootEntry.DIR_FileSize << " �ֽ�" << std::endl;

    //6.4д���������õ��� Ȼ�����д�������������� �����дغ���
    uint32_t offset = (31 + clus_num) * 512;
    fseek(diskFile, offset, SEEK_SET);
    fwrite(buffer, sizeof(char), rootEntry.DIR_FileSize, diskFile); // д���ļ�����


    //7.��д����Ŀ¼ ������todo�޸�
    cout << "д���ļ��ɹ�" << endl;
    write_to_directory_root(rootEntry);

    //8.���¶�ȡfat����
    read_fat_from_vfd(PATH);
    read_mbr_from_vfd(PATH);
    fflush(diskFile);

}