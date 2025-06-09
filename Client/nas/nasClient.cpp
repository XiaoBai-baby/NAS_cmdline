#include "./nasClient.h"

nasClient::nasClient(OS_TcpSocket sock, int bufsize) : m_SendSock(sock), m_buffer(bufsize), client_system(0)
{
	// 判断是否为 Windows系统;
#ifdef _WIN32
	client_system = 1;
#endif

	is_login = false;

	m_homeDir = ".";
	m_file(m_SendSock, m_homeDir, client_system, true);

	// 注意, m_buffer 只能在nasClient中构造; 
	// 否则会在 delete_scalar.cpp 中出现内存访问错误而触发断点 即越界写入错误, 导致堆内存被破坏;
}

void nasClient::sendData(unsigned short type)
{
	sendData(type, "", "");
}

void nasClient::sendData(unsigned short type, const char* data)
{
	sendData(type, data, "");
}

void nasClient::sendData(unsigned short type, const char* username, const char* password)
{
	try {
		requestServer(type, username, password);
	}
	catch (string e)
	{
		printf("%s \n", e.c_str());

		// 这里用看命令行报错的时间, 迅速将服务端冗余的数据接收掉, 以防止下次命令出错;
		// 例如: ll 目录 -> 输出: bad json format! -> ls 目录 -> 输出上一次命令的结果;
		int error_length = e.find("Error: ");
		if (error_length < 0)
			error_length = e.find("bad json format!");

		if (error_length >= 0)
		{
			char buf[1024] = { 0 };
			int buf_length = 0;
			m_SendSock.SetOpt_RecvTimeout(1000);
			do
			{
				buf_length = m_SendSock.Recv(buf, 1024);
			} while (buf_length > 0);
			m_SendSock.SetOpt_RecvTimeout(0);
		}
	}
}


int nasClient::getCheck(string homeDir, char* argv[128], int out_argc)
{
	// 用于检查 get 命令;
	string path = "";
	string check_data = "-o " + homeDir;			// 用于检查命令的数据;
	
	// 检查输出的目录是否存在;
	try
	{
		// 必须先将 处理器的根目录置为空, 才能使用文件检查;
		m_file("");
		m_file.checkFile(path, (char*)check_data.c_str(), MSG_GET);
		m_file(homeDir);					// 检查完将根目录重置回去;
	}
	catch (string& e)
	{
		printf("reason: output directory does %s \n", e.c_str());
		m_file(".");
		return -1;
	}
	
	
	// homeDir 末尾添加 '/';
	long long length = homeDir.length();
	if(homeDir[length - 1] != '/')
		homeDir += "/";
	
	// 检查下载的文件是否重名;
	for(int i = 1; i < out_argc; i++)
	{
		// 删除子命令 "/", 例如: 文件 FtpTest/aaa => 文件 aaa;
		string file = argv[i];
		length = file.find_last_of('/', file.length() - 2);
		if (length >= 0)
			file = file.erase(0, length + 1);

		check_data = "-o " + homeDir + file;
		try
		{
			// 必须先将 处理器的根目录置为空, 才能使用文件检查;
			m_file("");
			m_file.checkFile(path, (char*)check_data.c_str(), MSG_GET);
			m_file(homeDir);					// 检查完将根目录重置回去;
			
			printf("reason: Unable to download file, because the same files exist in the local directory \n\n");
			return -1;
		}
		catch (string& e)
		{
			m_file(homeDir);					// 检查完将根目录重置回去;
		}
	}
	
	return 0;
}

void nasClient::in_get(string cmdline, char* argv[128], int argc)
{
	int out_argc = 0;					// 存放'-o' 的位置;
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
			out_argc = i;
			break;
		}
		
		// 排除 "..", "." 符号;
		int length = test.find("..");
		if(length < 0)
			length = test.find("/.");

		if (length >= 0 || test == ".")
		{
			printf("reason: Don't use '.' or '..' symbol downloads the file, it is not allowed . \n\n");
			return;
		}
	}

	// 发送 '-o' 扩展命令;
	if (is_out)
	{
		// 获得 "-o"之前的命令;
		string cmdline2(cmdline);
		int find_pos = cmdline2.find("-o", 0);
		cmdline2.erase(find_pos);
		
		if(getCheck(homeDir, argv, out_argc) < 0)
			return ;
		
		m_cmdline = cmdline;
		sendData(MSG_GET, cmdline2.c_str());
	}
	else						// 发送普通命令;
	{
		homeDir = "./";
		if(getCheck(homeDir, argv, argc) < 0)
			return ;
		
		m_cmdline = cmdline;
		sendData(MSG_GET, cmdline.c_str());
	}
}

void nasClient::in_put(string cmdline, char* argv[128], int argc)
{
	string result = "put ";						// 发送命令行的结果;

	// 用于检查 put 命令;
	string path = "";
	try
	{
		// 必须先将 处理器的根目录置为空, 才能使用文件检查;
		m_file("");
		m_file.checkFile(path, (char*)cmdline.c_str(), MSG_PUT);
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
		
		// 排除 ".." "/." "." 符号;
		int length = cmd.find("/.");
		if (length >= 0 || cmd == "." || cmd == ".." || cmd == "./" || cmd == "../")
		{
			printf("reason: Don't use '.' or '..' symbol uploads the file, it is not allowed . \n\n");
			return;
		}
		
		// 用于记录'/'的位置;
		long long ret = 0;

		// 判断命令行末尾是否为'/';
		if (cmd[cmd_length] == '/')
		{
			// 将末尾的 "//" 排除掉;
			ret = cmd.find_last_of('/', cmd_length);
			while (ret > 0 && cmd[ret - 1] == '/')
			{
				ret -= 1;
			}
		}

		// 将末尾的'/' 排除掉;
		ret = cmd.find_last_of('/', ret - 1);
		if (ret >= 0)
		{
			result += cmd.substr(ret);
			result += " ";
		}
		else
		{
			// 命令行不存在'/';
			result = cmdline;
		}
	}
	
	m_cmdline = cmdline;
	sendData(MSG_PUT, result.c_str());
}

void nasClient::in_rm()
{
	char buf[32] = { 0 };

	while (true)
	{
		printf("need to delete it offline [y or n]: ");

	#ifdef _WIN32
		gets_s(buf, 32);
	#else
		fgets(buf, 32, stdin);

		// 在Linux下, fgets的缓冲区 需要手动将末尾置为 0;
		int length = strlen(buf);
		buf[length - 1] = 0;
	#endif

		if ((buf[0] == 'y' || buf[0] == 'Y') && buf[1] == '\0')
		{
			sendData(MSG_REMOVE2, m_cmdline.c_str());
			break;
		}

		if ((buf[0] == 'n' || buf[0] == 'N') && buf[1] == '\0')
		{
			break;
		}
	}
}

void nasClient::start(const char* username,const char* password)
{
	login(username, password);

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
		if (cmdline_length == 256)
		{
			printf("Too many file parameters are entered ! \n");
			continue;
		}

		// 检查是否有错误命令;
		bool error_flag = checkCmdine(cmdline, cmdline_length);
		
	#ifdef _WIN32
		strcpy_s(cmdline2, cmdline);
	#else
		strcpy(cmdline2, cmdline);
	#endif
		
		char* argv[64];
		int argc = FileUtils::Split(cmdline, argv);
		
		string cmd = argv[0];
		if (cmd == "ls" || cmd == "Ls")
		{
			if (argc > 1)
				sendData(MSG_LIST, cmdline2);
			else
				sendData(MSG_LIST);

			continue;
		}
		else if (cmd == "ll" || cmd == "Ll")
		{
			if (argc > 1)
				sendData(MSG_LIST2, cmdline2);
			else
				sendData(MSG_LIST2);

			continue;

		}
		else if (cmd == "cd" || cmd == "Cd")
		{
			if (error_flag)
			{
				printf("input bad command ! \n");
				continue;
			}

			if (argc > 1)
				sendData(MSG_CD, cmdline2);
			else
				sendData(MSG_CD, cmdline);

			continue;
		}
		else if (cmd == "rm" || cmd == "Rm")
		{
			if (argc > 1)
			{
				sendData(MSG_REMOVE, cmdline2);
				printf("\n");
			}

			continue;
		}
		else if (cmd == "get" || cmd == "Get")
		{
			if(argc > 1)
				in_get(cmdline2, argv, argc);

			continue;
		}
		else if (cmd == "put" || cmd == "Put")
		{
			if (argc > 1)
				in_put(cmdline2, argv, argc);

			continue;
		}
		else if (cmd == "help" || cmd == "Help")
		{
			string help = FileUtils::Help_Result();
			printf("%s", help.c_str());
			continue;
		}
		else if (cmd == "exit" || cmd == "Exit")
		{
			sendData(MSG_EXIT);
			break;
		}
		else
		{
			printf("input cmdline error ! \n");
			continue;
		}

		OS_Thread::Msleep(300);
	}

	printf("\nExit successfully . \n\n");
}

void nasClient::login(const char* username, const char* password)
{
	// 旧的登录协议, 已弃用;
	// ftp.sendData(MSG_LOGIN, "root", "123456");

#ifndef _WIN32
	system("clear");
#endif

	string user;
	user.append(username);
	user.append(password);

#ifndef _WIN32
	system("clear");
#endif

	sendData(MSG_LOGIN2, user.c_str());
}

bool nasClient::checkCmdine(char* cmdline, int cmdline_length)
{
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
		if (cmdline[i] == '\n' && cmdline[i + 1] == '\n' && i + 1 < cmdline_length)
			cmdline[i] == ' ';
	}
	

	// 删除子命令的 "./";
	string cmdline2 = cmdline;
	int find_pos = cmdline2.find(" ./", 0);
	if (find_pos > 0)
	{
		*(cmdline + find_pos + 1) = ' ';
		*(cmdline + find_pos + 2) = ' ';
	}

	return error_flag;
}

// 返回值: <=0 发送失败(socket可能已经断开), >0 发送成功;
// length: 数据字节的长度，可以为0;
int nasClient::sendMessages(unsigned short type, const void* data, unsigned int length)
{
	// 发送消息类型
	// m_SendSock.Send(&type, 8);
	// m_SendSock.Send(&length, 8);

	// 发送消息类型
	m_buffer.putUnit16(type);
	m_buffer.putUnit8(client_system);
	m_buffer.putUnit8(1);								// 预留一个字节位置, 为未来的升级使用;

	m_buffer.putUnit32(length);							// 大端传输
	m_SendSock.Send(m_buffer.Position(), 8, false);		// 先发送8个字节的数据, 用来指明身份信息的标头
	m_buffer.Clear();

	// 数据部分是0个字节则退出
	if (length <= 0) return 0;

	// 发送身份信息
	return m_SendSock.Send(data, length, false);
}

void nasClient::showResponse(Json::Value& jsonResponse)
{
	int errorCode = jsonResponse["errorCode"].asInt();
	string reason = jsonResponse["reason"].asString();

	char system = jsonResponse["system"].asInt();
	string result = jsonResponse["result"].asString();
	
	char buf[1024 * 6] = {0};				// 接收应答的缓存;
	int response_length = 0;				// 接收应答的长度;
	string character;						// 编码转换的结果;
	bool is_login = false;					// 是否成功登录;  

	// 显示应答结果;
	switch (errorCode)
	{
	case 11:			// 不同系统的 login, ls, ll
		response_length = serviceRequest(jsonResponse, buf, true);

		// 登录成功, 获取服务端的系统;
		if (!is_login)
		{
			is_login = true;
			server_system = system;
			m_file(system);
		}

		// 不同系统之间的编码转换;
		if (client_system != server_system)
		{
			character = m_file.characterEncoding(buf, response_length);
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
			response_length = serviceRequest(jsonResponse, buf, true);

			if (response_length > 0)					// 判断是否为目录;
			{
				// 不同系统之间的编码转换;
				if (client_system != server_system)
				{
					character = m_file.characterEncoding(buf, response_length);
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

	case 15:			// rm
	case 16:
		if (reason != "OK")
		{
			response_length = serviceRequest(jsonResponse, buf, true);

			if (response_length > 0)
			{
				printf("%s", buf);
				in_rm();
			}
		}
		break;

	case 20:			// get
		if (reason == "OK")
		{
			OS_Mutex mutex;
			m_file.downloadFile("/", mutex);
			
			m_file(string("."));						// 在Linux下, 不同函数之间的传参必须避免使用字符串(const char*);
		}
		else
		{
			throw string(result);
		}
		break;

	case 21:			// put
		if (reason == "OK")
		{
			char* argv[128] = {0};
			int argc = FileUtils::Split((char*)m_cmdline.c_str(), argv);

			OS_Mutex mutex;
			m_file("");
			m_file.uploadFile("",argv, argc, mutex);
			m_file.ACK_Send(ACK_FINISH);

			m_file(string("."));						// 在Linux下, 不同函数之间的传参必须避免使用字符串(const char*);
		}
		else
		{
			throw string(result);
		}
		break;

	case 0:				// login 或 ls

		// 登录成功, 获取服务端的系统;
		if (!is_login)
		{
			is_login = true;
			server_system = system;
			m_file(system);
		}
		throw string(result);
		break;

	default:
		throw string("reason: " + reason);
		throw string("result:" + result);
		break;
	}

}

// recv_sesponse 为 true 并且 buf 不为 0时, 函数为正常的接收响应数据;
// recv_sesponse 为 false 并且 buf 为 0 时, 函数为显示响应服务是否成功; 
int nasClient::serviceRequest(Json::Value& jsonResponse, char* buf, bool recv_response)
{
	int length = 0;
	int errorCode = jsonResponse["errorCode"].asInt();

	// 二次接收响应数据;
	if(recv_response && buf != NULL)
		m_SendSock.Recv(&length, 4, false);				// 先接收4个字节的数据, 用来指明长度;
	
	if (length > 0 && recv_response)
	{
		length = m_SendSock.Recv(buf, length, false);

		// 添加字符串的终止符
		buf[length] = 0;
		return length;
	}

	/* 显示响应服务 */

	// 显示传输错误的信息;
	if (m_file.fileError.size() > 0 && (errorCode == 20 || errorCode == 21))		// get, put
	{
		printf("\n%s", m_file.fileError.c_str());
		printf("Please perform the correct operation again as required ! \n\n");
		return 0;
	}

	// 成功响应服务;
	if (errorCode == 20 || errorCode == 21)							// get, put
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

int nasClient::requestServer(unsigned short type, const char* username, const char* password)
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
		// 发送普通数据;
		jsonreq = username;
	}
	
	// 发送身份认证;
	sendMessages(type, jsonreq.c_str(), jsonreq.length());
	

	// 直接退出客户端;
	if (type == MSG_EXIT)
		return 0;

	// 接收JSON请求的应答
	char buf[1024];					// 创建接收应答的缓存;
	int response_length = 0;

	// 先接收4个字节的数据, 用来指明长度;
	m_SendSock.Recv(&response_length, 4, false);
	response_length = m_SendSock.Recv(buf, response_length, false);			//接收数据
	if (response_length <= 0)
	{
		throw string("Error: failed to recv response! \n");
	}

	buf[response_length] = 0;			// 添加字符串的终止符



	// 解析JSON请求的应答
	string response(buf, response_length);
	Json::Reader jsonReader;
	Json::Value jsonResponse;
	if (!jsonReader.parse(response, jsonResponse, false))
	{
		jsonResponse.clear();				// 防止段错误;
		throw string("bad json format! \n");
	}

	// 显示和处理 服务端的所有响应;
	showResponse(jsonResponse);
	serviceRequest(jsonResponse);

	return 0;
}
