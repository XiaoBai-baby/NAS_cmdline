#include "FileUtils.h"

/*
* Chinese:
	���ļ��Ƚ�����, Ϊ���ܹ��� Windows �ͻ���������ʹ��, ֻ��֧�� ANSI �� GBK2312����;
	�ɲο������Ӳ鿴ԭ��: https://chariri.moe/archives/408/windows-cin-read-utf8/

* English:
	This file is relatively special, in order to be able to normal use on the Windows client, can only support ANSI or GBK2312 encoding;
	See this link as reason: https://chariri.moe/archives/408/windows-cin-read-utf8/
*/

#ifdef _WIN32

void FileUtils::CMD_GBK()
{
	//����̨��ʾ�������
   // system("chcp 936");
	SetConsoleOutputCP(936);
	CONSOLE_FONT_INFOEX info = { 0 }; // ��������������֧��������ʾ��
	info.cbSize = sizeof(info);
	info.dwFontSize.Y = 16; // leave X as zero
	info.FontWeight = FW_NORMAL;
	wcscpy_s(info.FaceName, L"Consolas");
	SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), NULL, &info);
}

// ����ļ�������
int FileUtils::CheckMode(const string& dir, FileEntry& entry, DWORD FileAttributes)
{
	// FILE_ATTRIBUTE_DIRECTORY ��ʶĿ¼�ľ��
	if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY)			// ��λ�� ����
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

// ע��, �˺���ֻ��ɾ��Ŀ¼(����Ŀ¼����ļ�), �����ļ����޷�ɾ��;
int FileUtils::rmdir(const string dir)
{
	List_size(dir, true);
	List_size(dir, true, true);

	//ȥ���ļ���Ŀ¼��ϵͳ����������;
	SetFileAttributesA(dir.c_str(), FILE_ATTRIBUTE_NORMAL);
	RemoveDirectoryA(dir.c_str());					// ɾ��Ŀ¼;

	return 0;
}

// msg_remove_directory �� msg_remove_file �ֱ�Ϊ���� MSG_REMOVE���ź�;
// �����ź��޷�ͬʱʹ��, ������ִ��msg_remove_file, ��ִ��msg_remove_file_directory, �ſ���ɾ������Ŀ¼;
long long FileUtils::List_size(const string& dir, bool msg_remove_file, bool msg_remove_directory)
{
	/* ��/ ����\\ , ĩβ��\\  */
	string d = dir;
	for (int i = 0; i < d.length(); i++)
		if (d[i] == '/') d[i] = '\\';
	if (d[d.length() - 1] != '\\')
		d.append("\\");


	// printf("dir: %s \n", d.c_str());
	
	char filter[256] = { 0 };
	sprintf_s(filter, "%s*.*", d.c_str());
	
	// ϵͳ�ļ���Ϣ;
	WIN32_FIND_DATAA info;
	HANDLE hFind = FindFirstFileA((LPCSTR)filter, &info);
	if (hFind == INVALID_HANDLE_VALUE)
		printf("FileUtils::List2 FindFirstFileA/W function failed! Error: %d \n", GetLastError());
	
	// �ļ���С
	long long fileSize = 0;

	while (hFind != INVALID_HANDLE_VALUE)
	{
		// ����ļ���Ϣ�Ľṹ��;
		FileEntry entry;
		entry.fileName = info.cFileName;
		// printf("%s \n", (char*)info2.cFileName);

		if (entry.fileName != "." && entry.fileName != "..")			// ����2������Ŀ¼
		{
			string file = d + entry.fileName;

			// FILE_ATTRIBUTE_DIRECTORY ��ʶĿ¼�ľ��
			if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)			// ��λ�� ����
			{
				fileSize += List_size(d + entry.fileName + "\\", msg_remove_file, msg_remove_directory);
			}

			if ((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && msg_remove_directory)
			{
				//ȥ���ļ���Ŀ¼��ϵͳ����������;
				SetFileAttributesA(file.c_str(), FILE_ATTRIBUTE_NORMAL);
				RemoveDirectoryA(file.c_str());					// ɾ��Ŀ¼;
			}
			else if (msg_remove_file)
			{
				//ȥ���ļ���Ŀ¼��ϵͳ����������;
				SetFileAttributesA(file.c_str(), FILE_ATTRIBUTE_NORMAL);
				DeleteFileA(file.c_str());						// ɾ���ļ�;
			}
			else
			{
				entry.fileSize = info.nFileSizeHigh;										// �߽��ļ���С
				entry.fileSize = (entry.fileSize << 32) + info.nFileSizeLow;				// �ͽ��ļ���С

				fileSize += entry.fileSize;
			}
		}

		// ������һ���ļ�
		if (!FindNextFileA(hFind, &info))
			break;
	}

	if (hFind != 0)
		FindClose(hFind);			// �ر��ļ����

	return fileSize;
}

// notDir Ϊ falseʱ, ��ʾ dir ΪĿ¼;
FileEntryList FileUtils::List(const string& dir, bool msg_list2, bool notDir)
{
	/* ��/ ����\\ , ĩβ��\\  */
	string d = dir;
	for (int i = 0; i < d.length(); i++)
		if (d[i] == '/') d[i] = '\\';
	if (d[d.length() - 1] != '\\' && !notDir)
		d.append("\\");

	// printf("dir: %s \n", d.c_str());

	// �����ļ�������
	FileEntryList result;

	char filter[256] = { 0 };
	if(!notDir)
		sprintf_s(filter, "%s*.*", d.c_str());
	else
		sprintf_s(filter, "%s", d.c_str());

	// ʹ�ÿ��ֽڴ���ļ����Ľṹ��, FindFirstFile()������ INVALID_HANDLE_VALUE
	// WIN32_FIND_DATA info;											// VCȥ��UNICODEѡ��ſ���
	// HANDLE hFind = FindFirstFile((LPCWSTR)&filter, &info);			// ���ֽڲ����ļ���
	WIN32_FIND_DATAA info;
	HANDLE hFind = FindFirstFileA((LPCSTR)filter, &info);
	if (hFind == INVALID_HANDLE_VALUE) 
		printf("FileUtils::List FindFirstFileA/W function failed! Error: %d \n", GetLastError());

	while (hFind != INVALID_HANDLE_VALUE)
	{
		// ����ļ���Ϣ�Ľṹ��;
		FileEntry entry;
		if(!notDir)
			entry.fileName = info.cFileName;
		// printf("%s \n", (char*)info2.cFileName);
		
		if (entry.fileName != "." && entry.fileName != "..")			// ����2������Ŀ¼
		{
			CheckMode(d, entry, info.dwFileAttributes);									// ����ļ�����
			entry.filetime = info.ftLastWriteTime.dwHighDateTime;						// ��ȡ�ļ�ʱ��
			entry.filetime = (entry.filetime << 32) + info.ftLastWriteTime.dwLowDateTime;

			if (entry.isDirectory && msg_list2)
			{
				entry.fileSize = List_size(d + entry.fileName + "\\");
			}
			else
			{
				entry.fileSize = info.nFileSizeHigh;										// �߽��ļ���С
				entry.fileSize = (entry.fileSize << 32) + info.nFileSizeLow;				// �ͽ��ļ���С
			}
			
			result.push_back(entry);
		}

		// ������һ���ļ�
		if (!FindNextFileA(hFind, &info)) break;
	}

	if(hFind != 0)
		FindClose(hFind);			// �ر��ļ����

	return result;
}

#endif

// add_set Ϊ�ֶ���ӷָ���;
int FileUtils::Split_Estimate(const char* add_set, char ch)
{
	int flag = 0;
	if(add_set != 0)					// add_set�����÷ָ���;
	{
		int len = strlen(add_set);
		for(int i = 0; i < len; i++)
		{
			if(ch == '\0' || ch == add_set[i])
				flag = 1;
		}
	}
	else								// ʹ��Ĭ�ϵķָ���;
	{
		if(ch == ',' || ch == '\0' || ch == ' ' || ch == '\t')
			flag = 1;
	}
	
	return flag;
}

// �ַ����ķָ�
// ע��, Split����ʹ��ʱ, ͬ���Ĳ��������Զ�ε��� �� �໥����(�ڴ汨��, �δ���);
int FileUtils::Split(char text[], char* parts[], const char* add_set)
{
	int count = 0;				// �ֶεĸ���
	int start = 0;				// ÿһ�ֶε��׵�ַ
	int flag = 0;				// �����ܶ�������ʶ��ǰ�Ƿ�����Ч�ַ�
	
	int stop = 0;				// �ж�ѭ���Ƿ����
	for (int i = 0; !stop; i++)
	{
		char ch = text[i];
		if (ch == 0)
			stop = 1;			// ִ����������������ѭ��

		// �ָ�ʱ, �ų��� ""���ַ���;
		if (ch == '\"')
		{
			if (flag)
				ch = '\0';
			else
				continue;
		}
		
		if (Split_Estimate(add_set, ch))
		{
			if (flag)				// �����ָ���
			{
				flag = 0;			// ��flag ��Ϊ0, ����佫����ִ��, ֱ���´�������Ч�ַ�

				text[i] = 0;		// �޸�Ϊ������, ��ɷֶ�
				parts[count] = text + start;			// д���׵�ַ
				count++;
			}
		}
		else
		{
			if (!flag)				// ������Ч�ַ�
			{
				flag = 1;			// ��flag ��Ϊ1, ����佫����ִ��, ֱ���´������ָ���
				start = i;			// ��¼�׵�ַ
			}
		}
	}

	return count;			// ���طֶθ���
}

int FileUtils::MaxFileUnit(Json::Value list)
{
	int count = 0;
	for (int i = 0; i < list.size(); i++)
	{
		long long fileSize = list[i]["fileSize"].asInt64();

		int n = log10(abs(fileSize)) + 1;		// ��������λ��
		if (count < n)
			count = n;
	}

	return count;
}

string FileUtils::AsDayTime(tm* Time)
{
	string result;

	// ��ȡСʱ;
	char buf[128] = { 0 };
	int hour = Time->tm_hour;
#ifdef _WIN32
	_itoa_s(hour, buf, 128, 10);
#else
	// itoa(hour, buf, 10);
	// ע��, itoa ������һ����׼��C����, ����Windows���е�, ���Ҫд��ƽ̨�ĳ���, ��ʹ��sprintf;
	sprintf(buf, "%d", hour);
#endif // _WIN32
	if (hour < 10)
		result += "0";
	result += buf;
	result += ":";
	
	// ��ȡ����;
	int minute = Time->tm_min;
#ifdef _WIN32
	_itoa_s(minute, buf, 128, 10);				// ���� Radix ��ʾת���Ľ�����, 2��ʾ2����, 10��ʾ10����;
#else
	// itoa(minute, buf, 10);
	// ע��, itoa ������һ����׼��C����, ����Windows���е�, ���Ҫд��ƽ̨�ĳ���, ��ʹ��sprintf;
	sprintf(buf, "%d", minute);
#endif // _WIN32
	if (minute < 10)
		result += "0";
	result += buf;
	result += ":";

	// ��ȡ����;
	int second = Time->tm_sec;
#ifdef _WIN32
	_itoa_s(second, buf, 128, 10);
#else
	sprintf(buf, "%d", second);
#endif // _WIN32
	if (second < 10)
		result += "0";
	result += buf;

	return result;
}

string FileUtils::AsTime(bool asDaytime)
{
	string result;
	tm* Time = new tm;
	time_t time1 = time(NULL);
#ifdef _WIN32
	localtime_s(Time, &time1);
#else
	Time = localtime(&time1);
#endif

	// ��ֹ�쳣;
	if (Time == NULL)
		return result;
	
	// ��ȡ���;
	char buf[128] = { 0 };
	int year = Time->tm_year + 1900;
#ifdef _WIN32
	_itoa_s(year, buf, 128, 10);
#else
	// itoa(year, buf, 10);
	// ע��, itoa ������һ����׼��C����, ����Windows���е�, ���Ҫд��ƽ̨�ĳ���, ��ʹ��sprintf;
	sprintf(buf, "%d", year);
#endif // _WIN32
	result = buf;
	result += "-";

	// ��ȡ�·�;
	int month = Time->tm_mon + 1;
#ifdef _WIN32
	_itoa_s(month, buf, 128, 10);				// ���� Radix ��ʾת���Ľ�����, 2��ʾ2����, 10��ʾ10����;
#else
	// itoa(month, buf, 10);
	// ע��, itoa ������һ����׼��C����, ����Windows���е�, ���Ҫд��ƽ̨�ĳ���, ��ʹ��sprintf;
	sprintf(buf, "%d", month);
#endif // _WIN32
	if (month < 10)
		result += "0";
	result += buf;
	result += "-";

	// ��ȡ����;
	int day = Time->tm_mday;
#ifdef _WIN32
	_itoa_s(day, buf, 128, 10);
#else
	sprintf(buf, "%d", day);
#endif // _WIN32
	result += buf;
	result += " ";
	

	// ��ȡ ʱ����;
	if (asDaytime)
		result += AsDayTime(Time);
	
	return result;
}


string FileUtils::Login_Result(Json::Value list)
{
	string result;
	result.append("* * * * * * * * * * * * * * * * * * * * * * * * \n");
	result.append("*                                             * \n");
	result.append("*          Welcome To The NAS Server .        * \n");
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
	result.append("������: \n");
	// int fileUnit = MaxFileUnit(list);			// ��ȡ���ֵ��λ��;

	for (int i = 0; i < list.size(); i++)
	{
		// ��ӡĿ¼��Ȩ��;
		bool isDir = list[i]["isDir"].asBool();
		if (isDir)
			result.append("d");
		else
			result.append("-");

		// ��ӡ�ļ���Ȩ��;
		int fileMode = list[i]["fileMode"].asInt();
		if (fileMode >= 6)
			result.append("rwx-.    ");
		else if(fileMode >= 4)
			result.append("r-x-.    ");
		else if (fileMode >= 2)
			result.append("-wx-.    ");
		else
			result.append("--x-.    ");

		// ��ȡ�ļ��Ĵ�С;
		long long fileSize = list[i]["fileSize"].asInt64();
		
		// ���ֽڻ����Kb, M, G �ȵ�λ;
		char ch = 0;
		long long gb_unit = 1024 * 1024 * 1024;
		long long mb_unit = 1024 * 1024;
		long long kb_unit = 1024;
		if (fileSize > gb_unit)
		{
			fileSize /= gb_unit;
			ch = 'G';
		}
		else if (fileSize > mb_unit)
		{
			fileSize /= mb_unit;
			ch = 'M';
		}
		else if (fileSize > kb_unit)
		{
			fileSize /= kb_unit;
			ch = 'K';
		}
		
		// int count = MaxFileUnit(list);			// ��ȡ���ֵ��λ��;
		
		int n = log10(abs(fileSize)) + 1;			// ��������λ��;
		if (n < 0)	n = 1;
		// int balance = fileUnit - n;				// �����ֵ֮���λ����;
		int balance = 4 - n;
		
		
		// �Ҷ����ļ���С;
		for (int i = 0; i < balance; i++)
		{
			result.append(" ");
		}
		
		// ���ļ���С ת���� �ַ���;
		string string = std::to_string(fileSize);
		result.append(string);
		if(ch != 0)
			result += ch;

		// ��ȡ�ļ���ʱ��;
		long long fileTime = list[i]["fileTime"].asInt64();

		// ���ļ�ʱ�� ת���� Windows�ļ���ʽ;
		FILETIME filetime;
		filetime.dwLowDateTime = fileTime;
		filetime.dwHighDateTime = fileTime >> 32;

		// �� Windows�ļ�ʱ�� ת���� ϵͳ����ʾʱ��;
		SYSTEMTIME systime;
		FileTimeToSystemTime(&filetime, &systime);
		
		int year = systime.wYear;
		int month = systime.wMonth;
		int day = systime.wDay;
		
		int hour = systime.wHour;
		int minute = systime.wMinute;

		// ��ӡ����;
		if (month >= 10)
			result.append("\t " + std::to_string(month) + "��");
		else
			result.append("\t  " + std::to_string(month) + "��");

		if(day >= 10)
			result.append("\t " + std::to_string(day));
		else
			result.append("\t  " + std::to_string(day));


		// ��ȡ��ǰ�������ʱ��;
		time_t computer_time = NULL;
		time(&computer_time);
		tm tp;
		localtime_s(&tp, &computer_time);

		// �������ʱ�� ת���� ����ʾʱ��;
		int computer_year = 1900 + tp.tm_year;
		if (year < computer_year)					// ��ӡ�ļ������
		{
			result.append("    " + std::to_string(year));
		}
		else										// ��ȷ���ļ���ʱ����
		{
			if (hour >= 10)
				result.append("   " + std::to_string(hour) + ":");
			else
				result.append("   0" + std::to_string(hour) + ":");

			if (minute >= 10)
				result.append(std::to_string(minute));
			else
				result.append("0" + std::to_string(minute));
		}


		// ��ӡ�ļ���;
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

	result.append("\nHELP: \n");
	result.append("\tls [FILE] - list directory contents . \n");
	result.append("\tll [FILE] - list detailed directory contents . \n");
	result.append("\tcd [FILE] - Change  the  current  directory . \n");
	result.append("\tget <FILE> [ -o Local_directory] - Requests the download files from the server . \n");
	result.append("\tput <FILE> - Requests the upload files from the server . \n");
	result.append("\trm  <FILE> - Requests the delete the file or directory from the server . \n");
	result.append("\texit - Exit the server . \n\n");
	
	return result;
}