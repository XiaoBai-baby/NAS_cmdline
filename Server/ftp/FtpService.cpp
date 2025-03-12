#include "FtpService.h"

// 定义全局 互斥锁 变量, 用于保护文件的完整性;
OS_Mutex Mutex;

// 判断是否有 用户 在下载文件, 用于保护文件的完整性;
int is_get = 0;

FtpService::FtpService(OS_TcpSocket& recv_sock, int bufsize)
	: m_RecvSock(recv_sock), m_buffer(bufsize), m_bufsize(bufsize)
{
	m_data = 0;
	m_type = 0;
	m_length = 0;

	exit_OK = false;
	login_OK = false;
	
	// ftp根目录
#ifdef _WIN32
	m_homeDir = "E://CProjects";
	server_system = 1;					// Windows 系统;
#else
	m_homeDir = "/Win_share/C++服务器项目/CProjects";
	server_system = 0;					// Linux 系统;
#endif

	m_path = "/";							// 起始目录
	
	m_file(m_RecvSock, m_homeDir, server_system);

	CheckDirectory();

	/// m_buffer 尽量在FtpService中构造; 
	// 为了防止在 delete_scalar.cpp 中内出现存访问错误而触发断点 即越界写入错误, 导致堆内存被破坏;
}

FtpService::FtpService(OS_TcpSocket& recv_sock, char* buffer, int offset, int bufsize)
	: m_RecvSock(recv_sock), m_buffer(buffer, offset, bufsize), m_bufsize(bufsize)
{
	m_data = 0;
	m_type = 0;
	m_length = 0;
	
	exit_OK = false;
	login_OK = false;

	// ftp根目录
#ifdef _WIN32
	m_homeDir = "E://CProjects";
	server_system = 1;					// Windows 系统;
#else
	m_homeDir = "/Win_share/C++服务器项目/CProjects";
	server_system = 0;					// Linux 系统;
#endif

	m_path = "/";							// 起始目录
	
	m_file(m_RecvSock, m_homeDir, server_system);

	CheckDirectory();
}

FtpService::~FtpService()
{
	Clear();
}


void FtpService::startReceiveData()
{
	ResponseClient();
}

void FtpService::CheckDirectory()
{
#ifdef _WIN32
	int exist = _access_s(m_homeDir.c_str(), 0);
	if (exist != 0)
	{
		int n = _mkdir(m_homeDir.c_str());
		printf("Error: No ftp directory or file! \n");
	}
#else
	int exist = access(m_homeDir.c_str(), F_OK);
	if (exist != 0)
	{
		int n = mkdir(m_homeDir.c_str(), S_IRWXU);
		printf("Error: No ftp directory or file! \n");
	}
#endif
}

void FtpService::Clear()
{
	m_type = 0;
	m_length = 0;
}


// 返回值: >=0时，表示接收到的数据长度 (可以为0）
//	, <0时，表示接收出错
int FtpService::ReceiveMessages()
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

int FtpService::on_Login(const string& jsonreq)
{
	// 解析JSON请求
	Json::Reader reader;
	Json::Value req;
	if (!reader.parse(jsonreq, req, false))
		throw string("bad json format! \n");

	this->username = req["username"].asString();
	this->password = req["password"].asString();
	
	m_user.UserDetection(username, password);

	return 0;
}

int FtpService::on_Login2()
{
	char* argv[64];
	string user(m_data);
	int argc = FileUtils::Split((char*)user.c_str(), argv);
	
	if (argc < 2)
		return -1;

	this->username = argv[0];
	this->password = argv[1];
	
	m_user.UserDetection(username, password);

	return 0;
}


// notDir 为单独文件的处理 (非目录文件);
string FtpService::on_ls(Json::Value& jresult, bool notDir)
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
string FtpService::on_ll(Json::Value& jresult, bool notDir)
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

string FtpService::on_cd(Json::Value& jresult)
{
	string result;
	string cmdline(m_data);

	char* argv[64];
	int argc = FileUtils::Split((char*)cmdline.c_str(), argv);
	if (argc > 1)
	{
		result = m_file.CheckFile(m_path, m_data, m_type);
		if (result.length() <= 0)
		{
			// 不同系统之间的编码转换;
			if (server_system != client_system)
				cmdline = m_file.CharacterEncoding(argv[1], strlen(argv[1]));
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

			CheckCd();
			last_directory = HandleCd(directory, find_pos, count, position, &argv);

			HandlerDir(count, isII, &argv);
			result = HandleCdResult(directory, last_directory, position, isII);
		}
	}
	else
	{
		m_path = '/';
	}

	return result;
}

string FtpService::on_get()
{
	string result;
	string cmdline(m_data);

	char* argv[128];
	int argc = FileUtils::Split((char*)cmdline.c_str(), argv);
	if (argc > 1)
	{
		m_file.UploadFile(m_path, argv, argc, Mutex);
		m_file.ACK_Send(ACK_FINISH);
		
		if (m_file.fileError.size() > 0)
		{
			printf("%s", m_file.fileError.c_str());
		}
		else
		{
			// 打印下载的文件;
			printf("\nDownloaded the ");
			for (int i = 1; i < argc; i++)
			{
				// 删除命令行 "." "/" 字符;
				string file = argv[i];
				if (file[0] == '.' && file[1] == '/')
					file = file.erase(0, 2);
				
				// 删除末尾的 "/";
				int size = file.size() - 1;
				if(file[size] == '/')
					file = file.erase(size);
				
				// 删除 "/" 之前的字符;
				size = file.find_last_of('/', file.size() - 2);
				if(size >= 0)
					file = file.erase(size);
				
				if (server_system != client_system)
					file = m_file.CharacterEncoding((char*)file.c_str(), file.length());
				printf("'%s' ", file.c_str());
			}
			printf("file (PID: %d). \n", m_SockAddr.GetPort());
		}
	}
	is_get--;

	return result;
}

string FtpService::on_put()
{
	string result;
	string cmdline(m_data);

	char* argv[128];
	int argc = FileUtils::Split((char*)cmdline.c_str(), argv);
	if (argc > 1)
	{
		m_file.DownloadFile(m_path, m_Mutex);
		if (m_file.fileError.size() > 0)
		{
			printf("%s", m_file.fileError.c_str());
		}
		else
		{
			// 打印上传的文件;
			printf("\nUploaded the ");
			for (int i = 1; i < argc; i++)
			{
				// 删除命令行 "." "/" 字符;
				string file = argv[i];
				if (file[0] == '.' && file[1] == '/')
					file = file.erase(0, 2);

				// 删除末尾的 "/";
				int size = file.size() - 1;
				if(file[size] == '/')
					file = file.erase(size);
				
				// 删除 "/" 之前的字符;
				size = file.find_last_of('/', file.size() - 2);
				if(size >= 0)
					file = file.erase(size);

				if (server_system != client_system)
					file = m_file.CharacterEncoding((char*)file.c_str(), file.length());
				printf("'%s' ", file.c_str());
			}
			printf("file (PID: %d). \n", m_SockAddr.GetPort());
		}
	}
	is_get--;

	return result;
}

void FtpService::CheckCd()
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

string FtpService::HandleCd(string& directory, int& find_pos, int& count, int& position, int (*argv)[128])				// 注意数组指针的优先级：()>[]> *
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

int FtpService::Difference_mininum(int arr[], int count)
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

void FtpService::HandlerDir(int& count, bool& isII, int(*argv)[128])
{
	// 将 m_path 返回上一级的目录;
	if (count > 0)
	{
		// 单独存在 "/.."的处理;
		if (Difference_mininum(argv[0], count) != 3)					// 相邻两个 "/.."之间相差三位数;
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

string FtpService::HandleCdResult(string directory, string last_directory, int position, bool isII)
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
string FtpService::LinuxHandler()
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

int FtpService::MessageHandler(string& result, string& reason, Json::Value fileblock)
{
	int errorCode = 0;
	string jsonreq(m_data, m_length);

	switch (m_type)
	{
	case MSG_LOGIN:
		Mutex.Lock();
		try {
			on_Login(jsonreq);
			login_OK = true;
			m_file(client_system);

			m_RecvSock.GetPeerAddr(m_SockAddr);
			printf("\nlogin seccussfully (PID: %d). \n", m_SockAddr.GetPort());
			
		#ifdef _WIN32
			on_ls(fileblock);
			result = FileUtils::Login_Result(fileblock).append("\n");
		#else
			result = FileUtils::Login_Result(fileblock).append("\n");
			result += on_ls(fileblock);
		#endif
			
			if(client_system != server_system || server_system == 1)
				errorCode = 11;
		}
		catch (string e)
		{
			errorCode = -1;
			reason = e;
			// m_RecvSock.GetPeerAddr(m_SockAddr);
			// printf("\nLogin failed: %s (PID: %d) \n", e.c_str(), m_SockAddr.GetPort());
		}
		Mutex.Unlock();
		break;

	case MSG_LOGIN2:
		Mutex.Lock();
		try {
			on_Login2();
			login_OK = true;
			m_file(client_system);

			m_RecvSock.GetPeerAddr(m_SockAddr);
			printf("\nlogin seccussfully (PID: %d). \n", m_SockAddr.GetPort());

		#ifdef _WIN32
			on_ls(fileblock);
			result = FileUtils::Login_Result(fileblock).append("\n");
		#else
			result = FileUtils::Login_Result(fileblock).append("\n");
			result += on_ls(fileblock);
		#endif

			if (client_system != server_system || server_system == 1)
				errorCode = 11;
		}
		catch (string e)
		{
			errorCode = -1;
			reason = e;
			// m_RecvSock.GetPeerAddr(m_SockAddr);
			// printf("\nLogin2 failed: %s (PID: %d) \n", e.c_str(), m_SockAddr.GetPort());
		}
		Mutex.Unlock();
		break;

	case MSG_LIST:
		if (!this->login_OK) break;

		Mutex.Lock();
		try {
			if (m_length > 0)
			{
				// result = CheckFile();
				result = m_file.CheckFile(m_path, m_data, m_type);
			}
			else
			{
				result = on_ls(fileblock);
			}

			if (client_system != server_system || server_system == 1)
				errorCode = 11;
		}
		catch (string e)
		{
			errorCode = -1;
			reason = e;
			// printf("\ncmd - list failed: %s (PID: %d) \n", e.c_str(), m_SockAddr.GetPort());
		}
		Mutex.Unlock();
		break;

	case MSG_LIST2:
		if (!this->login_OK) break;

		Mutex.Lock();
		try {
			if (m_length > 0)
			{
				// result = CheckFile();
				result = m_file.CheckFile(m_path, m_data, m_type);
			}
			else
			{
				result = on_ll(fileblock);
			}

			if (client_system != server_system || server_system == 1)
				errorCode = 11;
		}
		catch (string e)
		{
			errorCode = -1;
			reason = e;
			// printf("\ncmd - list2 failed: %s (PID: %d) \n", e.c_str(), m_SockAddr.GetPort());
		}
		Mutex.Unlock();
		break;

	case MSG_CD:
		if (!this->login_OK) break;

		Mutex.Lock();
		try {
			result = on_cd(fileblock);
			errorCode = 12;
		}
		catch (string e)
		{
			errorCode = -1;
			reason = e;
			// printf("\ncmd - cd failed: %s (PID: %d) \n", e.c_str(), m_SockAddr.GetPort());
		}
		Mutex.Unlock();
		break;

	case MSG_GET:
		if (!this->login_OK) break;

		Mutex.Lock();
		try
		{
			m_file.CheckFile(m_path, m_data, m_type);
			is_get++;

			reason = "OK";
			errorCode = 13;
		}
		catch (string e)
		{
			errorCode = -1;
			reason = e;
		}
		Mutex.Unlock();
		break;

	case MSG_PUT:
		if (!this->login_OK) break;

		Mutex.Lock();
		try
		{
			m_file.CheckFile(m_path, m_data, m_type);
			is_get++;
			
			errorCode = -1;
			reason = "Unable to upload file, because the same files exist in the directory \n";
		}
		catch (string e)
		{
			errorCode = 14;
			reason = "OK";
		}
		Mutex.Unlock();
		break;

	case MSG_HELP:
		if (!this->login_OK) break;

		try {
			result = FileUtils::Help_Result();
		}
		catch (string e)
		{
			errorCode = -1;
			reason = e;
			printf("\ncmd - help failed: %s (PID: %d) \n", e.c_str(), m_SockAddr.GetPort());
		}
		break;

	case MSG_EXIT:
		if (!this->login_OK) break;
		exit_OK = true;

		break;
	
		

	default:
		errorCode = -1;
		reason = "----Unknown Request, Please login again !----";
		printf("\nError: Unknown Request ! (PID: %d) \n", m_SockAddr.GetPort());
		
		// 强制退出, 避免服务异常;
		exit_OK = true;
		break;
	}

	return errorCode;
}

int FtpService::ServiceHandler()
{
	if (m_type == MSG_GET)
		on_get();
	else if (m_type == MSG_PUT)
		on_put();
	else
		return 1;
	
	string fileError = m_file.fileError;
	if (fileError.size() <= 0)
		fileError = "\nSucceeded in File transfer . \n";
	else
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

int FtpService::ResponseClient()
{
	// 按封包方式接收
	while (true && !this->exit_OK)
	{
		// 接收请求
		int msg_len = ReceiveMessages();
		if (msg_len < 0 || m_type == MSG_EXIT)
		{
			break;
		}


		// 分析请求
		string result;
		string reason = "OK";
		Json::Value fileblock;
		int errorCode = MessageHandler(result, reason, fileblock);


		// 回复JSON请求
		Json::Value response;
		Json::FastWriter writer;
		response["errorCode"] = errorCode;
		response["reason"] = reason;
		response["system"] = server_system;
		
		// 不同系统之间使用二次发送数据;
		if(errorCode == 11 || errorCode == 12)
			response["result"] = 0;
		else
			response["result"] = result;
		std::string jsonresp = writer.write(response);

		// 发送数据时必须指明长度;
		int length = jsonresp.length();
		m_RecvSock.Send(&length, 4, false);						// 先发送4个字节的数据, 用来指明长度;
		m_RecvSock.Send(jsonresp.c_str(), jsonresp.length(), false);
		
		// 重新发送一次result, 因为 jsoncpp无法完全解析中文;
		if (errorCode == 11 || errorCode == 12)
		{
			length = result.length();
			m_RecvSock.Send(&length, 4, false);					// 先发送4个字节的数据, 用来指明长度;

			if(length > 0)
				m_RecvSock.Send(result.c_str(), result.length(), false);
		}

		// 开始进行服务;
		if (errorCode > 0)
			ServiceHandler();
	}

	Clear();

	return 0;
}


int FtpService::ReceiveN(void* buf, int count, int timeout)
{
	// 设置超时
	if (timeout > 0)
	{
		m_RecvSock.SetOpt_RecvTimeout(timeout);
	}

	// 接收数据过长的异常处理
	if (count > m_bufsize)
	{
		printf("ReceiveData2 function error: send data is too long ! (PID: %d) \n", m_SockAddr.GetPort());
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
