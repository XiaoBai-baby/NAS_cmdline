#include "FileUtils.h"

/*
* Chinese:
	该文件比较特殊, 为了能够在 Windows 客户端上正常使用, 只能支持 ANSI 或 GBK2312编码;

* English:
	This file is relatively special, in order to be able to normal use on the Windows client, can only support ANSI or GBK2312 encoding;
*/

#ifdef _WIN32
// 检查文件的属性
int FileUtils::CheckMode(const string& dir, FileEntry& entry, DWORD FileAttributes)
{
	// FILE_ATTRIBUTE_DIRECTORY 标识目录的句柄
	if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY)			// 按位与 操作
		entry.isDirectory = true;

	string name = entry.fileName;

	for (int i = 6; i > 0; i -= 2)
	{
		int exist = _access_s((dir + name).c_str(), i);
		if (!exist)
		{
			entry.fileMode = i + 1;
			break;
		}
	}

	return 0;
}

long long FileUtils::List_size(const string& dir)
{
	/* 把/ 换成\\ , 末尾加\\  */
	string d = dir;
	for (int i = 0; i < d.length(); i++)
		if (d[i] == '/') d[i] = '\\';
	if (d[d.length() - 1] != '\\')
		d.append("\\");


	// printf("dir: %s \n", d.c_str());
	
	char filter[256] = { 0 };
	sprintf_s(filter, "%s*.*", d.c_str());
	
	// 系统文件信息;
	WIN32_FIND_DATAA info;
	HANDLE hFind = FindFirstFileA((LPCSTR)filter, &info);
	if (hFind == INVALID_HANDLE_VALUE)
		printf("FileUtils::List2 FindFirstFileA/W function failed! Error: %d \n", GetLastError());
	
	// 文件大小
	long long fileSize = 0;

	while (hFind != INVALID_HANDLE_VALUE)
	{
		// 存放文件信息的结构体;
		FileEntry entry;
		entry.fileName = info.cFileName;
		// printf("%s \n", (char*)info2.cFileName);

		if (entry.fileName != "." && entry.fileName != "..")			// 忽略2个特殊目录
		{
			// FILE_ATTRIBUTE_DIRECTORY 标识目录的句柄
			if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)			// 按位与 操作
			{
				fileSize += List_size(d + entry.fileName + "\\");
			}
			else
			{
				entry.fileSize = info.nFileSizeHigh;										// 高阶文件大小
				entry.fileSize = (entry.fileSize << 32) + info.nFileSizeLow;				// 低阶文件大小

				fileSize += entry.fileSize;
			}
		}

		// 查找下一个文件
		if (!FindNextFileA(hFind, &info)) break;
	}

	if (hFind != 0)
		FindClose(hFind);			// 关闭文件句柄

	return fileSize;
}

FileEntryList FileUtils::List(const string& dir, bool msg_list2, bool notDir)
{
	/* 把/ 换成\\ , 末尾加\\  */
	string d = dir;
	for (int i = 0; i < d.length(); i++)
		if (d[i] == '/') d[i] = '\\';
	if (d[d.length() - 1] != '\\' && !notDir)
		d.append("\\");

	// printf("dir: %s \n", d.c_str());

	// 查找文件的链表
	FileEntryList result;

	char filter[256] = { 0 };
	if(!notDir)
		sprintf_s(filter, "%s*.*", d.c_str());
	else
		sprintf_s(filter, "%s", d.c_str());

	// 使用宽字节存放文件名的结构体, FindFirstFile()将返回 INVALID_HANDLE_VALUE
	// WIN32_FIND_DATA info;											// VC去掉UNICODE选项才可用
	// HANDLE hFind = FindFirstFile((LPCWSTR)&filter, &info);			// 宽字节查找文件名
	WIN32_FIND_DATAA info;
	HANDLE hFind = FindFirstFileA((LPCSTR)filter, &info);
	if (hFind == INVALID_HANDLE_VALUE) 
		printf("FileUtils::List FindFirstFileA/W function failed! Error: %d \n", GetLastError());

	while (hFind != INVALID_HANDLE_VALUE)
	{
		// 存放文件信息的结构体;
		FileEntry entry;
		if(!notDir)
			entry.fileName = info.cFileName;
		// printf("%s \n", (char*)info2.cFileName);
		
		if (entry.fileName != "." && entry.fileName != "..")			// 忽略2个特殊目录
		{
			CheckMode(d, entry, info.dwFileAttributes);									// 检查文件属性
			entry.filetime = info.ftLastWriteTime.dwHighDateTime;						// 获取文件时间
			entry.filetime = (entry.filetime << 32) + info.ftLastWriteTime.dwLowDateTime;

			if (entry.isDirectory && msg_list2)
			{
				entry.fileSize = List_size(d + entry.fileName + "\\");
			}
			else
			{
				entry.fileSize = info.nFileSizeHigh;										// 高阶文件大小
				entry.fileSize = (entry.fileSize << 32) + info.nFileSizeLow;				// 低阶文件大小
			}
			
			result.push_back(entry);
		}

		// 查找下一个文件
		if (!FindNextFileA(hFind, &info)) break;
	}

	if(hFind != 0)
		FindClose(hFind);			// 关闭文件句柄

	return result;
}

#endif

// 字符串的分割
int FileUtils::Split(char text[], char* parts[])
{
	int count = 0;			// 分段的个数
	int start = 0;			// 每一分段的首地址
	int flag = 0;			// 遍历总段数，标识当前是否处于有效字符

	int stop = 0;			// 判断循环是否结束
	for (int i = 0; !stop; i++)
	{
		char ch = text[i];
		if (ch == 0)
			stop = 1;			// 执行完下面语句后便结束循环

		if (ch == ',' || ch == '\0' || ch == ' ' || ch == '\t')
		{
			if (flag)				// 遇到分隔符
			{
				flag = 0;			// 将flag 设为0, 此语句将不在执行, 直到下次遇到有效字符

				text[i] = 0;		// 修改为结束符, 完成分段
				parts[count] = text + start;			// 写入首地址
				count++;
			}
		}
		else
		{
			if (!flag)				// 遇到有效字符
			{
				flag = 1;			// 将flag 设为1, 此语句将不在执行, 直到下次遇到分隔符
				start = i;			// 记录首地址
			}
		}
	}

	return count;			// 返回分段个数
}

int FileUtils::MaxFileUnit(Json::Value list)
{
	int count = 0;
	for (int i = 0; i < list.size(); i++)
	{
		long long fileSize = list[i]["fileSize"].asInt64();

		int n = log10(abs(fileSize)) + 1;		// 求整数的位数
		if (count < n)
			count = n;
	}

	return count;
}


string FileUtils::Login_Result(Json::Value list)
{
	string result;
	result.append("* * * * * * * * * * * * * * * * * * * * * * * * \n");
	result.append("*                                             * \n");
	result.append("*          Welcome To The Ftp Server .        * \n");
	result.append("*                                             * \n");
	result.append("*  You can use 'help' to consult commands .   * \n");
	result.append("*                                             * \n");
	result.append("* * * * * * * * * * * * * * * * * * * * * * * * \n");
	result.append("--------------------------OK------------------------ \n");
	
#ifdef _WIN32
	List_Result(list, result);
#endif

	return result;
}

string FileUtils::List_Result(Json::Value list, string& result)
{
	for (int i = 0; i < list.size(); i++)
	{
		if (i % 4 == 0 && i != 0) result.append("\n");

		bool isDir = list[i]["isDir"].asBool();

		result.append(list[i]["fileName"].asString());

		if (isDir == true)
			result.append("[+]         ");
		else
			result.append("      ");
	}

	return result;
}

#ifdef _WIN32
string FileUtils::List2_Result(Json::Value list, string& result)
{
	result.append("总用量: \n");
	int fileUnit = MaxFileUnit(list);			// 获取最大值的位数;

	for (int i = 0; i < list.size(); i++)
	{
		// 打印目录的权限;
		bool isDir = list[i]["isDir"].asBool();
		if (isDir)
			result.append("d");
		else
			result.append("-");

		// 打印文件的权限;
		int fileMode = list[i]["fileMode"].asInt();
		if (fileMode >= 6)
			result.append("rwx-.    ");
		else if(fileMode >= 4)
			result.append("r-x-.    ");
		else if (fileMode >= 2)
			result.append("-wx-.    ");
		else
			result.append("--x-.    ");

		// 获取文件的大小;
		long long fileSize = list[i]["fileSize"].asInt64();
		
		// int count = MaxFileUnit(list);		// 获取最大值的位数;

		int n = log10(abs(fileSize)) + 1;		// 求整数的位数;
		if (n < 0)	n = 1;
		int balance = fileUnit - n;				// 与最大值之间的位数差;

		// 右对齐文件大小;
		for (int i = 0; i < balance; i++)
		{
			result.append(" ");
		}
		
		// 将文件大小 转换成 字符串;
		string string = to_string(fileSize);
		result.append(string);

		// 获取文件的时间;
		long long fileTime = list[i]["fileTime"].asInt64();

		// 将文件时间 转换成 Windows文件格式;
		FILETIME filetime;
		filetime.dwLowDateTime = fileTime;
		filetime.dwHighDateTime = fileTime >> 32;

		// 将 Windows文件时间 装换成 系统可显示时间;
		SYSTEMTIME systime;
		FileTimeToSystemTime(&filetime, &systime);
		
		int year = systime.wYear;
		int month = systime.wMonth;
		int day = systime.wDay;
		
		int hour = systime.wHour;
		int minute = systime.wMinute;

		// 打印日期;
		if (month >= 10)
			result.append("\t " + to_string(month) + "月");
		else
			result.append("\t  " + to_string(month) + "月");

		if(day >= 10)
			result.append("\t " + to_string(day));
		else
			result.append("\t  " + to_string(day));


		// 获取当前计算机的时间;
		time_t computer_time = NULL;
		time(&computer_time);
		tm tp;
		localtime_s(&tp, &computer_time);

		// 将计算机时间 转化成 可显示时间;
		int computer_year = 1900 + tp.tm_year;
		if (year < computer_year)					// 打印文件的年份
		{
			result.append("    " + to_string(year));
		}
		else										// 精确到文件的时分秒
		{
			if (hour >= 10)
				result.append("   " + to_string(hour) + ":");
			else
				result.append("   0" + to_string(hour) + ":");

			if (minute >= 10)
				result.append(to_string(minute));
			else
				result.append("0" + to_string(minute));
		}


		// 打印文件名;
		const char* fileName = list[i]["fileName"].asCString();
		result.append("   ");
		result.append(fileName);

		result.append("\n");
	}

	return result;
}

#endif

string FileUtils::Help_Result()
{
	string result;

	result.append("HELP: \n\n");
	result.append("\tls [FILE] - list directory contents . \n");
	result.append("\tll [FILE] - list detailed directory contents . \n");
	result.append("\tcd [FILE] - Change  the  current  directory . \n");
	result.append("\tget <FILE> - Requests the resource identified by ftp server . \n");
	result.append("\tput <FILE> - Requests the server to store or modify a resource . \n");
	result.append("\tdelete <FILE> - Requests the server to delete the identified resource . \n");
	result.append("\texit - Exit ftp server . \n");

	return result;
}