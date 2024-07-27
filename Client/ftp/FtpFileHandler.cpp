#include "FtpFileHandler.h"

FileHandler::FileHandler(OS_TcpSocket sock, string homeDir, bool is_win, int size)
{
	FileHandler(sock, homeDir, is_win, size);
}

void FileHandler::operator()(bool is_windows)
{
	isWindows = is_windows;
}

void FileHandler::operator()(const char* homeDir)
{
	// cmdline 的长度加 15, 为之后的内存拷贝 预留一定空间; 
	int home_len = strlen(homeDir) + 15;
	char* cmdline = new char[home_len];

#ifdef _WIN32
	// strcpy_s(cmdline, cmd_len, data);
	memcpy_s(cmdline, home_len, homeDir, home_len);
#else
	memcpy(cmdline, homeDir, strlen(homeDir));
#endif

	m_homeDir = cmdline;
}

void FileHandler::operator()(string homeDir)
{
	m_homeDir = homeDir;
}

void FileHandler::operator()(OS_TcpSocket sock, string homeDir, bool is_win, int size)
{
	m_type = 0;
	m_homeDir = homeDir;

	fp = NULL;
	fileSize = 0;

	isWin = is_win;
	Sock = sock;

	m_size = size;
	m_data = new char[m_size];
}

// notDir 为单独文件的处理 (非目录文件);
string FileHandler::on_ls(Json::Value& jresult, string& path, bool notDir)
{
	string result;
	
	// 解析请求
	// printf("ls %s%s ...\n", m_homeDir.c_str(), m_path.c_str());
	jresult.clear();		// 防止重复显示

#ifdef _WIN32
	// 判断消息
	// msg_list2 为false, 则此函数用于处理 on_ls消息请求; 为true则处理 on_ll消息请求;
	bool msg_list2 = (m_type == MSG_LIST2) ? true : false;

	// 遍历文件信息
	FileEntryList files = FileUtils::List(m_homeDir + path, msg_list2, notDir);
	for (FileEntryList::iterator iter = files.begin();
		iter != files.end(); iter++)
	{
		FileEntry& entry = *iter;
		Json::Value jobj;
		jobj["fileName"] = entry.fileName;
		jobj["isDir"] = entry.isDirectory;
		jobj["fileSize"] = (double)entry.fileSize;				// 在Linux下, jsoncpp不能处理long类型数据;
		jobj["fileTime"] = (double)entry.filetime;
		jobj["fileMode"] = entry.fileMode;
		jresult.append(jobj);
	}

	result = FileUtils::List_Result(jresult, result);
#else
	result = ls_linux(m_type, jresult, path);
#endif

	return result;
}

// notDir 为单独文件的处理 (非目录文件);
string FileHandler::on_ll(Json::Value& jresult, string& path, bool notDir)
{
	string result;
	jresult.clear();			// 防止重复显示

#ifdef _WIN32
	on_ls(jresult, path, notDir);
	result = FileUtils::List2_Result(jresult, result);
#else
	result = ls_linux(m_type, jresult, path);
#endif

	return result;
}

#ifndef _WIN32
string FileHandler::ls_linux(unsigned int type, Json::Value& jresult, string& path)
{
	string result;

	// 获取客户端的IP地址;
	OS_SockAddr SockAddr;
	Sock.GetPeerAddr(SockAddr);

	// 创建一个临时文件, 用来保存用户的操作结果;
	unsigned short pid = SockAddr.GetPort();
	string file = "outcmd" + to_string(pid) + ".txt";

	// 临时文件的保护, 防止文件重命名;
	do
	{
		int exist = access(file.c_str(), F_OK);
		if (exist != 0) break;

		file = "outcmd" + to_string(rand() % pid) + ".txt";
	} while (true);
	
	// 用来保存将要执行的命令;
	string cmdline;

	// 执行命令;
	if(type == MSG_LIST2)
		cmdline = "ls -l " + m_homeDir + path + " >> " + file;					// '>>'自动创建文件;
	else
		cmdline = "ls " + m_homeDir + path + " >> " + file;
	system(cmdline.c_str());
	
	// 打开文件;
	ifstream ifs;
	ifs.open(file.c_str());					// 只能打开文件, 无法创建文件;

	// 用来保存命令行输出的结果;
	string line;
	int count = 0;							// 遍历文件的次数;

	// 读取命令行的结果;
	while (getline(ifs, line))
	{
		if (count == 0)
		{
			// 判断'ls'命令是否有效;
			int ls_fail = line.find("ls:");
			if (ls_fail > 0)
			{
				result.append(line + "\n");
				break;
			}
		}

		// 'll'命令的处理;
		if (type == MSG_LIST2)
		{
			result.append(line + "\n");
			
			if(count == 0 && isWindows)
				result.append("\n");
			
			// 保存命令行的结果;
			if (count > 0)
			{
				char* file_info[128];
				FileUtils::Split((char*)line.c_str(), file_info);

				// Json::Value的计数;
				int number = count - 1;
				jresult[number]["fileName"] = string(file_info[8]);
				jresult[number]["isDir"] = (file_info[0][0] == 'd') ? true : false;
				jresult[number]["fileSize"] = (double)atoll(file_info[4]);							// 在Linux下, jsoncpp不能处理long类型数据;

				// 整合文件时间;
				string time = file_info[5];
				time += " ";
				time += file_info[6];
				time += " ";
				time += file_info[7];

				jresult[number]["fileTime"] = time;
				jresult[number]["fileMode"] = string(file_info[0]);					// 以字符串的形式进行保存;
			}
		}
		else						// 'ls'命令的处理;
		{
			// 每遍历5个文件进行一次换行;
			if (count % 5 == 0 && count != 0)
				result.append("\n");
			
			result.append(line + "   ");
			jresult[count]["fileName"] = line;
		}

		count++;
	}

	// 删除临时文件;
	cmdline = "rm -f " + file;
	system(cmdline.c_str());
	
	return result;
}
#endif


void FileHandler::CheckCdCommand(string& result, string& path, string& part)
{
	// 命令行中 ".."的数量;
	int cd_count = 0;
	// 命令行中 ".."的位置;
	long long cd_pos = 0;					// long long 用于 [] 内相加减, 避免 算数溢出 C26451;

	do
	{
		cd_pos = part.find("..", cd_pos);
		if (cd_pos < 0) break;


		// 防止恶意非法访问, 如: cd ../++../..;
		// 使用两个if 语句来防止内存泄漏;
		if (cd_pos != 0)
		{
			if (part[cd_pos - 1] != '/')
			{
				result = "input bad command ! \n";
				throw result;
			}
		}

		// 防止恶意非法访问, 如: cd ../..+-/..;
		// 使用两个if 语句来防止内存泄漏;
		if (cd_pos + 2 != part.length())
		{
			if (part[cd_pos + 2] != '/')
			{
				result = "input bad command ! \n";
				throw result;
			}
		}

		cd_pos += 2;
		cd_count++;
	} while (true);

	// 防止用户进行越界访问;
	int path_count = std::count(path.begin(), path.end(), '/') - 1;
	if (cd_count > path_count && m_type == MSG_CD)
	{
		result = "The access violation ! \n";
		throw result;
	}
}

void FileHandler::CheckCommand(string& result, string& path, string& part)
{
	int count = 0;						// part 迭代器的计数;

	// 文件名不能包含的字符: \ / : * ? " < > |
	for (string::iterator it = part.begin(); it != part.end(); it++)
	{
		if (count == 1 && *it == ':' && m_type == MSG_GET)
		{
			// on_get 的 输出命令-o, 需要避开;
		}
		else if (*it == '\\' || *it == ':' || *it == '*' || *it == '?'
			|| *it == '"' || *it == '<' || *it == '>' || *it == '|')
		{
			result = "No File or directory ! \n";
			throw result;
		}

		count++;
	}
	
	//  查找 "//" 字符的位置;
	int find_pos = 0;
	if (part.size() >= 4 && m_type == MSG_GET)
		find_pos = 3;
	

	// 命令行不支持 "//" 字符;
	long long bad_commend = part.find("//", find_pos);					// long long 避免 算数溢出 C26451;
	if (bad_commend >= 0)
	{
		result = "input bad command ! \n";
		throw result;
	}

	// 检查 "." 有关的错误命令, 如: ls file./file2
	bad_commend = part.find('.', 0);
	if (bad_commend > 0)				// 使用两个if 语句来防止内存泄漏;
	{
		if (part[bad_commend - 1] != '/' && part[bad_commend + 1] == '/' || bad_commend == part.length() - 2)
		{
			result = "input bad command ! \n";
			throw result;
		}
	}

	if (m_type == MSG_CD && path != "/" || m_type == MSG_GET)
	{

		CheckCdCommand(result, path, part);
	}
	else						// 防止用户跨越根目录进行越界访问;
	{

		int size = part.size();
		if (size <= 2)
			char CanAccess;				// 无不良指令;
		else if (size > 2 && part[0] == part[1] && part[0] == '.'
			|| size > 2 && part[2] == part[3] && part[3] == '.' && part[0] == '.')
		{
			result = "The access violation ! \n";
			throw result;
		}
	}
}

string FileHandler::CheckResult(Json::Value& jresult, string path, string part)
{
	string result;			// 返回检查的结果;
	
	bool is_Dir = IsDirectory(path, part);
	// 将文件和目录分开显示;
	//ifstream ifs(m_homeDir + path + part);
	if (!is_Dir)			// 是否为文件
	{
		if (m_type != MSG_LIST2)
		{
			result = part;
			result.append("\t");
		}
		else
		{
			path += part;

			result = on_ll(jresult, path, true);
			// path = "/";
		}
	}
	else				// 是否为目录
	{
		string directory;			// 单个目录信息;

		// 遍历目录的文件信息
		path += part += "/";
		if (m_type != MSG_LIST)
		{
			result += on_ll(jresult, path).append("\n");
		}
		else
		{
			result = on_ls(jresult, path);
			// result = FileUtils::List_Result(jresult, directory);
		}
		// path = "/";
	}

	return result;
}

bool FileHandler::IsDirectory(string path, string part)
{
	bool is_Dir = false;
#ifdef _WIN32
	struct _stat infos;
	_stat((m_homeDir + path + part).c_str(), &infos);
	
	if (infos.st_mode & _S_IFDIR)
    {
        is_Dir = true;    			//目录
    }
    else if (infos.st_mode & _S_IFREG)
    {
        is_Dir = false;				//文件
    }
#else
	struct stat infos;
	stat((m_homeDir + path + part).c_str(), &infos);
	
	if (infos.st_mode & S_IFDIR)
    {
        is_Dir = true;    			//目录
    }
    else if (infos.st_mode & S_IFREG)
    {
        is_Dir = false;				//文件
    }
#endif
	
	return is_Dir;
}


int FileHandler::ACK_Send(unsigned short type, const void* data, unsigned int length)
{
	// 发送消息类型
	Sock.Send(&type, 2);
	Sock.Send(&length, 4);

	// 数据部分是0个字节则退出
	if (length <= 0) return -1;

	// 发送数据
	return Sock.Send(data, length, false);
}

int FileHandler::ACK_Recv()
{
	// 接收消息类型
	Sock.Recv(&m_type, 2);
	Sock.Recv(&m_length, 4);

	// 数据部分是0个字节则退出
	if (m_length <= 0) return 0;

	// 接收数据
	return ReceiveN(m_data, m_length);
}

int for_count = 0;
void FileHandler::SendFile(string path, char** argv, int argc, OS_Mutex& Mutex)
{
	// 单个子命令;
	string part;

	char* argv2[128];
	Json::Value jresult;

	for (int i = 1; i < argc; i++)
	{
		// 获取子命令;
		part = argv[i];

		Mutex.Lock();
		
		// 判断是否为目录;
		bool isDir = IsDirectory(path, part);
		if (isDir)
		{
			ACK_Send(ACK_NEW_DIRECTORY, part.c_str(), part.length());
			
			// 将 part 加到 path 中, 方便后续调用;
			jresult.clear();
			path += part;
			if (part[part.length() - 1] != '/')						// path 尾数加上 '/';
				path += "/";
			
			on_ls(jresult, path);				// Not Linux
			// printf("path: %s \n", path.c_str());

			Mutex.Unlock();

			argv2[0] = 0;
			for (int i = 1; i <= jresult.size(); i++)
			{
				argv2[i] = (char*) jresult[i - 1]["fileName"].asCString();
			}

			for_count++;
			if(jresult.size() > 0)
				SendFile(path, argv2, jresult.size() + 1, Mutex);

			for_count--;

			long long dir_pos = path.find_last_of('/', path.length() - 2);
			path.erase(dir_pos + 1);

			ACK_Send(ACK_NEW_FINISH);
		}
		else					// 判断为文件处理;
		{
			// 打开文件;
			FILE* fp = NULL;
		#ifdef _WIN32
			fopen_s(&fp, (m_homeDir + path + part).c_str(), "rb");
		#else
			fp = fopen((m_homeDir + path + part).c_str(), "rb");
		#endif
			if(fp == NULL)
			{
				printf("File error ! failed to download file . \n");
				return;
			}
			
			
			ACK_Send(ACK_NEW_FILE, part.c_str(), part.length());

			Json::Value jsonFile;
			string file_path = path + part;

			on_ll(jsonFile, file_path, true);				// Not Linux
			// printf("path2: %s \n", file_path.c_str());

			long long fileSize = (double) jsonFile[0]["fileSize"].asInt64();
			ACK_Send(ACK_DATA_SIZE, &fileSize, 8);

			int numOfBytes = 0;
			int bufsize = 1024 * 24;		// 24KB
			char* buf = new char[bufsize];
			
			while (!feof(fp))
			{
				int n;
			#ifdef _WIN32
				n = fread_s(buf, bufsize, 1, bufsize, fp);
			#else
				n = fread(buf, 1, bufsize, fp);
			#endif

				if (n < 0) break;
				if (n == 0) continue;
				if (ACK_Send(ACK_DATA_CONTINUE, buf, n) < 0)
				{
					// 网络已中断
					break;
				}

				// 可以适当sleep,减少发送速率
				numOfBytes += n;
				if (numOfBytes > 1000000 * 5)
				{
					numOfBytes = 0;
					OS_Thread::Msleep(50);
				}
			}

			ACK_Send(ACK_DATA_FINISH);
			fclose(fp);
		}
		
		if(for_count == 0)
			ACK_Send(ACK_PATH_END);
		Mutex.Unlock();
	}
}

void FileHandler::RecvFile(string path, OS_Mutex& Mutex)
{
	string part;
	int recv_length = 0;

	while (recv_length >= 0)
	{
		Mutex.Lock();
		recv_length = ACK_Recv();

		if (m_type == ACK_FINISH)
			break;

		if (recv_length >= 0)
		{
			m_data[recv_length] = 0;
			part = m_data;
			RecvHandler(path, part);
		}

		Mutex.Unlock();
	}
	Mutex.Unlock();
}

void FileHandler::RecvHandler(string& path, string& part)
{
	if (m_type == ACK_NEW_DIRECTORY || m_type == ACK_NEW_FILE)
	{
		// 带有 '/'符号的路径进行上传和下载时, 需要删除 '/'之前的路径;
		long long pos_len = part.find_last_of('/', part.length() - 2);				// long long 避免 算数溢出;				
		if (pos_len >= 0)
			part = part.substr(pos_len + 1);						// 只保留文件名;
	}

	if (m_type == ACK_NEW_DIRECTORY)
	{
	#ifdef _WIN32
		int dir = _mkdir((m_homeDir + path + part).c_str());
	#else
		int dir = mkdir((m_homeDir + path + part).c_str(), S_IRWXU);
	#endif
		if (dir < 0)
		{
			printf("Download error! Failed to create a directory . \n");
			return;
		}

		path += part += "/";
	}

	if (m_type == ACK_NEW_FINISH)
	{
		long long cd_pos = path.find_last_of('/', path.length() - 2);				// long long 避免 算数溢出;
		if(cd_pos > 0)
			path.erase(cd_pos + 1);
	}

	if (m_type == ACK_NEW_FILE)
	{
	#ifdef _WIN32
		fopen_s(&fp, (m_homeDir + path + part).c_str(), "ab+");
	#else
		fp = fopen((m_homeDir + path + part).c_str(), "ab+");
	#endif
		if (fp == NULL)
		{
			printf("Download error! Failed to create a file . \n");
			return;
		}
	}

	if (m_type == ACK_DATA_SIZE)
	{
	#ifdef _WIN32
		memcpy_s(&fileSize, 8, m_data, sizeof(fileSize));
	#else
		memcpy(&fileSize, m_data, sizeof(fileSize));
	#endif
	}

	if (m_type == ACK_DATA_CONTINUE)
	{
		int numOfBytes = 0;

		// 写入文件
		int n = fwrite(m_data, 1, m_length, fp);
		numOfBytes += n;
		printf(".");
	}

	if (m_type == ACK_PATH_END)
	{
		path = "/";
	}

	if (m_type == ACK_DATA_FINISH)
	{
		fclose(fp);
	}
}


// 检查文件或目录 是否存在, 若存在则返回相关的文件名称;
string FileHandler::CheckFile(string& path, char* data, unsigned int type, bool isClient)
{
	char* argv[64];					// 命令行的子命令;
	string result;					// 检查文件的结果;
	Json::Value jsonResult;			// 文件的详细信息;
	m_type = type;
	
	// cmdline 的长度加 15, 为之后的内存拷贝 预留一定空间; 
	int cmd_len = strlen(data) + 15;
	char* cmdline = new char[cmd_len];

#ifdef _WIN32
	// strcpy_s(cmdline, cmd_len, data);
	memcpy_s(cmdline, cmd_len, data, cmd_len);
#else
	//memcpy(cmdline, data, strlen(data));
	strcpy(cmdline, data);
#endif
	
	// 不同系统之间的编码转换;
	if (isWindows != isWin)
	{
	#ifdef _WIN32
		string result = Utf8ToGbk(cmdline);
		memcpy_s(cmdline, cmd_len, result.c_str(), result.size());
	#else
		char cmdline2[128] = {0};
		GbkToUtf8(cmdline, cmd_len, cmdline2, 128);
		memcpy(cmdline, cmdline2, strlen(cmdline2));
	#endif
	}
	
	// 获取命令行的子命令;
	int argc = FileUtils::Split(cmdline, argv);
	if (argc > 0)
	{
		if (argc > 8)
		{
			result = "Access denied, Because command too long ! \n";
			throw result;
		}

		// 单个子命令;
		string part;

		// 检查文件或目录
		for (int i = 1; i < argc; i++)
		{
			// 获取子命令;
			part = argv[i];
			
			CheckCommand(result, path, part);

			// 判断文件或目录是否存在;
		#ifdef _WIN32
			int exist = _access_s((m_homeDir + path + part).c_str(), 0);

			// 未查找的异常处理;
			if (exist == ENOENT)
			{
				result = "No File or directory ! \n";
				throw result;
			}
			if (exist == EACCES)
			{
				result = "Access denied ! \n";
				throw result;
			}
		#else
			/*
			int length = part.length();
			if(part[length - 2] == '/' && part[length - 1] == 's')
				part.erase(length - 2);*/
			
			/*
			int find_pos = part.find('\\');
			if(find_pos > 0)
			{
				part.erase(find_pos);
				part[find_pos - 1] = 0;
			}*/
			
			int exist = 0;
			if(isClient)
				exist = access(part.c_str(), F_OK);
			else
				exist = access((m_homeDir + path + part).c_str(), F_OK);
			
			if (exist < 0)
			{
				result = "input bad command ! \n";
				throw result;
			}
		#endif


			// 防止访问 非目录文件;
			if (m_type == MSG_CD || m_type == MSG_GET && isClient)
			{
				/*
				ifstream ifs(m_homeDir + path + part);
				if (ifs.is_open())
				{
					result = "The access violation ! \n";
					throw result;
				}*/
				bool is_Dir = IsDirectory(path, part);
				if(!is_Dir)
				{
					result = "The access violation ! \n";
					throw result;
				}
			}

			// 返回文件或目录的结果;
			if (m_type == MSG_LIST || m_type == MSG_LIST2)
				result += CheckResult(jsonResult, path, part);
		}
	}

#ifdef _WIN32
	delete[] cmdline;		// 使用 memcpy_s 时, 才能删除;
#endif
	
	return result;			// 返回文件名称;
}


int FileHandler::ReceiveN(void* buf, int count, int timeout)
{
	// 设置超时
	if (timeout > 0)
	{
		Sock.SetOpt_RecvTimeout(timeout);
	}

	// 接收数据过长的异常处理
	if (count > m_size)
	{
		printf("ReceiveN function error, the received data is too long ! \n");
		return -12;
	}

	// 反复接收数据, 直到接满指定的字节数;
	int bytes_got = 0;
	while (bytes_got < count)
	{
		int n = Sock.Recv((char*)buf + bytes_got, count - bytes_got, false);
		if (n <= 0)
		{
			continue;
		}

		bytes_got += n;
	}

	return bytes_got;	// 返回接收数据的大小;
}