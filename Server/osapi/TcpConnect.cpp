#include "./TcpConnect.h"

TcpConnect::TcpConnect(OS_TcpSocket& work_sock)
{
	m_WorkSock = work_sock;
}

TcpConnect::~TcpConnect()
{

}

// count: 准备接收多少字节
// timeout: 如果为0，表示阻塞等待，否则，表示设置接收超时
int TcpConnect::Recv_SpecifyBytes(void* buf, int count, int timeout)
{
	// 设置超时
	if (timeout > 0)
	{
		m_WorkSock.SetOpt_RecvTimeout(timeout);
	}

	// 反复接收数据, 直到接满指定的字节数;
	int bytes_got = 0;
	while (bytes_got<count)
	{
		int n = m_WorkSock.Recv((char*)buf + bytes_got, count - bytes_got, false);
		if (n <= 0)
		{
			return bytes_got;
		}

		bytes_got += n;
	}

	return bytes_got;
}

int TcpConnect::Routine()
{
	// 为client提供服务
	unsigned char buf[1024];

	m_WorkSock.Recv(buf, 1024, false);

	OS_Thread::Msleep(3000);

	// 应答客户
	const char* ret = "send seccusssfully ...";
	m_WorkSock.Send(ret, strlen(ret), false);

	// 关闭socket
	m_WorkSock.Close();

	return 0;
}