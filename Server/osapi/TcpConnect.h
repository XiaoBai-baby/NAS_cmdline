#ifndef _TCP_CONNECT_H
#define _TCP_CONNECT_H

#include "./osapi.h"

/* TcpConnect:
	用一个线程来维护Client - WorkingSocket之间的通话连接
*/

class TcpConnect : public OS_Thread
{
public:
	TcpConnect(OS_TcpSocket& work_sock);
	~TcpConnect();

public:
	// 用于接收指定长度的数据
	int Recv_SpecifyBytes(void* buf, int count, int timeout = 0);

private:
	// 用于处理client请求的主要函数;
	virtual int Routine();

private:
	OS_TcpSocket m_WorkSock;
};

#endif