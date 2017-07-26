#ifndef HEADER_H
#define HEADER_H
#include <stdio.h>
#include <string.h>

#define BLOCK_SIZE 512                    //磁盘块大小512B

#define TOTAL_BLOCK_NUMBER 531           //本虚拟磁盘一共是531个磁盘块
#define INODE_NUM 128                     //索引节点数目128

#define INODE_BITMAP_START 1              //索引节点位示图开始磁盘块号 1
#define INODE_BITMAP_BKNUM 1              //索引节点位示图占据磁盘块数 1
#define INODE_BLOCK_START 2               //索引节点开始磁盘块号 2
#define INODE_BLOCK_NUM 16                //索引节点所占的磁盘块数目为16
#define INODE_NUM_PER_BLOCK 8            //每个磁盘块所拥有的索引节点数目为8
#define INODE_SIZE 60                     //每个索引节点占据60B大小的空间

#define DATA_BITMAP_START 18              //数据块位示图开始磁盘块号 18
#define DATA_BITMAP_BLNUM 1              //数据块位示图占据磁盘块数 1
#define DATA_BLOCK_START 19              //数据块开始磁盘块号 19
#define DATA_BLOCK_NUM  512               //数据块占据磁盘块数目  512


#define MAX_DISK_ADDRESS 13              //索引节点的寻址项，0~9为直接寻址，10,11为一次间址，12为二次间址

typedef struct region_t{
    int start_block;   // 起始块，每个region从块边界开始
    int block_count;   // region占用的块个数
    int byte_count;    // region占用的字节个数 
                       // byte_count <= block_count * BLOCK_SIZE
}region_t;
//超级块的定义
typedef struct super_block{
	int magic;
    int total_block_count;        //规定总共的磁盘块的数目
    int inode_count;              //规定总共的索引节点数目
	int data_block_count;         //规定总共的数据块的数目
    region_t inode_table_bitmap;  //索引节点位图
    region_t inode_table;        //索引节点表
    region_t data_area_bitmap;   //数据块位图
    region_t data_area;         //数据块区域
}super_block_t;


enum Type{
    INODE_REGULAR, // 普通文件，文本文件或则二进制文件
    INODE_DIR     // 目录文件
};

//索引节点，每个索引节点大小占60B,每个磁盘块只能存放8个索引节点
typedef struct inode_t{
    int type;      // 文件类型,是文件还是文件夹
    int size;      // 文件大小
    int disk_address[MAX_DISK_ADDRESS];  // 寻磁盘块方式，0~9为直接查找磁盘块(文件最大可以为5120B)
	                                     //10为一次间址（文件最大可为64KB）
	                                     //11为两次间址（文件最大可为8MB）
}inode_t;

//文件目录节点，每一个文件目录项占16B
typedef struct dir_entry 
{
	char name[12];  // 目录项名称
    int ino;        // 索引节点号
}directory;


#endif