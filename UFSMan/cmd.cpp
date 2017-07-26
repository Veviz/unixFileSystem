#include "cmd.h"

extern FILE *file;            //文件指针，用于各种对文件的操作
extern char file_system_name[20];           //创建的磁盘文件名
extern super_block_t  file_system_super;    //声明结构体变量
extern char inode_bitmap[INODE_NUM/8];         //索引节点位示图，总位数=8(char)*16=128bit,所以一共可以表示128个索引节点
extern char data_bitmap[DATA_BLOCK_NUM/8];    //数据块位示图，总位数=8(char)*64=512bit,所以一共可以标识521个数据块
extern directory dir_buf[BLOCK_SIZE/sizeof(directory)];   //目录数组，表示一个磁盘块可以存放32个目录项
extern struct inode_t iNode[BLOCK_SIZE/sizeof(inode_t)];                              //索引节点


extern char cmdHead[50];                     //记录当前在那个文件夹中
extern int cur_inode_no;                     //记载当前所在的目录的索引节点
extern int spare_datablk_num[512];           //记录空闲数据块的块号，用于分配数据块
extern int index[128];                             //间址时用于再寻址的中间磁盘块

extern char Buffer[1048576];                //1M的缓冲内存空间
extern char Block_Data[513];                //用于存放一个磁盘块中的数据的临时空间

char cmd1[50],cmd2[50],cmd3[50];           //存放分割后的命令以及参数

void Enter_File_System()   //进入虚拟文件系统
{
	char cmd[50];
	strcpy(cmdHead,"root");
	cur_inode_no = 0;
	int result;
	Showhelp();
	while(1)
	{
		printf("%s>",cmdHead);
		gets(cmd);
		//对输入的命令进行识别
		result = Identify_Cmd(cmd);
		switch(result)
		{
		case 0:
			break;
		case 1:                  //显示当前文件夹下的内容
			Show_Dir();
			break;
		case 2:                    //打印当前目录
			Show_Path();
			break;
		case 3:                   //打印超级快的内容
			Dump_FS();
			break;
		case 4:                     //显示帮助
			Showhelp();
			break;
		case 5:                       //列出指定目录下的文件，命令带参数
			Show_Dir(cmd2);
			break;
		case 6:                       //创建目录
			Create_Dir(cmd2);
			break;
		case 7:                     //改变当前目录
			Change_Dir(cmd2);
			break;
		case 8:                     //删除单个文件
			Delete_File(cmd2);
			break;
		case 9:                     //删除目录下的所有文件
			Delete_Dir(cmd2);
			break;
		case 10:                    //显示文件内容
			Show_Content(cmd2);
			break;
		case 11:                   //复制文件
			Copy_File(cmd2,cmd3);
			break;
		case 12:                   //将本机系统中的文件复制到disk.img中
			Copy_File_In(cmd2,cmd3);
			break;
		case 13:                    //将disk.img中的文件复制到本机系统中
			Copy_File_out(cmd2,cmd3);
			break;
		case 14:                     //退出当前系统
			goto lable;
			break;
		case 15:                   //新建一个文件
			New_file(cmd2);
			break;
		case -1:                    //命令有误，提醒用户
			printf("Command Error!\n");
			break;
		}
	}
lable:
	int useless;
}

//对输入的命令进行区分，返回值为-1表明命令有误！
int Identify_Cmd(char cmd[])
{
	if(cmd[0] == 0)
		return 0;
	int part_count = 1;                   //记录输入的命令由几个部分构成的
	int i,j;
	for(i=0;cmd[i];i++)
		if(cmd[i] == ' ')
			part_count++;
	switch(part_count)
	{
	case 1:
		if(strcmp(cmd,"ls") == 0)            //显示当前文件夹下的内容
			return 1;
		else if(strcmp(cmd,"pwd") == 0)     //打印当前目录
			return 2;
		else if(strcmp(cmd,"dumpfs") == 0)  //打印超级快的内容
			return 3;
		else if(strcmp(cmd,"help") == 0)   //显示帮助
			return 4;
		else if(strcmp(cmd,"exit") == 0)    //退出当前系统
			return 14;
		else
			return -1;
		break;
	case 2:
		//先将命令分割成两个部分
		for(i=0;cmd[i]!=' ';i++)
			cmd1[i] = cmd[i];
		cmd1[i] = 0;
		i++;
		for(j=0;cmd[i];i++,j++)
			cmd2[j] = cmd[i];
		cmd2[j] = 0;
		//下面进行比较
		if(strcmp(cmd1,"ls") == 0)     //列出指定目录下的文件，命令带参数
			return 5;
		else if(strcmp(cmd1,"mkdir") == 0)     //创建目录
			return 6;
		else if(strcmp(cmd1,"cd") == 0)        //改变当前目录
			return 7;
		else if(strcmp(cmd1,"rm") == 0)      //删除单个文件
			return 8;
		else if(strcmp(cmd1,"rmdir") == 0)   //删除目录下的所有文件
			return 9;
		else if(strcmp(cmd1,"cat") == 0)    //显示文件内容
			return 10;
		else if(strcmp(cmd1,"new") == 0)    //新建一个文件
			return 15;
		else
			return -1;
		break;
	case 3:
		//先将命令分割成三个部分
		for(i=0;cmd[i]!=' ';i++)
			cmd1[i] = cmd[i];
		cmd1[i] = 0;
		i++;
		for(j=0;cmd[i]!=' ';i++,j++)
			cmd2[j] = cmd[i];
		cmd2[j] = 0;
		i++;
		for(j=0;cmd[i];i++,j++)
			cmd3[j] = cmd[i];
		cmd3[j] = 0;
		//下面进行比较
		if(strcmp(cmd1,"cp") == 0)              //复制文件
			return 11;
		else if(strcmp(cmd1,"cpin") == 0)      //将本机系统中的文件复制到disk.img中
			return 12;
		else if(strcmp(cmd1,"cpout") == 0)    //将disk.img中的文件复制到本机系统中
			return 13;
		else
			return -1;
		break;
	}
}

bool Dump_FS()                              //显示文件系统的设置
{
	int i;
	file = fopen(file_system_name,"rb+");
	if(NULL == file)
	{
		printf("打开文件失败！\n");
		return false;
	}
	//先将超级块读入内存
	fseek(file,0L,SEEK_SET);
	fread(&file_system_super,sizeof(super_block_t),1,file);
	//再把索引节点位示图读入内存
	fseek(file,BLOCK_SIZE,SEEK_SET);
	fread(inode_bitmap,sizeof(inode_bitmap),1,file);
	//再把数据块位示图读入内存
	fseek(file,BLOCK_SIZE*18,SEEK_SET);
	fread(data_bitmap,sizeof(data_bitmap),1,file);
	fclose(file);
	printf("===================================================================\n");
	printf("The system's content are following:\n\n");

	printf("The super block:\n");
	printf("magic             = %d\n",file_system_super.magic);
	printf("total block count = %d\n",file_system_super.total_block_count);
	printf("inode count       = %d\n",file_system_super.inode_count);
	printf("data block count  = %d\n\n",file_system_super.data_block_count);

	printf("The four region:\n");
	printf("region                  start block        block count       byte count\n");
	printf("inode table bitmap       %d                  %d                  %d\n",file_system_super.inode_table_bitmap.start_block,
		file_system_super.inode_table_bitmap.block_count,file_system_super.inode_table_bitmap.byte_count);
	printf("inode table              %d                  %d                 %d\n",file_system_super.inode_table.start_block,
		file_system_super.inode_table.block_count,file_system_super.inode_table.byte_count);
	printf("data area bitmap         %d                 %d                  %d\n",file_system_super.data_area_bitmap.start_block,
		file_system_super.data_area_bitmap.block_count,file_system_super.data_area_bitmap.byte_count);
	printf("data area                %d                 %d                %d\n\n",file_system_super.data_area.start_block,
		file_system_super.data_area.block_count,file_system_super.data_area.byte_count);

	printf("The bitmap:\n");
	printf("inode table bitmap:\n");
	for(i=0;i<16;i++)
	{	
		Print_Byte(inode_bitmap[i]);
		//printf("\n");
	}
	printf("\ndata area bitmap:\n");
	for(i=0;i<64;i++)
	{
		Print_Byte(data_bitmap[i]);
	}
	printf("\n");
	return true;
}

void Print_Byte(char bit)
{
	int i;
	for(i=7;i>=0;i--)
	{
		if( ((bit>>i)&0x01 ) ==0)
			printf("0");
		else
			printf("1");
	}
}