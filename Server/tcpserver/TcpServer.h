#ifndef _TCP_SERVER_H
#define _TCP_SERVER_H

#include "../osapi/osapi.h"

/*
	TCP服务器主类
*/

class TcpServer : public OS_Thread
{
public:
	TcpServer();
	TcpServer(int port);

	~TcpServer();

	// 启动TCP服务
	int startService(int port);

	// 关闭TCP服务
	void stopService();

private:
	// 用于处理client请求的主要函数;
	virtual int Routine();

private:
	OS_TcpSocket m_WorkSock;		// 服务端socket地址
	bool m_quitflag;				// 控制线程的退出
};

#endif