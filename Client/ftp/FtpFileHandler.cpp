#include "FtpFileHandler.h"

FileHandler::FileHandler() : windows_transfer(false)
{
#ifdef _WIN32
	FileUtils::CMD_GBK();
#endif
}

FileHandler::FileHandler(OS_TcpSocket sock, string homeDir, bool is_win, bool isClient, int size) 
	: fileCheck(Sock, homeDir, is_win, isClient)
{
	FileHandler(sock, homeDir, size);
#ifdef _WIN32
	FileUtils::CMD_GBK();
#endif
}

void FileHandler::operator()(bool is_windows)
{
	fileCheck(is_windows);
}

void FileHandler::operator()(const char* homeDir)
{ 
	int home_len = strlen(homeDir);
	if(home_len <= 0)
	{
		// homeDir 为 "" 时, 跳过内存拷贝;
		m_homeDir = "";
		fileCheck("");
		return;
	}
	
	char* cmdline = new char[(home_len + 1)];

#ifdef _WIN32
	// strcpy_s(cmdline, cmd_len, data);
	memcpy_s(cmdline, home_len, homeDir, home_len);
#else
	memcpy(cmdline, homeDir, strlen(homeDir));
#endif

	m_homeDir = cmdline;
	fileCheck(m_homeDir);
}

void FileHandler::operator()(string homeDir)
{
	m_homeDir = homeDir;
	fileCheck(m_homeDir);
}

void FileHandler::operator()(OS_TcpSocket sock, string homeDir, bool is_win, bool isClient, int size)
{
	m_type = 0;
	m_homeDir = homeDir;

	fp = NULL;
	fileSize = 0;

	fileCheck(sock, homeDir, is_win, isClient);

	Sock = sock;
	windows_transfer = false;

	m_size = size;
	m_data = new char[m_size];
}


#ifndef _WIN32
string FileHandler::add_directory(Json::Value& jresult, char* file_info[], int argc, int& directory_path, int& count)
{
	string Directory;
	char* file_info2[384];

	// 文件路径存在空格形式;
	string file2(file_info[0]);
	for (int i = 1; i < argc; i++)
	{
		file2 += " ";
		file2 += file_info[i];
	}

	int argc2 = FileUtils::Split((char*)file2.c_str(), file_info2, "/");
	if (count == 0) directory_path = argc2 - 1;

	Directory.clear();
	for (int j = directory_path; j < argc2; j++)
	{
		Directory += file_info2[j];

		// 清除末尾的 ':', 方便后续操作;
		if (j == argc2 - 1)
		{
			int length = Directory.length() - 1;
			if (Directory[length] == ':')
				Directory = Directory.erase(length, 1);
		}

		// 预先导入 需要下载的根目录;
		if (count == 0)
		{
			jresult[count]["fileName"] = Directory;
			jresult[count]["isDir"] = true;
			count++;
		}

		Directory += "/";
	}

	return Directory;
}

string FileHandler::ls_linux(unsigned int type, Json::Value& jresult, string& path)
{
	return fileCheck.ls_linux(type, jresult, path);
}

void FileHandler::ls_linux2(unsigned int type, Json::Value& jresult, string& path)
{
	// 获取客户端的IP地址;
	OS_SockAddr SockAddr;
	Sock.GetPeerAddr(SockAddr);

	// 创建一个临时文件, 用来保存用户的操作结果;
	unsigned short pid = SockAddr.GetPort();
	string file = "outcmd" + std::to_string(pid) + ".txt";

	// 临时文件的保护, 防止文件重命名;
	do
	{
		int exist = access(file.c_str(), F_OK);
		if (exist != 0) break;

		file = "outcmd" + std::to_string(rand() % pid) + ".txt";
	} while (true);

	// 用来保存将要执行的命令;
	string cmdline;

	// 执行命令;
	cmdline = "ls -lR \'" + m_homeDir + path + "\' >> " + file;					// '>>'自动创建文件;
	system(cmdline.c_str());

	// 打开文件;
	std::ifstream ifs;
	ifs.open(file.c_str());					// 只能打开文件, 无法创建文件;

	// 用来保存命令行输出的结果;
	string line;
	int count = 0;							// 遍历文件的次数;
	int directory_path = 0;					// 保存根目录的下标;
	
	string fileName;						// 文件名;
	string Directory;						// 文件位置;
	
	// 读取命令行的结果;
	while (getline(ifs, line))
	{
		// 保存命令行的结果;
		char* file_info[384];
		int argc = FileUtils::Split((char*)line.c_str(), file_info);
		if (argc <= 0)	continue;						// 换行符

		// 总用量 436
		if (argc == 2)
		{
			string total = file_info[1];
			int total_length = total.size();

			if (total[total_length - 1] != ':')
				continue;
		}

		if (file_info[0][0] == '/' || file_info[0][0] == '.' && file_info[1][0] == '/' ||
			file_info[0][0] == '.' && file_info[1][0] == '.' && file_info[2][0] == '/')
		{
			// 添加文件的路径;
			Directory = add_directory(jresult, file_info, argc, directory_path, count);
			continue;
		}
		else
		{
			// 读到文件或目录
			if (argc > 8)
			{
				// 文件存在空格形式;
				fileName = Directory;
				for (int i = 8; i < argc; i++)
				{
					fileName += string(file_info[i]);
					if (i != argc - 1)
						fileName += " ";
				}
			}
			else		// 普通文件
			{
				fileName = Directory + string(file_info[8]);
			}
		}

		// Json::Value的计数;
		jresult[count]["fileName"] = fileName;
		jresult[count]["isDir"] = (file_info[0][0] == 'd') ? true : false;
		jresult[count]["fileSize"] = (double)atoll(file_info[4]);							// 在Linux下, jsoncpp不能处理long类型数据;

		// 整合文件时间;
		string time = file_info[5];
		time += " ";
		time += file_info[6];
		time += " ";
		time += file_info[7];

		jresult[count]["fileTime"] = time;
		jresult[count]["fileMode"] = string(file_info[0]);					// 以字符串的形式进行保存;

		count++;
	}

	// 删除临时文件;
	cmdline = "rm -f " + file;
	system(cmdline.c_str());
}
#endif


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

void FileHandler::SendFile(string& path, string& part)
{
	// 打开文件;
	FILE* fp = NULL;
#ifdef _WIN32
	fopen_s(&fp, (m_homeDir + path + part).c_str(), "rb");
#else
	fp = fopen((m_homeDir + path + part).c_str(), "rb");
#endif
	if (fp == NULL)
	{
		directory_distances == 0;
		fileError = "Error: failed to upload file, unable to create file . \n";
		return;
	}


	ACK_Send(ACK_NEW_FILE, part.c_str(), part.length());

	Json::Value jsonFile;
	string file_path = path + part;

	fileCheck.on_ll(jsonFile, file_path, true);
	// printf("path2: %s \n", file_path.c_str());

	long long fileSize = (double)jsonFile[0]["fileSize"].asInt64();
	ACK_Send(ACK_DATA_SIZE, &fileSize, 8);

	int numOfBytes = 0;

	int bufsize = 1024 * 24;					// 24KB
	if (fileSize > 1024 * 1024 * 800)
		bufsize = 1024 * 1024 * 240;			// 240MB
	else if (fileSize > 1024 * 1024 * 60)
		bufsize = 1024 * 1024 * 24;				// 24MB

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

		if (fileCheck.isClient) printf(".");

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

void FileHandler::SendDirectory(Json::Value& jresult, string& path, string& part, char* argv2[], OS_Mutex& Mutex)
{
	ACK_Send(ACK_NEW_DIRECTORY, part.c_str(), part.length());

	// 将 part 加到 path 中, 方便后续调用;
	jresult.clear();
	path += part;
	if (part[part.length() - 1] != '/')						// path 尾数加上 '/';
		path += "/";

	fileCheck.on_ls(jresult, path);
	// printf("path: %s \n", path.c_str());

	Mutex.Unlock();

	// 防止文件过多, 超过存放文件的缓冲区;
	if (jresult.size() > 1023)
	{
		directory_distances == 0;
		fileError = "Failed to upload file, too many files have been uploaded or downloaded, \
				the number of files cannot be greater than 1024 . \n";
		return;
	}

	argv2[0] = 0;
	for (int i = 1; i <= jresult.size(); i++)
	{
		argv2[i] = (char*)jresult[i - 1]["fileName"].asCString();
	}

	directory_distances++;
	// 使用递归 来加载一个目录中的所有目录;
	if (jresult.size() > 0)
		UploadFile(path, argv2, jresult.size() + 1, Mutex);

	directory_distances--;

	long long dir_pos = path.find_last_of('/', path.length() - 2);
	path.erase(dir_pos + 1);

	ACK_Send(ACK_NEW_FINISH);
}

// 检查文件或目录 是否存在;
string FileHandler::CheckFile(string& path, char* data, unsigned int type)
{
	return fileCheck.CheckFile(path, data, type);
}

void FileHandler::UploadFile(string path, char** argv, int argc, OS_Mutex& Mutex)
{
	// 单个子命令;
	string part;

	char* argv2[1024];						// 存放文件, 注意 argv2不可以小于文件的数量;
	Json::Value jresult;

	for (int i = 1; i < argc; i++)
	{
		// 获取子命令;
		part = argv[i];


	#ifdef _WIN32
		Mutex.Lock();
		
		// 判断是否为目录;
		bool isDir = fileCheck.IsDirectory(path, part);
		if (isDir)
			SendDirectory(jresult, path, part, argv2, Mutex);
		else						// 判断为文件处理;
			SendFile(path, part);
		
		// 上传文件错误, 本次传输结束;
		if (fileError.size() > 0)
			i = argc;

		if (directory_distances == 0)
		{
			ACK_Send(ACK_PATH_END);

			// 防止 put 命令上传出错;
			if (fileCheck.isClient)
				path.clear();
		}

		Mutex.Unlock();
	#else
		Linux_SendFile(jresult, part, Mutex);
	#endif
	}
}

void FileHandler::LinuxHandler(string& path)
{
	bool Win32 = false;
#ifdef _WIN32
	Win32 = true;
#endif

	// Linux 系统之间 或 不同操作系统之间的处理;
	if (fileCheck.isWin != fileCheck.isWindows && Win32)
		windows_transfer = true;

	// Windows 系统之间的处理;
	if (fileCheck.isWin == fileCheck.isWindows && fileCheck.isWindows)
		windows_transfer = true;
}

void FileHandler::DownloadFile(string path, OS_Mutex& Mutex)
{
	// 保存用户的相对目录;
	m_path = path;

	string part;
	int recv_length = 0;

	LinuxHandler(path);

	while (recv_length >= 0)
	{
		Mutex.Lock();
		recv_length = ACK_Recv();

		// 下载完成
		if (m_type == ACK_FINISH)
			break;

		if (recv_length >= 0)
		{
			m_data[recv_length] = 0;
			part = m_data;
			RecvHandler(path, part);
		}

		// 下载文件错误, 终止下载
		if (fileError.size() > 0)
			break;

		Mutex.Unlock();
	}
	Mutex.Unlock();
}

#ifndef _WIN32
void FileHandler::Linux_SendFile(Json::Value& jresult, string& path, OS_Mutex& Mutex)
{
	// 清除 path末尾的 '/', 方便后续操作;
	int length = path.length() - 1;
	if(path[length] == '/')
		path = path.erase(length, 1);
	
	// 添加 path首下标的 '/';
	if(path[0] != '/')
		path.insert(0, "/");
	
	ls_linux2(m_type, jresult, path);
	
	// 发送文件的根目录;
	if(fileCheck.isClient)		// 客户端
	{
		length = path.find_last_of('/', length - 1);
		if(length > 0)
			path = path.substr(0, length + 1);
	}
	else					// 服务端 
	{
		path = '/';
	}
	
	for (int i = 0; i < jresult.size(); i++)
	{
		Mutex.Lock();
		
		bool isDirectory = jresult[i]["isDir"].asBool();
		string part = jresult[i]["fileName"].asString();
		
		if (isDirectory)
		{
			ACK_Send(ACK_NEW_DIRECTORY, part.c_str(), part.length());
		}
		else
		{
			SendFile(path, part);
		}
		
		Mutex.Unlock();
	}
}
#endif

void FileHandler::RecvHandler(string& path, string& part)
{
	if (m_type == ACK_NEW_DIRECTORY || m_type == ACK_NEW_FILE)
	{
		bool Win32 = false;
	#ifdef _WIN32
		Win32 = true;
	#endif
		
		// 对端为 Windows系统 的额外操作;
		if (windows_transfer)
		{
			// 带有 '/'符号的路径进行上传和下载时, 需要删除 '/'之前的路径;
			long long pos_len = part.find_last_of('/', part.length() - 2);				// long long 避免 算数溢出;				
			if (pos_len >= 0)
				part = part.substr(pos_len + 1);						// 只保留文件名;
		}
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
			fileError = "Error: unable to create directory . \n";
			return;
		}


		bool Win32 = false;
	#ifdef _WIN32
		Win32 = true;
	#endif
		
		// 对端为 Windows系统 的额外操作;
		if (windows_transfer)
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
			fileError = "Error: unable to create files . \n";
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

		if(fileCheck.isClient) printf(".");
	}

	if (m_type == ACK_PATH_END)
	{
		// 返回到原来的目录;
		if (!fileCheck.isClient)
			path = m_path;
		else
			path = "/";
	}

	if (m_type == ACK_DATA_FINISH)
	{
		fclose(fp);
	}
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