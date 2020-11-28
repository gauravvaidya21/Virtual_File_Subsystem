/*
	Virtual File Subsystem.
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<iostream>

//for maximum inode size
#define MAXINODE 50

//for read and write permission
#define READ 1
#define WRITE 2

//for maximum file size
#define MAXFILESIZE 1024

//for file type we deal with regular file useually
#define REGULAR 1
#define SPECIAL 2

//for offset used in lseek
#define START 0
#define CURRENT 1
#define END 2


//bellow structure maintains record for free and consumed inode
typedef struct superblock
{
	int TotalInodes;
	int FreeInode;
}SUPERBLOCK, *PSUPERBLOCK;


//bellow structure maintains record for file
typedef struct inode
{
	char FileName[50];
	int InodeNumber;
	int FileSize;
	int FileActualSize;
	int FileType;
	char *Buffer;
	int LinkCount;
	int ReferenceCount;
	int permission; // 1 or 2 or 3

	struct inode *next;

}INODE,*PINODE,**PPINODE;


//bellow structure maintains record of open file
typedef struct filetable
{
	int readoffset;
	int writeoffset;
	int count;
	int mode; // 1 or 2 or 3

	PINODE ptrinode;

}FILETABLE,*PFILETABLE;


//bellow structure have pointer which points to filetable 
typedef struct ufdt
{
	PFILETABLE ptrfiletable;
}UFDT;


//global array file discriptor,superblock object and head for holding DILB linklist
	UFDT UFDTArr[50];
	SUPERBLOCK SUPERBLOCKobj;
	PINODE head = NULL;


//function to display information and usage of functions used in application  
void man(char *name)
{

	if(name == NULL) return;

	if(strcmp(name,"create") == 0)
	{

		printf("Description : Used to create new regular file\n");

		printf("Usage : create File_name Permission\n");
	}
	else if(strcmp(name,"read") == 0)
	{

		printf("Description : Used to read data from regular file\n");

		printf("Usage : read File_name No_Of_Bytes_To_Read\n");
	}
	else if(strcmp(name,"write") == 0)
	{

		printf("Description : Used to write into regular file\n");

		printf("Usage : write File_name\n After this enter the data that we want to write\n");
	}
	else if(strcmp(name,"ls") == 0)
	{

		printf("Description : Used to list all information of files\n");

		printf("Usage : ls\n");
	}
	else if(strcmp(name,"stat") == 0)
	{

		printf("Description : Used to display information of file\n");

		printf("Usage : stat File_name\n");
	}
	else if(strcmp(name,"fstat") == 0)
	{

		printf("Description : Used to display information of file\n");

		printf("Usage : stat File_Descriptor\n");
	}
	else if(strcmp(name,"truncate") == 0)
	{

		printf("Description : Used to remove data from file\n");

		printf("Usage : truncate File_name\n");
	}
	else if(strcmp(name,"open") == 0)
	{

		printf("Description : Used to open existing file\n");

		printf("Usage : open File_name mode\n");
	}
	else if(strcmp(name,"close") == 0)
	{

		printf("Description : Used to close opened file\n");

		printf("Usage : close File_name\n");
	}
	else if(strcmp(name,"closeall") == 0)
	{

		printf("Description : Used to close all opened file\n");

		printf("Usage : closeall\n");
	}
	else if(strcmp(name,"lseek") == 0)
	{

		printf("Description : Used to change file offset\n");

		printf("Usage : lseek File_Name ChangeInOffset StartPoint\n");
	}
	else if(strcmp(name,"rm") == 0)
	{

		printf("Description : Used to delete the file\n");

		printf("Usage : rm File_Name\n");
	}
	else
	{
	
		printf("ERROR : No manual entry available.\n");
	}
}

//function to display information of operations perform on file
void DisplayHelp()
{
	printf("ls : To List out all files\n");
	printf("clear : To clear console\n");
	printf("open : To open the file\n");
	printf("close : To close the file\n");
	printf("closeall : To close all opened files\n");
	printf("read : To Read the contents from file\n");
	printf("write :To Write contents into file\n");
	printf("exit : To Terminate file system\n");
	printf("stat : To Display information of file using name\n");
	printf("fstat :To Display information of file using file descriptor\n");
	printf("truncate : To Remove all data from file\n");
	printf("rm : To Delet the file\n");
}


//function to get file discribtor from file name
int GetFDFromName(char *name)
{
	int i = 0;

	while(i<50)
	{
		if(UFDTArr[i].ptrfiletable != NULL)
	
			if(strcmp((UFDTArr[i].ptrfiletable->ptrinode->FileName),name)==0)
					break;
	
					i++;
	}
	if(i == 50)	return -1;
	else		return i;
}

//function to get inode from file name
PINODE Get_Inode(char * name)
{
	PINODE temp = head;
	int i = 0;

	if(name == NULL)

		return NULL;

	while(temp!= NULL)
	{

		if(strcmp(name,temp->FileName) == 0)
			break;

		temp = temp->next;
	}
	return temp;
}


/*
	creation of linked list of inodes which is DILB block of file system.
*/

void CreateDILB()
{
	int i = 1;
	PINODE newn = NULL;
	PINODE temp = head;

	//allocating memory for new node and initializing it's members
	while(i<= MAXINODE)
	{
		
		newn = (PINODE)malloc(sizeof(INODE));

		newn->LinkCount =0;

		newn->ReferenceCount = 0;

		newn->FileType = 0;

		newn->FileSize = 0;

		newn->Buffer = NULL;

		newn->next = NULL;

		newn->InodeNumber = i;

		if(temp == NULL)
		{
			head = newn;
			temp = head;
		}
		else
		{
			temp->next = newn;
			temp = temp->next;
		}

		i++;
	}
		printf("DILB created successfully\n");
}


void InitialiseSuperBlock()
{
	int i = 0;
	while(i< MAXINODE)
	{

		UFDTArr[i].ptrfiletable = NULL;
		i++;
	}

	SUPERBLOCKobj.TotalInodes = MAXINODE;
	SUPERBLOCKobj.FreeInode = MAXINODE;
}


//actual file creation
int CreateFile(char *name,int permission)
{
	int i = 0;
	PINODE temp = head; //copying head of DILB in temp 

	//input validation
	if((name == NULL) || (permission == 0) || (permission > 3))

		return -1;

	//checking for free inode
	if(SUPERBLOCKobj.FreeInode == 0)
		
		return -2;

	//decrementing count of free inode
	(SUPERBLOCKobj.FreeInode)--;

	//checking for same file name exist or not
	if(Get_Inode(name) != NULL)

		return -3;

	//checking for free inode in DILB 
	while(temp!= NULL)
	{

		if(temp->FileType == 0)

			break;

		temp=temp->next;
	}	

	//checking for free file discribtor
	while(i<50)
	{

		if(UFDTArr[i].ptrfiletable == NULL)

			break;

		i++;
	}

	//allocating memory to file table
	UFDTArr[i].ptrfiletable = (PFILETABLE)malloc(sizeof(FILETABLE));

	//inialize file table members
	UFDTArr[i].ptrfiletable->count = 1;
	UFDTArr[i].ptrfiletable->mode = permission;
	UFDTArr[i].ptrfiletable->readoffset = 0;
	UFDTArr[i].ptrfiletable->writeoffset = 0;

	//file table pointer pointing to new inode
	UFDTArr[i].ptrfiletable->ptrinode = temp;

	//initializing members of inode
	strcpy(UFDTArr[i].ptrfiletable->ptrinode->FileName,name);
	UFDTArr[i].ptrfiletable->ptrinode->FileType = REGULAR;
	UFDTArr[i].ptrfiletable->ptrinode->ReferenceCount = 1;
	UFDTArr[i].ptrfiletable->ptrinode->LinkCount = 1;
	UFDTArr[i].ptrfiletable->ptrinode->FileSize = MAXFILESIZE;
	UFDTArr[i].ptrfiletable->ptrinode->FileActualSize = 0;
	UFDTArr[i].ptrfiletable->ptrinode->permission = permission;
	UFDTArr[i].ptrfiletable->ptrinode->Buffer = (char *)malloc(MAXFILESIZE);

	//return file discriptor of file
	return i;
}


//deleting file from file system
int rm_File(char * name)
{
	int fd = 0;

	fd = GetFDFromName(name);
	if(fd == -1)

		return -1;

	//decementing link count by 1
	(UFDTArr[fd].ptrfiletable->ptrinode->LinkCount)--;

	//if link is 0 free file table for file
	if(UFDTArr[fd].ptrfiletable->ptrinode->LinkCount == 0)
	{
		UFDTArr[fd].ptrfiletable->ptrinode->FileType = 0;

		free(UFDTArr[fd].ptrfiletable);
	}
	UFDTArr[fd].ptrfiletable = NULL;

	//increment freeinode by one 
	(SUPERBLOCKobj.FreeInode)++;
}


//reading from file
int ReadFile(int fd, char *arr, int isize)
{
	int read_size = 0;

	//validating file discribtor
	if(UFDTArr[fd].ptrfiletable == NULL)
		return -1;

	//validating file permissions and mode
	if(UFDTArr[fd].ptrfiletable->mode !=READ && UFDTArr[fd].ptrfiletable->mode !=READ+WRITE)
		return -2;

	if(UFDTArr[fd].ptrfiletable->ptrinode->permission != READ && UFDTArr[fd].ptrfiletable->ptrinode->permission != READ+WRITE)
		 return -2;

	//validation for offset
	if(UFDTArr[fd].ptrfiletable->readoffset == UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize)
		return -3;
	
	//file type validation
	if(UFDTArr[fd].ptrfiletable->ptrinode->FileType != REGULAR)
		return -4;

	//checking file read offset
	read_size = (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) - (UFDTArr[fd].ptrfiletable->readoffset);
	if(read_size < isize)
	{

		//copying expected size data into character array
		strncpy(arr,(UFDTArr[fd].ptrfiletable->ptrinode->Buffer) + (UFDTArr[fd].ptrfiletable->readoffset),read_size);

		UFDTArr[fd].ptrfiletable->readoffset = UFDTArr[fd].ptrfiletable->readoffset + read_size;
	}
	else
	{

		strncpy(arr,(UFDTArr[fd].ptrfiletable->ptrinode->Buffer) + (UFDTArr[fd].ptrfiletable->readoffset),isize);

		(UFDTArr[fd].ptrfiletable->readoffset) = (UFDTArr[fd].ptrfiletable->readoffset) + isize;
	}
	
	return isize;
}

 
//writing data into file
int WriteFile(int fd, char *arr, int isize)
{
	//validation for mode to access file
	if(((UFDTArr[fd].ptrfiletable->mode) !=WRITE) && ((UFDTArr[fd].ptrfiletable->mode) !=READ+WRITE)) 
		return -1;
	
	//validating permission for file
	if(((UFDTArr[fd].ptrfiletable->ptrinode->permission) !=WRITE) && ((UFDTArr[fd].ptrfiletable->ptrinode->permission) != READ+WRITE))
		return -1;

	//validating if offset reches to end of file
	if((UFDTArr[fd].ptrfiletable->writeoffset) == MAXFILESIZE) 
		return -2;

	//validation for file type
	if((UFDTArr[fd].ptrfiletable->ptrinode->FileType) != REGULAR) 
		return -3;

	//copying data into file data buffer
	strncpy((UFDTArr[fd].ptrfiletable->ptrinode->Buffer) + (UFDTArr[fd].ptrfiletable->writeoffset),arr,isize);

	//setting write offset to new offset after writting
	(UFDTArr[fd].ptrfiletable->writeoffset) = (UFDTArr[fd].ptrfiletable->writeoffset )+ isize;

	//setting actual file size to new size after writting
	(UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) = (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) + isize;

	return isize;
}

//opening existing file on system
int OpenFile(char *name, int mode)
{
	int i = 0;
	PINODE temp = NULL;

	//validating input credentials
	if(name == NULL || mode <= 0)
		return -1;

	//get inode for file
	temp = Get_Inode(name);
	if(temp == NULL)
		return -2;
	
	//validating file permissions
	if(temp->permission < mode)
		return -3;
	
	//checking empty file table entry.
	while(i<50)
	{
		if(UFDTArr[i].ptrfiletable == NULL)
			break;
		i++;
	}	
	
	//allocating memory for file table
	UFDTArr[i].ptrfiletable = (PFILETABLE)malloc(sizeof(FILETABLE));
	if(UFDTArr[i].ptrfiletable == NULL)
		return -1;

	//setting file count which indicate how many processes opens same file 
	//but in this application only one process opens file
	UFDTArr[i].ptrfiletable->count = 1;

	//setting mode to open file	
	UFDTArr[i].ptrfiletable->mode = mode;
	if(mode == READ + WRITE)
	{
		UFDTArr[i].ptrfiletable->readoffset = 0;

		UFDTArr[i].ptrfiletable->writeoffset = 0;
	}
	else if(mode == READ)
	{
		UFDTArr[i].ptrfiletable->readoffset = 0;
	}
	else if(mode == WRITE)
	{
		UFDTArr[i].ptrfiletable->writeoffset = 0;
	}

	//setting file table pointer to inode in DILB
	UFDTArr[i].ptrfiletable->ptrinode = temp;

	//incrementing reference count 	
	(UFDTArr[i].ptrfiletable->ptrinode->ReferenceCount)++;

	//returning file discribtor
	return i;
}

//closing file by file discriptor
void CloseFileByFD(int fd)
{
	//setting read and write offset to 0
	UFDTArr[fd].ptrfiletable->readoffset = 0;
	UFDTArr[fd].ptrfiletable->writeoffset = 0;

	//decrement reference count by one 
	(UFDTArr[fd].ptrfiletable->ptrinode->ReferenceCount)--;
}


//closing file by file name
int CloseFileByName(char *name)
{
	int i = 0;
	i = GetFDFromName(name);
	if(i == -1)
		return -1;

	//setting read and write offset to 0
	UFDTArr[i].ptrfiletable->readoffset = 0;
	UFDTArr[i].ptrfiletable->writeoffset = 0;

	//decrement reference count by one	
	(UFDTArr[i].ptrfiletable->ptrinode->ReferenceCount)--;
		return 0;
}

//closing all file fd 
void CloseAllFile()
{
	int i = 0;
	while(i<50)
	{
		if(UFDTArr[i].ptrfiletable != NULL)
		{
			UFDTArr[i].ptrfiletable->readoffset = 0;

			UFDTArr[i].ptrfiletable->writeoffset = 0;

			(UFDTArr[i].ptrfiletable->ptrinode->ReferenceCount)--;
		
			break;
		}
		i++;
	}
}

//to move read or write offset of file at perticuler position
int LseekFile(int fd, int size, int from)
{
	//validating input credential
	if((fd<0) || (from > 2)) 
		return -1;

	//validating filetable
	if(UFDTArr[fd].ptrfiletable == NULL)
		 return -1;

	//for read and write both operations
	if((UFDTArr[fd].ptrfiletable->mode == READ) || (UFDTArr[fd].ptrfiletable->mode ==READ+WRITE))
	{
		//to move offset from current position
		if(from == CURRENT)
		{
			if(((UFDTArr[fd].ptrfiletable->readoffset) + size) > UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize)
				return -1;

			if(((UFDTArr[fd].ptrfiletable->readoffset) + size) < 0) 
				return -1;

			(UFDTArr[fd].ptrfiletable->readoffset) = (UFDTArr[fd].ptrfiletable->readoffset) +size;
		}
		else if(from == START)//to move offset from starting position
		{
			if(size > (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize))
				 return -1;
			if(size < 0) 
				return -1;

			(UFDTArr[fd].ptrfiletable->readoffset) = size;
		}
		else if(from == END)//to move offset from end of actual size
		{
			if((UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) + size > MAXFILESIZE)
				return -1;

			if(((UFDTArr[fd].ptrfiletable->readoffset) + size) < 0) 
				return -1;

			(UFDTArr[fd].ptrfiletable->readoffset) = (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) + size;
		}
	}
	else if(UFDTArr[fd].ptrfiletable->mode == WRITE)//for only write operation
	{
		if(from == CURRENT)
		{
			if(((UFDTArr[fd].ptrfiletable->writeoffset) + size) > MAXFILESIZE)
				return -1;

			if(((UFDTArr[fd].ptrfiletable->writeoffset) + size) < 0)
				return -1;

			if(((UFDTArr[fd].ptrfiletable->writeoffset) + size) > (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize))

				(UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) =(UFDTArr[fd].ptrfiletable->writeoffset) + size;

				(UFDTArr[fd].ptrfiletable->writeoffset) = (UFDTArr[fd].ptrfiletable->writeoffset) +size;
		}
		else if(from == START)
		{
			if(size > MAXFILESIZE)
				 return -1;
			if(size < 0) 
				return -1;

			if(size > (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize))

				(UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) = size;

				(UFDTArr[fd].ptrfiletable->writeoffset) = size;

		}
		else if(from == END)
		{
			if((UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) + size > MAXFILESIZE)
					return -1;

			if(((UFDTArr[fd].ptrfiletable->writeoffset) + size) < 0) 
					return -1;

			(UFDTArr[fd].ptrfiletable->writeoffset) = (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) + size;
		}
	}
}		


//to display information of all files  
void ls_file()
{
	int i = 0;
	PINODE temp = head;

	//if all inodes are free
	if(SUPERBLOCKobj.FreeInode == MAXINODE)
	{

		printf("Error : There are no files\n");
			return;
	}

	printf("\nFile Name\tInode number\tFile size\tLink count\n");
	printf("---------------------------------------------------------------\n");
	while(temp != NULL)
	{

		if(temp->FileType != 0)
		{
			printf("%s\t\t%d\t\t%d\t\t%d\n",temp->FileName,temp->InodeNumber,temp->FileActualSize,temp->LinkCount);
		}

		temp = temp->next;
	}
	printf("---------------------------------------------------------------\n");
}

//to display stastical information of file using file discriptor
int fstat_file(int fd)
{
	PINODE temp = head;
	int i = 0;

	if(fd < 0)
		return -1;
	
	if(UFDTArr[fd].ptrfiletable == NULL)
		 return -2;

	temp = UFDTArr[fd].ptrfiletable->ptrinode;

	printf("\n---------Statistical Information about file----------\n");
	printf("File name : %s\n",temp->FileName);
	printf("Inode Number %d\n",temp->InodeNumber);
	printf("File size : %d\n",temp->FileSize);
	printf("Actual File size : %d\n",temp->FileActualSize);
	printf("Link count : %d\n",temp->LinkCount);
	printf("Reference count : %d\n",temp->ReferenceCount);

	if(temp->permission == 1)
		printf("File Permission : Read only\n");
	else if(temp->permission == 2)
		printf("File Permission : Write\n");
	else if(temp->permission == 3)
		printf("File Permission : Read & Write\n");

	printf("------------------------------------------------------\n\n");

	return 0;
}

//to dispaly stastical info of file using file name
int stat_file(char *name)
{
	PINODE temp = head;
	int i = 0;

	if(name == NULL)
		 return -1;

	while(temp!= NULL)
	{
		if(strcmp(name,temp->FileName) == 0)
			break;

		temp = temp->next;
	}

	if(temp == NULL) 
		return -2;

	printf("\n---------Statistical Information about file----------\n");
	printf("File name : %s\n",temp->FileName);
	printf("Inode Number %d\n",temp->InodeNumber);
	printf("File size : %d\n",temp->FileSize);
	printf("Actual File size : %d\n",temp->FileActualSize);
	printf("Link count : %d\n",temp->LinkCount);
	printf("Reference count : %d\n",temp->ReferenceCount);

	if(temp->permission == 1)
		printf("File Permission : Read only\n");
	else if(temp->permission == 2)
		printf("File Permission : Write\n");
	else if(temp->permission == 3)
		printf("File Permission : Read & Write\n");
	printf("------------------------------------------------------\n\n");

	return 0;
}

//truncating file or cleaning data from file
int truncate_File(char *name)
{
	int fd = GetFDFromName(name);
	if(fd == -1)
		return -1;

	memset(UFDTArr[fd].ptrfiletable->ptrinode->Buffer,0,1024);
	UFDTArr[fd].ptrfiletable->readoffset = 0;
	UFDTArr[fd].ptrfiletable->writeoffset = 0;
	UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize = 0;
}

int main()
{
	char *ptr = NULL;
	int ret = 0, fd = 0, count = 0;
	char command[4][80], str[80], arr[1024];

	InitialiseSuperBlock();
	CreateDILB();

	while(1)
	{
		fflush(stdin);
		strcpy(str,"");

		printf("\nVirtual File System : > ");

		fgets(str,80,stdin);    //similar as scanf("%[^'\n']s",str);

		//spliting input command into seprate chunk of sub arrays
		count = sscanf(str,"%s %s %s %s",command[0],command[1],command[2],command[3]);

		if(count == 1)
		{
			if(strcmp(command[0],"ls") == 0)
			{
				ls_file();
			}
			else if(strcmp(command[0],"closeall") == 0)
			{
				CloseAllFile();
				printf("All files closed successfully\n");
				continue;
			}
			else if(strcmp(command[0],"clear") == 0)
			{
				system("cls");
				continue;
			}
			else if(strcmp(command[0],"help") == 0)
			{
				DisplayHelp();
				continue;
			}
			else if(strcmp(command[0],"exit") == 0)	
			{
				printf("Terminating the Marvellous Virtual File System\n");
				break;
			}
			else
			{
				printf("\nERROR : Command not found !!!\n");
				continue;
			}
		}
		else if(count == 2)
		{
			if(strcmp(command[0],"stat") == 0)
			{
				ret = stat_file(command[1]);
				if(ret == -1)
					printf("ERROR : Incorrect parameters\n");	
				if(ret == -2)
					printf("ERROR : There is no such file\n");
					continue;
			}
			else if(strcmp(command[0],"fstat") == 0)
			{
				ret = fstat_file(atoi(command[1]));
					if(ret == -1)
						printf("ERROR : Incorrect parameters\n");
					if(ret == -2)
						printf("ERROR : There is no such file\n");
					continue;
			}
			else if(strcmp(command[0],"close") == 0)
			{
				ret = CloseFileByName(command[1]);
				if(ret == -1)
					printf("ERROR : There is no such file\n");
					continue;
			}
			else if(strcmp(command[0],"rm") == 0)
			{
				ret = rm_File(command[1]);
				if(ret == -1)
					printf("ERROR : There is no such file\n");
				continue;
			}
			else if(strcmp(command[0],"man") == 0)
			{
				man(command[1]);
			}
			else if(strcmp(command[0],"write") == 0)
			{
				fd = GetFDFromName(command[1]);
				if(fd == -1)
				{
					printf("Error : Incorrect parameter\n");
					continue;
				}
				printf("Enter the data : \n");
				scanf("%[^\n]",arr);		
				
				ret = strlen(arr);
				if(ret == 0)
				{
					printf("Error : Incorrect parameter\n");
					continue;
				}

				ret = WriteFile(fd,arr,ret);
				if(ret == -1)
					printf("ERROR : Permission denied\n");
				if(ret == -2)
					printf("ERROR : There is no sufficient memory to write\n");
				if(ret == -3)
					printf("ERROR : It is not regular file\n");
		    }
		    else if(strcmp(command[0],"truncate") == 0)
		   {
			ret = truncate_File(command[1]);

			if(ret == -1)
				printf("Error : Incorrect parameter\n");
		  }
		  else
		  {
				printf("\nERROR : Command not found !!!\n");
				continue;
		  }
	  }
	  else if(count == 3)
	  {
		if(strcmp(command[0],"create") == 0)
		{
			ret = CreateFile(command[1],atoi(command[2]));
			
			if(ret >= 0)
				printf("File is successfully created with file descriptor : %d\n",ret);
			if(ret == -1)
				printf("ERROR : Incorrect parameters\n");
			if(ret == -2)
				printf("ERROR : There is no inodes\n");
			if(ret == -3)
				printf("ERROR : File already exists\n");
			if(ret == -4)
				printf("ERROR : Memory allocation failure\n");
			continue;
		}
		else if(strcmp(command[0],"open") == 0)
		{
			ret = OpenFile(command[1],atoi(command[2]));

			if(ret >= 0)
				printf("File is successfully opened with file descriptor : %d\n",ret);
			if(ret == -1)
				printf("ERROR : Incorrect parameters\n");
			if(ret == -2)
				printf("ERROR : File not present\n");
			if(ret == -3)
				printf("ERROR : Permission denied\n");
			continue;
		}
		else if(strcmp(command[0],"read") == 0)
		{
			fd = GetFDFromName(command[1]);
		
			if(fd == -1)
			{
				printf("Error : Incorrect parameter\n");
				continue;
			}
			ptr = (char *)malloc(sizeof(atoi(command[2]))+1);
			if(ptr == NULL)
			{
				printf("Error : Memory allocation failure\n");
				continue;
			}
			ret = ReadFile(fd,ptr,atoi(command[2]));
			if(ret == -1)
				printf("ERROR : File not existing\n");
			if(ret == -2)
				printf("ERROR : Permission denied\n");
			if(ret == -3)
				printf("ERROR : Reached at end of file\n");
			if(ret == -4)
				printf("ERROR : It is not regular file\n");
			if(ret == 0)
				printf("ERROR : File empty\n");
			if(ret > 0)
			{
				write(2,ptr,ret);
			}
			continue;
			}
			else
			{	
				printf("\nERROR : Command not found !!!\n");
				continue;
			}

		}
		else if(count == 4)
		{
			if(strcmp(command[0],"lseek") == 0)
			{
				fd = GetFDFromName(command[1]);
				if(fd == -1)
				{
					printf("Error : Incorrect parameter\n");
					continue;
				}
				ret = LseekFile(fd,atoi(command[2]),atoi(command[3]));
				if(ret == -1)
				{
					printf("ERROR : Unable to perform lseek\n");
				}

			}
			else
			{
					printf("\nERROR : Command not found !!!\n");
					continue;
			}

		}
		else
		{
			printf("\nERROR : Command not found !!!\n");
			continue;
		}
	}

	return 0;
}
