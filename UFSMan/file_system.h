#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H
//===================================================================================================
//对文件系统中需要的各种操作进行声明
//
//===================================================================================================
#include "Header.h"


extern char file_system_name[20];           //创建的磁盘文件名
extern directory dir_buf[BLOCK_SIZE/sizeof(directory)];   //目录数组，表示一个磁盘块可以存放32个目录项

bool Format();                       //格式化整个磁盘文件，如果没有，则创建该磁盘文件
                                     //格式化成功后，返回true；其中出现错误，则返回false
bool Install();                      //装载磁盘系统，此时从原先DOS界面进入新建的文件系统

bool Showhelp();                    //通过该函数向用户显示所有用户可以的操作指令以及操作格式


inode_t Find_Inode(int ino);                     //给定一个索引节点号，该函数返回一个inode_t类型的结果，从索引节点区
                                                 //将该节点对应的数据区提供给需要的函数
bool Create_File(char file_name[],int index_node);  //通过传入的文件名，在本文件系统中创建一个文件
                                                 //同时将文件内容存放在buffer中，buffer缓冲区占据1M内存
                                                //创建的文件存放在指定的目录(参数index_node所指定的目录文件)中
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//对位示图的更改只有下面的两个函数，所以当进入系统之后，将位示图读入内存后，不用当心其值被别人肆意更改
//
int Scan_Inode_Bitmap();                         //扫描索引节点位示图，返回一个可用的索引节点号，同时将对应的索引节点位示图
                                                 //相应位置1
bool Scan_Data_Bitmap(int data_block_count);     //扫描数据块位示图，传入的参数是需要的数据块的个数，按顺序扫描后，将空闲数据块
                                                 //的块号存入全局变量spare_datablk_num数组中
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Fill_Index_In_Datablk(int indexblock_no,int buf_block_count);//间接寻址时用到，在数据块中分配一个盘块用于专门指向大文件所需要的磁盘块

void Spill(char big[],char small[],int data_count,int size);  //将一个大的数据块分割成多个小数据块,data_count标识将big[]的第几块存放在small[]中

int Lookup_File(char *path);                                  //检索文件的核心函数为lookup_file

bool Copy_File(char filename1[],char filename2[]);                      //复制文件，支持当前文件夹下当前文件，也支持路径文件复制

bool isPath(char *name);                                         //判断该参数是文件名还是路径

int Lookup_Dir(int index_node);                              //检索目录，输入目录文件节点，结果是将目录节点对应的目录输入到全局变量dir_buf[]中
                                                             //同时返回该目录文件所在的磁盘块号

bool Read_File(int file_index);                             //读取文件，根据文件的索引节点号，将对应的文件的内容读取到内存缓冲buffer中                             

bool Get_Data(int block_index);                               //通过磁盘块号，将磁盘上的数据读入缓冲区Block_Data中

bool Merge(int count);                                       //合并函数，用于将每次读出的一个磁盘块上的数据Block_Data归并到Buffer中
                                                             //count为第几个磁盘块上的数据
bool Copy_File_out(char filename1[],char filename2[]);      //将本系统中的文件拷贝到物理磁盘上，参数2仅仅只有一个文件名
                                                           //参数1可以直接是文件名，也可以是带路径的文件名
bool Copy_File_In(char filename1[],char filename2[]);     //将物理磁盘上的文件拷贝到本文件系统中，参数1只有一个文件名
                                                          //参数2可以直接是文件名，也可以是带路径的文件名
bool Delete_File(char filename[]);                         //删除一个文件，只支持删除在当前文件夹下的文件

bool Delete_From_Inode_Bitmap(int file_index);           //根据给定的文件的索引节点，在索引节点位示图中将对应的bit位置0

bool Delete_From_Data_Bitmap(int block_index);           //根据给定的磁盘的索引节点，在数据块位示图中将对应的bit位置0

bool Clear_Dir_Buf();                                       //将缓存Dir_Buf清空

bool Show_Content(char file_name[]);                      //显示文件内容，如果参数给定的是路径，必须能够根据路径找到文件并显示其内容

bool New_file(char filename[]);                                         //创建一个新文件，文件内容有用户自己输入

bool isLegal(int dir_index,char filename[]);            //在指定的目录下，该文件名是否合法

#endif