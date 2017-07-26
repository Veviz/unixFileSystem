#include "file_system.h"


FILE *file;            //文件指针，用于各种对文件的操作
char file_system_name[20];           //创建的磁盘文件名
super_block_t  file_system_super;    //声明结构体变量
char inode_bitmap[INODE_NUM/8];         //索引节点位示图，总位数=8(char)*16=128bit,所以一共可以表示128个索引节点
char data_bitmap[DATA_BLOCK_NUM/8];    //数据块位示图，总位数=8(char)*64=512bit,所以一共可以标识521个数据块
directory dir_buf[BLOCK_SIZE/sizeof(directory)];   //目录数组，表示一个磁盘块可以存放32个目录项
struct inode_t iNode[BLOCK_SIZE/sizeof(inode_t)];                              //索引节点


char cmdHead[50];                     //记录当前在那个文件夹中
int cur_inode_no;                     //记载当前所在的目录的索引节点
int spare_datablk_num[512];           //记录空闲数据块的块号，用于分配数据块
int index[128];                             //间址时用于再寻址的中间磁盘块

char Buffer[1048576];                //1M的缓冲内存空间
char Block_Data[513];                //用于存放一个磁盘块中的数据的临时空间

bool Format()         //格式化磁盘文件，初始化其中的数据
{
	int i;
	file = fopen(file_system_name,"wb");//创建虚拟磁盘文件
	if(NULL == file)
	{
		printf("创建虚拟磁盘文件失败！");
		return false;
	}

	//初始化超级块
	file_system_super.magic = 0;
	file_system_super.total_block_count = TOTAL_BLOCK_NUMBER;          //文件系统的总磁盘数
	file_system_super.inode_count = INODE_NUM;                         //文件系统的总索引节点数
	file_system_super.data_block_count = DATA_BLOCK_NUM;               //文件系统的总数据块数目

	file_system_super.inode_table_bitmap.start_block = INODE_BITMAP_START;                //索引节点位示图开始磁盘块
	file_system_super.inode_table_bitmap.block_count = INODE_BITMAP_BKNUM;                //索引节点位示图占据磁盘块数
	file_system_super.inode_table_bitmap.byte_count = INODE_NUM/8;                       //索引节点位示图占用的字节个数 

	file_system_super.inode_table.start_block = INODE_BLOCK_START;                       //索引节点开始磁盘块
	file_system_super.inode_table.block_count = INODE_BLOCK_NUM;                         //索引节点占据磁盘块数
	file_system_super.inode_table.byte_count = INODE_SIZE * INODE_NUM_PER_BLOCK * INODE_BLOCK_NUM;//索引节点占用的字节个数 

	file_system_super.data_area_bitmap.start_block = DATA_BITMAP_START;                //数据块位示图开始磁盘块
	file_system_super.data_area_bitmap.block_count = DATA_BITMAP_BLNUM;                //数据块位示图占据磁盘块数
	file_system_super.data_area_bitmap.byte_count = DATA_BLOCK_NUM/8;                  //数据块位示图占用的字节个数

	file_system_super.data_area.start_block = DATA_BLOCK_START;                //数据块开始磁盘块
	file_system_super.data_area.block_count = DATA_BLOCK_NUM;                 //数据块占据磁盘块数
	file_system_super.data_area.byte_count = DATA_BLOCK_NUM * BLOCK_SIZE;     //数据块占用的字节个数

	fseek(file,0L,SEEK_SET);
	fwrite(&file_system_super,BLOCK_SIZE,1,file);

	//初始化索引节点位示图
	inode_bitmap[0] = 0x80;            //索引节点位示图的第一位标识根目录,0x80表示100000000
	for(i = 1;i < INODE_NUM/8;i ++)
	{
		inode_bitmap[i] = 0;
	}
	fseek(file,BLOCK_SIZE,SEEK_SET);
	fwrite(inode_bitmap,sizeof(inode_bitmap),1,file);

	//初始化索引节点，此时只有一个索引节点，即根目录的索引节点
	//对应的根目录表存放在数据块区域的第一个数据块中
	iNode[0].type = 0;                 //表示该文件是目录文件
	iNode[0].size = 0;                 //目录文件的size都是0
	iNode[0].disk_address[0] = 0+DATA_BLOCK_START;      //根目录所占据的数据块号<数据块号从0#到511#>，对应于磁盘号还要加上19
	fseek(file,BLOCK_SIZE*2,SEEK_SET);
	fwrite(&iNode,sizeof(iNode),1,file);

	//初始化数据块位示图
	data_bitmap[0] = 0x80;          //数据块位示图的第一位也被占用，用于存放根目录表
	for(i = 1; i < DATA_BLOCK_NUM/8; i ++)
	{
		data_bitmap[i] = 0;
	}
	fseek(file,BLOCK_SIZE*18,SEEK_SET);
	fwrite(data_bitmap,BLOCK_SIZE,1,file);

	//初始化数据块，此时只有第一个数据块中存放着根目录的目录表
	//该目录表中只有两项，上级目录和当前目录的索引节点号都是0
	Clear_Dir_Buf();
	strcpy(dir_buf[0].name,".");
	dir_buf[0].ino = 0;
	strcpy(dir_buf[1].name,"..");
	dir_buf[1].ino = 0;
	fseek(file,BLOCK_SIZE*19,SEEK_SET);
	fwrite(dir_buf,BLOCK_SIZE,1,file);

	fclose(file);
	return true;
}

bool Install()                        //装载磁盘文件系统
{
	int i;
	printf("Installing...\n");
	file = fopen(file_system_name,"rb+");       //以只读方式打开磁盘文件
	if(NULL == file)
	{
		printf("%s磁盘文件打开失败！\n",file_system_name);
		return false;
	}
	//先将超级块读入内存
	fseek(file,0L,SEEK_SET);
	fread(&file_system_super,sizeof(super_block_t),1,file);
	//再把索引节点位示图读入内存
	fseek(file,BLOCK_SIZE,SEEK_SET);
	fread(inode_bitmap,sizeof(inode_bitmap),1,file);
	//将索引节点读入内存中
	fseek(file,BLOCK_SIZE*2,SEEK_SET);
	fread(iNode,sizeof(iNode),1,file);

	//再把数据块位示图读入内存
	fseek(file,BLOCK_SIZE*18,SEEK_SET);
	fread(data_bitmap,sizeof(data_bitmap),1,file);

	//将根目录的目录文件读入到内存中
	fseek(file,BLOCK_SIZE*19,SEEK_SET);
	fread(dir_buf,BLOCK_SIZE,1,file);

	fclose(file);
	return true;
}


bool Showhelp()
{
	printf("help      Display all the command you can use.\n");
	printf("dumpfs    Display the information of the file system.\n");
	printf("ls        Display all the files the directory has.\n");
	printf("cat       Display the content of the file.\n");
	printf("cp        Copy the files.\n");
	printf("cpin      Copy the files from the Physical disk to the current disk.\n");
	printf("cpout     Copy the files from the current disk to the Physical disk.\n");
	printf("mkfir     Make a new directory.\n");
	printf("new       Make a new file.\n");
	printf("cd        Change the current directory.\n");
	printf("pwd       Print the current directory.\n");
	printf("rm        Remove a single file.\n");
	printf("rmdir     Remove all files in the directory.\n");
	printf("exit      Exit form the current system.\n");
	return true;
}


//该函数用于给定一个索引节点号，该函数返回一个inode_t类型的结果，从索引节点区
//将该节点对应的数据区提供给需要的函数
inode_t Find_Inode(int ino)
{
	inode_t inode;
	int i,j,k;
	i = ino/8;                //i表示相对于索引节点开始块（INODE_BLOCK_START#磁盘块）的位置（即2+i#磁盘块）
	j = ino - 8*i;            //j标识在该磁盘块中第一个索引节点记录

	file = fopen(file_system_name,"rb+");
	if(NULL == file)
	{
		printf("打开文件失败！\n");
		return inode;
	}
	fseek(file,BLOCK_SIZE*(i+INODE_BLOCK_START),SEEK_SET);
	fread(iNode,sizeof(iNode),1,file);
	fclose(file);

	inode.size = iNode[j].size;
	inode.type = iNode[j].type;
	for(k=0;k<MAX_DISK_ADDRESS;k++)
		inode.disk_address[k] = iNode[j].disk_address[k];
	return inode;
}
//当前索引节点号：cur_inode_no
bool Create_File(char file_name[],int index_node)   //对于Buffer数组，记得将具体数据后一个字节赋0，不然计算字节数时会出错
{
	int inode_no;          //存放即将分配给该文件的索引节点
	int buf_block_count;   //缓冲区数据所占数据块的个数
	int buf_byte_count;    //缓冲区数据的字节总数
	int block_num;         //该索引号指向的索引节点所处的盘块号(2#~17#)
	int inode_num;         //该索引节点指向的索引节点在磁盘里的位置(0~7)
	int i,j;
	int indirect_indexblooc_no; //间接寻址时，专门用于寻址的数据块
	///////////////////////////////////////////////////////////////////////////////
	//step1.获取空闲索引节点
	///////////////////////////////////////////////////////////////////////////////
	inode_no = Scan_Inode_Bitmap();     
	if(-1 == inode_no)
	{
		printf("The index code has run out!\nCreate failed!\n");
		return false;
	}
	///////////////////////////////////////////////////////////////////////////
	//step2.填写inode结构
	//////////////////////////////////////////////////////////////////////////
	buf_byte_count = strlen(Buffer);
	buf_block_count = buf_byte_count/BLOCK_SIZE;       
	if(buf_byte_count % BLOCK_SIZE != 0)                   //如果Buffer的字节总数不是BLOCK_SIZE的整数倍，则还需要分配一个数据块存放那多出来的数据
		buf_block_count++;   
	//根据索引节点号获得该索引节点的具体位置
	block_num = inode_no/8 + 2;
	inode_num = inode_no % 8;
	
	file = fopen(file_system_name,"rb+");
	if(NULL == file)
	{
		printf("打开文件失败！\n");
		return false;
	}
	fseek(file,BLOCK_SIZE*block_num,SEEK_SET);
	fread(iNode,sizeof(iNode),1,file);
	fclose(file);

	iNode[inode_num].type = 1;                           //创建的文件是普通文件
	iNode[inode_num].size = buf_byte_count;              //创建的文件的大小为刚才计算出来的字节数

	char data[513];                                     //用于临时存放文件的数据，即将写入磁盘块的数据
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	//根据文件的大小，选择不同的寻址方式
	if(buf_block_count <= 10)                            //当文件大小<512B*10时,只需要用到直接地址项的10个
	{
		if(Scan_Data_Bitmap(buf_block_count))
		{
			for(i=0;i<buf_block_count;i++)
			{
				iNode[inode_num].disk_address[i] = spare_datablk_num[i];
				//====================================================================
			    //填写好寻址方式就可以写数据了
				memset(data,0,sizeof(data));                          //先对将要写的磁盘块进行0覆盖
				file = fopen(file_system_name,"rb+");
				fseek(file,BLOCK_SIZE*spare_datablk_num[i],SEEK_SET);
				fwrite(data,BLOCK_SIZE,1,file);
				fclose(file);

				Spill(Buffer,data,i+1,512);                //将Buffer数据块分割成size=512的小数据块，并分别写入磁盘块中
				file = fopen(file_system_name,"rb+");
				fseek(file,BLOCK_SIZE*spare_datablk_num[i],SEEK_SET);
				fwrite(data,BLOCK_SIZE,1,file);
				fclose(file);
			}
			
		}
		else
		{
			printf("已经没有更多的数据块可以分配！创建失败！\n");
		}
	}
	else if(buf_block_count <=128)                                   //当文件大小512B*10<size<=512B*128时，只用到一级间址地址项的1个
	{
		Scan_Data_Bitmap(1);                                          //先获取一个数据块，然后在该数据块中填写下一步寻址的磁盘号
		indirect_indexblooc_no = spare_datablk_num[0];                //将刚才扫描出的一个磁盘块号赋给变量indirect_indexblooc_no
		iNode[inode_num].disk_address[10] = indirect_indexblooc_no;   //索引节点只需要记下索引磁盘块，便能够查找到文件所需要的全部磁盘块号
		if(Fill_Index_In_Datablk(indirect_indexblooc_no,buf_block_count))//将该文件需要的buf_block_count个磁盘号填入索引磁盘块
		{
			for(i=0;i<buf_block_count;i++)
			{
				memset(data,0,sizeof(data));                          //先对将要写的磁盘块进行0覆盖
				file = fopen(file_system_name,"rb+");
				fseek(file,BLOCK_SIZE*index[i],SEEK_SET);
				fwrite(data,BLOCK_SIZE,1,file);
				fclose(file);

				Spill(Buffer,data,i+1,512);                //将Buffer数据块分割成size=512的小数据块，并分别写入磁盘块中
				file = fopen(file_system_name,"rb+");
				fseek(file,BLOCK_SIZE*index[i],SEEK_SET);  //index[i]表示第i+1块数据存放的磁盘块号
				fwrite(data,BLOCK_SIZE,1,file);
				fclose(file);
			}
		}
		else
		{
		    printf("已经没有更多的数据块可以分配！创建失败！\n");
		}
	}
	else if(buf_block_count <=128*2)                     //当文件大小512B*128<size<=512B*256时，只用到一级间址地址项的两个
	{
		Scan_Data_Bitmap(1);                                                // 
		indirect_indexblooc_no = spare_datablk_num[0];                      // 
		iNode[inode_num].disk_address[10] = indirect_indexblooc_no;
		if(Fill_Index_In_Datablk(indirect_indexblooc_no,128))                  //第一个一级索引需要能够找到128个磁盘块
		{
			for(i=0;i<128;i++)
			{
				memset(data,0,sizeof(data));                          //先对将要写的磁盘块进行0覆盖
				file = fopen(file_system_name,"rb+");
				fseek(file,BLOCK_SIZE*index[i],SEEK_SET);
				fwrite(data,BLOCK_SIZE,1,file);
				fclose(file);

				Spill(Buffer,data,i+1,512);
				file = fopen(file_system_name,"rb+");
				fseek(file,BLOCK_SIZE*index[i],SEEK_SET);
				fwrite(data,BLOCK_SIZE,1,file);
				fclose(file);
			}
		}
		else
		{
			printf("已经没有更多的数据块可以分配！创建失败！\n");
		}

		Scan_Data_Bitmap(1);                                                // 
		indirect_indexblooc_no = spare_datablk_num[0];                      // 
		iNode[inode_num].disk_address[11] = indirect_indexblooc_no;
		if(Fill_Index_In_Datablk(indirect_indexblooc_no,buf_block_count - 128))//剩下的buf_block_count - 128个磁盘块交给第二个一级索引
		{
			for(i=128,j=0;i<buf_block_count;i++,j++)
			{
				memset(data,0,sizeof(data));                          //先对将要写的磁盘块进行0覆盖
				file = fopen(file_system_name,"rb+");
				fseek(file,BLOCK_SIZE*index[j],SEEK_SET);
				fwrite(data,BLOCK_SIZE,1,file);
				fclose(file);

				Spill(Buffer,data,i+1,512);
				file = fopen(file_system_name,"rb+");
				fseek(file,BLOCK_SIZE*index[j],SEEK_SET);
				fwrite(data,BLOCK_SIZE,1,file);
				fclose(file);
			}
		}
		else
		{
			printf("已经没有更多的数据块可以分配！创建失败！\n");
		}
	}
	else                                                //否则，只需要用到二级间址地址项的一个，其实此时所存在的数据块最多只有
		                                                //512-128-1=383个
	{
		Scan_Data_Bitmap(1);                            //作为第一步的索引数据块的磁盘号
		indirect_indexblooc_no = spare_datablk_num[0];
		iNode[inode_num].disk_address[12] = indirect_indexblooc_no;
		int index_count;                               //表示第二步中所需要索引数据块的个数
		index_count = buf_block_count / 128;
		if(buf_block_count % 128 != 0)
			index_count ++;
		Fill_Index_In_Datablk(indirect_indexblooc_no,index_count);    //向第一个索引数据块中填写数据，这些数据是指向第二级索引数据块的磁盘块号
		int temp[128];
		for(i=0;i<index_count;i++)//让temp记下第一个索引数据块中指向二级索引数据块的磁盘块号，便于下一步根据这些磁盘块号继续分配数据块给数据块
			temp[i] = index[i];
		int k;
		int buffer_part_count = 1;              //记录数据缓存buffer的块序
		for(k=0;k<index_count-1;k++)//temp[k]表示第k个二级索引磁盘块
		{	
			if(Fill_Index_In_Datablk(temp[k],128))//向二级索引磁盘块中填写分给数据的数据磁盘块号，这些索引磁盘块中都是128项
			{
				//然后根据这些磁盘块号将待写入的数据一一写进去
				for(i=0;i<128;i++)
				{
					memset(data,0,sizeof(data));                          //先对将要写的磁盘块进行0覆盖
					file = fopen(file_system_name,"rb+");
					fseek(file,BLOCK_SIZE*index[i],SEEK_SET);
					fwrite(data,BLOCK_SIZE,1,file);
					fclose(file);

					Spill(Buffer,data,buffer_part_count,512);
					buffer_part_count ++;
					file = fopen(file_system_name,"rb+");
					fseek(file,BLOCK_SIZE*index[i],SEEK_SET);
					fwrite(data,BLOCK_SIZE,1,file);
					fclose(file);
				}
			}
			else
			{
				printf("已经没有更多的数据块可以分配！创建失败！\n");
				return false;
			}
		}
		//最后一个（即第index_count个）二级索引数据块中可能不满128项,
		if(Fill_Index_In_Datablk(temp[k],buf_block_count-128*(index_count-1)))
		{
			//填完数据索引块后还需要将剩下的数据填到相应的磁盘块中
			for(i=0;i<buf_block_count-128*(index_count-1);i++)
			{
				memset(data,0,sizeof(data));                          //先对将要写的磁盘块进行0覆盖
				file = fopen(file_system_name,"rb+");
				fseek(file,BLOCK_SIZE*index[i],SEEK_SET);
				fwrite(data,BLOCK_SIZE,1,file);
				fclose(file);

				Spill(Buffer,data,buffer_part_count,512);
				buffer_part_count ++;
				file = fopen(file_system_name,"rb+");
				fseek(file,BLOCK_SIZE*index[i],SEEK_SET);
				fwrite(data,BLOCK_SIZE,1,file);
				fclose(file);
			}
		}
		else
		{
			printf("已经没有更多的数据块可以分配！创建失败！\n");
				return false;
		}
		/*---------------  二级间址暂时用不上  ------------------*/
	}
	/*----------------------再将填写好的iNode结构写回文件中----------------------------*/
	file = fopen(file_system_name,"rb+");
	fseek(file,BLOCK_SIZE*block_num,SEEK_SET);
	fwrite(iNode,sizeof(iNode),1,file);   //其实只更新了iNode[inode_num]的这一项
	fclose(file);

	//step4.回写文件目录，将新增的文件增加到当前目录下
	int dir_blkno;                      //当前文件夹所在的磁盘块
	block_num = index_node/8 + 2;       //根据给定的索引节点，将新建的文件存放在该节点指定的位置
	inode_num = index_node % 8;
	file = fopen(file_system_name,"rb+");
	fseek(file,BLOCK_SIZE*block_num,SEEK_SET);
	fread(iNode,sizeof(iNode),1,file);
	fclose(file);
	dir_blkno = iNode[inode_num].disk_address[0];   //根据当前索引节点号查找索引节点，然后找到对应的目录文件在的磁盘块号
	Clear_Dir_Buf();
	file = fopen(file_system_name,"rb+");
	fseek(file,BLOCK_SIZE*dir_blkno,SEEK_SET);
	fread(dir_buf,sizeof(dir_buf),1,file);
	fclose(file);
	i=0;
	while(dir_buf[i].name[0])
		i++;
	strcpy(dir_buf[i].name,file_name);
	dir_buf[i].ino = inode_no;

	file = fopen(file_system_name,"rb+");
	fseek(file,BLOCK_SIZE*dir_blkno,SEEK_SET);
	fwrite(dir_buf,sizeof(dir_buf),1,file);
	fclose(file);
}

//扫描索引节点位示图，返回一个可用的索引节点号，同时将对应的索引节点位示图相应位置1
int Scan_Inode_Bitmap()
{
	file = fopen(file_system_name,"rb+");
	if(NULL == file)
	{
		printf("打开文件失败！\n");
		return false;
	}
	fseek(file,BLOCK_SIZE,SEEK_SET);
	fread(inode_bitmap,sizeof(inode_bitmap),1,file);
	fclose(file);
	int column,row;                  //column记录位示图的行，一共16行;row记录位示图的列，一共8列
	char temper;
	int inode_no;                     //扫描索引节点位示图的结果，返回扫描的第一个空闲索引节点的号（从0到127）
	for(column = 0;column < 16;column ++)
	{
		for(row = 7;row >= 0;row --)
		{
		    temper = inode_bitmap[column];
			if( ((temper>>row) & 0x01) == 0)           //表示第column行的位示图的第(8-row) bit值为0，即对应的索引节点为空闲
			{
				inode_no = column * 8 + (7 - row);
				inode_bitmap[column] = inode_bitmap[column] | (0x01<<row);   //更改已经分配出去的索引节点位示图（置1）
				file = fopen(file_system_name,"rb+");
				fseek(file,BLOCK_SIZE,SEEK_SET);
				fwrite(inode_bitmap,sizeof(inode_bitmap),1,file);
				fclose(file);
				return inode_no;
			}
		}
	}
	return -1;    //标识已经没有索引节点可供分配了，创建文件失败
}

 //扫描数据块位示图，传入的参数是需要的数据块的个数，按顺序扫描后，将空闲数据块的块号存入全局变量spare_datablk_num数组中
bool Scan_Data_Bitmap(int data_block_count)
{
	int count = 0;                   //记录已经找到的空闲块的数目
	int column,row;                  //column记录位示图的行，一共16行;row记录位示图的列，一共8列
	char temper;
	int trans;
	memset(spare_datablk_num,0,sizeof(spare_datablk_num));

	file = fopen(file_system_name,"rb+");
	if(NULL == file)
	{
		printf("打开文件失败！\n");
		return false;
	}
	fseek(file,BLOCK_SIZE*18,SEEK_SET);
	fread(data_bitmap,sizeof(data_bitmap),1,file);

	for(column = 0;column < 64;column ++)   //数据块位示图的组成是8 bit * 64 = 512 bit
	{
		for(row = 7;row >= 0;row --)       //扫描每一行的8个bit位
		{
		    temper = data_bitmap[column];
			if( ((temper>>row) & 0x01) == 0)           //表示第column行的位示图的第(8-row) bit值为0，即对应的索引节点为空闲
			{
				trans = column * 8 + (7 - row);             //得到的只是对应的数据块号，还需要将之转换为磁盘号
				spare_datablk_num[count++] = trans + 19;    //数据块号加19之后便是磁盘号
				data_bitmap[column] = data_bitmap[column] | (0x01<<row);   //更改已经分配出去的索引节点位示图（置1）
				if(count == data_block_count)
				{
					file = fopen(file_system_name,"rb+");
					fseek(file,BLOCK_SIZE*18,SEEK_SET);
					fwrite(data_bitmap,sizeof(data_bitmap),1,file);
					fclose(file);
					return true;
				}
			}
		}
	}
	if(count < data_block_count)
	{
	     return false;    //标识已经没有索引节点可供分配了，创建文件失败
	}
}



bool Fill_Index_In_Datablk(int indexblock_no,int buf_block_count)
{
	int i;
	memset(index,0,sizeof(index));
	
	if(Scan_Data_Bitmap(buf_block_count))          //获取buf_block_count个空闲数据块
	{
		for(i = 0;i < buf_block_count; i ++)      //将获取的buf_block_count个空闲数据块号写入indexblock_no指向的数据块中
		{
			index[i] = spare_datablk_num[i];
		}
		file = fopen(file_system_name,"rb+");
	    if(NULL == file)
	    {
		     printf("打开文件失败！\n");
		    return false;
	     }
		fseek(file,BLOCK_SIZE*indexblock_no,SEEK_SET);//indexblock_no直接对应的就是磁盘号
		fwrite(index,sizeof(index),1,file);
		fclose(file);
	}
	else
	{
		return false;
	}
	return true;
}
//将一个大的数据块分割成多个小数据块,data_count标识将big[]的第几块存放在small[]中,size标识每一块的大小
void Spill(char big[],char small[],int data_count,int size) 
{
	int i,j;
	i = (data_count-1)*size;
	j = 0;
	for(;big[i] && j<size;i++,j++)
	{
		small[j] = big[i];
	}
	small[j] = 0;
}

//文件检索操作，输入：文件路径    输出：文件的索引节点号
int Lookup_File(char *path)
{
	int i,j;
	char filename[20];        //临时存放文件名
	int index_node_num ;      //文件对应的索引节点号
	bool found ;              //路径是否有错
	int file_count = 0;       //该路径下有多少个文件夹
	int file_start;           //将路径分割时记录断点的
	inode_t inode;            //根据索引节点号找到对应的索引节点块
	int dir_blk_num;         //目录文件对应的磁盘块号
	for(i=0;path[i];i++)
		if(path[i] == '/')
			file_count++;


	if(path[0] != '/')
	{
		return -1;
	}

	file_start = 1;
	index_node_num = 0;//根目录的索引节点为0
	for(j=0;j<file_count;j++)
	{
		found = false;
		for(i=file_start;path[i]!='/' && path[i];i++)
			filename[i-file_start] = path[i];
		filename[i-file_start] = 0;
		file_start = i+1;
		inode = Find_Inode(index_node_num);
		dir_blk_num = inode.disk_address[0];

		Clear_Dir_Buf();
		file = fopen(file_system_name,"rb+");
	    if(NULL == file)
	    {
		   printf("打开文件失败！\n");
		   return -1;
	    }
		fseek(file,BLOCK_SIZE*dir_blk_num,SEEK_SET);
		fread(dir_buf,sizeof(dir_buf),1,file);
		fclose(file);
		for(i=0;dir_buf[i].name[0];i++)
		{
			if(strcmp(dir_buf[i].name,filename) == 0)
			{	
				index_node_num = dir_buf[i].ino;
				found = true;
				break;
			}
		}
		if(false == found)
		{
			printf("路径有误!\n");
			return -1;
		}
	}
	return index_node_num;
}

//复制文件，支持当前文件夹下当前文件，也支持路径文件复制
bool Copy_File(char filename1[],char filename2[])
{
	int block_num;         //该索引号指向的索引节点所处的盘块号(2#~17#)
	int inode_num;         //该索引节点指向的索引节点在磁盘里的位置(0~7)
	int file_index;        //文件对应的索引号
	int i,j;
	//获取第一个参数所对应的文件的索引节点号
	if( isPath(filename1) )
	{
		file_index = Lookup_File(filename1);
	}
	else
	{
		Lookup_Dir(cur_inode_no);
		for(i=0;dir_buf[i].name[0];i++)
		{
			if(strcmp(dir_buf[i].name,filename1) == 0)
				file_index = dir_buf[i].ino;
		}
	}
	if(!Read_File(file_index))
	{
		printf("Can't read the directory!\n");
		return false;
	}
	//获取第二个参数所对应的文件的索引节点号
	int dir_file_index;                //目录文件的索引节点号
	char file_name[20];                //文件名
	char path[20];                     //文件存放的路径
	int middle;                       //分界
	int count=0;
	i=0;
	j=0;
	if( isPath(filename2) )
	{
		while(filename2[i])
		{
			if(filename2[i] == '/')
				count++;
			i++;
		}
		for(i=0;filename2[i];i++)
		{	
			if(filename2[i] == '/')
				j++;
			if(j == count)
				break;
		}
		middle = i;              //找到参数2的分界
		///////////////////////////////////////////////////
		//将参数2分解成路径和文件名，分别存放在path和file_name中
		for(i=0;i<middle;i++)
			path[i] = filename2[i];
		path[i] = 0;
		for(j=0,i=middle+1;filename2[i];i++,j++)
			file_name[j]=filename2[i];
		file_name[j] = 0;
		dir_file_index = Lookup_File(path);   //根据路径，得到即将创建的文件应存放的文件目录的索引号
	}
	else
	{
		dir_file_index = cur_inode_no;
		strcpy(file_name,filename2);
	}
	if(isLegal(dir_file_index,file_name))
	{
	     Create_File(file_name,dir_file_index);    //创建的文件文件名和文件应在的目录的目录文件的索引号
	}
	else
	{
		printf("\tError!\nThe file has existed!\n");
		return false;
	}
	return true;
}

//判断该参数是文件名还是路径
bool isPath(char *name)
{
	int i = 0;
	while(name[i])
	{
		if(name[i] == '/')     //文件名中不存在‘/’
			return true;
		i++;
	}
	return false;
}

//检索目录，输入目录文件节点，结果是将目录节点对应的目录输入到全局变量dir_buf[]中,同时将该目录文件所在的磁盘号返回
int Lookup_Dir(int index_node)
{
	int block_num;         //该索引号指向的索引节点所处的盘块号(2#~17#)
	int inode_num;         //该索引节点指向的索引节点在磁盘里的位置(0~7)
	int dir_file_block;    //目录文件对应的磁盘块号
	block_num = index_node / 8 + 2;
	inode_num = index_node % 8;
	file = fopen(file_system_name,"rb+");
	if(NULL == file)
	{
		printf("打开文件失败！\n");
		return false;
	}

	fseek(file,BLOCK_SIZE * block_num,SEEK_SET);
	fread(iNode,sizeof(iNode),1,file);
	fclose(file);
	dir_file_block = iNode[inode_num].disk_address[0];     //得到目录文件对应的磁盘号
	Clear_Dir_Buf();
	file = fopen(file_system_name,"rb+");
	fseek(file,BLOCK_SIZE*dir_file_block,SEEK_SET);
	fread(dir_buf,sizeof(dir_buf),1,file);                //将目录文件中的内容读入缓冲dir_buf中
	fclose(file);
	return dir_file_block;
}

//读取文件，根据文件的索引节点号，将对应的文件的内容读取到内存缓冲Buffer中
bool Read_File(int file_index)
{
	//1.读取索引节点号对应的索引节点
	int block_num;         //该索引号指向的索引节点所处的盘块号(2#~17#)
	int inode_num;         //该索引节点指向的索引节点在磁盘里的位置(0~7)
	int file_size;         //索引节点指示该文件的大小，单位是字节数
	int file_block;        //有文件大小计算出该文件所拥有的磁盘数的个数
	block_num = file_index / 8 + 2;
	inode_num = file_index % 8;
	file = fopen(file_system_name,"rb+");
	if(NULL == file)
	{
		printf("打开文件失败！\n");
		return false;
	}

	fseek(file,BLOCK_SIZE * block_num,SEEK_SET);
	fread(iNode,sizeof(iNode),1,file);
	fclose(file);

	if(iNode[inode_num].type == 0)          //如果给定的文件是一个目录文件时，读取失败！
		return false;
	file_size = iNode[inode_num].size;
	file_block = file_size / 512;
	if(file_size % 512 != 0)
	{
		file_block ++;
	}
	//2.根据文件所拥有的磁盘块数，获得磁盘寻址方式
	//磁盘块数目不大于10时，文件采用直接寻址
	int i,j;
	int block_index;
	if( file_block <= 10 )
	{
		for(i=0;i<file_block;i++)
		{	
			block_index = iNode[inode_num].disk_address[i];
			Get_Data(block_index);
			Merge(i);
		}
	}
	//磁盘块数目不大于128时,采用一级间址
	else if( file_block <= 128 )
	{
		block_index = iNode[inode_num].disk_address[10];
		file = fopen(file_system_name,"rb+");
		fseek(file,BLOCK_SIZE * block_index,SEEK_SET);
		fread(index,sizeof(index),1,file);                    //先将索引磁盘中内容读取出来
		fclose(file);
		for(i = 0;i < file_block;i ++)
		{
			block_index = index[i];                          //遍历索引磁盘中的项，得到文件的各个磁盘块号
			Get_Data(block_index);                           //读取各个磁盘块，获取其中的内容
			Merge(i);                                        //将各磁盘块的内容合并归入Buffer中
		}
	}
	//磁盘块数目不大于256时,两个采用一级间址
	else if(file_block <= 128 * 2)
	{
		block_index = iNode[inode_num].disk_address[10];
		file = fopen(file_system_name,"rb+");
		fseek(file,BLOCK_SIZE * block_index,SEEK_SET);
		fread(index,sizeof(index),1,file);                    //先将索引磁盘中内容读取出来
		fclose(file);
		for(i = 0;i < 128;i ++)
		{
			block_index = index[i];                          //遍历索引磁盘中的项，得到文件的各个磁盘块号
			Get_Data(block_index);                           //读取各个磁盘块，获取其中的内容
			Merge(i);                                        //将各磁盘块的内容合并归入Buffer中
		}

		block_index = iNode[inode_num].disk_address[11];
		file = fopen(file_system_name,"rb+");
		fseek(file,BLOCK_SIZE * block_index,SEEK_SET);
		fread(index,sizeof(index),1,file);                    //先将索引磁盘中内容读取出来
		fclose(file);
		for(i = 0;i < file_block - 128;i ++)
		{
			block_index = index[i];                          //遍历索引磁盘中的项，得到文件的各个磁盘块号
			Get_Data(block_index);                           //读取各个磁盘块，获取其中的内容
			Merge(i + 128);                                        //将各磁盘块的内容合并归入Buffer中
		}
	}
	//否则，采用一个二级间址
	else
	{
		//暂时不写
		int temp_len;
		int temp[128];
		int buffer_part = 0;
		block_index = iNode[inode_num].disk_address[12];
		file = fopen(file_system_name,"rb+");
		fseek(file,BLOCK_SIZE * block_index,SEEK_SET);
		fread(index,sizeof(index),1,file);                    //先将索引磁盘中内容读取出来
		fclose(file);
		for(i=0;index[i]!=0;i++)
		{
			temp[i] = index[i];
		}
		temp_len = i;
		for(i=0;i<temp_len-1;i++)
		{
			block_index = temp[i];
			file = fopen(file_system_name,"rb+");
		    fseek(file,BLOCK_SIZE * block_index,SEEK_SET);
		    fread(index,sizeof(index),1,file);                    //先将索引磁盘中内容读取出来
		    fclose(file);
			for(j = 0;j < 128;j ++)
		    {
			   block_index = index[j];                          //遍历索引磁盘中的项，得到文件的各个磁盘块号
			   Get_Data(block_index);                           //读取各个磁盘块，获取其中的内容
			   Merge(buffer_part);                              //将各磁盘块的内容合并归入Buffer中
			   buffer_part ++;
		    }
		}
		block_index = temp[i];
		file = fopen(file_system_name,"rb+");
		fseek(file,BLOCK_SIZE * block_index,SEEK_SET);
		fread(index,sizeof(index),1,file);                    //先将索引磁盘中内容读取出来
		fclose(file);
		for(j = 0;j < file_block - 128 * (temp_len-1);j ++)
		{
			block_index = index[j];                          //遍历索引磁盘中的项，得到文件的各个磁盘块号
			Get_Data(block_index);                           //读取各个磁盘块，获取其中的内容
			Merge(buffer_part);                              //将各磁盘块的内容合并归入Buffer中
			buffer_part ++;
		}
	}

	return true;
}

//通过磁盘块号，将磁盘上的数据读入缓冲区Block_Data中
bool Get_Data(int block_index)
{
	memset(Block_Data,0,sizeof(Block_Data));
	file = fopen(file_system_name,"rb+");
	if(NULL == file)
	{
		printf("打开文件失败！\n");
		return false;
	}
	fseek(file,BLOCK_SIZE*block_index,SEEK_SET);
	fread(Block_Data,sizeof(Block_Data),1,file);

	fclose(file);
}


//合并函数，用于将每次读出的一个磁盘块上的数据Block_Data归并到Buffer中count为第几个磁盘块上的数据
bool Merge(int count)
{
	int i,j;
	j = count * BLOCK_SIZE;
	for(i=0;Block_Data[i]!=0;i++,j++)
		Buffer[j] = Block_Data[i];
	Buffer[j] = 0;                             //清楚可能出现的烂尾巴
	return true;
}

bool Copy_File_out(char filename1[],char filename2[])
{
	int file_index;        //文件对应的索引号
	bool find;
	int i,j;
	//获取第一个参数所对应的文件的索引节点号
	if( isPath(filename1) )
	{
		file_index = Lookup_File(filename1);
		if(file_index == -1)
		{
			printf("文件路径有问题!\n");
			return false;
		}
	}
	else
	{
		Lookup_Dir(cur_inode_no);
		find = false;
		for(i=0;dir_buf[i].name[0];i++)
		{
			if(strcmp(dir_buf[i].name,filename1) == 0)
			{	
				file_index = dir_buf[i].ino;
				find = true;
			}
		}
		if(find == false)
		{
			printf("要复制的文件不存在！\n");
			return false;
		}
	}
	if(!Read_File(file_index))                      //将参数1指定的文件中的内容读取到Buffer中
	{	
	 	printf("Can't read the directory!\n");
		return false;
	}
	file = fopen(filename2,"w");
	if(NULL == file)
	{
		printf("文件创建失败！\n");
		return false;
	}
	fseek(file,0,SEEK_SET);
	fwrite(Buffer,strlen(Buffer),1,file);
	fclose(file);
	return true;
}

bool Copy_File_In(char filename1[],char filename2[])
{
	file = fopen(filename1,"rb+");
	if(NULL == file)
	{
		printf("文件创建失败！\n");
		return false;
	}
	fseek(file,0,SEEK_SET);
	fread(Buffer,sizeof(Buffer),1,file);        //将物理磁盘文件中的内容读入Buffer中
	fclose(file);

	//获取第二个参数所对应的文件的索引节点号
	int dir_file_index;                //目录文件的索引节点号
	char file_name[20];                //文件名
	char path[20];                     //文件存放的路径
	int middle;                       //分界
	int count=0;
	int i=0;
	int j=0;
	if( isPath(filename2) )
	{
		while(filename2[i])
		{
			if(filename2[i] == '/')
				count++;
			i++;
		}
		for(i=0;filename2[i];i++)
		{	
			if(filename2[i] == '/')
				j++;
			if(j == count)
				break;
		}
		middle = i;              //找到参数2的分界
		///////////////////////////////////////////////////
		//将参数2分解成路径和文件名，分别存放在path和file_name中
		for(i=0;i<middle;i++)
			path[i] = filename2[i];
		path[i] = 0;
		for(j=0,i=middle+1;filename2[i];i++,j++)
			file_name[j]=filename2[i];
		file_name[j] = 0;
		dir_file_index = Lookup_File(path);   //根据路径，得到即将创建的文件应存放的文件目录的索引号
	}
	else
	{
		dir_file_index = cur_inode_no;
		strcpy(file_name,filename2);
	}
	if(isLegal(dir_file_index,file_name))
	{
	    Create_File(file_name,dir_file_index);    //创建的文件文件名和文件应在的目录的目录文件的索引号
	}
	else
	{
		printf("\tError!\nThe file has existed!\n");
		return false;
	}
	return true;
}

bool Delete_File(char filename[])
{
	int i,j;
	bool find;
	int file_index;                         //待删除的文件的索引号
	int father_block;                       //当前文件夹所在的磁盘块
	father_block = Lookup_Dir(cur_inode_no);

	find = false;
	for(i=0;dir_buf[i].name[0];i++)
	{
		if(strcmp(dir_buf[i].name,filename) == 0)   //在当前目录下找到该文件名，将其在该目录文件中删除，同时将对应的文件索引节点记下
		{
			file_index = dir_buf[i].ino;
			find = true;
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
	if(find == false)
	{
		printf("Can't find the file to be deleted!\n");
		return false;
	}
	//2.通过文件的索引节点号在索引节点位示图中置相应bit为0
	Delete_From_Inode_Bitmap(file_index);

	//3.根据文件的索引节点号找到对应的索引节点，计算出该文件拥有的数据块
	//同时在数据块位示图中置相应bit为0
	int block_num;         //该索引号指向的索引节点所处的盘块号(2#~17#)
	int inode_num;         //该索引节点指向的索引节点在磁盘里的位置(0~7)

	int file_size;        //记载文件的大小，单位为字节
	int file_block;       //记录文件所占的数据块数目
	block_num = file_index / 8 + 2;
	inode_num = file_index % 8;

	file = fopen(file_system_name,"rb+");
	fseek(file,BLOCK_SIZE*block_num,SEEK_SET);
	fread(iNode,sizeof(iNode),1,file);
	fclose(file);

	file_size = iNode[inode_num].size;
	file_block = file_size / 512;
	if(file_size % 512 != 0)
		file_block ++;
	int block_index;
	if(file_block <= 10)
	{
		for(i=0;i<file_block;i++)
		{
			block_index = iNode[inode_num].disk_address[i];     //每次获得一个数据块的索引节点
			Delete_From_Data_Bitmap(block_index);               //根据这个数据块的索引节点号，该数据块位示图中相应的bit位置0
		}
	}
	else if(file_block <= 128)
	{
		block_index = iNode[inode_num].disk_address[10];
		file = fopen(file_system_name,"rb+");
		fseek(file,BLOCK_SIZE*block_index,SEEK_SET);
		fread(index,sizeof(index),1,file);
		fclose(file);
		Delete_From_Data_Bitmap(block_index);               //将索引数据块读出来之后，便可以释放索引数据块的索引号了
		for(i=0;i<file_block;i++)
		{
			block_index = index[i];
			Delete_From_Data_Bitmap(block_index);               //根据这个数据块的索引节点号，该数据块位示图中相应的bit位置0
		}
	}
	else if(file_block <= 128*2)
	{
		//先对第一个索引数据块进行释放索引节点
		block_index = iNode[inode_num].disk_address[10];
		file = fopen(file_system_name,"rb+");
		fseek(file,BLOCK_SIZE*block_index,SEEK_SET);
		fread(index,sizeof(index),1,file);
		fclose(file);
		Delete_From_Data_Bitmap(block_index);               //将索引数据块读出来之后，便可以释放索引数据块的索引号了
		for(i=0;i<128;i++)
		{
			block_index = index[i];
			Delete_From_Data_Bitmap(block_index);               //根据这个数据块的索引节点号，该数据块位示图中相应的bit位置0
		}
		//再对第二个索引数据块进行释放索引节点
		block_index = iNode[inode_num].disk_address[11];
		file = fopen(file_system_name,"rb+");
		fseek(file,BLOCK_SIZE*block_index,SEEK_SET);
		fread(index,sizeof(index),1,file);
		fclose(file);
		Delete_From_Data_Bitmap(block_index);               //将索引数据块读出来之后，便可以释放索引数据块的索引号了
		for(i=0;i<file_block-128;i++)
		{
			block_index = index[i];
			Delete_From_Data_Bitmap(block_index);               //根据这个数据块的索引节点号，该数据块位示图中相应的bit位置0
		}
	}
	else
	{
		//二级间址部分，暂时未包含
		int temp[128];
		int temp_len;
		block_index = iNode[inode_num].disk_address[12];
		file = fopen(file_system_name,"rb+");
		fseek(file,BLOCK_SIZE*block_index,SEEK_SET);
		fread(index,sizeof(index),1,file);
		fclose(file);
		for(i=0;index[i];i++)
			temp[i] = index[i];
		temp_len = i;
		Delete_From_Data_Bitmap(block_index);               //将索引数据块读出来之后，便可以释放索引数据块的索引号了
		for(i=0;i<temp_len-1;i++)
		{
			block_index = temp[i];                         //查找第i个二级索引块
			file = fopen(file_system_name,"rb+");
			fseek(file,BLOCK_SIZE*block_index,SEEK_SET);
			fread(index,sizeof(index),1,file);
			fclose(file);
			Delete_From_Data_Bitmap(block_index);           //将索引数据块读出来之后，便可以释放索引数据块的索引号了
			for(j=0;j<128;j++)
		    {
			    block_index = index[j];
			    Delete_From_Data_Bitmap(block_index);      //释放二级索引块对应的数据块
		    }
		}
		//最后一个二级索引块需要特别处理
		block_index = temp[i];                         //查找最后一个二级索引块
		file = fopen(file_system_name,"rb+");
		fseek(file,BLOCK_SIZE*block_index,SEEK_SET);
		fread(index,sizeof(index),1,file);
		fclose(file);
		Delete_From_Data_Bitmap(block_index);           //将索引数据块读出来之后，便可以释放索引数据块的索引号了
		for(j=0;j<file_block - 128*(temp_len-1);j++)
		{
			block_index = index[j];
			Delete_From_Data_Bitmap(block_index);      //释放二级索引块对应的数据块
		}
	}
	fclose(file);
	return true;
}

 //根据给定的文件的索引节点，在索引节点位示图中将对应的bit位置0
bool Delete_From_Inode_Bitmap(int file_index)
{
	int column,row;
	row = file_index / 8;
	column = file_index % 8;
	file = fopen(file_system_name,"rb+");
	if(NULL == file)
	{
		printf("打开文件失败！\n");
		return false;
	}
	fseek(file,BLOCK_SIZE,SEEK_SET);
	fread(inode_bitmap,sizeof(inode_bitmap),1,file);
	fclose(file);
	switch(column)
	{
	case 0:
		inode_bitmap[row] = inode_bitmap[row] & 0x7f;
		break;
	case 1:
		inode_bitmap[row] = inode_bitmap[row] & 0xbf;
		break;
	case 2:
		inode_bitmap[row] = inode_bitmap[row] & 0xdf;
		break;
	case 3:
		inode_bitmap[row] = inode_bitmap[row] & 0xef;
		break;
	case 4:
		inode_bitmap[row] = inode_bitmap[row] & 0xf7;
		break;
	case 5:
		inode_bitmap[row] = inode_bitmap[row] & 0xfb;
		break;
	case 6:
		inode_bitmap[row] = inode_bitmap[row] & 0xfd;
		break;
	case 7:
		inode_bitmap[row] = inode_bitmap[row] & 0xfe;
		break;
	}
	file = fopen(file_system_name,"rb+");
	fseek(file,BLOCK_SIZE,SEEK_SET);
	fwrite(inode_bitmap,sizeof(inode_bitmap),1,file);
	fclose(file);
	return true;
}

 //根据给定的磁盘的索引节点，在数据块位示图中将对应的bit位置0
bool Delete_From_Data_Bitmap(int block_index_no)
{
	int column,row;
	int block_index = block_index_no - 19;
	row = block_index / 8;
	column = block_index % 8;
	file = fopen(file_system_name,"rb+");
	if(NULL == file)
	{
		printf("打开文件失败！\n");
		return false;
	}
	fseek(file,BLOCK_SIZE*18,SEEK_SET);
	fread(data_bitmap,sizeof(data_bitmap),1,file);
	fclose(file);
	switch(column)
	{
	case 0:
		data_bitmap[row] = data_bitmap[row] & 0x7f;
		break;
	case 1:
		data_bitmap[row] = data_bitmap[row] & 0xbf;
		break;
	case 2:
		data_bitmap[row] = data_bitmap[row] & 0xdf;
		break;
	case 3:
		data_bitmap[row] = data_bitmap[row] & 0xef;
		break;
	case 4:
		data_bitmap[row] = data_bitmap[row] & 0xf7;
		break;
	case 5:
		data_bitmap[row] = data_bitmap[row] & 0xfb;
		break;
	case 6:
		data_bitmap[row] = data_bitmap[row] & 0xfd;
		break;
	case 7:
		data_bitmap[row] = data_bitmap[row] & 0xfe;
		break;
	}
	file = fopen(file_system_name,"rb+");
	fseek(file,BLOCK_SIZE*18,SEEK_SET);
	fwrite(data_bitmap,sizeof(data_bitmap),1,file);
	fclose(file);
	return true;
}

bool Clear_Dir_Buf()
{
	int i;
	for(i=0;i<32;i++)
		memset(dir_buf[i].name,0,sizeof(dir_buf[i].name));
	return true;
}

//显示文件内容，如果参数给定的是路径，必须能够根据路径找到文件并显示其内容
bool Show_Content(char FileName[])
{
	int file_index;                     //待显示的文件索引
	int dir_file_index;                //目录文件的索引节点号
	char file_name[20];                //文件名
	char path[20];                     //文件存放的路径
	int middle;                       //分界
	int count=0;
	int i=0;
	int j=0;
	if( isPath(FileName) )
	{
		while(FileName[i])
		{
			if(FileName[i] == '/')
				count++;
			i++;
		}
		for(i=0;FileName[i];i++)
		{	
			if(FileName[i] == '/')
				j++;
			if(j == count)
				break;
		}
		middle = i;              //找到参数2的分界
		///////////////////////////////////////////////////
		//将参数2分解成路径和文件名，分别存放在path和file_name中
		for(i=0;i<middle;i++)
			path[i] = FileName[i];
		path[i] = 0;
		for(j=0,i=middle+1;FileName[i];i++,j++)
			file_name[j]=FileName[i];
		file_name[j] = 0;
		dir_file_index = Lookup_File(path);   //根据路径，将待读取的文件所在的目录文件读到缓存dir_buf中
		Lookup_Dir(dir_file_index);
		for(i=2;dir_buf[i].name[0];i++)
		{
			if(strcmp(dir_buf[i].name,file_name) == 0)
				file_index = dir_buf[i].ino;
		}

	}
	else
	{
		Lookup_Dir(cur_inode_no);
		for(i=2;dir_buf[i].name[0];i++)
		{
			if(strcmp(dir_buf[i].name,FileName) == 0)
				file_index = dir_buf[i].ino;
		}
	}
	if(!Read_File(file_index))
	{
		printf("Can't read the directory!\n");
		return false;
	}
	for(i=0;i<strlen(Buffer);i++)
	{	
		printf("%c",Buffer[i]);
	}
	printf("\n");
	return true;
}

//创建一个新文件，文件内容有用户自己输入
bool New_file(char filename[])
{
	if( !isLegal(cur_inode_no,filename) )
	{
		printf("\tError!\nThe file has existed!\n");
		return false;
	}
	char selection;
	int count = 0;
	int i;
	char temp[100];
	printf("Please edit now:(End with ##)\n");
	gets(temp);
	while( !(temp[0] == '#' && temp[1] == '#') )
	{
		for(i=0;temp[i];i++)
			Buffer[count++] = temp[i];
		Buffer[count++] = 10;
		gets(temp);
	}
	count -- ;
	Buffer[count] = 0;
	Create_File(filename,cur_inode_no);
	return true;
}

//在指定的目录下，该文件名是否合法
bool isLegal(int dir_index,char filename[])
{
	Lookup_Dir(dir_index);
	int i;
	for(i=2;dir_buf[i].name[0];i++)
	{
		if(strcmp(dir_buf[i].name,filename) == 0)
			return false;
	}
	return true;
}