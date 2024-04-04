#include "FtpService.h"

// 判断是否为 Windows系统;
#ifdef _WIN32
bool isWin = true;
#else
bool isWin = false;
#endif

FtpService::FtpService(OS_TcpSocket& recv_sock, int bufsize)
	: m_RecvSock(recv_sock), m_buffer(bufsize), m_bufsize(bufsize)
{
	m_data = 0;
	m_type = 0;
	m_length = 0;

	exit_OK = false;
	login_OK = false;

	m_homeDir = "E:/CProjects";			// ftp根目录
	m_path = "/";						// 起始目录
	
	/// m_buffer 尽量在CustomTcpRecv中构造; 
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

	m_homeDir = "E:/CProjects";			// ftp根目录
	m_path = "/";						// 起始目录
}

FtpService::~FtpService()
{
	Clear();
}


void FtpService::startReceiveData()
{
	ResponseClient();
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
	this->isWindows = m_buffer.getUnit16();

	this->m_length = m_buffer.getUnit32();			// JSON格式的大小;
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
	if (username != "root")
		throw string("bad username!");
	if (password != "123456")
		throw string("bad password!");

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

	// 不同系统之间的编码转换;
	if (isWindows != ::isWin)
	{
		#ifdef _WIN32
			this->username = Utf8ToGbk(username.c_str());
		#else
			string name;
			GbkToUtf8((char*)username.c_str(), username.length(), (char*)name.c_str(), name.length());
			this->username = name;
		#endif
	}

	if (username != "我ss1_")
		throw string("bad username!");
	if (password != "123456")
		throw string("bad password!");

	return 0;
}


// notDir 为单独文件的处理 (非目录文件);
// msg_list2 为false, 则此函数用于处理 on_ls消息请求; 为true则处理 on_ll消息请求;
#ifdef _WIN32
string FtpService::on_ls(Json::Value& jresult, bool notDir)
{	
	// 解析请求
	// printf("ls %s%s ...\n", m_homeDir.c_str(), m_path.c_str());
	jresult.clear();		// 防止重复显示

	// 判断消息
	bool msg_list2 = (m_type == MSG_LIST2) ? true : false;

	// 遍历文件信息
	string result;
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

	return result;
}

// notDir 为单独文件的处理 (非目录文件);
string FtpService::on_ll(Json::Value& jresult, bool notDir)
{
	string result;
	on_ls(jresult, notDir);
	result = FileUtils::List2_Result(jresult, result);

	return result;
}
#endif

string FtpService::on_cd(Json::Value& jresult)
{
	string result;
	string cmdline(m_data);

	char* argv[64];
	int argc = FileUtils::Split((char*) cmdline.c_str(), argv);
	if (argc > 1)
	{
		result = CheckFile();
		if (result.length() <= 0)
		{
			m_path += argv[1];				// 只用第一个子命令, 后续命令无效;

			int find_pos = 0;				// 查找 "."或 ".."的位置; 
			int count = 0;					// 查找到的 ".." 字符串数量;
			int argv[128] = {0};			// 用来存放 ".." 字符串的下标;
			bool isII = false;				// 用来确定命令行是否存在 ".."的值;


			// 将m_path 中的 "." 字符串删除;
			do
			{
				find_pos = m_path.find("/./", find_pos);
				
				if(find_pos >= 0)
					m_path.erase(find_pos, 2);
			} while (find_pos > 0);

			// 再将末尾的 "." 字符串删掉;
			find_pos = m_path.length();
			if(m_path[find_pos - 2] == '/' && m_path[find_pos - 1] == '.')
				m_path.erase(find_pos - 2, 2);

			// 查找 ".." 字符串的位置;
			do
			{
				// find_last_of 是从后往前查找属于字符串的字符, 成功找到则返回字符串的最后一个字符;
				find_pos = m_path.find_last_of("..", find_pos);
				if (find_pos < 0) break;

				isII = true;
				argv[count] = find_pos - 2;
				find_pos -= 3;
				count++;

			} while (true);
			
			// 判断二次命令时, 是否存在 ".." 字符串;
			int dir = m_path.find_last_of('/', m_path.length() - 2);
			if (dir == argv[0])
				dir = -1;

			// 获取二次命令中的目录名;
			string dir2;				// 二次命令的目录名;
			string dir3;				// 二次命令的最后一个目录名;
			if (count > 0)
			{
				if (dir > 0)
				{
					dir2 = m_path.substr(argv[0] + 3);				// 取得最后一个 ".."之后的字符串;

					// 获得dir2目录中的最后一个目录;
					int dir33 = dir2.find_last_of('/', dir2.length() - 2);
					dir3 = dir2.substr(dir33, dir2.length());
				}

				m_path = m_path.substr(0, argv[count - 1]);			// m_path 等于".."之前的字符串;
			}
			
			// 将 m_path 返回上一级的目录;
			while (count > 0)
			{
				int cd_pos = m_path.find_last_of('/', m_path.length());
				m_path.erase(cd_pos);

				count -= 1;
			}

			// 删除末尾的'/', 方便后续处理;
			int length = m_path.length();
			if (length > 0)
			{
				if(m_path[length - 1] == '/')
					m_path.erase(length - 1, 1);
			}

			// 将目录名加到 m_path里面, 并返回目录名;  
			if (dir > 0 && isII)
			{
				result = dir3;

				// 添加目录名;
				m_path += dir2;

				// 删除result末尾的'/';
				length = result.length();
				if (result[length - 1] == '/')
					result.erase(length - 1, 1);
			}
			else				// 命令行没有".." 字符串的处理;
			{
				result = m_path;

				// 返回最后一个目录名;
				int cd_pos = m_path.find_last_of('/', result.length());
				if(cd_pos >= 0)
					result = result.substr(cd_pos, result.length());
			}


			// 删除result句首的'/';
			if(result.size() > 0)
				result.erase(0, 1);

			m_path += '/';
		}
	}
	else
	{
		m_path = '/';
	}

	return result;
}


#ifdef _WIN32
string FtpService::CheckResult(Json::Value& jresult, string& path)
{
	string result;			// 返回检查的结果;

	// 将文件和目录分开显示;
	ifstream ifs(m_homeDir + m_path + path);
	if (ifs.is_open())		// 是否为文件
	{
		if (m_type != MSG_LIST2)
		{
			result = path;
			result.append("\t");
		}
		else
		{
			m_path += path;

			result = on_ll(jresult, true);
			m_path = "/";
		}
	}
	else				// 是否为目录
	{
		string directory;			// 单个目录信息;

		// 遍历目录的文件信息
		m_path += path += "/";
		if (m_type != MSG_LIST)
		{
			result += on_ll(jresult).append("\n");
		}
		else
		{
			on_ls(jresult);
			result = FileUtils::List_Result(jresult, directory);
		}

		m_path = "/";
	}

	return result;
}
#endif

// 检查文件或目录 是否存在, 若存在则返回相关的文件名称;
string FtpService::CheckFile()
{
	char* argv[64];					// 命令行的子命令;
	string result;					// 检查文件的结果;
	Json::Value jsonResult;			// 文件的详细信息;

	// 不同系统之间的编码转换;
	string cmdline = m_data;
	if (isWindows != ::isWin)
	{
		#ifdef _WIN32
			cmdline = Utf8ToGbk(cmdline.c_str());
		#else
			string cmdline2;
			GbkToUtf8((char*)cmdline.c_str(), cmdline.length(), (char*)cmdline2.c_str(), cmdline2.length());
			cmdline = cmdline2;
		#endif
	}

	// 获取命令行的子命令;
	int argc = FileUtils::Split((char*)cmdline.c_str(), argv);
	if (argc > 0)
	{
		// 防止用户跨越根目录进行越界访问;
		string part = argv[1];			// 获取子命令;
		part += '\0';

		if (m_type == MSG_CD && m_path != "/")
		{
			int cd_pos = 0;
			int cd_count = 0;

			do
			{
				cd_pos = part.find("..", cd_pos);
				if (cd_pos < 0) break;

				cd_pos += 2;
				cd_count++;
			} while (true);
			
			int path_count = std::count(m_path.begin(), m_path.end(), '/') - 1;
			
			if (cd_count > path_count)
			{
				result = "The access violation ! \n";
				throw result;
			}
		}
		else
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

		string path;		// 单个子命令;

		// 检查文件或目录
		for (int i = 1; i < argc; i++)
		{
			path = argv[i];

			// 判断文件或目录是否存在;
		#ifdef _WIN32
			int exist = _access_s((m_homeDir + m_path + path).c_str(), 0);
		#else
			int exist = access((m_homeDir + m_path + path).c_str(), F_OK);
		#endif

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

			ifstream ifs(m_homeDir + m_path + path);
			if(m_type == MSG_CD && ifs.is_open())
			{
				result = "The access violation ! \n";
				throw result;
			}
			
			// 返回文件或目录的结果;
		#ifdef _WIN32
			if(m_type == MSG_LIST || m_type == MSG_LIST2)
				result += CheckResult(jsonResult, path);
		#endif
		}
	}
	
	return result;			// 返回文件名称;
}

#ifndef _WIN32
string FtpService::LinuxHandler()
{
	string result;

	unsigned short pid = m_SockAddr.GetPort();
	string file = "outcmd" + to_string(pid) + ".txt";

	do
	{
		int exist = access(file.c_str(), F_OK);
		if (exist != 0) break;

		pid = rand() % pid;
		file = "outcmd" + to_string(pid) + ".txt";
	} while (true);
	

	string cmdline;
	switch (m_type)
	{
	case (MSG_LOGIN || MSG_LOGIN2):
		cmdline = "ls -F >> ";
		break;

	case MSG_LIST:
		if (m_length > 0)
		{
			cmdline = m_data;
			cmdline += " -F >> ";
		}
		else
		{
			cmdline = "ls -F >> ";
		}
		break;

	case MSG_LIST2:
		if (m_length > 0)
		{
			cmdline = m_data;
			cmdline += " >> ";
		}
		else
		{
			cmdline = "ls -l >> ";
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

	ifstream ifs;
	system(cmdline.c_str());				// 自动创建文件
	ifs.open(file.c_str());					// 只能打开文件, 无法创建文件;
	

	if (m_type == MSG_LOGIN || m_type == MSG_LOGIN2)
	{
		Json::Value fileblock;				// 只用于传递参数, 无实际用途;
		result = FileUtils::Login_Result(fileblock);
	}

	string s;
	int count = 0;
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
			result.append(s + "\n");
		}
		result.append("\n");
	}
	else
	{
		while (getline(ifs, s))
		{
			if (count % 5 == 0 && count != 0)
				result.append("\n");

			result.append(s + "   ");
			count++;
		}
	}

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
		try {
			on_Login(jsonreq);
			login_OK = true;

			m_RecvSock.GetPeerAddr(m_SockAddr);
			printf("\nlogin seccussfully (PID: %d). \n", m_SockAddr.GetPort());
			
		#ifdef _WIN32
			on_ls(fileblock);
			result = FileUtils::Login_Result(fileblock).append("\n");
		#else
			result = LinuxHandler();
		#endif
			
			if(isWindows != ::isWin || ::isWin)
				errorCode = 11;
		}
		catch (string e)
		{
			errorCode = -1;
			reason = e;
			m_RecvSock.GetPeerAddr(m_SockAddr);
			printf("\nLogin failed: %s (PID: %d) \n", e.c_str(), m_SockAddr.GetPort());
		}
		break;

	case MSG_LOGIN2:
		try {
			on_Login2();
			login_OK = true;

			m_RecvSock.GetPeerAddr(m_SockAddr);
			printf("\nlogin seccussfully (PID: %d). \n", m_SockAddr.GetPort());

		#ifdef _WIN32
			on_ls(fileblock);
			result = FileUtils::Login_Result(fileblock).append("\n");
		#else
			result = LinuxHandler();
		#endif

			if (isWindows != ::isWin || ::isWin)
				errorCode = 11;
		}
		catch (string e)
		{
			errorCode = -1;
			reason = e;
			m_RecvSock.GetPeerAddr(m_SockAddr);
			printf("\nLogin2 failed: %s (PID: %d) \n", e.c_str(), m_SockAddr.GetPort());
		}
		break;

	case MSG_LIST:
		if (!this->login_OK) break;

	#ifdef _WIN32
		try {
			if (m_length > 0)
			{
				result = CheckFile();
			}
			else
			{
				result = on_ls(fileblock);
			}

			if (isWindows != ::isWin || ::isWin)
				errorCode = 11;
		}
		catch (string e)
		{
			errorCode = -1;
			reason = e;
			printf("\ncmd - list failed: %s (PID: %d) \n", e.c_str(), m_SockAddr.GetPort());
		}
	#else
		result = LinuxHandler();
	#endif
		break;

	case MSG_LIST2:
		if (!this->login_OK) break;

	#ifdef _WIN32
		try {
			if (m_length > 0)
			{
				result = CheckFile();
			}
			else
			{
				result = on_ll(fileblock);
			}

			if (isWindows != ::isWin || ::isWin)
				errorCode = 11;
		}
		catch (string e)
		{
			errorCode = -1;
			reason = e;
			printf("\ncmd - list2 failed: %s (PID: %d) \n", e.c_str(), m_SockAddr.GetPort());
		}
	#else
		result = LinuxHandler();
	#endif
		break;

	case MSG_CD:
		if (!this->login_OK) break;

	#ifdef _WIN32
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
	#else
		result = LinuxHandler();
	#endif
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
		reason = "----Unknown Request !----";
		printf("\nError: Unknown Request ! (PID: %d) \n", m_SockAddr.GetPort());
		break;
	}

	return errorCode;
}

int FtpService::ResponseClient()
{
	// 按封包方式接收
	while (true && !this->exit_OK)
	{
		// 接收请求
		int msg_len = ReceiveMessages();
		if (msg_len < 0)
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
		response["isWindows"] = isWin;
		
		// 不同系统之间使用二次发送数据;
		if(errorCode != 11)
			response["result"] = result;
		else
			response["result"] = 0;
		std::string jsonresp = writer.write(response);

		// 发送数据时必须指明长度;
		int length = jsonresp.length();
		m_RecvSock.Send(&length, 4, false);			// 先发送4个字节的数据, 用来指明长度;
		m_RecvSock.Send(jsonresp.c_str(), jsonresp.length(), false);
		
		// 重新发送一次result, 因为 jsoncpp无法完全解析中文;
		if (errorCode == 11)
			m_RecvSock.Send(result.c_str(), result.length(), false);
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
