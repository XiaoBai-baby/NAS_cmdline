#include "./FtpClient.h"

FtpClient::FtpClient(OS_TcpSocket sock, int bufsize) : m_SendSock(sock), m_buffer(bufsize), isWindows(false)
{
	// 判断是否为 Windows系统;
#ifdef _WIN32
	isWindows = true;
#endif

	// 注意, m_buffer 只能在CustomTcpSend中构造; 
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


void FtpClient::Start()
{
	char cmdline[256];
	char cmdline2[256];

	const char* root = "[root@localhost";
	const char* root2 = "]#  ";

	int count = 0;
	while (true)
	{
	#ifndef _WIN32
		if (count == 0)	system("pause");
	#endif

		cout << root << m_path << root2;

	#ifdef _WIN32
		gets_s(cmdline);
	#else
		fgets(cmdline, strlen(cmdline), NULL);
	#endif
		count++;

		if (strlen(cmdline) == 0) continue;

		strcpy(cmdline2, cmdline);
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
			if (argc > 1)
				SendData(MSG_CD, cmdline2);
			else
				SendData(MSG_CD, cmdline);

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

#ifndef _WIN32
	if (count == 0)	system("pause");
#endif

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
	
	char buf[1024 * 6];					// 接收应答的缓存;
	int response_length = 0;			// 接收应答的长度;
	string character;					// 编码转换的结果;

	// 显示应答结果;
	switch (errorCode)
	{
	case 11:
		// 不同系统之间使用二次接收数据;
		response_length = m_SendSock.Recv(buf, 1024 * 6, false);
		buf[response_length] = 0;			// 添加字符串的终止符

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

	case 12:
		if (reason == "OK")
		{
			if (result.size() > 0)
				m_path = " " + result;
			else
				m_path = result;
		}
		else
		{
			throw string(result);
		}
		break;

	case 0:
		throw string(result);
		break;

	default:
		throw string("reason: " + reason);
		throw string("result:" + result);
		break;
	}

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

	ShowResponse(jsonResponse);				// 显示应答

	return 0;
}
