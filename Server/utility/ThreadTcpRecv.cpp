#include "ThreadTcpRecv.h"

// 控制接收、解析数据的互斥锁;
// 互斥锁必须用全局变量, 因为解析数据函数不是线程操作, 
namespace ThreadTcpReceive
{
	OS_Mutex m_Mutex;
}

ThreadTcpRecv::ThreadTcpRecv(OS_TcpSocket& recv_sock, int bufsize)
	: m_RecvSock(recv_sock), m_buffer(bufsize), m_Sem(0)
{
	m_bufsize = bufsize;
}

ThreadTcpRecv::ThreadTcpRecv(OS_TcpSocket& recv_sock, char* buffer, int offset, int bufsize)
	: m_RecvSock(recv_sock), m_buffer(buffer, offset, bufsize), m_Sem(0)
{
	m_bufsize = bufsize;
}

ThreadTcpRecv::~ThreadTcpRecv()
{
	Join(this);
}


void ThreadTcpRecv::StartReceiveData(void* head_buf, int head_size)
{
	// 加载数据报头
	m_buffer.loadHeader(head_buf, head_size);

	Run();
}

// buf: 表示接收数据的缓冲区
// timeout: 如果为0，表示阻塞等待，否则，表示设置接收超时
int ThreadTcpRecv::ReceiveData(void* buf, unsigned int total, int timeout)
{
	// 设置超时
	if (timeout > 0)
	{
		m_RecvSock.SetOpt_RecvTimeout(timeout);
	}

	// 接收数据过长的异常处理
	if (total > m_bufsize)
	{
		printf("ReceiveData2 function error: send data is too long ! \n");
		return -12;
	}
	
	int bytes_got = 0;		// 缓冲区接收数据后的字节偏移量; 
	int sendCount = m_buffer.sendCount();		// 发送的数据段个数;


	// 反复接收数据, 直到接满指定的字节数;
	for (int i = 0; i < sendCount; i++)
	{
		ThreadTcpReceive::m_Mutex.Lock();

		// 接收完数据则直接 Post();
		if (bytes_got >= m_buffer.Total())
		{
			m_Sem.Post();

			continue;
		}

		// byte_got: 单数据的字节偏移量
		int byte_got = 0;
		int count = *(m_buffer.Individual() + i);

		// 接收最后一个数据时, 必须比它大几倍的数据量接收, 此函数的异常多数也是在这里;
		if (i == sendCount - 1)
		{
			count = count * 4;		// 将count * 4 改成 count 将会引起内存访问冲突;
		}

		while (byte_got < count)
		{
			// 注意, 这里的Recv()是向系统自带的RecvBuf缓冲区接收数据, 而非客户端本身;
			int n = m_RecvSock.Recv((char*)buf + bytes_got, count - byte_got, false);
			if (n <= 0)
			{
				break;		// 如果没有数据就结束循环;
			}
			else if (n >= count && i != sendCount)
			{
				m_Sem.Post();		// 防止异常, 每收到一个数据就直接 Post();
			}
			else
			{
				m_Sem.Post();	// 接收最后一个数据时, 末尾再加一个Post();
			}

			byte_got += n;
			bytes_got += byte_got;
		}

		ThreadTcpReceive::m_Mutex.Unlock();
	}

	return bytes_got;
}

int ThreadTcpRecv::ReceiveData2(void* buf, unsigned int total, int timeout)
{
	// 设置超时
	if (timeout > 0)
	{
		m_RecvSock.SetOpt_RecvTimeout(timeout);
	}

	// 接收数据过长的异常处理
	if (total > m_bufsize)
	{
		printf("ReceiveData2 function error: send data is too long ! \n");
		return -12;
	}

	// 反复接收数据, 直到接满指定的字节数;
	int bytes_got = 0;
	while (bytes_got < total)
	{
		ThreadTcpReceive::m_Mutex.Lock();
		int n = m_RecvSock.Recv((char*)buf + bytes_got, total - bytes_got, false);
		ThreadTcpReceive::m_Mutex.Unlock();
		if (n <= 0)
		{
			continue;
		}

		bytes_got += n;
	}

	return bytes_got;	// 返回接收数据的大小;
}

void ThreadTcpRecv::DelayReceive(unsigned int numberSize)
{
	if (numberSize > 1024 * 1024 * 1024)
	{
		OS_Thread::Sleep(3);		// 延迟3秒;
	}
	else if (numberSize > 1024 * 1024)		// 解析大数据之前可以加上延迟, 来防止因接收缓慢而缓冲区溢出的问题;
	{
		OS_Thread::Sleep(1);		// 延迟1秒;
	}
	else if (numberSize > 1024)
	{
		OS_Thread::Msleep(200);		// 延迟200毫秒;
	}

	m_Sem.Wait();		// 信号量减1
}

void ThreadTcpRecv::ParseData()
{
	// OS_Thread::Msleep(200);		// 这里须给他一个200毫秒以上的延迟, 因为线程创建比普通函数慢;

	int sendCount = m_buffer.sendCount();		// 发送的数据段个数;

	// 开始解码数据;
	for (int i = 0; i < sendCount; i++)
	{
		ThreadTcpReceive::m_Mutex.Lock();

		bool isNumber = *(m_buffer.isNumber() + i);

		unsigned int numberSize = *(m_buffer.Individual() + i);

		// DelayReceive(numberSize);
		// m_Sem.Wait();		// 版本1.0时使用
		
		if (isNumber)	// 解码数据;
		{
			switch (numberSize)
			{
			case 1:
				printf("get number: %d \n", m_buffer.getUnit8());
				break;
			case 2:
				printf("get number: %d \n", m_buffer.getUnit16());
				break;
			case 4:
				printf("get number: %d \n", m_buffer.getUnit32());
				break;
			case 8:
				printf("get number: %lld \n", m_buffer.getUnit64());	// long long
				break;
			default:
				break;
			}
		}
		else
		{
			printf("get string: %s \n", m_buffer.getString().c_str());
		}

		ThreadTcpReceive::m_Mutex.Unlock();
	}

}

int ThreadTcpRecv::Routine()
{
	return ReceiveData2(m_buffer.Position(), m_buffer.Total());
}