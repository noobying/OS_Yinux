#pragma once
#include "obj.h"
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <Windows.h>
#include <math.h>

using namespace std;

string motto[10] = {
	"Life was like a box of chocolates, you never know what you're gonna get.",
	"Stupid is as stupid does.",
	"You know some birds are not meant to be caged, their feathers are just too bright.",
	"Everything you see exists together in a delicate balance.",
	"All life is a game of luck.　",
	"Work hard! Work will save you.Work is the only thing that will see you through this.",
	"Don't cheat me like cheating a child.Don't love me like loving a friend.",
	"I wish I was a kid again，because skinned knees are easier to fix than broken hearts",
	"Why so serious?",
	"Get busy living or get busy dieing." };

yinux::yinux()
{
	if ((file = fopen("DISK", "rb+")) == NULL)
	{
		cout << "Can't find file system，reinstall the file system now!" << endl;
		format();
	}
	/*user *uu = new user();
	fseek(file, DATA_START + BLOCK_SIZE, 0);
	fread(uu, sizeof(user), 1, file);
	cout << uu->userName;*/
	readDisk();
	login();  //after login we get the user information
	kernal(); //the most important part of yinux
}

bool yinux::format()
{
	if ((file = fopen("DISK", "wb+")) == NULL)
	{
		cout << "Sorry,format error!" << endl;
		return false;
	}
	else
	{
		//step1       write (and read) superblock
		fseek(file, BLOCK_SIZE, 0); //the first block is empty
		fwrite(&Superblock, sizeof(superblock), 1, file);
		fseek(file, BLOCK_SIZE, 0);
		fread(&Superblock, sizeof(superblock), 1, file);
		//step end

		//step2       initial the block stack

		int stack[51];
		for (int i = 0; i < BLOCK_NUM / 50; i++)
		{
			memset(stack, 0, sizeof(stack));
			for (int j = 1; j < 50; j++)
			{
				stack[50 - j] = 50 * i + j;
			}
			stack[50] = 49; //first is used to store stack in every 50 groups
			fseek(file, DATA_START + 50 * i * BLOCK_SIZE, 0);
			fwrite(stack, sizeof(stack), 1, file);
		}

		//first now to use first 50 group
		for (int i = 1; i <= 49; i++)
			Superblock.currentStack[i] = 50 - i;
		Superblock.currentStack[50] = 49;
		stackPtr = 0;

		/*fseek(file, DATA_START + 50 * 10*BLOCK_SIZE, 0);
		fwrite(stack, sizeof(stack), 1, file);
		for (int i = 0; i < 51; i++)
			cout << stack[i] << " ";
		cout << endl;*/

		//reamin 12 blocks
		for (int i = 0; i < 12; i++)
		{
			stack[49 - i] = 511 - i;
		}
		stack[50] = 12;
		fseek(file, DATA_START + 500 * BLOCK_SIZE, 0);
		fwrite(stack, sizeof(stack), 1, file);
		//step2 end

		//step3 create root directory
		inode inode_temp;
		inode_temp.inodeNum = 0;
		inode_temp.dirMode = 0; //o represent directory
		inode_temp.dirSize = 0;
		inode_temp.dirPermission = MAX_OWNER_PERMISSION;
		strcpy(inode_temp.dirGroup, "group");
		inode_temp.dirOwner = 0; //root user id = 0
		inode_temp.dirConnect = 0;
		time_t t = time(0);
		strftime(inode_temp.dirTime, sizeof(inode_temp.dirTime), "%Y/%m/%d %X %A %jday %z", localtime(&t));
		fseek(file, INODE_START, 0);
		fwrite(&inode_temp, sizeof(inode), 1, file);
		Superblock.inodeBit[inode_temp.inodeNum] = 1;
		Superblock.inodeFree--;
		fseek(file, BLOCK_SIZE, 0);
		fwrite(&Superblock, sizeof(superblock), 1, file);

		//step3 end

		//step4 create root directory

		directory droot_tmp;
		memset(droot_tmp.fname, 0, 10);
		strcpy(droot_tmp.fname, "/");
		//droot_tmp.fname[0] ='/';
		droot_tmp.index = 0;
		droot_tmp.isdir = 1;
		droot_tmp.content = "";
		droot_tmp.parent = -1;     //-1 represent null
		droot_tmp.firstChild = -1; //-1 represent null
		droot_tmp.nextSibling = -1;
		allocateSpace(0, &droot_tmp);
		writeDirectory(0, &droot_tmp);
		//cout << Superblock.currentStack[50] << endl;

		/*directory* a = readDirectory(0);
		cout << a->index << endl;*/

		//step4 end

		//step5 create and store usr file

		user *USER = new user[10];
		for (int i = 0; i < 10; i++)
		{
			memset(USER[i].userName, 0, 6);
		}
		USER[0].userId = 0;
		strcpy(USER[0].group, "admin");
		strcpy(USER[0].userName, "admin");
		strcpy(USER[0].password, "admin");

		USER[1].userId = 1;
		strcpy(USER[1].group, "guest");
		strcpy(USER[1].userName, "guest");
		strcpy(USER[1].password, "guest");

		fseek(file, DATA_START + 2 * BLOCK_SIZE, 0);
		fwrite(USER, sizeof(user), 10, file);

		Superblock.userNum = 2; //a admin and a guest
		Superblock.blockFree--;
		Superblock.currentStack[50]--;
		fseek(file, BLOCK_SIZE, 0);
		fwrite(&Superblock, sizeof(superblock), 1, file);

		fseek(file, BLOCK_SIZE, 0);
		superblock *a = new superblock();
		fread(a, sizeof(superblock), 1, file);
		cout << 1;

		//step5 end
		return true;
		//format is done!!
		//Failure is the mother of success. - Thomas Paine

		/*fseek(file, DATA_START, 0);
		fwrite(&droot_tmp, sizeof(directory), 1, file);*/
	}
}

bool yinux::readDisk()
{
	if ((file = fopen("DISK", "rb+")) == NULL)
	{
		cout << "Read the DISK error!" << endl;
		return false;
	}
	else
	{
		/*directory* a = readDirectory(0);
		cout << a->index << endl;*/
		//step1 read superblockf
		fseek(file, BLOCK_SIZE, 0);
		fread(&Superblock, sizeof(superblock), 1, file);
		//cout << Superblock.userNum << endl;
		//cout << "meme" << endl;
		//step1 end

		//step2 read root
		inode *inode_root = readInode(0);
		currDirectory = readDirectory(0);
		//step2 end
		;

		/*	a = readDirectory(0);
		cout << a->index << endl;*/

		//Nothing seek, nothing find.
		return true;
	}
}

bool yinux::login()
{
	int m = 1;
	cout << "Try not to become a man of success but rather try to become a man of value. -- A. Einstein" << endl;
	while (m)
	{
		int flag = 0;
		int t;
		//cout << "Try not to become a man of success but rather try to become a man of value. -- A. Einstein" << endl;
		cout << "Please enter username:";
		char username[6] = "admin", password[6] = "admin";
		cin >> username;
		cout << "Please enter correspond password:";
		cin >> password;

		user *USER = new user[10];
		fseek(file, DATA_START + 2 * BLOCK_SIZE, 0);
		fread(USER, sizeof(user), 10, file);
		//cout << USER[0].userName << endl;
		//cout << "10000hours" << endl;
		for (int i = 0; i < 10; i++)
			if (strcmp(username, USER[i].userName) == 0)
			{
				t = i;
				flag = 1;
				break;
			}
		if (flag)
		{
			if (strcmp(password, USER[t].password) == 0)
			{
				system("cls");
				m = 0; //login successful!
				User = USER[t];
				return true;
			}
			else
			{
				cout << "Account and password do not match!Please try again!" << endl;
			}
		}
		else
		{
			cout << "Account dose not exist！Please try aagin" << endl;
		}
	}
}

void yinux::kernal()
{
	while (1)
	{

		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN);
		printf("%s@yinux-ZJUT201706061420:", User.userName);
		pwd(currDirectory);
		cout << "$ ";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
		/*cout << "僕らの手には何もないけど、" << endl;*/
		string command;
		cin >> command;
		if (command == "help")
		{
			cout << "All things in their being are good for something." << endl;
			cout << "ls         显示文件目录" << endl;
			cout << "chmod		改变文件权限" << endl;
			cout << "chgrp	    改变文件所属组" << endl;
			cout << "pwd		显示当前目录" << endl;
			cout << "mkdir		创建子目录" << endl;
			cout << "rmdir		删除子目录" << endl;
			cout << "umask		文件创建屏蔽码" << endl;
			cout << "mv			改变文件名" << endl;
			cout << "cp			文件拷贝" << endl;
			cout << "rm			文件删除" << endl;
			cout << "ln         建立文件联接" << endl;
			cout << "cat		连接显示文件内容" << endl;
			cout << "passwd		修改用户口令" << endl;
			cout << "time       输出现在的时间" << endl;
			cout << "lines      输出一句经典台词" << endl;
			cout << "clear      清空控制台" << endl;
			cout << "exit       保存并且退出" << endl;
			cout << "cd         切换路径" << endl;
		}
		else if (command == "ln")
		{
			char * a = new char[100];
			char* b = new char[100];
			cin >> a >> b;
			ln(a, b);
		}
		else if (command == "cp")
		{
			char *path = new char[100];
			char *des = new char[100];
			cin >> path;
			cin >> des;
			cp(path, des);
		}
		else if (command == "mv")
		{
			char *path = new char[100];
			char *des = new char[100];
			cin >> path;
			cin >> des;
			mv(path, des);
		}
		else if (command == "chmod")
		{
			int num;
			char *filename = new char[100];
			cin >> num;
			cin >> filename;
			chmod(num, filename);
		}
		else if (command == "chgrp")
		{
			char *groupname = new char[100];
			char *filename = new char[100];
			cin >> groupname;
			cin >> filename;
			chgrp(groupname, filename);
		}
		else if (command == "append")
		{
			string filename;
			//char* appendContent = new char[10000];
			string appendContent;
			cin >> filename;
			getline(cin, appendContent);
			//cin >> appendContent;
			append(filename, appendContent);
		}
		else if (command == "umask")
		{
			for (int i = 0; i < 3; i++)
				cout << User.umask[i];
			cout << endl;
		}
		else if (command == "cat")
		{
			string filename;
			cin >> filename;
			cat(filename);
		}
		else if (command == "pwd")
		{
			pwd(currDirectory);
			cout << endl;
		}
		else if (command == "passwd")
		{
			passwd(&User);
		}
		else if (command == "ls")
		{
			ls(currDirectory);
		}
		else if (command == "mkdir")
		{
			string parm;
			cin >> parm;
			mkdir(parm);
		}
		else if (command == "touch")
		{
			string parm;
			cin >> parm;
			touch(parm);
		}
		else if (command == "rm")
		{
			string parm;
			cin >> parm;
			rm(parm);
		}
		else if (command == "rmdir")
		{
			string parm;
			cin >> parm;
			rmdir(parm);
		}
		else if (command == "lines")
		{
			cout << motto[rand() % 10] << endl;
		}
		else if (command == "clear")
		{
			system("cls");
		}
		else if (command == "exit")
		{
			fseek(file, BLOCK_SIZE, 0);
			fwrite(&Superblock, sizeof(superblock), 1, file); //store the superblock
			fseek(file, DATA_START + 2 * BLOCK_SIZE + sizeof(user) * User.userId, 0);
			fwrite(&User, sizeof(user), 1, file);
			exit(0);
		}
		else if (command == "time")
		{
			time_t t;
			time(&t);
			printf("%s\n", ctime(&t));
		}
		else if (command == "cd")
		{
			string parm;
			cin >> parm;
			cd(parm);
		}
		else
		{
			cout << "Error!" << command << " is not found!\n";
		}
	}
}

void yinux::mv(char *path, char *des)
{
	int inodeNum;
	directory *dir_temp;

	//if (path[0] != '/')//relative path
	//{
	//	dir_temp = currDirectory;
	//	char *PATH = new char[100];
	//	strcpy(PATH, path);
	//	char *result = NULL;
	//	char delims[] = "/";
	//	result = strtok(PATH, delims);
	//	while (result != NULL)
	//	{
	//		int inodeNum = findDirectory(result);
	//		dir_temp = readDirectory(inodeNum);
	//		result = strtok(NULL, delims);
	//	}
	//	/*int inodeNum = findDirectory(path);
	//	dir_temp = readDirectory(inodeNum);*/
	//}
	//else//absolute path  a little difficult    maybe
	//{
	//	if (strcmp("/", path) == 0)
	//	{
	//		cout << "Root direcotry is not valid!" << endl;
	//		return;
	//	}
	//	else
	//	{
	//		dir_temp = readDirectory(0);
	//		char *PATH = new char[100];
	//		strcpy(PATH, path);
	//		char *result = NULL;
	//		char delims[] = "/";
	//		result = strtok(PATH, delims);

	//		/*currDirectory = readDirectory(0);
	//		char *PATH = new char[25];
	//		strcpy(PATH, path);
	//		char *result = NULL;
	//		char delims[] = "/";
	//		result = strtok(PATH, delims);*/

	//		while (result != NULL)
	//		{
	//			int inodeNum = findDirectory(result);
	//			/*if (readDirectory(inodeNum)->isdir != 1)
	//			{
	//				printf("%s is not valid!\n", PATH);
	//				return;
	//			}*/
	//			dir_temp = readDirectory(inodeNum);
	//			result = strtok(NULL, delims);
	//		}
	//	}
	//}
	//have found the dirctory ,now prepare to Do related aftermath operations,
	//Mainly modify related brother and father nodes
	//step1   modify the correspond brother or parent
	dir_temp = readEnoughDirectory(findBestDirectory(path));

	if (readDirectory(dir_temp->parent)->firstChild == dir_temp->index) //case 1
	{
		directory *dir1 = readDirectory(dir_temp->parent);
		dir1->firstChild = dir_temp->nextSibling;
		writeEnoughDirectory(dir1);
		delete dir1;
	}
	else //case 2
	{
		int t = findPreviousDirectory(dir_temp->index);
		directory *dirdir = readEnoughDirectory(t);
		dirdir->nextSibling = dir_temp->nextSibling;
		writeEnoughDirectory(dirdir);
	}
	//done

	//now search for the new parent dir
	//then modify the parent and brother node respectly
	inodeNum = findBestDirectory(des);
	directory *dir_temp2 = readEnoughDirectory(inodeNum);
	dir_temp->parent = dir_temp2->index;
	dir_temp->nextSibling = -1;
	if (dir_temp2->firstChild == -1)
	{
		dir_temp2->firstChild == dir_temp->index;
		writeEnoughDirectory(dir_temp2);
	}
	else
	{
		directory *dirdirdir = readEnoughDirectory(dir_temp2->firstChild);
		while (dirdirdir->nextSibling != -1)
		{
			dirdirdir = readEnoughDirectory(dirdirdir->nextSibling);
		}
		dirdirdir->nextSibling = dir_temp->index;
		writeEnoughDirectory(dirdirdir);
	}
	writeEnoughDirectory(dir_temp);
}

void yinux::cp(char *path, char *des)
{
	int inodeNum;
	directory *dir_temp;

	dir_temp = readEnoughDirectory(findBestDirectory(path));
	inode *inode_temp = readInode(dir_temp->index);
	string a = "";
	directory *cur = currDirectory;
	a.append(des);
	cd(a);
	touch(dir_temp->fname);
	append(dir_temp->fname, dir_temp->content);
	currDirectory = cur;

	////done

	////now search for the new parent dir
	////then modify the parent and brother node respectly
	//inodeNum = findBestDirectory(des);
	//directory* dir_temp2 = readEnoughDirectory(inodeNum);
	//dir_temp->parent = dir_temp2->index;
	//dir_temp->nextSibling = -1;
	//if (dir_temp2->firstChild == -1)
	//{
	//	dir_temp2->firstChild == dir_temp->index;
	//	writeEnoughDirectory(dir_temp2);
	//}
	//else
	//{
	//	directory* dirdirdir = readEnoughDirectory(dir_temp2->firstChild);
	//	while (dirdirdir->nextSibling != -1)
	//	{
	//		dirdirdir = readEnoughDirectory(dirdirdir->nextSibling);
	//	}
	//	dirdirdir->nextSibling = dir_temp->index;
	//	writeEnoughDirectory(dirdirdir);
	//}
	//writeEnoughDirectory(dir_temp);
}

void yinux::ln(char *path, char *name)
{
	int inodeNum = findBestDirectory(path);
	int t = allocateInode();
	inode *temp1 = readInode(inodeNum);
	inode *temp2 = readInode(t);
	for (int i = 0; i < 4; i++)
	{
		;
	};
}

void yinux::chmod(int num, char *filename)
{
	int inodeNum = findDirectory(filename);
	if (inodeNum == -1)
	{
		printf("No file named %s\n", filename);
		return;
	}
	inode *inode_temp = readInode(inodeNum);
	inode_temp->dirMode = num;
	writeInode(inodeNum, inode_temp);
}

void yinux::chgrp(char *groupname, char *filename)
{
	int inodeNum = findDirectory(filename);
	if (inodeNum == -1)
	{
		printf("No file named %s\n", filename);
		return;
	}
	inode *inode_temp = readInode(inodeNum);
	strcpy(inode_temp->dirGroup, groupname);
	writeInode(inodeNum, inode_temp);
}

void yinux::pwd(directory *dir)
{
	if (dir->parent == -1)
	{
		cout << "/"; //这说明是到了根目录
		return;
	}
	else
	{
		//到父母节点在进行遍历
		pwd(readDirectory(dir->parent));
		if (dir->parent != 0)
			cout << '/';
		cout << dir->fname;
	}
}

void yinux::passwd(user *User)
{
	cout << "Enter current password:";
	char password[6];
	cin >> password;
	if (strcmp(password, User->password) == 0)
	{
		cout << "Enter new password:";
		cin >> password;
		cout << "Retype new password:";
		char newpassword[6];
		cin >> newpassword;
		if (strcmp(password, newpassword) != 0)
		{
			cout << "The passwords entered twice are inconsistent!" << endl;
			return;
		}
		else
		{
			strcpy(User->password, password);
			cout << "Password updated successfully!" << endl;
			fseek(file, DATA_START + BLOCK_SIZE + sizeof(user) * User->userId, 0);
			fwrite(User, sizeof(user), 1, file);
			return;
		}
	}
	else
	{
		cout << "Password and current user do not match!Who are you?" << endl;
		return;
	}
}

void yinux::ls(directory *dir)
{
	if (dir->isdir == -1)
	{
		printf("File %s is not a directory!\n", dir->fname);
		return;
	}
	else
	{
		if (dir->firstChild == -1)
		{
			return;
		}
		dir = readDirectory(currDirectory->firstChild);
		int num = dir->index;
		int count = 0;
		while (num != -1)
		{
			dir = readDirectory(num);
			cout << dir->fname << '\t';
			count++;
			if (count % 4 == 0)
				cout << endl;
			num = readDirectory(num)->nextSibling;
		}
	}
	cout << endl;
}

void yinux::mkdir(string Filename)
{
	char *filename = new char[20];
	strcpy(filename, Filename.c_str());
	int t = allocateInode();
	createDirectory(t, filename, true);
}

void yinux::rmdir(string filename)
{
	const char *Filename = filename.c_str();
	bool flag = false;
	if (currDirectory->firstChild == -1)
	{
		printf("No directory named %s.\n", Filename);
		return;
	}
	directory *dir_temp = readDirectory(currDirectory->firstChild);
	int num = dir_temp->index;
	while (num != -1)
	{
		dir_temp = readDirectory(num);
		if (strcmp(dir_temp->fname, Filename) == 0 && dir_temp->isdir == true)
		{
			flag = true;
			break;
		}
		num = dir_temp->nextSibling;
	}
	if (flag) //Determine if found
	{
		//two case :1、dir_temp 's parent has only one child
		//2 is a list
		// problem is not big
		if (dir_temp->firstChild != -1)
		{
			cout << "Cannot be deleted because the directory has subfiles" << endl;
			return;
		}

		//step1   modify the correspond brother or parent
		if (readDirectory(dir_temp->parent)->firstChild == dir_temp->index) //case 1
		{
			currDirectory->firstChild = dir_temp->nextSibling;
			writeDirectory(0, currDirectory);
		}
		else //case 2
		{
			int t = findPreviousDirectory(dir_temp->index);
			directory *dirdir = readDirectory(t);
			dirdir->nextSibling = dir_temp->nextSibling;
			writeDirectory(t, dirdir);
		}

		//step2 release BLOCK
		releaseSpace(dir_temp->index);
	}
	else
	{
		printf("No directory named %s.\n", Filename);
		return;
	}
}

void yinux::rm(string filename)
{
	const char *Filename = filename.c_str();
	bool flag = false;
	if (currDirectory->firstChild == -1)
	{
		printf("No file named %s.\n", Filename);
		return;
	}
	directory *dir_temp = readDirectory(currDirectory->firstChild);
	int num = dir_temp->index;
	while (num != -1)
	{
		dir_temp = readDirectory(num);
		if (strcmp(dir_temp->fname, Filename) == 0 && dir_temp->isdir != true)
		{
			flag = true;
			break;
		}
		num = dir_temp->nextSibling;
	}
	if (flag) //Determine if found
	{
		//two case :1、dir_temp 's parent has only one child
		//2 is a list
		// problem is not big

		//step1   modify the correspond brother or parent
		if (readDirectory(dir_temp->parent)->firstChild == dir_temp->index) //case 1
		{
			currDirectory->firstChild = dir_temp->nextSibling;
			writeDirectory(0, currDirectory);
		}
		else //case 2
		{
			int t = findPreviousDirectory(dir_temp->index);
			directory *dirdir = readDirectory(t);
			dirdir->nextSibling = dir_temp->nextSibling;
			writeDirectory(t, dirdir);
		}

		//step2 release BLOCK
		releaseEnoughSpace(dir_temp->index);
		Superblock.inodeFree++;
		Superblock.inodeBit[dir_temp->index] = 0;
		//end
		writeSuperblock();
	}
	else
	{
		printf("No file named %s.\n", Filename);
		return;
	}
}

void yinux::touch(string Filename)
{
	char *filename = new char[20];
	strcpy(filename, Filename.c_str());
	int t = allocateInode();
	createDirectory(t, filename, false);
}

void yinux::cd(string filename)
{
	const char *path = filename.c_str();
	if (path[0] != '/') //relative path
	{
		//currDirectory = readDirectory(0);
		char *PATH = new char[25];
		strcpy(PATH, path);
		char *result = NULL;
		char delims[] = "/";
		result = strtok(PATH, delims);
		while (result != NULL)
		{
			int inodeNum = findDirectory(result);
			if (readDirectory(inodeNum)->isdir != 1)
			{
				printf("%s is not valid!\n", PATH);
				return;
			}
			currDirectory = readDirectory(inodeNum);
			result = strtok(NULL, delims);
		}
		/*int inodeNum = findDirectory(path);
		if (readDirectory(inodeNum)->isdir != 1)
		{
			printf("%s is not a directory,is a file!\n", path);
			return;
		}
		currDirectory = readDirectory(inodeNum);*/
	}
	else //absolute path  a little difficult    maybe
	{
		if (strcmp("/", path) == 0)
		{
			currDirectory = readDirectory(0);
		}
		else
		{
			currDirectory = readDirectory(0);
			char *PATH = new char[25];
			strcpy(PATH, path);
			char *result = NULL;
			char delims[] = "/";
			result = strtok(PATH, delims);
			while (result != NULL)
			{
				int inodeNum = findDirectory(result);
				if (readDirectory(inodeNum)->isdir != 1)
				{
					printf("%s is not valid!\n", PATH);
					return;
				}
				currDirectory = readDirectory(inodeNum);
				result = strtok(NULL, delims);
			}
		}
	}
}

void yinux::umask(int number)
{
	//no use
	int u = number / 100;
	int g = number / 10 % 10;
	int o = number % 10;
	if (u > 7 || g > 7 || o > 7)
	{
		cout << "Right is valid!" << endl;
		return;
	}
}

void yinux::cat(string filename)
{
	const char *Filename = filename.c_str();
	bool flag = false;
	if (currDirectory->firstChild == -1)
	{
		printf("No file named %s.\n", Filename);
		return;
	}
	directory *dir_temp = readEnoughDirectory(currDirectory->firstChild);
	int num = dir_temp->index;
	while (num != -1)
	{
		dir_temp = readEnoughDirectory(num);
		if (strcmp(dir_temp->fname, Filename) == 0 && dir_temp->isdir == false)
		{
			flag = true;
			break;
		}
		num = dir_temp->nextSibling;
	}
	if (flag) //Determine if found
	{
		if (dir_temp->isdir != 1)
		{
			cout << dir_temp->content << endl;
			;
		}
		else
		{
			printf("%s is a directory.\n", Filename);
			return;
		}
	}
	else
	{
		printf("No file named %s.\n", Filename);
		return;
	}
}

void yinux::append(string Filename, string appendContent)
{
	char *filename = new char[20];
	strcpy(filename, Filename.c_str());
	int inodeNum = findDirectory(filename);
	if (inodeNum == -1)
	{
		printf("No file named %s\n", filename);
		return;
	}
	inode *inode_temp = readInode(inodeNum);
	inode *testinode;
	directory *dir_temp = readEnoughDirectory(inodeNum);
	int appendSize = 0;
	appendSize = appendContent.length();
	inode_temp->dirSize += appendSize;
	time_t t;
	time(&t);
	strcpy(inode_temp->dirTime, ctime(&t));
	writeInode(inodeNum, inode_temp); //the above is to modify the inode information
	testinode = readInode(inodeNum);

	releaseEnoughSpace(inodeNum);
	testinode = readInode(inodeNum);
	//writeInode(inodeNum, inode_temp);
	allocateEnoughSpace(inodeNum); //allocate the enough space to store the file
	testinode = readInode(inodeNum);
	//writeInode(inodeNum, inode_temp);
	//inode_temp = readInode(inodeNum);
	dir_temp->content.append(appendContent);
	writeEnoughDirectory(dir_temp);
}

void yinux::append(string Filename, char *appendContent)
{
	char *filename = new char[20];
	strcpy(filename, Filename.c_str());
	int inodeNum = findDirectory(filename);
	if (inodeNum == -1)
	{
		printf("No file named %s\n", filename);
		return;
	}
	inode *inode_temp = readInode(inodeNum);
	inode *testinode;
	directory *dir_temp = readEnoughDirectory(inodeNum);
	int appendSize = 0;
	appendSize = strlen(appendContent);
	inode_temp->dirSize += appendSize;
	time_t t;
	time(&t);
	strcpy(inode_temp->dirTime, ctime(&t));
	writeInode(inodeNum, inode_temp); //the above is to modify the inode information
	testinode = readInode(inodeNum);

	releaseEnoughSpace(inodeNum);
	testinode = readInode(inodeNum);
	//writeInode(inodeNum, inode_temp);
	allocateEnoughSpace(inodeNum); //allocate the enough space to store the file
	testinode = readInode(inodeNum);
	//writeInode(inodeNum, inode_temp);
	//inode_temp = readInode(inodeNum);
	dir_temp->content.append(appendContent);
	writeEnoughDirectory(dir_temp);
}

int yinux::findBestDirectory(char *path)
{
	directory *dir_temp;
	directory *currdir = currDirectory;
	if (path[0] != '/') //relative path
	{
		dir_temp = currDirectory;
		char *PATH = new char[100];
		strcpy(PATH, path);
		char *result = NULL;
		char delims[] = "/";
		result = strtok(PATH, delims);
		while (result != NULL)
		{
			int inodeNum = findDirectory2(result, currdir);
			/*if (readDirectory(inodeNum)->isdir != 1)
			{
				printf("%s is not valid!\n", PATH);
				return -1;
			}*/
			dir_temp = readDirectory(inodeNum);
			currdir = dir_temp;
			result = strtok(NULL, delims);
		}
		//int inodeNum = findDirectory(path);
		///*if (readDirectory(inodeNum)->isdir != 1)
		//{
		//	printf("%s is not a directory,is a file!\n", path);
		//	return -1;
		//}*/
		//dir_temp = readDirectory(inodeNum);
	}
	else //absolute path  a little difficult    maybe
	{
		if (strcmp("/", path) == 0)
		{
			return 0;
		}
		else
		{
			dir_temp = readDirectory(0);
			currdir = dir_temp;
			char *PATH = new char[100];
			strcpy(PATH, path);
			char *result = NULL;
			char delims[] = "/";
			result = strtok(PATH, delims);
			while (result != NULL)
			{
				int inodeNum = findDirectory2(result, currdir);
				/*if (readDirectory(inodeNum)->isdir != 1)
				{
					printf("%s is not valid!\n", PATH);
					return -1;
				}*/
				dir_temp = readEnoughDirectory(inodeNum);
				currdir = dir_temp;

				result = strtok(NULL, delims);
			}
		}
	}
	return dir_temp->index;
	return 0;
}

int yinux::findrelativeDirectory(char *path, directory *nowcur)
{
	//directory* dir_temp;
	//if (path[0] != '/')//relative path
	//{
	//	dir_temp = currDirectory;
	//	char *PATH = new char[100];
	//	strcpy(PATH, path);
	//	char *result = NULL;
	//	char delims[] = "/";
	//	result = strtok(PATH, delims);
	//	while (result != NULL)
	//	{
	//		int inodeNum = findDirectory(result);
	//		/*if (readDirectory(inodeNum)->isdir != 1)
	//		{
	//			printf("%s is not valid!\n", PATH);
	//			return -1;
	//		}*/
	//		dir_temp = readDirectory(inodeNum);
	//		result = strtok(NULL, delims);
	//	}
	//	int inodeNum = findDirectory(path);
	//	/*if (readDirectory(inodeNum)->isdir != 1)
	//	{
	//		printf("%s is not a directory,is a file!\n", path);
	//		return -1;
	//	}*/
	//	dir_temp = readDirectory(inodeNum);
	//}
	//else//absolute path  a little difficult    maybe
	//{
	//	if (strcmp("/", path) == 0)
	//	{
	//		return 0;
	//	}
	//	else
	//	{
	//		dir_temp = readDirectory(0);
	//		char *PATH = new char[100];
	//		strcpy(PATH, path);
	//		char *result = NULL;
	//		char delims[] = "/";
	//		result = strtok(PATH, delims);
	//		while (result != NULL)
	//		{
	//			int inodeNum = findDirectory(result);
	//			/*if (readDirectory(inodeNum)->isdir != 1)
	//			{
	//				printf("%s is not valid!\n", PATH);
	//				return -1;
	//			}*/
	//			dir_temp = readDirectory(inodeNum);
	//			result = strtok(NULL, delims);
	//		}
	//	}
	//}
	//return dir_temp->index;
	return 0;
}

int yinux::findPathDirectory(char *path)
{
	directory *dir_temp;
	if (path[0] != '/') //relative path
	{
		dir_temp = currDirectory;
		char *PATH = new char[100];
		strcpy(PATH, path);
		char *result = NULL;
		char delims[] = "/";
		result = strtok(PATH, delims);
		while (result != NULL)
		{
			int inodeNum = findDirectory(result);
			/*if (readDirectory(inodeNum)->isdir != 1)
			{
				printf("%s is not valid!\n", PATH);
				return -1;
			}*/
			dir_temp = readDirectory(inodeNum);
			result = strtok(NULL, delims);
		}
		int inodeNum = findDirectory(path);
		/*if (readDirectory(inodeNum)->isdir != 1)
		{
			printf("%s is not a directory,is a file!\n", path);
			return -1;
		}*/
		dir_temp = readDirectory(inodeNum);
	}
	else //absolute path  a little difficult    maybe
	{
		if (strcmp("/", path) == 0)
		{
			return 0;
		}
		else
		{
			dir_temp = readDirectory(0);
			char *PATH = new char[100];
			strcpy(PATH, path);
			char *result = NULL;
			char delims[] = "/";
			result = strtok(PATH, delims);
			while (result != NULL)
			{
				int inodeNum = findDirectory(result);
				/*if (readDirectory(inodeNum)->isdir != 1)
				{
					printf("%s is not valid!\n", PATH);
					return -1;
				}*/
				dir_temp = readDirectory(inodeNum);
				result = strtok(NULL, delims);
			}
		}
	}
	return dir_temp->index;
}

void yinux::releaseSpace(int inodeNum)
{
	//no consider of size>512
	inode *inode_temp = readInode(inodeNum);
	//release the block
	if (Superblock.currentStack[50] = 49)
	{
		int stack[51];
		//search for enough
		fseek(file, DATA_START + stackPtr * 50 * BLOCK_SIZE, 0);
		fwrite(Superblock.currentStack, sizeof(Superblock.currentStack), 1, file);
		int t = -1;
		for (int i = 0; i <= BLOCK_NUM / 50; i++)
		{
			fseek(file, DATA_START + i * 50 * BLOCK_SIZE, 0);
			fread(stack, sizeof(stack), 1, file);
			if (stack[50] != 49)
			{
				stackPtr = i;
				t = i;
				break;
			}
		}
		for (int i = 0; i < 51; i++)
			Superblock.currentStack[i] = stack[i];
		writeSuperblock();
	}

	Superblock.currentStack[50]++;
	Superblock.blockFree++;
	Superblock.currentStack[Superblock.currentStack[50]] = inode_temp->dirAddr[0];
	//release the inode
	Superblock.inodeFree++;
	Superblock.inodeBit[inodeNum] = 0;
	//end
	writeSuperblock();
}

int yinux::findPreviousDirectory(int inodeNum) //find previous directory , but no consider exist or not
{
	int previousInodeNum = currDirectory->firstChild;
	while (readDirectory(previousInodeNum)->nextSibling != inodeNum)
	{
		previousInodeNum = readDirectory(readDirectory(previousInodeNum)->nextSibling)->index;
	}
	return previousInodeNum;
}

int yinux::findBrotherDirectory(int inodeNum)
{
	directory *dir_temp = readDirectory(inodeNum);
	while (dir_temp->nextSibling != -1)
	{
		dir_temp = readDirectory(dir_temp->nextSibling);
	}
	return dir_temp->index;
}

directory *yinux::createDirectory(int inodeNum, char *filename, bool flag)
{
	directory *dir_temp = new directory();
	strcpy(dir_temp->fname, filename);
	dir_temp->index = inodeNum;
	if (flag) //whether file or directory
	{
		dir_temp->isdir = 1;
		dir_temp->parent = currDirectory->index;
		dir_temp->firstChild = -1;
		dir_temp->nextSibling = -1;
		if (currDirectory->firstChild == -1) //parent has no child
		{
			currDirectory->firstChild = dir_temp->index;
		}
		else
		{
			directory *brotherDirectory = readDirectory(findBrotherDirectory(currDirectory->firstChild));
			brotherDirectory->nextSibling = dir_temp->index;
			writeDirectory(brotherDirectory->index, brotherDirectory);
		}
	}
	else //is a file
	{
		dir_temp->isdir = 0;
		dir_temp->parent = currDirectory->index;
		dir_temp->firstChild = -1;
		dir_temp->nextSibling = -1;
		dir_temp->content = "";

		if (currDirectory->firstChild == -1) //parent has no child
		{
			currDirectory->firstChild = dir_temp->index;
		}
		else
		{
			directory *brotherDirectory = readDirectory(findBrotherDirectory(currDirectory->firstChild));
			brotherDirectory->nextSibling = dir_temp->index;
			writeDirectory(brotherDirectory->index, brotherDirectory);
		}
	}
	allocateSpace(inodeNum, dir_temp);
	//writeDirectory(inodeNum, dir_temp);
	writeDirectory(inodeNum, dir_temp);
	writeDirectory(currDirectory->index, currDirectory);
	return dir_temp;
}

int yinux::allocateInode()
{
	int inodeNum = -1;
	;
	for (int i = 0; i < Superblock.blockNum; i++)
	{
		if (Superblock.inodeBit[i] == 0)
		{
			inodeNum = i;
			break;
		}
	}
	if (inodeNum == -1)
	{
		cout << "File system has no inode to store file information!" << endl;
		return -1;
	}
	else
	{
		inode *inode_temp = new inode();
		inode_temp->inodeNum = inodeNum;
		inode_temp->dirSize = 0;
		inode_temp->dirConnect = 0;
		for (int i = 0; i < 4; i++)
			inode_temp->dirAddr[i] = -1;
		inode_temp->dirAddr1 = -1;
		inode_temp->dirAddr2 = -1;
		inode_temp->dirOwner = User.userId;
		strcpy(inode_temp->dirGroup, User.group);
		time_t t;
		time(&t);
		strcpy(inode_temp->dirTime, ctime(&t));
		//reamin dirMode and dirPermission
		fseek(file, INODE_START + inodeNum * INODE_SIZE, 0);
		fwrite(inode_temp, sizeof(inode), 1, file);
		//fseek(file, INODE_START + inodeNum * INODE_SIZE, 0);
		//fwrite(inode_temp, sizeof(inode), 1, file);
		//cout << inode_temp->inodeNum << endl;
		Superblock.inodeBit[inode_temp->inodeNum] = 1;
		Superblock.inodeFree--;
		fseek(file, BLOCK_SIZE, 0);
		fwrite(&Superblock, sizeof(superblock), 1, file);

		return inodeNum;
	}
}

void yinux::allocateSpace(int inodeNum, directory *dir)
{
	if (Superblock.currentStack[50] == 0)
	{
		//current stack is empty
		int stack[51];
		fseek(file, DATA_START + stackPtr * 50 * BLOCK_SIZE, 0);
		fwrite(Superblock.currentStack, sizeof(Superblock.currentStack), 1, file);

		for (int i = 0; i < BLOCK_NUM / 50 + 1; i++)
		{
			fseek(file, DATA_START + i * 50 * BLOCK_SIZE, 0);
			fread(stack, sizeof(stack), 1, file);
			if (stack[50] != 0)
			{
				stackPtr = i;
				for (int i = 0; i <= 50; i++)
					Superblock.currentStack[i] = stack[i];
				break;
			}
		}
	}
	inode *newInode = readInode(inodeNum);
	newInode->dirAddr[0] = Superblock.currentStack[Superblock.currentStack[50]--];
	writeInode(inodeNum, newInode);
	Superblock.blockFree--;
	fseek(file, BLOCK_SIZE, 0);
	fwrite(&Superblock, sizeof(superblock), 1, file);
}

inode *yinux::readInode(int inodeNum)
{
	inode *newInode = new inode();
	if (fseek(file, INODE_START + inodeNum * INODE_SIZE, 0))
	{
		cout << "File point error!!" << endl;
		exit(1);
	}
	fread(newInode, sizeof(inode), 1, file);
	/*fread(&(newInode->inodeNum), sizeof(int), 1, file);
	fread(&(newInode->dirSize), sizeof(int), 1, file);
	fread(&(newInode->dirConnect), sizeof(int), 1, file);
	fread(newInode->dirAddr, sizeof(int), 4, file);
	fread(&(newInode->dirAddr1), sizeof(int), 1, file);
	fread(&(newInode->dirAddr2), sizeof(int), 1, file);
	fread(&(newInode->dirOwner), sizeof(int), 1, file);
	fread(&(newInode->dirGroup), sizeof(char), 6, file);
	fread(&(newInode->dirMode), sizeof(int), 1, file);
	fread(&(newInode->dirPermission), sizeof(int), 1, file);
	fread(newInode->dirTime, sizeof(char), 64, file);*/
	return newInode;
}

void yinux::writeInode(int inodeNum, inode *newInode)
{
	if (fseek(file, INODE_START + inodeNum * INODE_SIZE, 0))
	{
		cout << "File point error!!" << endl;
		exit(1);
	}
	fwrite(newInode, sizeof(inode), 1, file);
	/*fwrite(&(newInode->inodeNum), sizeof(int), 1, file);
	fwrite(&(newInode->dirSize), sizeof(int), 1, file);
	fwrite(&(newInode->dirConnect), sizeof(int), 1, file);
	fwrite(newInode->dirAddr, sizeof(int), 4, file);
	fwrite(&(newInode->dirAddr1), sizeof(int), 1, file);
	fwrite(&(newInode->dirAddr2), sizeof(int), 1, file);
	fwrite(&(newInode->dirOwner), sizeof(int), 1, file);
	fwrite(&(newInode->dirGroup), sizeof(char), 4, file);
	fwrite(&(newInode->dirMode), sizeof(int), 1, file);
	fwrite(&(newInode->dirPermission), sizeof(int), 1, file);
	fwrite(newInode->dirTime, sizeof(char), 64, file);*/
}

void yinux::writeDirectory(int inodeNum, directory *dir)
{
	inode *inode_temp = readInode(inodeNum);
	//no consider one block cant store
	fseek(file, DATA_START + inode_temp->dirAddr[0] * BLOCK_SIZE, 0); //need attenstion
	fwrite(dir->fname, sizeof(char), 10, file);
	fwrite(&(dir->index), sizeof(int), 1, file);
	fwrite(&(dir->isdir), sizeof(bool), 1, file);
	fwrite(&(dir->parent), sizeof(int), 1, file);
	fwrite(&(dir->firstChild), sizeof(int), 1, file);
	fwrite(&(dir->nextSibling), sizeof(int), 1, file);
	if (!dir->isdir) //dir is not directory ,then store its content
	{
		int size = inode_temp->dirSize - 0; //0 represent dir size without content
		char *a = new char[size];
		for (int i = 0; i < inode_temp->dirSize - 0; i++)
			a[i] = dir->content[i];
		fwrite(a, sizeof(char), inode_temp->dirSize - 0, file);
		delete[] a;
	}
	/*directory* a = readDirectory(0);
	cout << a->index << endl;*/
}

directory *yinux::readDirectory(int inodeNum)
{
	inode *inode_temp = readInode(inodeNum);
	directory *dir = new directory();
	fseek(file, DATA_START + inode_temp->dirAddr[0] * BLOCK_SIZE, 0);
	fread(dir->fname, sizeof(char), 10, file);
	fread(&(dir->index), sizeof(int), 1, file);
	fread(&(dir->isdir), sizeof(bool), 1, file);
	fread(&(dir->parent), sizeof(int), 1, file);
	fread(&(dir->firstChild), sizeof(int), 1, file);
	fread(&(dir->nextSibling), sizeof(int), 1, file);
	if (!dir->isdir) //dir is not directory ,then store its content
	{
		int size = inode_temp->dirSize - 0; //0 represent dir size without content
		char *a = new char[size];
		fread(a, sizeof(char), size, file);
		for (int i = 0; i < inode_temp->dirSize - 0; i++)
			dir->content += a[i];
		delete[] a;
	}
	return dir;
}

void yinux::writeSuperblock()
{
	fseek(file, BLOCK_SIZE, 0);
	fwrite(&Superblock, sizeof(superblock), 1, file);
}

int yinux::findDirectory(const char *filename)
{
	if (currDirectory->firstChild == -1)
	{
		return -1; //no find
	}
	else
	{
		int inum = -1;
		directory *dir_temp = readDirectory(currDirectory->firstChild);
		int num = dir_temp->index;
		while (num != -1)
		{
			dir_temp = readDirectory(num);
			if (strcmp(dir_temp->fname, filename) == 0)
			{
				inum = dir_temp->index;
				break;
			}
			num = dir_temp->nextSibling;
		}
		return inum;
	}
}

int yinux::findDirectory2(const char *filename, directory *currdir)
{
	if (currdir->firstChild == -1)
	{
		return -1; //no find
	}
	else
	{
		int inum = -1;
		directory *dir_temp = readDirectory(currdir->firstChild);
		int num = dir_temp->index;
		while (num != -1)
		{
			dir_temp = readDirectory(num);
			if (strcmp(dir_temp->fname, filename) == 0)
			{
				inum = dir_temp->index;
				break;
			}
			num = dir_temp->nextSibling;
		}
		return inum;
	}
}

void yinux::allocateEnoughSpace(int inodeNum)
{

	//file and directory directly define as 0 if the content is empty

	inode *inode_temp = readInode(inodeNum);
	if (inode_temp->dirSize <= 3 * BLOCK_SIZE) //ONLY UES DIRECT BLOCK
	{
		//1 = direcotry information   2, 3, 4 = content
		int t = ceil(double(inode_temp->dirSize) / BLOCK_SIZE); //ceil Rounded up
		for (int i = 0; i < t + 1; i++)
		{
			inode_temp->dirAddr[i] = findAvailableBlock();
		}
	}
	else if (inode_temp->dirSize <= (16 + 3) * BLOCK_SIZE) //use the Once indirectly
	{
		//first allocate the first 4 block        1=direcotry information   2,3,4=content
		for (int i = 0; i < 4; i++)
		{
			inode_temp->dirAddr[i] = findAvailableBlock();
		}
		int t = ceil(double(inode_temp->dirSize - 3 * BLOCK_SIZE) / BLOCK_SIZE);
		int a[16];
		for (int i = 0; i < 16; i++) //initialization the addr1
		{
			a[i] = -1;
		}
		for (int i = 0; i < t; i++)
		{
			a[i] = findAvailableBlock();
		}
		inode_temp->dirAddr1 = findAvailableBlock();
		fseek(file, DATA_START + inode_temp->dirAddr1 * BLOCK_SIZE, 0);
		fwrite(a, sizeof(a), 1, file);
	}
	else if (inode_temp->dirSize <= (16 + 3 + 16 * 16) * BLOCK_SIZE) //use the Two indirect
	{
		;
	}
	writeInode(inodeNum, inode_temp);
}

void yinux::releaseEnoughSpace(int inodeNum)
{
	inode *inode_temp = readInode(inodeNum);
	for (int i = 0; i < 4; i++)
	{
		if (inode_temp->dirAddr[i] != -1)
		{
			releasecCorrespondingBlock(inode_temp->dirAddr[i]);
			inode_temp->dirAddr[i] = -1;
		}
	}
	if (inode_temp->dirAddr1 != -1)
	{
		int *a = new int[16];
		fseek(file, DATA_START + inode_temp->dirAddr1 * BLOCK_SIZE, 0);
		fread(a, sizeof(int), 16, file);
		for (int i = 0; i < 16; i++)
			if (a[i] != -1)
				releasecCorrespondingBlock(a[i]);
	}
	writeInode(inodeNum, inode_temp);
	//remaining the last part twice direct
}

int yinux::findAvailableBlock()
{
	if (Superblock.blockFree == 0)
	{
		cout << "Not enough storage space" << endl;
		return -1;
	}
	if (Superblock.currentStack[50] != 0)
	{
		Superblock.blockFree--;
		int t = Superblock.currentStack[Superblock.currentStack[50]--];
		writeSuperblock();
		return t;
	}
	else
	{
		int stack[51];
		fseek(file, DATA_START + stackPtr * 50 * BLOCK_SIZE, 0);
		fwrite(Superblock.currentStack, sizeof(Superblock.currentStack), 1, file);

		for (int i = 0; i < BLOCK_NUM / 50 + 1; i++)
		{
			fseek(file, DATA_START + i * 50 * BLOCK_SIZE, 0);
			fread(stack, sizeof(stack), 1, file);
			if (stack[50] != 0)
			{
				stackPtr = i;
				for (int i = 0; i <= 50; i++)
					Superblock.currentStack[i] = stack[i];
				break;
			}
		}
		Superblock.blockFree--;
		int t = Superblock.currentStack[Superblock.currentStack[50]--];
		writeSuperblock();
		return t;
	}
}

void yinux::releasecCorrespondingBlock(int blocknum)
{
	if (Superblock.currentStack[50] == 49)
	{
		findNotFull();
		Superblock.currentStack[++Superblock.currentStack[50]] = blocknum;
		writeSuperblock();
	}
	else
	{
		Superblock.currentStack[++Superblock.currentStack[50]] = blocknum;
		writeSuperblock();
	}
}

void yinux::writeEnoughDirectory(directory *dir)
{
	//good very good
	inode *inode_temp = readInode(dir->index);
	//no consider one block cant store
	fseek(file, DATA_START + inode_temp->dirAddr[0] * BLOCK_SIZE, 0); //need attenstion   store in dirAddr[0]
	fwrite(dir->fname, sizeof(char), 10, file);
	fwrite(&(dir->index), sizeof(int), 1, file);
	fwrite(&(dir->isdir), sizeof(bool), 1, file);
	fwrite(&(dir->parent), sizeof(int), 1, file);
	fwrite(&(dir->firstChild), sizeof(int), 1, file);
	fwrite(&(dir->nextSibling), sizeof(int), 1, file);
	if (!dir->isdir) //dir is not directory ,then store its content
	{
		int size = inode_temp->dirSize; //0 represent dir size without content
		char *a = new char[size];
		memset(a, 0, size);
		for (int i = 0; i < inode_temp->dirSize - 0; i++)
			a[i] = dir->content[i];
		if (inode_temp->dirSize <= 3 * BLOCK_SIZE) //ONLY UES DIRECT BLOCK done
		{
			int t = ceil(double(inode_temp->dirSize) / BLOCK_SIZE); //
			for (int i = 1; i < t + 1; i++)
			{
				fseek(file, DATA_START + BLOCK_SIZE * inode_temp->dirAddr[i], 0);
				fwrite(a + (i - 1) * BLOCK_SIZE, sizeof(char), size - (i - 1) * BLOCK_SIZE <= 512 ? size - (i - 1) * BLOCK_SIZE : 512, file);
				fseek(file, DATA_START + BLOCK_SIZE * inode_temp->dirAddr[i], 0);
				/*char c[5];
				fread(c + (i - 1)*BLOCK_SIZE, sizeof(char), size - (i - 1)*BLOCK_SIZE <= 512 ? size - (i - 1) * BLOCK_SIZE : 512, file);
				cout << c;*/
			}
		}
		else if (inode_temp->dirSize <= (16 + 4) * BLOCK_SIZE) //use the Once indirectly
		{
			for (int i = 1; i < 4; i++) //first sotre the first 3
			{
				fseek(file, DATA_START + BLOCK_SIZE * inode_temp->dirAddr[i], 0);
				fwrite(a + (i - 1) * BLOCK_SIZE, sizeof(char), 512, file);
			}
			int *b = new int[16];
			fseek(file, DATA_START + BLOCK_SIZE * inode_temp->dirAddr1, 0); //read from disk
			fread(b, sizeof(b), 16, file);
			for (int i = 0; i < 16 && b[i] != -1; i++)
			{
				fseek(file, DATA_START + BLOCK_SIZE * b[i], 0);
				fwrite(a + 3 * BLOCK_SIZE + i * BLOCK_SIZE, sizeof(char), (size - (i + 3) * BLOCK_SIZE) <= 512 ? (size - (i + 3) * BLOCK_SIZE) : 512, file);
			}
		}
		else if (inode_temp->dirSize <= (16 + 4 + 16 * 16) * BLOCK_SIZE) //use the Two indirect
		{
			;
		}

		delete[] a;
	}
}

int yinux::allocateInodeAndEmptyDirecotry(bool flag, string Filename)
{
	int inodeNum = allocateInode();
	char *filename = new char[20];
	strcpy(filename, Filename.c_str());
	createDirectory(inodeNum, filename, flag);
	return inodeNum;
}

directory *yinux::readEnoughDirectory(int num)
{
	inode *inode_temp = readInode(num); //first read the corespond inode
	directory *dir = new directory();
	fseek(file, DATA_START + inode_temp->dirAddr[0] * BLOCK_SIZE, 0);
	fread(dir->fname, sizeof(char), 10, file);
	fread(&(dir->index), sizeof(int), 1, file);
	fread(&(dir->isdir), sizeof(bool), 1, file);
	fread(&(dir->parent), sizeof(int), 1, file);
	fread(&(dir->firstChild), sizeof(int), 1, file);
	fread(&(dir->nextSibling), sizeof(int), 1, file);
	if (!dir->isdir)
	{
		int size = inode_temp->dirSize - 0; //0 represent dir size without content
		char *a = new char[size];

		if (inode_temp->dirSize <= 3 * BLOCK_SIZE) //ONLY UES DIRECT BLOCK done
		{
			int t = ceil(double(inode_temp->dirSize) / BLOCK_SIZE); //
			for (int i = 1; i < t + 1; i++)
			{
				fseek(file, DATA_START + BLOCK_SIZE * inode_temp->dirAddr[i], 0);
				fread(a + (i - 1) * BLOCK_SIZE, sizeof(char), size - (i - 1) * BLOCK_SIZE <= 512 ? size - (i - 1) * BLOCK_SIZE : 512, file);
			}
		}
		else if (inode_temp->dirSize <= (16 + 4) * BLOCK_SIZE) //use the Once indirectly
		{
			for (int i = 1; i < 4; i++) //first sotre the first 3
			{
				fseek(file, DATA_START + BLOCK_SIZE * inode_temp->dirAddr[i], 0);
				fread(a + (i - 1) * BLOCK_SIZE, sizeof(char), 512, file);
			}
			int *b = new int[16];
			fseek(file, DATA_START + BLOCK_SIZE * inode_temp->dirAddr1, 0); //read from disk
			fread(b, sizeof(b), 16, file);
			for (int i = 0; i < 16 && b[i] != -1; i++)
			{
				fseek(file, DATA_START + BLOCK_SIZE * b[i], 0);
				fread(a + 3 * BLOCK_SIZE + i * BLOCK_SIZE, sizeof(char), (size - (i + 3) * BLOCK_SIZE) <= 512 ? (size - (i + 3) * BLOCK_SIZE) : 512, file);
			}
		}
		else if (inode_temp->dirSize <= (16 + 4 + 16 * 16) * BLOCK_SIZE) //use the Two indirect
		{
			;
		}
		for (int i = 0; i < inode_temp->dirSize - 0; i++)
			dir->content += a[i];
		delete[] a;
	}
	return dir;
}

int yinux::findNotFull()
{
	bool flag = false;
	fseek(file, DATA_START + stackPtr * 50 * BLOCK_SIZE, 0);
	fwrite(Superblock.currentStack, sizeof(int), 51, file);
	int stack[51];
	memset(stack, 0, sizeof(stack));
	for (int i = 0; i < 11; i++)
	{
		fseek(file, DATA_START + BLOCK_SIZE * i * 50, 0);
		fread(stack, sizeof(int), 51, file);
		if (stack[50] != 49)
		{
			stackPtr = i;
			for (int t = 0; t < 51; t++)
			{
				flag = true;
				Superblock.currentStack[t] = stack[t];
			}
			break;
		}
	}
	if (!flag)
		cout << "No find not full block stack!" << endl;
	return stackPtr;
}

int yinux::findNotEmpty()
{
	bool flag = false;
	fseek(file, DATA_START + stackPtr * 50 * BLOCK_SIZE, 0);
	fwrite(Superblock.currentStack, sizeof(int), 51, file);
	int stack[51];
	memset(stack, 0, sizeof(stack));
	for (int i = 0; i < 11; i++)
	{
		fseek(file, DATA_START + BLOCK_SIZE * i * 50, 0);
		fread(stack, sizeof(int), 51, file);
		if (stack[50] != 0)
		{
			stackPtr = i;
			for (int t = 0; t < 51; t++)
			{
				flag = true;
				Superblock.currentStack[t] = stack[t];
			}
			break;
		}
	}
	if (!flag)
		cout << "No find not full block stack!" << endl;
	return stackPtr;
}

directory *yinux::test1()
{
	int inodeNum = allocateInodeAndEmptyDirecotry(false, "123");
	inode *inode_temp = readInode(inodeNum);
	inode_temp->dirSize = 5;
	directory *dir_temp = readEnoughDirectory(inodeNum);
	dir_temp->content = "Hello";
	writeEnoughDirectory(dir_temp);
	return nullptr;
}

superblock::superblock()
{
	inodeNum = INODE_NUM;
	inodeFree = INODE_NUM;
	inodeSize = INODE_SIZE;
	for (int i = 0; i < INODE_NUM; i++)
		inodeBit[i] = 0;
	blockNum = BLOCK_NUM + INODE_NUM * INODE_SIZE / BLOCK_SIZE + 2; //2 means one empty and one superblock
	blockFree = BLOCK_NUM;
}