#include "FtpFileHandler.h"

FileHandler::FileHandler() : windows_transfer(false)
{
#ifdef _WIN32
	FileUtils::CMD_GBK();
#endif
}

FileHandler::FileHandler(OS_TcpSocket sock, string homeDir, char local_system, bool isClient, int size)
	: fileCheck(Sock, homeDir, local_system, isClient)
{
	FileHandler(sock, homeDir, size);
#ifdef _WIN32
	FileUtils::CMD_GBK();
#endif
}

void FileHandler::operator()(char peerTheSystem)
{
	fileCheck(peerTheSystem);
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

void FileHandler::operator()(OS_TcpSocket sock, string homeDir, char local_system, bool isClient, int size)
{
	m_type = 0;
	m_homeDir = homeDir;

	fp = NULL;
	fileSize = 0;

	fileCheck(sock, homeDir, local_system, isClient);

	cmdline_count = 0;
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

// linux_notDir: Linux命令行中, 子命令是否为单个文件;  
void FileHandler::SendFile(string& path, string& part, bool linux_notDir)
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

	// 在Linux系统中, 将命令行的文件去掉 "/", 为下面发送 ACK_NEW_FILE 指令使用;
	if (linux_notDir)
	{
		long long length = part.find_last_of('/', part.length() - 2);
		part = part.erase(0, length + 1);
	}

	ACK_Send(ACK_NEW_FILE, part.c_str(), part.length());

	Json::Value jsonFile;
	string file_path = path + part;

	fileCheck.on_ll(jsonFile, file_path, true);
	// printf("path2: %s \n", file_path.c_str());

	long long fileSize = (double)jsonFile[0]["fileSize"].asInt64();
	ACK_Send(ACK_DATA_SIZE, &fileSize, 8);

	int bufsize = 1024 * 24;					// 24KB
	if (fileSize > 1024 * 1024 * 800)
		bufsize = 1024 * 1024 * 240;			// 240MB
	else if (fileSize > 1024 * 1024 * 60)
		bufsize = 1024 * 1024 * 24;				// 24MB

	char* buf = new char[bufsize];

	/* 发送文件数据 */
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
		if (n > 1000000 * 5)
		{
			OS_Thread::Msleep(30);
		}
		else
		{
			// 必须加上几毫秒的延迟, 否则在局域网中, 会因为传输过快而导致接收数据出错;
			OS_Thread::Msleep(3);
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

// argv_count > 0 时, 为处理文件的前置操作;
void FileHandler::UploadHandler(string& path, char* argv[], int argv_count)
{
	// 计算命令行多余 "/"的数量;
	if (argv[0] != 0 && argv_count > 0)					// argv[0] != 0, argv 为客户端传输文件的参数; 
	{
		// 为后续重置 path参数使用, 如: get a/b/c => get c
		string cmdline = argv[argv_count];
		cmdline_count = std::count(cmdline.begin() + 1, cmdline.end() - 1, '/');
		
		// 不能使用'.'进行传输;
		long long length = cmdline.length() - 1;
		if (cmdline[length] == '.')
			throw("Cannot use \'.\' or \'..\' for transmission . \n");
		if (length > 0 && cmdline[length - 1] == '.' && cmdline[length] == '/')
			throw("Cannot use \'.\' or \'..\' for transmission . \n");

		return ;
	}


	/* 发送完子命令的后置处理 */
	if (directory_distances == 0)
	{
		ACK_Send(ACK_PATH_END);

		// 防止 put 命令上传出错;
		if (fileCheck.isClient)
			path.clear();
	}

	// 删除 path 多余的 "/", 以便下一个子命令通过, 如:  get a/b/c => get c
	for (int i = 0; i < cmdline_count && argv[0] != 0; i++)
	{
		long long dir_pos = path.find_last_of('/', path.length() - 2);
		path.erase(dir_pos + 1);
	}

	// 将 "/"的计数重置为 0, 为 下个子命令使用;
	if (cmdline_count > 0 && argv[0] != 0)
		cmdline_count = 0;
}

// 检查文件或目录 是否存在;
string FileHandler::CheckFile(string& path, char* data, unsigned int type)
{
	return fileCheck.CheckFile(path, data, type);
}

// 中文字符集转换, 用于外部函数封装调用;
string FileHandler::CharacterEncoding(char* src_str, int src_length)
{
	// convert 为 false 时, 则用于外部函数的调用;
	return fileCheck.CharacterEncoding(src_str, src_length, false);
}

void FileHandler::UploadFile(string path, char** argv, int argc, OS_Mutex& Mutex)
{
	// 单个子命令;
	string part;

	char* argv2[1024];						// 存放文件, 注意 argv2不可以小于文件的数量;
	Json::Value jresult;

	for (int i = 1; i < argc; i++)
	{
		// 上传文件之前的处理;
		UploadHandler(path, argv, i);

		// 获取文件;
		if (argv[0] != 0)
			part = fileCheck.CharacterEncoding(argv[i], strlen(argv[i]), true);
		else
			part = argv[i];					// 注意, 只有命令行的子命令才可以进行 字符串的转换;

	#ifdef _WIN32
		Mutex.Lock();
		
		// 判断是否为目录;
		bool isDir = fileCheck.IsDirectory(path, part);
		if (isDir)
			SendDirectory(jresult, path, part, argv2, Mutex);
		else							// 判断为文件处理;
			SendFile(path, part);
		
		// 上传文件错误, 本次传输结束;
		if (fileError.size() > 0)
			i = argc;

		// 上传文件之后的处理;
		UploadHandler(path, argv);

		Mutex.Unlock();
	#else
		// 保存用户的相对目录;
		m_path = path;
		Linux_SendFile(jresult, part, Mutex);
	#endif
	}
}

void FileHandler::DownloadFile(string path, OS_Mutex& Mutex)
{
	// 保存用户的相对目录;
	m_path = path;

	string part;
	int recv_length = 0;
	
	// 对端为 Windows系统 的附加处理;
	if (fileCheck.peer_system == 1)
		windows_transfer = true;

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
void FileHandler::Linux_PathHandler(Json::Value& jresult, string& path)
{
	// 清除 path末尾的 '/', 方便后续操作;
	long long length = path.length() - 1;
	if (path[length] == '/')
		path = path.erase(length, 1);


	// 客户端程序 调用 ls_linux2函数时, 需要 在子命令前 添加相对目录;
	if (!fileCheck.isClient)	// 客户端
	{
		if (path[0] == '/')
			path = path.erase(0, 1);
		path = m_path + path;
	}
	else			// 服务端
	{
		// 添加 path[0] 的首下标 '/';
		if (path[0] != '/')
			path.insert(0, "/");
	}

	bool linux_notDir = false;				// 文件处理;

	// 判断命令行的子命令是否为目录;
	// 如果为目录, 则 需要将相对目录处理, 为 SendFile函数传参使用;
	bool isDirectory = fileCheck.IsDirectory("", path);
	if (!isDirectory)
		linux_notDir = true;

	// 注意, 必须将 jresult 清空才可以存储文件信息;
	jresult.clear();
	ls_linux2(m_type, jresult, path);

	// 将上传或下载的根目录进行处理, 为 SendFile函数传参使用;
	length = path.find_last_of('/', length - 1);
	if (length > 0 && isDirectory)
		path = path.substr(0, length + 1);				// path = 相对目录 + 命令行上的路径;
	else
		path = m_path;							// path = 相对目录;
}

void FileHandler::Linux_SendFile(Json::Value& jresult, string& path, OS_Mutex& Mutex)
{
	// 处理发送文件的相对目录, 并对 jresult 进行文件加载;
	Linux_PathHandler(jresult, path);

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
			SendFile(path, part, linux_notDir);
		}
		
		Mutex.Unlock();
	}

	// 单个子命令发送完成;
	ACK_Send(ACK_PATH_END);
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
		
		// 不同系统之间的编码转换;
		part = fileCheck.CharacterEncoding((char*)part.c_str(), part.length(), true);

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
			// 必须关闭文件, 防止段错误;
			if (fp != NULL)	
			{
				fclose(fp);
				fp = NULL;
			}
			
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
		path = m_path;
	}

	if (m_type == ACK_DATA_FINISH)
	{
		fclose(fp);
		fp = NULL;
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