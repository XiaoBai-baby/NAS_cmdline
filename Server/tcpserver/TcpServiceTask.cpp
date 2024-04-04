#include "TcpServiceTask.h"

TcpServiceTask::TcpServiceTask(OS_TcpSocket client_sock)
	: m_clientSock(client_sock), m_alive(true)
{
	// 申请一个大缓冲区, 存放接收及发送的数据
	m_bufsize = 64 * 1024;
	m_buf = new unsigned char[m_bufsize];

	Start();	// 启动线程
}

TcpServiceTask::~TcpServiceTask()
{
	m_clientSock.Close();
	m_alive = false;
	printf("The Service thread has exited (client = %s) ...\n", clientAddress().c_str());
}

// 启动线程
int TcpServiceTask::Start()
{
	// 客户端信息
	m_clientSock.GetPeerAddr(m_clientAddr);
	printf("Got a connect: %s: %d \n", clientAddress().c_str(), m_clientAddr.GetPort());

	m_alive = true;
	Run();
	return 0;
}

// 判断线程是否活着
bool TcpServiceTask::Alive() const
{
	return m_alive;
}

// 返回客户端的IP地址
string TcpServiceTask::clientAddress() const
{
	return m_clientAddr.GetIp_str();
}

// 主要函数用来放置不同的业务服务函数;
int TcpServiceTask::Routine()
{
	return BusinessService();
}