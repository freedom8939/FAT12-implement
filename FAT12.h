/**
 * Created by 王金园 on 2024/10/21.
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
* 定义磁盘的信息（并未引入教师的磁盘)
*/
#define SECTOR_SIZE 512        //扇区大小默认512
#define DATA_SECTOR_NUM 2847   //数据扇区数
#define ROOT_DICT_NUM 224      //根目录数
#define DIRECTORY_CODE 0x10
#define FAT_SECTOR_NUM 9        //FAT扇区数
#define DIRECTORY_POS  19  //目录去开始的位置
#define DATA_POS 33   //数据区开始的位置
#define FAT_SIZE 4608   //FAT区的总字节大小

/**
* 定义数据类型
*/
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef char Sector[SECTOR_SIZE];   //数据区大小
#pragma pack(1)
/**
 * FAT表项结构
 */
typedef struct FATEntry {
    uint8_t data[3]; //3字节 24位  两个簇号
} FATEntry;

/**
 * MBR
 */
typedef struct MBRHeader {
    char BS_jmpBOOT[3];
    char BS_OEMName[8];
    // 每个扇区字节数 512
    uint16_t BPB_BytesPerSec;
    // 每簇扇区数 1
    uint8_t BPB_SecPerClus;
    // boot引导占扇区数 1
    uint16_t BPB_ResvdSecCnt;
    //一共有几个FAT表 2
    uint8_t BPB_NumFATs;
    //根目录文件最大数  0xe0 = 224
    uint16_t BPB_RootEntCnt;
    //扇区总数  0xb40 = 2880
    uint16_t BPB_TotSec16;
    //介质描述  0xf0
    uint8_t BPB_Media;
    //每个FAT表占扇区数 9
    uint16_t BPB_FATSz16;
    // 每个磁道占扇区数 0x12
    uint16_t BPB_SecPerTrk;
    // 磁头数   0x2
    uint16_t BPB_NumHeads;
    // 隐藏扇区数 0
    uint32_t BPB_HiddSec;
    // 如果BPB_TotSec16=0,则由这里给出扇区数 0
    uint32_t BPB_TotSec32;
    // INT 13H的驱动号 0
    uint8_t BS_DrvNum;
    //保留，未用    0
    uint8_t BS_Reserved1;
    //扩展引导标记  0x29
    uint8_t BS_BootSig;
    // 卷序列号 0
    uint32_t BS_VollD;
    // 卷标 'yxr620'
    uint8_t BS_VolLab[11];
    // 文件系统类型 'FAT12'
    uint8_t BS_FileSysType[8];
    //引导代码
    char code[448];
    //结束标志
    char end_point[2];
} MBRHeader;

/**
 *  根目录区格式
 */
typedef struct RootEntry {
    uint8_t DIR_Name[11];//文件名与扩展名
    uint8_t DIR_Attr;//文件属性
    uint8_t DIR_reserve[10];//保留位
    uint8_t DIR_WrtTime[2];//最后一次写入时间
    uint8_t DIR_WrtDate[2];//最后一次写入日期
    uint16_t DIR_FstClus;//文件开始的簇号
    uint32_t DIR_FileSize;//文件大小
} RootEntry;

/**
 * 定义磁盘
 */
struct Disk {
    MBRHeader MBR;  //1个扇区
    FATEntry FAT1[1536];     // 9个扇区 全部表示出来的
    FATEntry FAT2[1536];     // copy fat1
    RootEntry rootDirectory[ROOT_DICT_NUM];   //根目录区
    Sector dataSector[DATA_SECTOR_NUM]; //2880个扇区
};
extern Disk disk;
#pragma pack()

//初始化MBR
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

//打印MBR
void print_MBR(MBRHeader *temp) {
    printf("Disk Name - 磁盘名称: %s\n", temp->BS_OEMName);
    printf("Sector Size - 扇区大小: %d\n", temp->BPB_BytesPerSec);
    printf("Sectors per cluster - 每簇扇区数: %d\n", temp->BPB_SecPerClus);
    printf("Number of sectors occupied by boot - 引导占用的扇区数: %d\n", temp->BPB_ResvdSecCnt);
    printf("Number of FATs - FAT 表数量: %d\n", temp->BPB_NumFATs);
    printf("The max capacity of root entry - 根目录最大条目数: %d\n", temp->BPB_RootEntCnt);
    printf("The number of all Sectors - 扇区总数: %d\n", temp->BPB_TotSec16);
// printf("Media Descriptor - 媒体描述符: %c\n", temp->BPB_Media);
    printf("The number of Sectors of each Fat table - 每个 FAT 表占用的扇区数: %d\n", temp->BPB_FATSz16);
    printf("Number of sectors per track - 每个磁道的扇区数: %d\n", temp->BPB_SecPerTrk);
    printf("The number of magnetic read head - 磁头数量: %d\n", temp->BPB_NumHeads);
    printf("Number of hidden sectors - 隐藏扇区数: %d\n", temp->BPB_HiddSec);
    printf("Volume serial number - 卷序列号: %d\n", temp->BS_VollD);
    printf("Volume label - 卷标: ");
    for (int i = 0; i < 11; i++) {
        printf("%c", temp->BS_VolLab[i]);
    }
    printf("\n");
    printf("File system type - 文件系统类型: ");
    for (int i = 0; i < 8; i++) {
        printf("%c", temp->BS_FileSysType[i]);
    }
    printf("\n");
}

//初始化FAT表
void InitFAT() {
    memset(disk.FAT1, 0x00, sizeof(disk.FAT1));
    memset(disk.FAT2, 0x00, sizeof(disk.FAT2));

    // 初始化第一个 FAT 表条目
//    disk.FAT1[0].firstEntry = 0xFF0;  // 第一个条目表示介质类型（0xF0表示软盘）
//    disk.FAT1[0].secondEntry = 0xFFF; // 第二个条目表示结束标记

    // 将 FAT1 复制到 FAT2
    memcpy(disk.FAT2, disk.FAT1, sizeof(disk.FAT1));

    // 初始化其他 FAT 表条目（根据需要设置）
    for (int i = 1; i < sizeof(disk.FAT1) / sizeof(FATEntry); i++) {
//        disk.FAT1[i].firstEntry = 0x000;  // 未使用的簇
//        disk.FAT1[i].secondEntry = 0x000; // 未使用的簇
    }

    // 复制初始化后的 FAT1 到 FAT2
    memcpy(disk.FAT2, disk.FAT1, sizeof(disk.FAT1));
}

//从VFD中读出mbr
void read_mbr_from_vfd(char *vfd_file);

//进入文件夹命令
void cd(string &_name);

//查看文件
void cat(string &_name);

//转字符串为小写
void toLowerCase(string &str) {
    for (char &c: str) {
        c = tolower(c); // 将每个字符转换为小写
    }
}

//读取数据
void readFileData(uint16_t firstCluster, uint32_t fileSize);

//根目录下查找文件FATEntry
RootEntry *findFile(const string &fileName);

//执行命令
void executeCommand(string &command);

//从vfd读取fat表到本地磁盘
void read_fat_from_vfd(char *vfd_file);

//查看所有目录
void dir();

//获取簇号
unsigned short getClus(unsigned char *buffer, char flag);

// 命令列表
string command;

void showCommandList() {
    cout << "可使用命令:" << endl;
    cout << "(1):dir" << endl;
    cout << "(2):cat 文件名" << endl;
    cout << "(3):cd 文件夹" << endl;
    cout << "(4):pwd" << endl;
    cout << "(0):exit" << endl;
}

//当前根目录是否有子文件夹(已排除.和..)
bool hasSubdirectories();

//开两个栈而非扔进去一个Entry
//目录栈(簇号)
stack<uint16_t> clusterStack; // 用来存储每一层目录的簇号
//目录栈（路径名）
stack<string> pathStack;

//做测试用
void te(const stack<uint16_t> &clusterStack) {
    // 创建一个临时容器来存储栈中的元素
    vector<uint16_t> tempStack;
    stack<uint16_t> temp = clusterStack; // 复制栈

    // 将栈元素转移到临时容器
    while (!temp.empty()) {
        tempStack.push_back(temp.top());
        temp.pop();
    }

    // 输出临时容器中的元素
    cout << "当前目录簇号栈：";
    for (size_t i = 0; i < tempStack.size(); ++i) {
        cout << tempStack[i];
        if (i < tempStack.size() - 1) {
            cout << " -> "; // 只在元素之间添加分隔符
        }
    }
    bool subdirectories = hasSubdirectories();
    cout << endl;
//    if (subdirectories) {
//        cout << "有子目录" << endl;
//    } else {
//        cout << "没有子目录" << endl;
//    }

}

//初始化FAT和MBR
void Init() {
    read_mbr_from_vfd(PATH);
    read_fat_from_vfd(PATH);
    clusterStack.push(2); //初始化
    pathStack.push("/");

}

//获取当前所在簇号
uint32_t getNowClu() {
    return now_clu;
}

//是否有子目录
bool hasSubdirectories() {
    for (uint16_t i = 0; i < disk.MBR.BPB_RootEntCnt; i++) {
        // 检查目录项是否有效
        if (disk.rootDirectory[i].DIR_Name[0] == 0) continue;

        // 检查是否是目录
        if (disk.rootDirectory[i].DIR_Attr & DIRECTORY_CODE) {
            // 排除特殊的 . 和 .. 目录
            uint8_t file_name[9];
            memcpy(file_name, disk.rootDirectory[i].DIR_Name, 8);
            file_name[8] = '\0';
            string dirName(reinterpret_cast<char *>(file_name));
            dirName = dirName.substr(0, dirName.find_last_not_of(' ') + 1);

            if (dirName != "." && dirName != "..") {
                return true; // 找到子目录
            }
        }
    }
    return false; // 没有找到子目录
}

//创建文件夹
void mkdir(string &dirName);

//设置wrTime时间等
void setTime(RootEntry &entry);

//写入FAT表 ,
void setClus(uint16_t clusNum) {
    uint8_t *entry = &disk.FAT1[clusNum / 2].data[clusNum % 2];  // 对应FAT表项的字节地址

    if (!(clusNum % 2)) {
        // 偶数簇号：第一个簇的低12位
        entry[0] = clusNum & 0xFF;                // 低8位
        entry[1] = (entry[1] & 0xF0) | ((clusNum >> 8) & 0x0F);  // 高4位
    } else {
        // 奇数簇号：第二个簇的高12位
        entry[0] = (entry[0] & 0x0F) | ((clusNum << 4) & 0xF0);  // 低4位
        entry[1] = (clusNum >> 4) & 0xFF;         // 高8位
    }

    fseek(diskFile, 512, SEEK_SET); // 移动到文件开头
    fwrite(disk.FAT1, sizeof(disk.FAT1), 1, diskFile); // 写入 FAT1

    // 假设 FAT2 紧接在 FAT1 后面
    fseek(diskFile, 10 * 512 , SEEK_SET); // 移动到 FAT2 的位置
    fwrite(disk.FAT2, sizeof(disk.FAT2), 1, diskFile); // 写入 FAT2

}

//获取空闲的簇号
uint16_t getFreeClusNum() {
    for (uint8_t i = 2; i < 1536; i++) {
        uint16_t freeClusNum = getClus(&disk.FAT1[i / 2].data[i % 2], i % 2);
        if (freeClusNum == 0) { // freeClusNum 空闲簇

            return i; // 返回空闲簇号
        }
    }
    return 0xFFF; // 没有空闲簇
}

//创建dot目录项
void createDotDirectory(RootEntry *dotEntry, uint8_t cur_clu_num) {
    memcpy(dotEntry, &disk.rootDirectory, sizeof(RootEntry));

    memset(dotEntry->DIR_Name, 0x20, sizeof(dotEntry->DIR_Name));
    memset(dotEntry->DIR_Name, 0x2E, 1);

    dotEntry->DIR_FstClus = cur_clu_num;
    dotEntry->DIR_Attr = DIRECTORY_CODE;

    memset(dotEntry->DIR_reserve, 0x00, 10);
    dotEntry->DIR_FileSize = 0;
}

//创建..目录项
void createDotDotDirectory(RootEntry *dotdotENtry) {

    memset(dotdotENtry->DIR_Name, 0x20, sizeof(dotdotENtry->DIR_Name));
    dotdotENtry->DIR_Name[0] = '.';
    dotdotENtry->DIR_Name[1] = '.';
    dotdotENtry->DIR_Name[2] = 0x20;


    dotdotENtry->DIR_Attr = DIRECTORY_CODE;
    memset(dotdotENtry->DIR_reserve, 0x00, 10);
    setTime(*dotdotENtry);
    dotdotENtry->DIR_FileSize = 0;
    dotdotENtry->DIR_FstClus = 0; // ddot的FstClus字段为父目录FstClus，
    // 若父目录为根目录，则设置为0
}

//mkdir时写入数据区
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
    // 将 rootEntry 写入 VFD 文件
    if (fwrite(&entry, ENTRY_SIZE, 1, diskFile) != 1) {
        cerr << "写入失败，请检查磁盘文件状态和权限rb+" << endl;
    }
}

//写到根目录
void write_to_directory_root(RootEntry &rootEntry) {
    const uint32_t ROOT_DIRECTORY_OFFSET = 19 * 512; // 根目录起始偏移量
    const uint16_t ROOT_ENTRIES = 224;               // 根目录条目数
    const uint16_t ENTRY_SIZE = 32;                  // 每个RootEntry条目大小

    for (uint16_t i = 0; i < ROOT_ENTRIES; i++) {
        uint32_t entryOffset = ROOT_DIRECTORY_OFFSET + i * ENTRY_SIZE;
        // 定位到当前RootEntry条目
        fseek(diskFile, entryOffset, SEEK_SET);
        RootEntry tempEntry;
        fread(&tempEntry, ENTRY_SIZE, 1, diskFile);
        // 找到空闲条目
        if (tempEntry.DIR_Name[0] == 0x00) {
            // 回到空闲条目位置
            fseek(diskFile, entryOffset, SEEK_SET);
            fwrite(&rootEntry, ENTRY_SIZE, 1, diskFile);
            return;
        }
    }
    cout << "根目录已满，无法写入新的条目" << endl;
}

//将当前disk.rootDirectory替换为根目录（目录区）
void go_back_to_directory_position() {
    now_clu = 2;
    uint16_t offset = DIRECTORY_POS * SECTOR_SIZE;
    fseek(diskFile, offset, SEEK_SET);
    fread(disk.rootDirectory, 1, disk.MBR.BPB_RootEntCnt, diskFile);
    cout << "已回到根目录" << endl;
}

//将当前disk.rootDIrctory替换为指定目录(数据区)
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
    // 从栈底到栈顶拼接路径
    stack<string> reverseStack;
    while (!tempStack.empty()) {
        reverseStack.push(tempStack.top());
        tempStack.pop();
    }
    // 直接拼接路径，不添加分隔符
    while (!reverseStack.empty()) {
        path += reverseStack.top();
        reverseStack.pop();
    }
    cout << path << endl;
}

//解析时间
void parseTime(RootEntry entry) {
    //解析
    uint16_t lastWriteTime = (entry.DIR_WrtTime[1] << 8) | entry.DIR_WrtTime[0];
    uint16_t lastWriteDate = (entry.DIR_WrtDate[1] << 8) | entry.DIR_WrtDate[0];
    // 解析最后一次写入时间
    int hours = (lastWriteTime >> 11) & 0x1F; // 获取小时
    int minutes = (lastWriteTime >> 5) & 0x3F; // 获取分钟
    // 解析最后一次写入日期
    int year = ((lastWriteDate >> 9) & 0x7F) + 1980; // 获取年份
    int month = (lastWriteDate >> 5) & 0x0F; // 获取月份
    int day = lastWriteDate & 0x1F; // 获取日期
    cout << year << "/" << month << "/" << day << "," << hours << ":" << minutes << endl;
}

//touch命令中的设置名称11位部分抽离到这里
void setDirName(RootEntry &rootEntry,string str){
    //1.处理文件名
    string fileName = str.substr(6);
    uint8_t len = fileName.length();
    if (len > 11 || len < 2) {   // 超过11字符或小于2字符
        cerr << "文件名过长或太短" << endl;
        return;
    }
    // 找到文件名和扩展名的分隔点
    uint16_t dotPosition = fileName.find('.');

    memset(rootEntry.DIR_Name, 0x20, sizeof(rootEntry.DIR_Name));  // 将DIR_Name初始化为空格填充
    // 分离文件名和扩展名
    string baseName = (dotPosition == string::npos) ? fileName : fileName.substr(0, dotPosition);
    string extension = (dotPosition == string::npos) ? "" : fileName.substr(dotPosition + 1);
    // 检查文件名和扩展名的长度限制
    if (baseName.length() > 8) {
        cerr << "文件名过长，最多8字符" << endl;
        return;
    }
    if (extension.length() > 3) {
        cerr << "扩展名过长，最多3字符" << endl;
        return;
    }
    // 将文件名和扩展名拷贝到 rootEntry.DIR_Name 中，空白位置自动用空格填充
    memcpy(rootEntry.DIR_Name, baseName.c_str(), baseName.length());
    memcpy(rootEntry.DIR_Name + 8, extension.c_str(), extension.length());

}

//touch命令
void touch(string str){
    RootEntry rootEntry;
    //1.设置文件名称
    setDirName(rootEntry,str);

    //2.设置文件属性
    rootEntry.DIR_Attr = 0x00;

    //3.10个保留位
    memset(rootEntry.DIR_reserve, 0, sizeof(rootEntry.DIR_reserve));

    //4.设置时间
    setTime(rootEntry);

    //5.分配簇号
    uint16_t clus_num = getFreeClusNum();
    if (clus_num == 0xFFF) {
        cout << "没有可分配的簇号" << endl;
    }
    setClus(clus_num);
    rootEntry.DIR_FstClus = clus_num;

    //6.文件大小设置 TODO 稍后设定  需要根据读取区域的大小来分配几个扇区
    //6.1开辟并且清空缓冲区
    char buffer[512];  // 创建512字节的缓冲区
    memset(buffer, 0, sizeof(buffer));
    //6.2输入内容
    cout << "请输入想要存入的内容:" ;
    cin.getline(buffer,sizeof(buffer));
    //6.3设置好文件的大小
    rootEntry.DIR_FileSize = strlen(buffer);
    cout << "输入的文件大小为：" << rootEntry.DIR_FileSize << " 字节" << std::endl;

    //6.4写道数据区得到簇 然后把他写进数据区即可了 不是有簇号吗
    uint32_t offset = (31 + clus_num) * 512;
    fseek(diskFile, offset, SEEK_SET);
    fwrite(buffer, sizeof(char), rootEntry.DIR_FileSize, diskFile); // 写入文件内容


    //7.先写到根目录 后期在todo修改
    cout << "写入文件成功" << endl;
    write_to_directory_root(rootEntry);

    //8.重新读取fat表项
    read_fat_from_vfd(PATH);
    read_mbr_from_vfd(PATH);
    fflush(diskFile);

}