#include "TcpTaskMonitor.h"

TcpTaskMonitor* TcpTaskMonitor::i()
{
	static TcpTaskMonitor one;
	return &one;
}

TcpTaskMonitor* TcpTaskMonitor::i(TcpServiceTask* task)
{
	static TcpTaskMonitor one;
	one.monitor(task);

	return &one;
}

TcpTaskMonitor::TcpTaskMonitor()
{
	// startService();
}

TcpTaskMonitor::TcpTaskMonitor(TcpServiceTask* task)
{
	monitor(task);
	// startService();
}

TcpTaskMonitor::~TcpTaskMonitor()
{
	stopService();
}


int TcpTaskMonitor::startService()
{
	m_quitflag = false;
	Run();
	return 0;
}

void TcpTaskMonitor::stopService()
{
	m_quitflag = true;
}

void TcpTaskMonitor::monitor(TcpServiceTask* task)
{
	// 访问m_tasks时需要使用锁控制
	m_Mutex.Lock();
	m_Tasks.push_back(task);
	m_Mutex.Unlock();

	m_Sem.Post();	// 把信号量的值加1
}

int TcpTaskMonitor::Routine()
{
	while (!m_quitflag)
	{
		// 使用信号量机制, 代替无限循环的轮询机制;
		m_Sem.Wait();	//信号量减1

		// 访问m_tasks时需要使用锁控制
		m_Mutex.Lock();

		// 遍历m_tasks，找到已经终止的线程并回收之
		for (list<TcpServiceTask*>::iterator iter = m_Tasks.begin();
			iter != m_Tasks.end(); )
		{
			TcpServiceTask* task = *iter;
			if (task->Alive())		// 检查线程对象是否已经退出 
			{
				iter++;
			}
			else
			{
				// 线程已经终止, 回收这里的线程资源
				printf("====Retired TcpServiceTask (client = %s) \n", task->clientAddress().c_str());
				iter = m_Tasks.erase(iter);
				Join(task);
				delete task;
			}
		}

		m_Mutex.Unlock();

		OS_Thread::Msleep(10);
	}

	return 0;
}
