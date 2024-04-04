#ifndef _THREAD_TCP_RECV_H
#define _THREAD_TCP_RECV_H

#include "../osapi/osapi.h"
#include "./ByteBuffer.h"

/* ThreadTcpRecv 线程接收器
	用于多开一个线程来处理接收任务, 适用于多线程、高并发的场景;
*/

class ThreadTcpRecv : public OS_Thread
{
public:
	ThreadTcpRecv(OS_TcpSocket& recv_sock, int bufsize);

	ThreadTcpRecv(OS_TcpSocket& recv_sock, char* buffer, int offset, int bufsize);

	~ThreadTcpRecv();

public:
	// 开启接收数据的线程服务;
	void StartReceiveData(void* head_buf, int head_size);

public:
	// 接收数据, 版本1.0; 请慎用, 时有BUG;
	int ReceiveData(void* buf, unsigned int bufsize, int timeout = 2000);		// 必须设置一个接收超时来处理数据

	// 接收数据, 版本2.0; 接收速度快, 且无BUG;
	int ReceiveData2(void* buf, unsigned int bufsize, int timeout = 6000);

	// 解析数据的辅助延迟, 版本1.0时使用;
	void DelayReceive(unsigned int numberSize);

	// 解析数据, 其他业务任务
	void ParseData();

private:
	// 线程函数;
	virtual int Routine();

protected:
	int m_bufsize;		// 接收缓冲区大小

protected:
	OS_TcpSocket m_RecvSock;		// 连接客户端的socket地址
	OS_Semaphore m_Sem;				// 控制接收、解析数据的信号量, 版本1.0时使用;
	ByteBuffer m_buffer;			// 缓存数据的字节编码器
};

#endif