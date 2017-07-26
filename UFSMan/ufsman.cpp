//#include "Header.h"
#include "file_system.h"
#include "Directory.h"
#include "cmd.h"
#include <windows.h>
// disk layout:
//
// super block
// inode_table_bitmap
// inode_table
// data_area_bitmap
// data_area
//-----------------------------------------------------------
//类Unix的文件系统由5个region组成，超级块，索引节点表位图，索引节点表，数据块位图和数据块区域
//文件系统需要在磁盘中创建一个虚拟磁盘文件disk.img，所有对磁盘的操作变为对文件的操作。
//定义每个磁盘块的大小为512B，磁盘块从0到n，0磁盘块存放超级块，
//1#磁盘块存放 inode_table_bitmap，即索引节点位示图，使用int型数据，512B/4B = 128 ，即文件系统一共可以同时拥有128个索引节点
//2# ~ 16#磁盘块存放 inode_table ,即索引节点,每个索引节点项占56B，每个磁盘块可以存放9个索引节点，所以一共需要15个磁盘块
//17#磁盘块存放 data_table_bitmap ,即数据块位示图，使用char型数据，512B/1B = 512,即文件系统一共拥有512个数据块，加上前面的17个，一共是529个磁盘块
//18# ~ 529#磁盘块用于存放数据
//-----------------------------------------------------------

/*============       全局变量(外部变量)       ====================*/
extern char cmdHead[20];                     //记录当前在那个文件夹中
extern directory dir_buf[BLOCK_SIZE/sizeof(directory)];   //目录数组，表示一个磁盘块可以存放32个目录项
extern super_block_t  file_system_super;    //声明结构体变量
extern char inode_bitmap[INODE_NUM/8];         //索引节点位示图，总位数=8(char)*16=128bit,所以一共可以表示128个索引节点
extern char data_bitmap[DATA_BLOCK_NUM/8];    //数据块位示图，总位数=8(char)*64=512bit,所以一共可以标识521个数据块



int main(int argc,char * argv[])
{
    char cmd[40];
	char cmd1[20],cmd2[20],cmd3[20];
	int i,j;
    int space_count;
	while( 1 )
	{
		printf("%s>",argv[0]);
		gets(cmd);
		if(cmd[0])
		{
			i = 0;
			space_count = 0;
			while(cmd[i])
				if(cmd[i++] == ' ')
					space_count++;
			if(0 == space_count)
			{	
				if(strcmp(cmd,"ls") == 0)                       //原系统中的显示当前目录下文件
					system("dir");
				else if(strcmp(cmd,"exit") == 0)               //exit
					break;
				else
					printf("command error!\n");
			}
			else if(1 == space_count)
			{
				for(i=0;cmd[i]!=' ';i++)
					cmd1[i] = cmd[i];
				cmd1[i] = 0;
				i++;
				for(j=0;cmd[i];i++,j++)
					cmd2[j] = cmd[i];
				cmd2[j] = 0;
				if(strcmp(cmd1,"ufsman") == 0 && cmd2[0])  //ufsman disk.img
				{
					strcpy(file_system_name,cmd2);        //当存在多个磁盘文件系统时，可以通过后面的参数进行选择自己所进入的文件系统
					if(Install())
						Enter_File_System();
				}
				else
					printf("command error!\n");
			}
			else if(2 == space_count)
			{
				for(i=0;cmd[i] != ' ';i++)
					cmd1[i] = cmd[i];
				cmd1[i++]=0;
				for(j=0;cmd[i] != ' ';i++,j++)
					cmd2[j] = cmd[i];
				cmd2[j++]=0;
				i++;
				for(j=0;cmd[i];i++,j++)
					cmd3[j] = cmd[i];
				cmd3[j]=0;
				if((strcmp(cmd1,"ufsman") == 0) && (strcmp(cmd2,"--mkfs")==0) && (cmd3[0] != 0))  //创建虚拟磁盘文件，同时进行格式化  /*ufsman --mkfs disk.img*/
				{	
					strcpy(file_system_name,cmd3);
					Format();
				}
				else
					printf("command error!\n");
			}
			else
				printf("command error!\n");
		}
	}
	return 0;
}