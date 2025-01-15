#include "./FtpClient.h"

FtpClient::FtpClient(OS_TcpSocket sock, int bufsize) : m_SendSock(sock), m_buffer(bufsize), isWindows(false)
{
	// 判断是否为 Windows系统;
#ifdef _WIN32
	isWindows = true;
#endif

	m_homeDir = ".";
	m_file(m_SendSock, m_homeDir, isWindows, true);

	// 注意, m_buffer 只能在FtpClient中构造; 
	// 否则会在 delete_scalar.cpp 中出现内存访问错误而触发断点 即越界写入错误, 导致堆内存被破坏;
}

void FtpClient::SendData(unsigned short type)
{
	SendData(type, "", "");
}

void FtpClient::SendData(unsigned short type, const char* data)
{
	SendData(type, data, "");
}

void FtpClient::SendData(unsigned short type, const char* username, const char* password)
{
	try {
		RequestServer(type, username, password);
	}
	catch (string e)
	{
		printf("%s \n", e.c_str());
	}
}


void FtpClient::in_get(string cmdline, char* argv[128], int argc)
{
	bool is_out = false;				// 判断是否有'-o' 输出命令;
	string homeDir;						// 存放输出文件的目录;

	// 查找命令行中 是否有'-o' 输出文件的命令;
	for (int i = 1; i < argc; i++)
	{
		string test(argv[i]);
		if (test == "-o" && i + 1 < argc)
		{
			homeDir = argv[i + 1];

			is_out = true;
			break;
		}
	}

	// 发送 '-o' 扩展命令;
	if (is_out)
	{
		// 获得 "-o"之前的命令;
		string cmdline2(cmdline);
		int find_pos = cmdline2.find("-o", 0);
		cmdline2.erase(find_pos);

		string path = "";										// 用于检查 get 命令;
		string check_data = "-o " + homeDir;					// 用于检查命令的数据;

		string result;
		try
		{
			// 必须先将 处理器的根目录置为空, 才能使用文件检查;
			m_file("");
			m_file.CheckFile(path, (char*)check_data.c_str(), MSG_GET);
			m_file(homeDir);					// 检查完将根目录重置回去;
		}
		catch (string& e)
		{
			printf("%s", e.c_str());
			m_file(".");
			return ;
		}
		
		m_cmdline = cmdline;
		SendData(MSG_GET, cmdline2.c_str());
	}
	else						// 发送普通命令;
	{
		m_cmdline = cmdline;
		SendData(MSG_GET, cmdline.c_str());
	}
}

void FtpClient::in_put(string cmdline, char* argv[128], int argc)
{
	string result = "put ";						// 发送命令行的结果;

	// 用于检查 put 命令;
	string path = "";
	try
	{
		// 必须先将 处理器的根目录置为空, 才能使用文件检查;
		m_file("");
		m_file.CheckFile(path, (char*)cmdline.c_str(), MSG_PUT);
	}
	catch (string& e)
	{
		printf("%s", e.c_str());
		return;
	}


	for (int i = 1; i < argc; i++)
	{	
		// 获取子命令;
		string cmd = argv[i];

		int cmd_length = cmd.length() - 1;
		if (cmd_length <= 0)
			return ;
		
		// 用于记录'/'的位置;
		long long ret = 0;

		// 判断命令行末尾是否为'/';
		if (cmd[cmd_length] == '/')
		{
			// 将末尾的'/' 排除掉;
			ret = cmd.find_last_of('/', cmd_length);
			while (ret > 0 && cmd[ret - 1] == '/')
			{
				ret -= 1;
			}
		}

		ret = cmd.find_last_of('/', ret - 1);
		result += cmd.substr(ret);
		result += " ";
	}
	
	m_cmdline = cmdline;
	SendData(MSG_PUT, result.c_str());
}

void FtpClient::Start()
{
	char cmdline[256];
	char cmdline2[256];

#ifdef _WIN32
	const char* root = "[root@localhost";
	const char* root2 = "]#  ";
#else
	const char* root = "<root@";
	const char* root2 = ">  ";
#endif

	while (true)
	{
		std::cout << root << m_path << root2;
		
	#ifdef _WIN32
		gets_s(cmdline);
	#else
		fgets(cmdline, 256, stdin);
		
		// 在Linux下, fgets的缓冲区 需要手动将末尾置为 0;
		int length = strlen(cmdline);
		cmdline[length - 1] = 0;
	#endif
		
		int cmdline_length = strlen(cmdline);
		if (cmdline_length == 0) continue;

		// 用于检查是否有错误命令;
		bool error_flag = false;

		// 将全部 '\' 转换成 '/' 统一处理;
		for (int i = 0; i < cmdline_length; i++)
		{
			if (cmdline[i] == '\\')
				cmdline[i] = '/';

			// 检查 "," 有关的错误命令, 如: cd ,,/,,
			if (cmdline[i] == ',')
				error_flag = true;
			
			// 在Linux下, 多个换行符的出现 会导致指令上传错误;
			if(cmdline[i] == '\n' && cmdline[i + 1] == '\n' && i + 1 < cmdline_length)
				cmdline[i] == ' ';
		}
		
	#ifdef _WIN32
		strcpy_s(cmdline2, cmdline);
	#else
		strcpy(cmdline2, cmdline);
	#endif
		
		char* argv[64];
		int argc = FileUtils::Split(cmdline, argv);
		
		string cmd = argv[0];
		if (cmd == "ls")
		{
			if (argc > 1)
				SendData(MSG_LIST, cmdline2);
			else
				SendData(MSG_LIST);

			continue;
		}
		else if (cmd == "ll")
		{
			if (argc > 1)
				SendData(MSG_LIST2, cmdline2);
			else
				SendData(MSG_LIST2);

			continue;

		}
		else if (cmd == "cd")
		{
			if (error_flag)
			{
				printf("input bad command ! \n");
				continue;
			}

			if (argc > 1)
				SendData(MSG_CD, cmdline2);
			else
				SendData(MSG_CD, cmdline);

			continue;
		}
		else if (cmd == "get")
		{
			if(argc > 1)
				in_get(cmdline2, argv, argc);

			continue;
		}
		else if (cmd == "put")
		{
			if (argc > 1)
				in_put(cmdline2, argv, argc);

			continue;
		}
		else if (cmd == "help")
		{
			SendData(MSG_HELP);
			continue;
		}
		else if (cmd == "exit")
		{
			SendData(MSG_EXIT);
			break;
		}
		else
		{
			printf("input cmdline error ! \n");
			continue;
		}

		OS_Thread::Msleep(300);
	}

	printf("Exit successfully . \n");
}

// 返回值: <=0 发送失败(socket可能已经断开), >0 发送成功;
// length: 数据字节的长度，可以为0;
int FtpClient::SendMessages(unsigned short type, const void* data, unsigned int length)
{
	// 发送消息类型
	// m_SendSock.Send(&type, 8);
	// m_SendSock.Send(&length, 8);

	// 发送消息类型
	m_buffer.putUnit16(type);
	m_buffer.putUnit16(isWindows);

	m_buffer.putUnit32(length);			// 大端传输
	m_SendSock.Send(m_buffer.Position(), 8, false);		// 先发送8个字节的数据, 用来指明身份信息的标头
	m_buffer.Clear();

	// 数据部分是0个字节则退出
	if (length <= 0) return 0;

	// 发送身份信息
	return m_SendSock.Send(data, length, false);
}

void FtpClient::ShowResponse(Json::Value& jsonResponse)
{
	int errorCode = jsonResponse["errorCode"].asInt();
	string reason = jsonResponse["reason"].asString();
	bool isWin = jsonResponse["isWindows"].asBool();
	string result = jsonResponse["result"].asString();
	
	char buf[1024 * 6] = {0};				// 接收应答的缓存;
	int response_length = 0;				// 接收应答的长度;
	string character;						// 编码转换的结果;

	// 显示应答结果;
	switch (errorCode)
	{
	case 11:			// 不同系统的 login, ls, ll
		// 不同系统之间使用二次接收数据;
		m_SendSock.Recv(&response_length, 4, false);				// 先接收4个字节的数据, 用来指明长度;
		if(response_length > 0)
			response_length = m_SendSock.Recv(buf, response_length, false);

		buf[response_length] = 0;					// 添加字符串的终止符

		// 不同系统之间的编码转换;
		if (isWindows != isWin)
		{
		#ifdef _WIN32
			character = Utf8ToGbk(buf);
		#else
			GbkToUtf8((char*)buf, response_length, (char*)character.c_str(), character.length());
		#endif
			throw string(character);
		}
		else
		{
			throw string(buf);
		}
		break;

	case 12:			// cd
		if (reason == "OK")
		{
			// 不同系统之间使用二次接收数据;
			m_SendSock.Recv(&response_length, 4, false);				// 先接收4个字节的数据, 用来指明长度;
			if (response_length > 0)
			{
				response_length = m_SendSock.Recv(buf, response_length, false);
				buf[response_length] = 0;					// 添加字符串的终止符
			}

			if (response_length > 0)					// 判断是否为目录;
			{
				// 不同系统之间的编码转换;
				if (isWindows != isWin)
				{
				#ifdef _WIN32
					character = Utf8ToGbk(buf);
				#else
					GbkToUtf8((char*)buf, response_length, (char*)character.c_str(), character.length());
				#endif
					m_path = " " + character;
				}
				else
				{
					m_path = " ";
					m_path += buf;
				}
			}
			else										 // 返回根目录;
			{
				m_path = "";
			}
		}
		else
		{
			throw string(result);
		}
		break;

	case 13:			// get
		if (reason == "OK")
		{
			OS_Mutex mutex;
			m_file.DownloadFile("/", mutex);
			
			m_file(string("."));						// 在Linux下, 不同函数之间的传参必须避免使用字符串(const char*);
		}
		else
		{
			throw string(result);
		}
		break;

	case 14:			// put
		if (reason == "OK")
		{
			char* argv[128] = {0};
			int argc = FileUtils::Split((char*)m_cmdline.c_str(), argv);

			OS_Mutex mutex;
			m_file("");
			m_file.UploadFile("",argv, argc, mutex);
			m_file.ACK_Send(ACK_FINISH);

			m_file(string("."));						// 在Linux下, 不同函数之间的传参必须避免使用字符串(const char*);
		}
		else
		{
			throw string(result);
		}
		break;

	case 0:				// login 或 ls
		throw string(result);
		break;

	default:
		throw string("reason: " + reason);
		throw string("result:" + result);
		break;
	}

}

int FtpClient::ServiceRequest(Json::Value& jsonResponse)
{
	int length = 0;
	int errorCode = jsonResponse["errorCode"].asInt();

	// 显示传输错误的信息;
	if (m_file.fileError.size() > 0)						// get
	{
		printf("\n%s", m_file.fileError.c_str());
		printf("Please perform the correct operation again as required ! \n\n");
		return 0;
	}

	// 成功响应服务;
	if (errorCode == 13 || errorCode == 14)							// get, put
	{
		// 先接收4个字节的数据, 用来指明长度;
		m_SendSock.Recv(&length, 4, false);
		if (length > 0)
		{
			char buf[256] = { 0 };
			m_SendSock.Recv(buf, length, false);			//接收数据
			printf("\n%s\n", buf);
		}
	}

	return 0;
}

int FtpClient::RequestServer(unsigned short type, const char* username, const char* password)
{
	// 构造JSON请求
	string jsonreq;
	if (strcmp(password, "") != 0)
	{
		Json::Value jsonRequest;
		Json::FastWriter writer;
		jsonRequest["username"] = username;
		jsonRequest["password"] = password;
		jsonreq = writer.write(jsonRequest);
	}
	else
	{
		jsonreq = username;
	}
	
	// 发送身份认证;
	SendMessages(type, jsonreq.c_str(), jsonreq.length());

	

	// 接收JSON请求的应答
	char buf[1024];					// 创建接收应答的缓存;
	int response_length = 0;

	// 先接收4个字节的数据, 用来指明长度;
	m_SendSock.Recv(&response_length, 4, false);
	response_length = m_SendSock.Recv(buf, response_length, false);			//接收数据
	if (response_length <= 0)
	{
		throw string("failed to recv response! login failed! \n");
	}

	buf[response_length] = 0;			// 添加字符串的终止符



	// 解析JSON请求的应答
	string response(buf, response_length);
	Json::Reader jsonReader;
	Json::Value jsonResponse;
	if (!jsonReader.parse(response, jsonResponse, false))
		throw string("bad json format! \n");

	// 显示和处理 服务端的所有响应;
	ShowResponse(jsonResponse);
	ServiceRequest(jsonResponse);

	return 0;
}
