//
// Created by ����԰ on 2024/10/21.
//
#include "bits/stdc++.h"  //���ؿ���ֱ����������ͷ

/**
* ������̵���Ϣ����δ�����ʦ�Ĵ��̣�
*/
#define SECTOR_SIZE 512  //������СĬ��512
#define DATA_SECTOR_NUM 2847  //����������
#define ROOT_DICT_NUM 224  //��Ŀ¼��
#define FAT_SECTOR_NUM 9 //FAT�������� 9
#define DISK_ALL_SIZE 2880

/**
* ������������
*/
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef char Sector[SECTOR_SIZE];   //��������С

/**
* FAT����ṹ
 * ע��ȡ���ֽڶ���
*/
typedef struct FATEntry {
    int firstEntry: 12;
    int secondEntry: 12;
} __attribute__((packed)) FATEntry;

/**
 * MBR����
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
} __attribute__((packed)) MBRHeader;

/**
 *  ��Ŀ¼����Ŀ��ʽ
 */
typedef struct RootEntry {
    uint8_t DIR_Name[11];//�ļ�������չ��
    uint8_t DIR_Attr;//�ļ�����
    uint8_t DIR_reserve[10];//����λ
    uint8_t DIR_WrtTime[2];//���һ��д��ʱ��
    uint8_t DIR_WrtDate[2];//���һ��д������
    uint16_t DIR_FstClus;//�ļ���ʼ�Ĵغ�
    uint32_t DIR_FileSize;//�ļ���С
} __attribute__((packed)) RootEntry;

/**
 * �������FAT12����
 */
struct Disk {
    MBRHeader MBR;  //1������
    FATEntry FAT1[192];     //9������ ȫ����ʾ������
    FATEntry FAT2[192];     // copy fat1
    RootEntry rootDirectory[ROOT_DICT_NUM];   //
    Sector dataSector[DATA_SECTOR_NUM]; //2880������
};

void InitMBR(Disk *disk);

void print_MBR(MBRHeader *temp);

void InitFAT();

void read_mbr_from_vfd(char *vfd_file);
void dir();



