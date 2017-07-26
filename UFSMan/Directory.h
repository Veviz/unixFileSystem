#ifndef DIRECTORY_H
#define DIRECTORY_H
#include "Header.h"
//-----------------------------------------------------------------
//该文件主要包括多文件目录的所有操作
//------------------------------------------------------------------


bool Show_Dir();                                 //显示当前目录下的所有文件

bool Show_Dir(char dir_path[]);                  //显示指定目录下的所有文件

bool Change_Dir(char dir_path[]);               //从当前目录下进入指定路径的目录

bool Show_Path();                               //显示当前路径

bool Create_Dir(char dir_path[]);               //创建一个目录,支持在当前目录下创建一个新目录，也支持给定一个路径，在指定路径下创建新目录

bool Make_Dir(int dir_index,char dir_name[]);   //根据给定的目录索引号，在该目录文件下创建目录名为给定的名字的目录

bool Delete_Dir(char dir_name[]);                //只支持删除当前文件夹下的文件夹，即参数只能是一个文件夹的名字,删除的文件夹中如果还存在文件
                                                 //则必须将该文件夹下的全部文件全部删除
int Get_Type(int file_index);                    //根据文件索引节点号判断文件类型的函数，如果是普通文件，则返回1；如果是目录文件，则返回0

#endif