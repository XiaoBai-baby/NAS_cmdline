#include "nasServer.h"

// 定义全局 互斥锁 变量, 用于保护文件的完整性;
OS_Mutex Mutex;

// 定义离线服务对象;
offLineService m_offLineService;

nasServer::nasServer(OS_TcpSocket& recv_sock, int bufsize)
	: m_RecvSock(recv_sock), m_buffer(bufsize), m_bufsize(bufsize)
{
	m_data = 0;
	m_type = 0;
	m_length = 0;

	exit_OK = false;
	login_OK = false;
	
	// 设置 NAS 的根目录;
	m_homeDir = m_file.homeDir;
	printf("\n");

#ifdef _WIN32
	server_system = 1;							// Windows 系统;
	SetConsoleTitleA("NAS Cmdline Server");		// 更改窗口标题;

	// 打开 Windows 的 'mkdir'扩展命令;
	system("mkdir \\a\\b\\c\\d");
	m_cmdline.removeWindowsCharacter(50, 1);
#else
	server_system = 0;					// Linux 系统;
#endif

	m_path = "/";							// 起始目录
	
	m_offLineService(m_RecvSock, m_homeDir);
	m_file(m_RecvSock, m_homeDir, server_system);

	checkDirectory();

	/// m_buffer 尽量在nasServer中构造; 
	// 为了防止在 delete_scalar.cpp 中内出现存访问错误而触发断点 即越界写入错误, 导致堆内存被破坏;
}

nasServer::nasServer(OS_TcpSocket& recv_sock, char* buffer, int offset, int bufsize)
	: m_RecvSock(recv_sock), m_buffer(buffer, offset, bufsize), m_bufsize(bufsize)
{
	m_data = 0;
	m_type = 0;
	m_length = 0;
	
	exit_OK = false;
	login_OK = false;

	// 设置 NAS 的根目录;
	m_homeDir = m_file.homeDir;
	printf("\n");

#ifdef _WIN32
	server_system = 1;							// Windows 系统;
	SetConsoleTitleA("NAS Cmdline Server");		// 更改窗口标题;

	// 打开 Windows 的 'mkdir'扩展命令;
	system("mkdir \\a\\b\\c\\d");
	m_cmdline.removeWindowsCharacter(50, 1);
#else
	server_system = 0;					// Linux 系统;
#endif

	m_path = "/";							// 起始目录
	
	m_offLineService(m_RecvSock, m_homeDir);
	m_file(m_RecvSock, m_homeDir, server_system);

	checkDirectory();
}

nasServer::~nasServer()
{
	clear();
}

void nasServer::clear()
{
	m_type = 0;
	m_length = 0;
}

void nasServer::start()
{
	responseClient();
}

void nasServer::checkDirectory(string directory, int _exist)
{
	// 如果没有目录 或目录错误, 则设置默认目录;
	if (_exist != 0)
	{
	#ifdef _WIN32
		m_homeDir = "C://CProjects";
		int n = _mkdir(m_homeDir.c_str());
	#else
		m_homeDir = "/CProjects";
		int n = mkdir(m_homeDir.c_str(), S_IRWXU);
	#endif
		return;
	}

	/* 检查目录是否存在 */
	int exist = 0;
#ifdef _WIN32
	exist = _access_s(m_homeDir.c_str(), 0);
	if (exist != 0)
	{
		int n = _mkdir(m_homeDir.c_str());
	}
#else
	exist = access(m_homeDir.c_str(), F_OK);
	if (exist != 0)
	{
		int n = mkdir(m_homeDir.c_str(), S_IRWXU);
	}
#endif
	
	// 设置默认目录, 防止系统出错;
	if (exist != 0)
	{
		checkDirectory("NAS_cmdline", 1);
		printf("Warning: No homeDir directory has been set up. \n");
		printf("default values have been enabled: homeDir = %s. \n\n", m_homeDir.c_str());
	}
}


// 返回值: >=0时，表示接收到的数据长度 (可以为0）
//	, <0时，表示接收出错
int nasServer::receiveMessages()
{
	// 接收头部消息
	if (8 != ReceiveN(m_buffer.Position(), 8))
	{
		return -11;		// 接收消息出错;
	}

	// 指明头部消息
	this->m_type = m_buffer.getUnit16();
	this->client_system = m_buffer.getUnit8();

	char upgrade = m_buffer.getUnit8();							// 预留一个字节位置, 为未来的升级使用;
	this->m_length = m_buffer.getUnit32();						// JSON格式的大小;
	m_buffer.Clear();

	// 数据部分是0个字节则退出
	if (m_length <= 0)
		return 0;

	m_data = new char[m_length];

	// 接收数据部分
	int n = ReceiveN(m_data, m_length);
	if (n != m_length)
	{
		return -13;		// 接收数据部分出错;
	}

	m_data[m_length] = 0;		// 添加字符串的终止符

	return n;
}

void nasServer::printFile(int argc, char* argv[])
{
	for (int i = 1; i < argc; i++)
	{
		// 删除命令行 "." "/" 字符;
		string file = argv[i];
		if (file[0] == '.' && file[1] == '/')
			file = file.erase(0, 2);

		// 删除末尾的 "/";
		long long size = file.size() - 1;
		if (file[size] == '/')
			file = file.erase(size);

		// 删除 "/" 之前的字符;
		size = file.find_last_of('/', file.size() - 2);
		if (size >= 0)
			file = file.erase(0, size + 1);

		if (server_system != client_system)
			file = m_file.characterEncoding((char*)file.c_str(), file.length());
		printf("'%s' ", file.c_str());
	}
	printf("file ");
}

int nasServer::on_Login(const string& jsonreq)
{
	// 解析JSON请求
	Json::Reader reader;
	Json::Value req;
	if (!reader.parse(jsonreq, req, false))
		throw string("bad json format! \n");

	this->username = req["username"].asString();
	this->password = req["password"].asString();
	
	m_user.userDetection(username, password);

	return 0;
}

int nasServer::on_Login2()
{
	char* argv[64];
	string user(m_data);
	int argc = FileUtils::Split((char*)user.c_str(), argv);
	
	if (argc < 2)
		return -1;

	this->username = argv[0];
	this->password = argv[1];
	
	m_user.userDetection(username, password);

	return 0;
}


// notDir 为单独文件的处理 (非目录文件);
string nasServer::on_ls(Json::Value& jresult, bool notDir)
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
	FileEntryList files = FileUtils::List(m_homeDir + m_path, msg_list2, notDir);
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
	result = m_file.ls_linux(m_type, jresult, m_path);
#endif
	
	return result;
}

// notDir 为单独文件的处理 (非目录文件);
string nasServer::on_ll(Json::Value& jresult, bool notDir)
{
	string result;
	jresult.clear();			// 防止重复显示

#ifdef _WIN32
	on_ls(jresult, notDir);
	result = FileUtils::List2_Result(jresult, result);
#else
	result = m_file.ls_linux(m_type, jresult, m_path);
#endif

	return result;
}

string nasServer::on_cd(Json::Value& jresult)
{
	string result;
	string cmdline(m_data);

	char* argv[64];
	int argc = FileUtils::Split((char*)cmdline.c_str(), argv);
	if (argc > 1)
	{
		result = m_file.checkFile(m_path, m_data, m_type);
		if (result.length() <= 0)
		{
			// 不同系统之间的编码转换;
			if (server_system != client_system)
				cmdline = m_file.characterEncoding(argv[1], strlen(argv[1]));
			else
				cmdline = argv[1];				// 只用第一个子命令;
			
			// 在Linux下, 内存容易溢出 (即真实的内存为 m_path + cmdline + "\000\322/");
			// m_path += cmdline;
			
			m_path += cmdline.c_str();			// 只用第一个子命令, 后续命令无效;
			
			int find_pos = 0;					// 查找 "."或 ".."的位置; 
			int count = 0;						// 查找到的 "/.." 字符串数量;
			int argv[128] = {0};				// 用来存放 "/.." 字符串的下标;
			bool isII = false;					// 用来确定是否存在相邻两个 "/.."的值;
			
			int position;						// 二次命令是否存在 "/.." 字符串;
			string directory;					// 二次命令的目录名;
			string last_directory;				// 二次命令的最后一个目录名;

			checkCd();
			last_directory = handleCd(directory, find_pos, count, position, &argv);

			handlerDir(count, isII, &argv);
			result = handleCdResult(directory, last_directory, position, isII);
		}
	}
	else
	{
		m_path = '/';
	}

	return result;
}

// off_line 为 离线命令;
string nasServer::on_rm(bool off_line)
{
	string result;
	string cmdline;

	cmdline = m_data;

	char* argv[64];
	int argc = FileUtils::Split((char*)cmdline.c_str(), argv);
	if (argc < 2)
	{
		return result;
	}
	
	printf("Removed the ");
	for (int i = 1; i < argc; i++)
	{
		string file = on_pwd(argv[i]);
		string file_path = on_pwd(argv[i], true);

		// 将整个文件路径保存, 以便后续离线删除;
		if (off_line)
		{
			string complete_path = m_homeDir + m_path + file;
			m_offLineService.offLineRemove(complete_path);
			continue;
		}

	#ifdef _WIN32
		if (m_file.isDirectory(m_path, file))
			FileUtils::rmdir(file_path);
		else
			DeleteFileA(file_path.c_str());
	#else
		string cmdline2 = "rm -Rf ";
		cmdline2 += file_path;
		system(cmdline2.c_str());
	#endif
	}

	// 打印下载的文件;
	printf("file (PID: %d, %s). \n", m_SockAddr.GetPort(), FileUtils::AsTime().c_str());

	return result;
}

string nasServer::on_mkdir()
{
	string result;
	string cmdline(m_data);
	
	char* argv[128];
	int argc = FileUtils::Split((char*)cmdline.c_str(), argv);
	if (argc < 2)
	{
		return result;
	}

	printf("Created the ");
	for (int i = 1; i < argc; i++)
	{
		string directory = on_pwd(argv[i], true);
		int exist = m_offLineService.isExistFile(directory);
		
		// 如果目录存在, 则跳过;
		if (exist == 0)
			continue;

	#ifdef _WIN32
		directory = FileUtils::BackSlash(directory);
		directory.insert(0, "mkdir ");
	#else
		directory.insert(0, "mkdir -p ");
	#endif

		// 执行命令;
		system(directory.c_str());
	}

	// 打印下载的文件;
	printf("directory (PID: %d, %s). \n", m_SockAddr.GetPort(), FileUtils::AsTime().c_str());

	return result;
}

// off_line 为 离线命令;
string nasServer::on_mv(bool off_line)
{
	string result;
	string cmdline(m_data);

	char* argv[128];
	long long argc = FileUtils::Split((char*)cmdline.c_str(), argv);
	if (argc < 3)
	{
		return result;
	}
	
	string file;								// 单个文件;
	string destination_file;					// 目标文件;
	vector<string> print_file;					// 需要打印的文件;
	vector<string> total_file;					// 执行命令的总文件;
	long long end_len = argc - 1;				// 目标文件的位置;

	int exist = 0;
	for (int i = 1; i < argc; i++)
	{
		file = on_pwd(argv[i], false);

		// 获取目标文件;
		destination_file = m_file.destinationFile(print_file, file, m_path, m_type, argc, i);

		// 将文件添加到列表;
		m_file.add_fileList(total_file, print_file, m_path, file, end_len, i);
	}
	
	if (m_type == MSG_MOVE || m_type == MSG_MOVE2)
		printf("Moved the ");
	else if(m_type == MSG_COPY || m_type == MSG_COPY2)
		printf("Coped the ");

	// 打印文件;
	for (int j = 0; j < end_len - 1; j++)
	{
		printf("\'%s\'", print_file[j].c_str());

		if (j < end_len - 2)
			printf(",");
	}
	printf(" to \'%s\' directory or file ", print_file[end_len - 1].c_str());

	// 打印下载的文件;
	printf("(PID: %d, %s). \n", m_SockAddr.GetPort(), FileUtils::AsTime().c_str());

	// 移动文件;
	vector<string> off_line_service = m_file.move_copy_file(total_file, print_file, destination_file, m_type, off_line);

	if(m_type == MSG_MOVE || m_type == MSG_MOVE2)
		m_offLineService.offLineMove(off_line_service);
	else if(m_type == MSG_COPY || m_type == MSG_COPY2)
		m_offLineService.offLineCopy(off_line_service);
	return result;
}

// off_line 为 离线命令;
string nasServer::on_cp(bool off_line)
{
	return on_mv(off_line);
}

// all_print 为是否打印文件或目录名, 并返回全部路径;
// 当 m_type 非 MSG_PWD时, 则返回完整的工作路径, 为辅助 on_rm, on_mkdir, on_mv, on_cp 使用;
string nasServer::on_pwd(char* src_str, bool all_print)
{
	string cmdline;
	int src_length = 0;
	
	if (src_str != NULL)
	{
		src_length = strlen(src_str);
	}
	else
	{
		cmdline = m_path;
		src_str = (char*)cmdline.c_str();
	}

	// 不同系统之间的编码转换;
	// 注意, 为 string 赋值时, 必须将char* 转换成 const char*, 否则内存容易出错;
	if (server_system != client_system)
		cmdline = m_file.characterEncoding(src_str, src_length).c_str();
	else
		cmdline = string(src_str);

	// 删除 cmdline 末尾的"/";
	src_length = cmdline.length() - 1;
	if (cmdline[src_length] == '/' && src_length > 1)
		cmdline.erase(src_length, 1);

	string path;
	if (!all_print)
	{
		path = cmdline;
	}
	else
	{
		printf("\'%s\' ", cmdline.c_str());
		path = m_homeDir + m_path + cmdline;
	}

	return path;
}

string nasServer::on_get()
{
	string result;
	string cmdline(m_data);

	char* argv[128];
	int argc = FileUtils::Split((char*)cmdline.c_str(), argv);
	if (argc > 1)
	{
		m_file.uploadFile(m_path, argv, argc, Mutex);
		m_file.ACK_Send(ACK_FINISH);
		
		if (m_file.fileError.size() > 0)
		{
			printf("%s", m_file.fileError.c_str());
		}
		else
		{
			// 打印下载的文件;
			printf("\nDownloaded the ");
			printFile(argc, argv);
			printf("(PID: %d, %s). \n", m_SockAddr.GetPort(), FileUtils::AsTime().c_str());
		}
	}
	--m_offLineService;

	return result;
}

string nasServer::on_put()
{
	string result;
	string cmdline(m_data);

	char* argv[128];
	int argc = FileUtils::Split((char*)cmdline.c_str(), argv);
	if (argc > 1)
	{
		m_file.downloadFile(m_path, m_Mutex);
		if (m_file.fileError.size() > 0)
		{
			// 删除错误上传的文件;
			for (int i = 0; i < argc; i++)
				on_rm();
			printf("%s", m_file.fileError.c_str());
		}
		else
		{
			// 打印上传的文件;
			printf("\nUploaded the ");
			printFile(argc, argv);
			printf("(PID: %d, %s). \n", m_SockAddr.GetPort(), FileUtils::AsTime().c_str());
		}
	}
	--m_offLineService;

	return result;
}

void nasServer::checkCd()
{
	// 查找 "."或 ".."的位置; 
	long long find_pos = 0;							// long long 避免 算数溢出 C26451;

	// 将m_path 中的 "." 字符串删除;
	do
	{
		find_pos = m_path.find("/./", find_pos);

		if (find_pos >= 0)
			m_path.erase(find_pos, 2);
	} while (find_pos > 0);

	// 再将末尾的 "." 字符串删掉;
	find_pos = m_path.length();
	if (m_path[find_pos - 2] == '/' && m_path[find_pos - 1] == '.')
		m_path.erase(find_pos - 2, 2);
}

string nasServer::handleCd(string& directory, int& find_pos, int& count, int& position, int (*argv)[128])				// 注意数组指针的优先级：()>[]> *
{
	// 二次命令的最后一个目录名;
	string last_directory;

	// 查找 "/.." 字符串的位置;
	find_pos = m_path.length() - 1;				// 从后往前迭代的位置;
	do
	{
		// find_last_of 是从后往前查找属于字符串的任何一个字符;
		// find_pos = m_path.find_last_of("..", find_pos);						// 不支持字符串查找;

		//逆向查找, 返回第一次出现子字串的首字母下标
		find_pos = m_path.rfind("..", find_pos);
		if (find_pos < 0) break;
		
		argv[0][count] = find_pos - 1;					// 行指针也是一维数组, 如: argv[0][...]
		find_pos -= 2;
		count++;

	} while (true);

	// 判断二次命令时, 是否存在 "/.." 字符串;
	position = m_path.find_last_of('/', m_path.length() - 2);
	if (position == *argv[0])
		position = -1;

	// 获取二次命令中的目录名;
	if (count > 0)
	{
		if (position > 0)
		{
			// 在Linux下, 无法使用long类型进行强制转换;
			// long long substr_len = long long(*argv[0]) + 3;
			
			// 取得最后一个 "/../"之后的字符串;
			long long substr_len = *argv[0] + 3;
			directory = m_path.substr(substr_len);

			// 删除directory末尾的'/';
			long long length = directory.length();						// long long 避免 算数溢出 C26451;
			if (directory[length - 1] == '/')
				directory.erase(length - 1, 1);


			// 获得dir2目录中的最后一个目录;
			int dir_pos = directory.find_last_of('/', directory.length() - 2);
			last_directory = directory.substr(dir_pos, directory.length());
		}
	}
	
	return last_directory;
}

int nasServer::difference_mininum(int arr[], int count)
{
	// sort(arr, arr + count, [](int a, int b) {return a > b; });			// 降序排列
	int min = 100;
	for (int i = 1; i < count; i++)
	{
		int value = arr[i - 1] - arr[i];
		if (min > value)
			min = value;
	}

	return min;
}

void nasServer::handlerDir(int& count, bool& isII, int(*argv)[128])
{
	// 将 m_path 返回上一级的目录;
	if (count > 0)
	{
		// 单独存在 "/.."的处理;
		if (difference_mininum(argv[0], count) != 3)					// 相邻两个 "/.."之间相差三位数;
		{
			int argv_count = 0;
			while (count > 0)
			{
				long long dir_pos = argv[0][argv_count];							// long long 避免 算数溢出 C26451;
				int cd_pos = m_path.find_last_of('/', dir_pos - 1);
				if (cd_pos >= 0)
					m_path.erase(cd_pos, dir_pos - cd_pos + 3);

				argv_count++;
				count -= 1;
			}
		}
		else			// 存在相邻两个 "/.."的特殊处理;
		{
			// m_path 等于"/.."之前的字符串;
			m_path = m_path.substr(0, argv[0][count - 1]);
			isII = true;

			while (count > 0)
			{
				int cd_pos = m_path.find_last_of('/', m_path.length());
				m_path.erase(cd_pos);

				count -= 1;
			}
		}
	}
}

string nasServer::handleCdResult(string directory, string last_directory, int position, bool isII)
{
	string result;

	// 删除末尾的'/', 方便后续处理;
	long long length = m_path.length();						// long long 避免 算数溢出 C26451;
	if (length > 0)
	{
		if (m_path[length - 1] == '/')
			m_path.erase(length - 1, 1);
	}

	// 将目录名加到 m_path里面, 并返回目录名;  
	if (position > 0 && isII)
	{
		result = last_directory;

		// 添加目录名;
		m_path += directory;

		// 删除result末尾的'/';
		length = result.length();
		if (result[length - 1] == '/')
			result.erase(length - 1, 1);
	}
	else				// 命令行没有两个相邻 "/.." 字符串的处理;
	{
		result = m_path;

		// 返回最后一个目录名;
		int cd_pos = m_path.find_last_of('/', result.length());
		if (cd_pos >= 0)
			result = result.substr(cd_pos, result.length());
	}

	// 删除result句首的'/';
	if (result.size() > 0)
		result.erase(0, 1);
	
	m_path += '/';
	
	return result;
}

#ifndef _WIN32
string nasServer::linux_Handler()
{
	string result;

	unsigned short pid = m_SockAddr.GetPort();
	string file = "outcmd" + std::to_string(pid) + ".txt";

	do
	{
		int exist = access(file.c_str(), F_OK);
		if (exist != 0) break;
		
		file = "outcmd" + std::to_string(rand() % pid) + ".txt";
	} while (true);

	string cfile = "touch ";
	cfile += file;
	system(cfile.c_str());

	string cmdline;
	switch (m_type)
	{
	case MSG_LOGIN:
		cmdline = "ls ";
		cmdline += m_homeDir + m_path;
		cmdline += " -F >> ";
		break;
		
	case MSG_LOGIN2:
		cmdline = "ls ";
		cmdline += m_homeDir + m_path;
		cmdline += " -F >> ";
		break;

	case MSG_LIST:
		if (m_length > 0)
		{
			char* argv[64];
			int argc = FileUtils::Split(m_data, argv);
			
			cmdline += argv[0];
			cmdline += " ";
			
			for(int i = 1; i < argc; i++)
			{
				cmdline += m_homeDir + m_path + argv[i] + " ";
			}
			
			cmdline += " >> ";
		}
		else
		{
			cmdline = "ls ";
			cmdline += m_homeDir + m_path;
			cmdline += " -F >> ";
		}
		break;

	case MSG_LIST2:
		if (m_length > 0)
		{
			char* argv[64];
			int argc = FileUtils::Split(m_data, argv);
			
			cmdline += "ls -l ";
			
			for(int i = 1; i < argc; i++)
			{
				cmdline += m_homeDir + m_path + argv[i] + " ";
			}
			
			cmdline += " >> ";
		}
		else
		{
			cmdline = "ls -l ";
			cmdline += m_homeDir + m_path;
			cmdline += " >> ";
		}
		break;

	case MSG_CD:
		if (m_length > 0)
		{
			cmdline = m_data;
			cmdline += " -L >> ";
		}
		else
		{
			cmdline = "cd -L >> ";
		}
		break;

	default:
		break;
	}

	cmdline += file;
	
	std::ifstream ifs;
	system(cmdline.c_str());				// 自动创建文件
	ifs.open(file.c_str());					// 只能打开文件, 无法创建文件;
	

	if (m_type == MSG_LOGIN || m_type == MSG_LOGIN2)
	{
		Json::Value fileblock;				// 只用于传递参数, 无实际用途;
		result = FileUtils::Login_Result(fileblock);
	}

	string s;
	if (m_type == MSG_CD)
	{
		getline(ifs, s);

		// 返回最后一个目录名;
		int cd_pos = s.find_last_of('/', s.length() - 2);
		if (cd_pos >= 0)
			result = s.substr(cd_pos + 1, s.length());
	}
	else if (m_type == MSG_LIST2)
	{
		while (getline(ifs, s))
		{
			// 返回最后一个目录名;
			int cd_pos = s.find_last_of('/', s.length() - 2);
			if (cd_pos >= 0)
				s = s.substr(cd_pos + 1, s.length());
			
			result.append(s + "\n");
		}
	}
	else
	{
		int number = 0;
		int count = 0;
		while (getline(ifs, s))
		{
			// 返回最后一个目录名;
			int cd_pos = s.find_last_of('/', s.length() - 2);
			if (cd_pos >= 0)
			{
				s = s.substr(cd_pos + 1, s.length());
				number++;
				
				if(number <= 1)
					result.append(s + "\n");
				else
					result.append("\n\n" + s + "\n");
				
				count = 0;
			}
			else
			{
				if (count % 5 == 0 && count != 0)
					result.append("\n");
				
				result.append(s + "   ");
				count++;
			}
		}
	}
	
	if(m_type != MSG_LIST2)
		result.append("\n");
	ifs.close();

	cmdline = "rm -f " + file;
	system(cmdline.c_str());

	return result;
}
#endif

int nasServer::messageHandler(string& result, string& reason, Json::Value fileblock)
{
	int code = JSON_ERROR;					// JSON 响应;
	bool off_line = false;					// 离线删除;
	string jsonreq(m_data, m_length);

	switch (m_type)
	{
	case MSG_LOGIN:
	case MSG_LOGIN2:
		Mutex.Lock();
		try {
			// on_Login(jsonreq);			MSG_LOGIN
			on_Login2();
			login_OK = true;
			m_file(client_system);

			m_RecvSock.GetPeerAddr(m_SockAddr);
			printf("login seccussfully (PID: %d, %s). \n", m_SockAddr.GetPort(), FileUtils::AsTime().c_str());

		#ifdef _WIN32
			on_ls(fileblock);
			FileUtils::List_Result(fileblock, result);
		#else
			FileUtils::List_Result(fileblock, result);
			result += on_ls(fileblock);
		#endif

			code = JSON_LOGIN_AND_LIST;
		}
		catch (string e)
		{
			code = JSON_ERROR;
			reason = e;
		}
		Mutex.Unlock();
		break;

	case MSG_LIST:
		if (!this->login_OK) break;

		Mutex.Lock();
		try {
			if (m_length > 0)
			{
				// result = checkFile();
				result = m_file.checkFile(m_path, m_data, m_type);
			}
			else
			{
				result = on_ls(fileblock);
			}

			code = JSON_LOGIN_AND_LIST;
		}
		catch (string e)
		{
			code = JSON_ERROR;
			reason = e;
		}
		Mutex.Unlock();
		break;

	case MSG_LIST2:
		if (!this->login_OK) break;

		Mutex.Lock();
		try {
			if (m_length > 0)
			{
				// result = checkFile();
				result = m_file.checkFile(m_path, m_data, m_type);
			}
			else
			{
				result = on_ll(fileblock);
			}

			code = JSON_LOGIN_AND_LIST;
		}
		catch (string e)
		{
			code = JSON_ERROR;
			reason = e;
		}
		Mutex.Unlock();
		break;

	case MSG_CD:
		if (!this->login_OK) break;

		Mutex.Lock();
		try {
			result = on_cd(fileblock);
			code = JSON_CD;
		}
		catch (string e)
		{
			code = JSON_ERROR;
			reason = e;
		}
		Mutex.Unlock();
		break;

	case MSG_REMOVE:
		if (!this->login_OK) break;
		
		Mutex.Lock();
		try {
			if (m_length > 0)
				result = m_file.checkFile(m_path, m_data, m_type);
			
			code = JSON_REMOVE;
			
			if (m_offLineService.user_size() == 0)
			{
				on_rm(false);
			}
			else if (m_offLineService.user_size() > 0 && m_offLineService.user_size() < 3)
			{
				on_rm(true);
			}
			else if (m_offLineService.user_size() > 3)
			{
				off_line = true;
				string e = "too many users are downloading files, ";
				result = e;					// 发送客户端的数据;
				throw(e);
			}
		}
		catch (string e)
		{
			if (!off_line)
				code = JSON_ERROR;				// 文件存在错误;
			else
				code = JSON_REMOVE2;			// 离线删除服务;
			reason = e;
		}
		Mutex.Unlock();
		break;

	case MSG_REMOVE2:
		if (!this->login_OK) break;

		Mutex.Lock();
		try {
			if (m_length > 0)
				result = m_file.checkFile(m_path, m_data, m_type);

			on_rm(true);
			code = JSON_REMOVE;
		}
		catch (string e)
		{
			code = JSON_ERROR;
			reason = e;
		}
		Mutex.Unlock();
		break;

	case MSG_MAKEDIR:
		if (!this->login_OK) break;

		Mutex.Lock();
		try {
				m_file.checkFile(m_path, m_data, m_type);
				on_mkdir();
		}
		catch (string e)
		{
			code = JSON_ERROR;
			reason = e;
		}
		Mutex.Unlock();
		break;

	case MSG_MOVE:
		if (!this->login_OK) break;

		Mutex.Lock();

		code = JSON_MOVE;

		try {
			m_file.checkFile(m_path, m_data, m_type);

			if (m_offLineService.user_size() == 0)
			{
				on_mv(false);
			}
			else if (m_offLineService.user_size() > 0 && m_offLineService.user_size() < 3)
			{
				on_mv(true);
			}
			else if (m_offLineService.user_size() > 3)
			{
				off_line = true;
				string e = "too many users are downloading files, ";
				result = e;					// 发送客户端的数据;
				throw(e);
			}
		}
		catch (string e)
		{
			if (!off_line)
				code = JSON_ERROR;				// 文件存在错误;
			else
				code = JSON_MOVE2;				// 离线删除服务;
			reason = e;
		}
		Mutex.Unlock();
		break;

	case MSG_MOVE2:
		if (!this->login_OK) break;

		Mutex.Lock();
		try {
			if (m_length > 0)
				result = m_file.checkFile(m_path, m_data, m_type);

			on_mv(true);
			code = JSON_MOVE;
		}
		catch (string e)
		{
			code = JSON_ERROR;
			reason = e;
		}
		Mutex.Unlock();
		break;

	case MSG_COPY:
		if (!this->login_OK) break;

		Mutex.Lock();

		code = JSON_COPY;

		try {
			m_file.checkFile(m_path, m_data, m_type);

			if (m_offLineService.user_size() == 0)
			{
				on_cp(false);
			}
			else if (m_offLineService.user_size() > 0 && m_offLineService.user_size() < 3)
			{
				on_cp(true);
			}
			else if (m_offLineService.user_size() > 3)
			{
				off_line = true;
				string e = "too many users are downloading files, ";
				result = e;					// 发送客户端的数据;
				throw(e);
			}
		}
		catch (string e)
		{
			if (!off_line)
				code = JSON_ERROR;				// 文件存在错误;
			else
				code = JSON_COPY2;				// 离线删除服务;
			reason = e;
		}
		Mutex.Unlock();
		break;

	case MSG_COPY2:
		if (!this->login_OK) break;

		Mutex.Lock();
		try {
			if (m_length > 0)
				result = m_file.checkFile(m_path, m_data, m_type);

			on_cp(true);
			code = JSON_COPY;
		}
		catch (string e)
		{
			code = JSON_ERROR;
			reason = e;
		}
		Mutex.Unlock();
		break;

	case MSG_GET:
		if (!this->login_OK) break;

		Mutex.Lock();
		try
		{
			m_file.checkFile(m_path, m_data, m_type);
			++m_offLineService;

			reason = "OK";
			code = JSON_GET;
		}
		catch (string e)
		{
			code = JSON_ERROR;
			reason = e;
		}
		Mutex.Unlock();
		break;

	case MSG_PUT:
		if (!this->login_OK) break;

		Mutex.Lock();
		try
		{
			m_file.checkFile(m_path, m_data, m_type);
			++m_offLineService;

			code = JSON_ERROR;
			reason = "Unable to upload file, because the same files exist in the directory \n";
		}
		catch (string e)
		{
			code = JSON_PUT;
			reason = "OK";
		}
		Mutex.Unlock();
		break;

	case MSG_PWD:
		if (!this->login_OK) break;
		code = JSON_PWD;
		result = on_pwd();
		reason = "OK";
		break;

	case MSG_EXIT:
		if (!this->login_OK) break;
		exit_OK = true;
		break;
		

	default:
		code = JSON_ERROR;
		reason = "----Unknown Request, Please login again !----";
		printf("\nError: Unknown Request ! (PID: %d, %s) \n", m_SockAddr.GetPort(), FileUtils::AsTime().c_str());
		
		// 强制退出, 避免服务异常;
		exit_OK = true;
		break;
	}

	return code;
}

int nasServer::serviceHandler()
{
	// 处理离线服务;
	Mutex.Lock();
	m_offLineService.offLineHandler();
	Mutex.Unlock();

	if (m_type == MSG_GET)
		on_get();
	else if (m_type == MSG_PUT)
		on_put();
	else if (m_type == MSG_REMOVE) {}
	else if (m_type == MSG_MOVE) {}
	else if(m_type == MSG_COPY) {}
	else
		return 1;

	// 利用在线客户端的线程服务, 处理离线服务;
	Mutex.Lock();
	m_offLineService.offLineHandler();
	Mutex.Unlock();
	
	string fileError = m_file.fileError;
	if (fileError.size() <= 0 && m_type >= MSG_GET)
		fileError = "\nSucceeded in File transfer . \n";
	else if(m_type >= MSG_GET)				// 上传和下载错误;
		fileError += "This problem can be solved by reducing the number of transferred files or file parameters ! \n\n";
	
	// 将传输错误的信息回复客户端;
	int error_size = fileError.size();
	if (fileError.size() > 0)
	{
		// 发送数据时必须指明长度;
		m_RecvSock.Send(&error_size, 4, false);						// 先发送4个字节的数据, 用来指明长度;
		m_RecvSock.Send(fileError.c_str(), error_size, false);
	}

	return 0;
}

int nasServer::responseHandler(string& result, Json::Value& response)
{
	Json::FastWriter writer;

	// 不同系统之间使用二次发送数据, 因为 jsoncpp 的格式只支持 UTF-8, 不支持 GBK;
	if (result.length() > 0)
		response["result"] = 0;
	else
		response["result"] = result;
	std::string jsonresp = writer.write(response);

	// 发送数据时必须指明长度;
	int length = jsonresp.length();
	m_RecvSock.Send(&length, 4, false);							// 先发送4个字节的数据, 用来指明长度;
	m_RecvSock.Send(jsonresp.c_str(), jsonresp.length(), false);

	// 重新发送一次result, 因为 jsoncpp 的格式只支持 UTF-8, 不支持 GBK;
	if (result.length() > 0)
	{
		length = result.length();

		// 先发送4个字节的数据, 用来指明长度;
		m_RecvSock.Send(&length, 4, false);
		m_RecvSock.Send(result.c_str(), result.length(), false);
	}

	return 0;
}

int nasServer::responseClient()
{
	// 按封包方式接收
	while (true && !this->exit_OK)
	{
		// 接收请求
		int msg_len = receiveMessages();
		if (msg_len < 0 || m_type == MSG_EXIT)
		{
			break;
		}


		// 分析请求
		string result;
		string reason = "OK";
		Json::Value fileblock;
		int code = messageHandler(result, reason, fileblock);


		// 回复JSON请求
		Json::Value response;
		response["code"] = code;
		response["reason"] = reason;
		response["system"] = server_system;
		response["again"] = result.length() > 0 ? true : false;
		response["maxUploadUnit"] = (int) m_file.maxUploadUnit;
		response["maxUploadSize"] = (long long) m_file.maxUploadSize;
		response["maxCmdlineParameters"] = m_file.maxCmdlineParameters;

		responseHandler(result, response);

		// 开始进行服务;
		if (code != JSON_ERROR || m_offLineService.user_size() > 0)
			serviceHandler();
	}

	clear();

	return 0;
}


int nasServer::ReceiveN(void* buf, int count, int timeout)
{
	// 设置超时
	if (timeout > 0)
	{
		m_RecvSock.SetOpt_RecvTimeout(timeout);
	}

	// 接收数据过长的异常处理
	if (count > m_bufsize)
	{
		printf("ReceiveData2 function error: send data is too long ! (PID: %d, %s) \n", m_SockAddr.GetPort(), FileUtils::AsTime().c_str());
		return -12;
	}

	// 反复接收数据, 直到接满指定的字节数;
	int bytes_got = 0;
	while (bytes_got < count)
	{
		m_Mutex.Lock();
		int n = m_RecvSock.Recv((char*)buf + bytes_got, count - bytes_got, false);
		m_Mutex.Unlock();
		if (n <= 0)
		{
			continue;
		}

		bytes_got += n;
	}

	return bytes_got;	// 返回接收数据的大小;
}
