#include "TcpServer.h"
#include "TcpServiceTask.h"
#include "TcpTaskMonitor.h"

TcpServer::TcpServer()
{
	// startService(8888);
}

TcpServer::TcpServer(int port)
{
	startService(port);
}

TcpServer::~TcpServer()
{
	stopService();
}

int TcpServer::startService(int port)
{
	if (m_WorkSock.Open(OS_SockAddr(port), true) < 0)
	{
		// 此端口不可用
		return -11;
	}

	if (m_WorkSock.Listen() < 0)
	{
		return -22;
	}

	// 启动监听服务
	TcpTaskMonitor::i()->startService();
	printf("Server listening on : %d \n", port);
	
	// 启动线程
	m_quitflag = false;
	Run();
	return 0;
}

void TcpServer::stopService()
{
	m_quitflag = true;
	m_WorkSock.Close();
	Join(this);
}

int TcpServer::Routine()
{
	printf("Server working . \n");

	while (!m_quitflag)
	{
		// 用于处理Working的Socket, 每个client对应一个Socket;
		OS_TcpSocket work_sock;
		if (m_WorkSock.Accept(work_sock) < 0)
		{
			break;
		}

		printf("----------------------------- \n\n");

		// 创建业务服务线程
		TcpServiceTask* task = new TcpServiceTask(work_sock);
		TcpTaskMonitor::i()->monitor(task);		// 把task业务线程加入回收管理器
	}

	printf("Server exit . \n");
	return 0;
}