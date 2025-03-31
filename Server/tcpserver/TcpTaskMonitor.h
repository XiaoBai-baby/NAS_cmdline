#ifndef _TCP_TASK_MONITOR_H
#define _TCP_TASK_MONITOR_H

#include "../osapi/osapi.h"
#include "TcpServiceTask.h"
#include <list>

using std::list;

/*
   监视TcpServiceTask业务类的运行
   当目标线程终止时，回收该线程的资源
*/
class TcpTaskMonitor : public OS_Thread
{
public:
	// 单例模式, 用于监视添加类时使用;
	static TcpTaskMonitor* i();
	static TcpTaskMonitor* i(TcpServiceTask* task);

public:
	// 多例模式, 有生命周期;
	TcpTaskMonitor();
	TcpTaskMonitor(TcpServiceTask* task);

	~TcpTaskMonitor();

public:
	// 启动监视线程;
	int startService();

	// 关闭监视线程;
	void stopService();

	// 添加业务类
	void monitor(TcpServiceTask* task);

private:
	// 用于处理线程的主要函数;
	virtual int Routine();

private:
	list<TcpServiceTask*> m_Tasks;		// 存放业务类的双向链表
	bool m_quitflag;					// 控制线程的退出
	OS_Mutex m_Mutex;					// 访问链表的互斥锁
	OS_Semaphore m_Sem;					// 访问链表的信号量
};

#endif