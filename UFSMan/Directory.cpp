#include "Directory.h"
#include "file_system.h"


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

bool Show_Dir()          //当前的索引节点号为cur_inode_no,通过查找索引节点磁盘块，找到该节点对应的数据块
	                    //通过将对应的数据块读入内存，即得到该目录下所有的文件
{
	int size;                                   //显示该文件的大小
	int type;                                   //显示该文件的类型
	inode_t inode;                              //根据所给的文件索引节点，读取文件索引节点的内容
	char str1[50] = "Common file";
	char str2[50] = "Directory file";
	char str[50];
	inode = Find_Inode(cur_inode_no);
	int diskNo;
	int i;
	diskNo = inode.disk_address[0];      //默认目录文件只占用一个磁盘并且该磁盘号存放在地址项的第一个地址项中
	
	file = fopen(file_system_name,"rb+");
	if(NULL == file)
	{
		printf("打开文件失败！\n");
		return false;
	}
	Clear_Dir_Buf();
	fseek(file,BLOCK_SIZE*diskNo,SEEK_SET);
	fread(dir_buf,sizeof(dir_buf),1,file);//找到当前目录对应的磁盘块后，将其读出并打印该磁盘块中全部内容
	fclose(file);

	
	int j;
	int name_len;
	int size_len;
	int len;
	i = 0;
	printf("inode_no   file_name    file_size     file_type\n");
	while(dir_buf[i].name[0])
	{
		size_len = 0;
		inode = Find_Inode(dir_buf[i].ino);
		size = inode.size;
		type = inode.type;
		if(type == 0)
			strcpy(str,str2);
		else
			strcpy(str,str1);
		name_len = strlen(dir_buf[i].name);
		printf("%d          %s",dir_buf[i].ino,dir_buf[i].name);
		for(j = 0;j <= 12-name_len;j++)
			printf(" ");
		printf("%d",size);
		len = size;
		if(len == 0)
			size_len ++;
		while(len > 0)
		{	
			len = len/10;
			size_len ++;
		}
		for(j=0;j<= 13 - size_len;j++)
			printf(" ");
		printf("%s\n",str);
		i++;
	}
	return true;
}

//显示指定目录下的所有文件
bool Show_Dir(char dir_path[])                  //显示指定目录下的所有文件
{
	int size;                                   //显示该文件的大小
	int type;                                   //显示该文件的类型
	inode_t inode;                              //根据所给的文件索引节点，读取文件索引节点的内容
	char str1[50] = "Common file";
	char str2[50] = "Directory file";
	char str[50];
	int dir_no;
	dir_no = Lookup_File(dir_path);            //通过路径找到对应目录的索引节点号
	inode = Find_Inode(dir_no);
	int diskNo;
	int i;
	diskNo = inode.disk_address[0];      //默认目录文件只占用一个磁盘并且该磁盘号存放在地址项的第一个地址项中
	
	file = fopen(file_system_name,"rb+");
	if(NULL == file)
	{
		printf("打开文件失败！\n");
		return false;
	}
	Clear_Dir_Buf();
	fseek(file,BLOCK_SIZE*diskNo,SEEK_SET);
	fread(dir_buf,sizeof(dir_buf),1,file);//找到当前目录对应的磁盘块后，将其读出并打印该磁盘块中全部内容
	fclose(file);

	int j;
	int name_len;      //计算待输出的文件名的长度
	int size_len;     //计算待输出的文件大小的位数
	int len;
	i = 0;
	printf("inode_no   file_name    file_size     file_type\n");
	while(dir_buf[i].name[0])
	{
		size_len = 0;
		inode = Find_Inode(dir_buf[i].ino);    //根据文件中的索引节点号，得到文件索引节点
		size = inode.size;
		type = inode.type;
		if(type == 0)                          //获取文件类型
			strcpy(str,str2);
		else
			strcpy(str,str1);
		name_len = strlen(dir_buf[i].name);
		printf("%d          %s",dir_buf[i].ino,dir_buf[i].name);   //输出文件索引节点号和文件名
		for(j = 0;j <= 12-name_len;j++)     //控制文件输出的格式
			printf(" ");
		printf("%d",size);                 //输出文件大小
		len = size;
		if(len == 0)
			size_len ++;
		while(len > 0)
		{	
			len = len/10;
			size_len ++;
		}
		for(j=0;j<= 13 - size_len;j++)
			printf(" ");
		printf("%s\n",str);              //输出文件类型
		i++;
	}
	return true;
}

//从当前目录下进入指定路径的目录,该函数会更改cmdHead和cur_inode_no
bool Change_Dir(char dir_path[])
{
	int dir_no;
	int i;
	//1.根据给定的path可以得到指定的目录的索引节点号，并将其记录在cur_inode_no中
	if( isPath(dir_path) )
	{
		dir_no = Lookup_File(dir_path);
		cur_inode_no = dir_no;
	}
	else
	{
		Lookup_Dir(cur_inode_no);
		for(i=0;dir_buf[i].name[0];i++)
		{
			if(strcmp(dir_buf[i].name,dir_path) == 0)
			{	
				cur_inode_no = dir_buf[i].ino;
				break;
			}
		}
	}
	//2.根据cur_inode_no，不断向上级退回，直到根节点，记录下这一系列的目录文件索引节点号
	int dir_index_path[20];                         //记录下从当前索引节点到根节点的目录文件索引节点号
	int dir_index_no;                               //记录已经退回到的目录文件索引节点号
	int count = 0;
	memset(dir_index_path,0,sizeof(dir_index_path));
	dir_index_no = cur_inode_no;
	dir_index_path[count++] = cur_inode_no;
	while(0 != dir_index_no)                        //直到检索到根目录
	{
	     Lookup_Dir(dir_index_no);
		 for(i=0;dir_buf[i].name[0];i++)
		 {	 
			 if(strcmp(dir_buf[i].name,"..") == 0)
			{
				dir_index_no = dir_buf[i].ino;
				dir_index_path[count++] = dir_index_no;
			 }
		 }
	}
	count -- ;
	//3.从根节点出发，根据这些索引节点号一步步往下推，直到当前目录索引号
	memset(cmdHead,0,sizeof(cmdHead));
	strcpy(cmdHead,"root");
	int j;
	for(i=count;i>0;i--)
	{
		dir_index_no = dir_index_path[i];
		Lookup_Dir(dir_index_no);
		for(j=0;dir_buf[j].name[0];j++)
		{	
			if(dir_buf[j].ino == dir_index_path[i-1])
		    {
				strcat(cmdHead,"/");
			    strcat(cmdHead,dir_buf[j].name);
				break;
			}
		}
	}
	int cmd_head_size = strlen(cmdHead);
	cmdHead[cmd_head_size] = 0;                      //防止烂尾
	return 0;
}

//显示当前路径，将记录当前路径的数组中的内容输出即可
bool Show_Path()
{
	printf("%s\n",cmdHead);
	return true;
}

//创建一个目录,支持在当前目录下创建一个新目录，也支持给定一个路径，在指定路径下创建新目录
bool Create_Dir(char dir_path[])
{
	int dir_index;                //目录文件索引号
	char path[50];                //绝对路径
	char dir_name[50];            //待创建的目录文件名
	int middle;                       //分界
	int count=0;
	int i=0;
	int j=0;
	if( isPath(dir_path) )
	{
		while(dir_path[i])
		{
			if(dir_path[i] == '/')
				count++;
			i++;
		}
		for(i=0;dir_path[i];i++)
		{	
			if(dir_path[i] == '/')
				j++;
			if(j == count)
				break;
		}
		middle = i;              //找到参数2的分界
		///////////////////////////////////////////////////
		//将参数2分解成路径和文件名，分别存放在path和file_name中
		for(i=0;i<middle;i++)
			path[i] = dir_path[i];
		path[i] = 0;
		for(j=0,i=middle+1;dir_path[i];i++,j++)
			dir_name[j]=dir_path[i];
		dir_name[j] = 0;
		dir_index = Lookup_File(path);
	}
	else
	{
		dir_index = cur_inode_no;
		strcpy(dir_name,dir_path);
	}
	if(!Make_Dir(dir_index,dir_name))
		return false;
	return true;
}

//根据给定的目录索引号，在该目录文件下创建目录名为给定的名字的目录
//规定目录文件最多只能占用一个数据块
bool Make_Dir(int dir_index,char dir_name[])
{
	int inode_no;                 //分配给即将创建的目录文件的索引节点号
	int dir_block_no;             //分配给即将创建的目录文件的磁盘块号
	int block_num;         //该索引号指向的索引节点所处的盘块号(2#~17#)
	int inode_num;         //该索引节点指向的索引节点在磁盘里的位置(0~7)
	int father_dir_blk;         //新建的文件夹的父文件夹所在的磁盘块号
	int i,j;
	///////////////////////////////////////////////////////////////////////////////
	//step1.获取空闲索引节点
	///////////////////////////////////////////////////////////////////////////////
	inode_no = Scan_Inode_Bitmap();
	if(-1 == inode_no)
	{
		printf("The index code has run out!\nCreate failed!\n");
		return false;
	}
	//根据索引节点号获得该索引节点的具体位置
	block_num = inode_no/8 + 2;
	inode_num = inode_no % 8;
	///////////////////////////////////////////////////////////////////////////
	//step2.填写inode结构
	//////////////////////////////////////////////////////////////////////////
	file = fopen(file_system_name,"rb+");
	if(NULL == file)
	{
		printf("打开文件失败！\n");
		return false;
	}
	fseek(file,BLOCK_SIZE*block_num,SEEK_SET);
	fread(iNode,sizeof(iNode),1,file);
	fclose(file);
	iNode[inode_num].type = 0;                           //创建的文件是目录文件
	iNode[inode_num].size = 0;                          //目录文件的size字段不起作用

	Scan_Data_Bitmap(1);
	dir_block_no = spare_datablk_num[0];
	iNode[inode_num].disk_address[0] = dir_block_no;
	file = fopen(file_system_name,"rb+");
	fseek(file,BLOCK_SIZE*block_num,SEEK_SET);
	fwrite(iNode,sizeof(iNode),1,file);
	fclose(file);

	//step3.在分配给新建的目录文件的磁盘块上初始化一些数据
	strcpy(dir_buf[0].name,".");
	dir_buf[0].ino = inode_no;
	strcpy(dir_buf[1].name,"..");
	dir_buf[1].ino = dir_index;
	for(i=2;i<32;i++)
		memset(dir_buf[i].name,0,sizeof(dir_buf[i].name));
	//写回文件
	file = fopen(file_system_name,"rb+");
	fseek(file,BLOCK_SIZE*dir_block_no,SEEK_SET);
	fwrite(dir_buf,sizeof(dir_buf),1,file);
	fclose(file);
	//step4.在原来的目录文件中增加一项：新增目录项
	father_dir_blk = Lookup_Dir(dir_index);
	i=0;
	while(dir_buf[i].name[0])
		i++;
	strcpy(dir_buf[i].name,dir_name);
	dir_buf[i].ino = inode_no;
	//写回文件
	file = fopen(file_system_name,"rb+");
	fseek(file,BLOCK_SIZE*father_dir_blk,SEEK_SET);
	fwrite(dir_buf,sizeof(dir_buf),1,file);
	fclose(file);
	return true;
}

//只支持删除当前文件夹下的文件夹，即参数只能是一个文件夹的名字,删除的文件夹中如果还存在文件
//则必须将该文件夹下的全部文件全部删除
bool Delete_Dir(char dir_name[])
{
	int i,j;
	int father_block;                               //待删除的文件夹所在的文件夹的目录文件所在的磁盘号
	int dir_index;                                 //用于存放待删除的文件夹的目录文件索引号
	int mark;                                      //用于文件夹跳转时记录文件夹索引编号
	father_block = Lookup_Dir(cur_inode_no);       //将当前目录全部读取到内存中
	//1.获取待删除文件夹的索引节点号,同时在当前文件夹中清除含有待删除文件夹的表项
	for(i=2;dir_buf[i].name[0];i++)
	{
		if(strcmp(dir_buf[i].name,dir_name) == 0)
		{	
			dir_index = dir_buf[i].ino;
			//找到该目录项后，记得将其在本目录中清除,还要写回磁盘文件中
			for(j=i;dir_buf[j].name[0];j++)
			{
				strcpy(dir_buf[j].name,dir_buf[j+1].name);
				dir_buf[j].ino = dir_buf[j+1].ino;
			}
			file = fopen(file_system_name,"rb+");
	        if(NULL == file)
	        {
		        printf("打开文件失败！\n");
	     	    return false;
	        }
			fseek(file,BLOCK_SIZE*father_block,SEEK_SET);
			fwrite(dir_buf,sizeof(dir_buf),1,file);
			fclose(file);
			break;
		}
	}
	//2.进入待删除的文件夹中，将待删除目录文件读入内存，查看其是否还有其他文件
	mark = cur_inode_no;
	cur_inode_no = dir_index;
	Lookup_Dir(cur_inode_no);
	for(i=2;dir_buf[i].name[0];i++)          //清空该目录下的文件，如果是空文件夹，这一步会跳过
	{
		if( Get_Type(dir_buf[i].ino) == 1)   //文件类型是普通文件
		{	
			Delete_File(dir_buf[i].name);
			Lookup_Dir(cur_inode_no);        //重新将当前目录读入内存
			i = 1;                           //由于之前已经将待删除的文件在目录中抹去礼物
			                                 //所以又要从头开始
		}
		else if(Get_Type(dir_buf[i].ino) == 0)
		{	
			Delete_Dir(dir_buf[i].name);
			Lookup_Dir(cur_inode_no);
			i = 1;
		}
		else
			return false;
	}
	cur_inode_no = mark;        //清空该文件夹后，再跳回原来的文件夹中
	//3.修改inode_bitmap和data_bitmap中的内容
	Delete_From_Inode_Bitmap(dir_index);
	//通过打开文件获取待删除的文件夹对应的索引节点,并获得文件夹对应的磁盘块号
	int block_num;         //该索引号指向的索引节点所处的盘块号(2#~17#)
	int inode_num;         //该索引节点指向的索引节点在磁盘里的位置(0~7)
	int dir_block;         //该目录文件存放的磁盘块号
	char data[512];
	block_num = dir_index /8 + 2;
	inode_num = dir_index % 8;
	file = fopen(file_system_name,"rb+");
	if(NULL == file)
	{
		printf("打开文件失败！\n");
		return false;
	}
	fseek(file,BLOCK_SIZE*block_num,SEEK_SET);
	fread(iNode,sizeof(iNode),1,file);
	fclose(file);
	dir_block = iNode[inode_num].disk_address[0];
	//将删除的文件夹的目录文件所在磁盘清空
	memset(data,0,sizeof(data));
	file = fopen(file_system_name,"rb+");
	if(NULL == file)
	{
		printf("打开文件失败！\n");
		return false;
	}
	fseek(file,BLOCK_SIZE*dir_block,SEEK_SET);
	fwrite(data,sizeof(data),1,file);
	fclose(file);

	Delete_From_Data_Bitmap(dir_block);
	return true;
}

//根据文件索引节点号判断文件类型的函数，如果是普通文件，则返回1；如果是目录文件，则返回0
int Get_Type(int file_index)
{
	int block_num;         //该索引号指向的索引节点所处的盘块号(2#~17#)
	int inode_num;         //该索引节点指向的索引节点在磁盘里的位置(0~7)
	block_num = file_index /8 + 2;
	inode_num = file_index % 8;

	file = fopen(file_system_name,"rb+");
	if(NULL == file)
	{
		printf("打开文件失败！\n");
		return false;
	}
	fseek(file,BLOCK_SIZE*block_num,SEEK_SET);
	fread(iNode,sizeof(iNode),1,file);
	fclose(file);
	return iNode[inode_num].type;
}