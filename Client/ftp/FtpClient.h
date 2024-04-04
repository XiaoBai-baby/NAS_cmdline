#ifndef _FTP_CLIENT_H
#define _FTP_CLIENT_H

#include "./MsgDefine.h"
#include "../osapi/osapi.h"
#include "../utility/utility.h"
#include "../jsoncpp/include/json.h"

/*	FtpClient
	实现类似 FTP的服务
*/

class FtpClient
{
public:
	FtpClient(OS_TcpSocket sock, int bufsize);

public:
	// 发送任务;
	void SendData(unsigned short type);

	void SendData(unsigned short type, const char* data);

	void SendData(unsigned short type, const char* username, const char* password);

public:
	// 启动服务;
	void Start();

private:
	// 发送消息类型;
	int SendMessages(unsigned short type, const void* data, unsigned int length);

	// 显示JSON请求的应答;
	void ShowResponse(Json::Value& jsonResponse);

	// 请求服务端的操作;
	int RequestServer(unsigned short type, const char* username, const char* password);

private:
	bool isWindows;					// 判断客户端的系统

	string m_path;

private:
	OS_TcpSocket m_SendSock;		// 客户端的socket地址
	ByteBuffer m_buffer;			// 缓存数据的字节编码器
};

#endif