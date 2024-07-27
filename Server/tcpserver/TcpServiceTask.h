#ifndef _TCP_SERVICE_TASK_H
#define _TCP_SERVICE_TASK_H

#include "../osapi/osapi.h"
#include "../utility/utility.h"

/* 测试用的服务线程
  具体的业务类
*/

class TcpServiceTask : public OS_Thread
{
public:
	TcpServiceTask(OS_TcpSocket client_sock);
	~TcpServiceTask();

public:
	// 启动线程
	int Start();

	// 判断线程是否活着
	bool Alive() const;

	// 获取客户端IP地址
	string clientAddress() const;

private:
	// 具体业务服务函数;
	int BusinessService();

private:
	// 用于处理线程的主要函数;
	int Routine();


private:
	unsigned char* m_buf;		// 数据缓冲区
	int m_bufsize;				// 缓冲区大小

private:
	OS_TcpSocket m_clientSock;		// 连接客户端的socket地址
	OS_SockAddr m_clientAddr;		// 客户端的IP地址
	bool m_alive;					// 标识线程是否存在
};

#endif