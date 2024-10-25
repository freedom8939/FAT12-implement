//
// Created by 王金园 on 2024/10/21.
//
#include "bits/stdc++.h"  //本地开发直接引入万能头

/**
* 定义磁盘的信息（并未引入教师的磁盘）
*/
#define SECTOR_SIZE 512  //扇区大小默认512
#define DATA_SECTOR_NUM 2847  //数据扇区数
#define ROOT_DICT_NUM 224  //根目录数
#define FAT_SECTOR_NUM 9 //FAT扇区数字 9
#define DISK_ALL_SIZE 2880

/**
* 定义数据类型
*/
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef char Sector[SECTOR_SIZE];   //数据区大小

/**
* FAT表项结构
 * 注意取消字节对齐
*/
typedef struct FATEntry {
    int firstEntry: 12;
    int secondEntry: 12;
} __attribute__((packed)) FATEntry;

/**
 * MBR部分
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
} __attribute__((packed)) MBRHeader;

/**
 *  根目录区条目格式
 */
typedef struct RootEntry {
    uint8_t DIR_Name[11];//文件名与扩展名
    uint8_t DIR_Attr;//文件属性
    uint8_t DIR_reserve[10];//保留位
    uint8_t DIR_WrtTime[2];//最后一次写入时间
    uint8_t DIR_WrtDate[2];//最后一次写入日期
    uint16_t DIR_FstClus;//文件开始的簇号
    uint32_t DIR_FileSize;//文件大小
} __attribute__((packed)) RootEntry;

/**
 * 定义磁盘FAT12属性
 */
struct Disk {
    MBRHeader MBR;  //1个扇区
    FATEntry FAT1[192];     //9个扇区 全部表示出来的
    FATEntry FAT2[192];     // copy fat1
    RootEntry rootDirectory[ROOT_DICT_NUM];   //
    Sector dataSector[DATA_SECTOR_NUM]; //2880个扇区
};

void InitMBR(Disk *disk);

void print_MBR(MBRHeader *temp);

void InitFAT();

void read_mbr_from_vfd(char *vfd_file);
void dir();



