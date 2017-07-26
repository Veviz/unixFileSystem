#ifndef CMD_H
#define CMD_H
#include "Directory.h"
#include "file_system.h"
#include "Header.h"


void Enter_File_System();           //格式化磁盘文件之后，物理磁盘中已经存在刚才创建的磁盘文件
                                   //通过命令"ufsman 文件名",安装磁盘系统
int Identify_Cmd(char cmd[]);      //对输入的命令进行区分，返回值为-1表明命令有误！

bool Dump_FS();                              //显示文件系统的设置

void Print_Byte(char bit);                 //将一个字节的各个bit位输出来


#endif