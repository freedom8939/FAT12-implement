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

#define PATH "G:\\OS-homework\\grp07\\wjyImg.vfd"
FILE *diskFile = fopen(PATH, "rb");
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
    FATEntry FAT1[1536];     //9������ ȫ����ʾ������
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

//����FAT�� ����FAT���ҵ�����FAT��
int findEmptyRootEntry() {  //1536
    for (int i = 0; i < 1536; i++) {
        if (disk.FAT1[i].data[0] == 0x00 &&
            disk.FAT1[i].data[1] == 0x00 &&
            disk.FAT1[i].data[2] == 0x00) {
            return i; //���شغ�
        }
    }
    return -1;
}

//������ô�
uint16_t allocateFATCluster() {
    for (uint16_t i = 2; i < DATA_SECTOR_NUM; i++) { // �Ӵغ�2��ʼ����
        unsigned short clusterStatus = getClus(&disk.FAT1[i / 2].data[i % 2], i % 2);
        if (clusterStatus == 0x000) { // �ҵ����еĴ�
            // ���ôر��Ϊ�ѷ���
            disk.FAT1[i / 2].data[i % 2] = 0xFFF;
            return i;
        }
    }
    return 0xFFFF; // �޿��ô�
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
    std::stack<std::string> tempStack = pathStack;
    std::string path;
    cout << "current path is: ";

    // ��ջ�׵�ջ��ƴ��·��
    std::stack<std::string> reverseStack;
    while (!tempStack.empty()) {
        reverseStack.push(tempStack.top());
        tempStack.pop();
    }

    // ֱ��ƴ��·��������ӷָ���
    while (!reverseStack.empty()) {
        path += reverseStack.top();
        reverseStack.pop();
    }

    std::cout << path << std::endl;
}

uint32_t getNowClu() {
    return now_clu;
}
