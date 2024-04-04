#include "osapi.h"

#ifdef _WIN32

#pragma comment(lib, "ws2_32")

class OS_SocketInit
{
public:

	OS_SocketInit()
	{
		WSADATA wsData;
		int iResult = WSAStartup(MAKEWORD(2, 2), &wsData);
		if (iResult != NO_ERROR)
		{
			printf("Socket初始化失败... \n");
			printf("失败原因: %d \n", iResult);
		}
	}

	~OS_SocketInit()
	{
		WSACleanup();
	}
};

static OS_SocketInit socket_init_win32;

#endif

/* ---------------- OS_SockAddr() ---------------- */

OS_SockAddr::OS_SockAddr()
{
	iAddr.sin_family = AF_INET;		//IPv4
	iAddr.sin_addr.s_addr = 0;
	iAddr.sin_port = 0;
}

OS_SockAddr::OS_SockAddr(const char* ip, unsigned short port)
{
	iAddr.sin_family = AF_INET;		//IPv4
	iAddr.sin_addr.s_addr = inet_addr(ip);
	iAddr.sin_port = htons(port);		// 大端传输
}

OS_SockAddr::OS_SockAddr(const char* ip)
{
	iAddr.sin_family = AF_INET;		//IPv4
	iAddr.sin_addr.s_addr = inet_addr(ip);
	iAddr.sin_port = 0;
}

OS_SockAddr::OS_SockAddr(unsigned short port)
{
	iAddr.sin_family = AF_INET;		//IPv4
	iAddr.sin_addr.s_addr = 0;
	iAddr.sin_port = htons(port);
}

OS_SockAddr::OS_SockAddr(sockaddr_in addr)
{
	iAddr = addr;
}

void OS_SockAddr::SetIp(const char* ip)
{
	iAddr.sin_addr.s_addr = inet_addr(ip);
}

void OS_SockAddr::SetIp(unsigned int ip)
{
	iAddr.sin_addr.s_addr = ip;
}

void OS_SockAddr::SetIp(unsigned short port)
{
	iAddr.sin_port = htons(port);		//大端传输
}

string OS_SockAddr::GetIp_str() const
{
	return string(inet_ntoa(iAddr.sin_addr));
}

unsigned int OS_SockAddr::GetIp_n() const
{
	return iAddr.sin_addr.s_addr;
}

unsigned short OS_SockAddr::GetPort() const
{
	return ntohs(iAddr.sin_port);		//小端字节
}

/* --------------- OS_Socket() -------------- */

OS_Socket::OS_Socket()
	:hSock(-1)
{

}

int OS_Socket::SetOpt_RecvTimeout(int ms)
{
#ifdef _WIN32
	int sockopt_value = socket_setsockopt(hSock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &ms, sizeof(ms));

	// 检查错误代码;
	if (Check_WSAError(sockopt_value, __FUNCTION__))
	{
		return -1;
	}
#else
	timeval ts;
	ts.tv_sec = ms / 1000;
	ts.tv_usec = ms % 1000 * 1000;

	int sockopt_value = socket_setsockopt(hSock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &ts, sizeof(timeval));

	// 检查错误代码;
	if (Check_WSAError(sockopt_value, __FUNCTION__))
	{
		return -1;
	}
#endif

	return 0;
}

int OS_Socket::SetOpt_SendTimeout(int ms)
{
#ifdef _WIN32
	int sockopt_value = socket_setsockopt(hSock, SOL_SOCKET, SO_SNDTIMEO, (const char*) &ms, sizeof(ms));

	// 检查错误代码;
	if (Check_WSAError(sockopt_value, __FUNCTION__))
	{
		return -1;
	}
#else
	timeval ts;
	ts.tv_sec = ms / 1000;
	ts.tv_usec = ms % 1000 * 1000;

	int sockopt_value = socket_setsockopt(hSock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&ts, sizeof(timeval));

	// 检查错误代码;
	if (Check_WSAError(sockopt_value, __FUNCTION__))
	{
		return -1;
	}
#endif

	return 0;
}

int OS_Socket::GetOpt_RecvTimeout()
{
#ifdef _WIN32
	int arge = 0;
	socklen_t len = sizeof(int);
	int sockopt_value = socket_setsockopt(hSock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&arge, sizeof(socklen_t));

	// 检查错误代码;
	if (Check_WSAError(sockopt_value, __FUNCTION__))
	{
		return -1;
	}

	return arge;
#else
	timeval ts;
	socklen_t len = sizeof(timeval);
	int sockopt_value = socket_setsockopt(hSock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&ts, sizeof(len));

	// 检查错误代码;
	if (Check_WSAError(sockopt_value, __FUNCTION__))
	{
		return -1;
	}

	return ts.tv_sec * 1000 + ts.tv_usec / 1000;
#endif
}

int OS_Socket::GetOpt_SendTimeout()
{
#ifdef _WIN32
	int arge = 0;
	socklen_t len = sizeof(int);
	int sockopt_value = socket_setsockopt(hSock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&arge, sizeof(len));

	// 检查错误代码;
	if (Check_WSAError(sockopt_value, __FUNCTION__))
	{
		return -1;
	}

	return arge;
#else
	timeval ts;
	socklen_t len = sizeof(timeval);
	int sockopt_value = socket_setsockopt(hSock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&ts, sizeof(len));

	// 检查错误代码;
	if (Check_WSAError(sockopt_value, __FUNCTION__))
	{
		return -1;
	}

	return ts.tv_sec * 1000 + ts.tv_usec / 1000;
#endif
}

int OS_Socket::Ioctl_SetBlockedIo(bool blocked)
{
	// 在 套接字上启用或禁用非阻塞模式;
	unsigned long arge = blocked ? 0 : 1;
	// 如果要启用非阻塞模式，则为非零;如果要禁用非阻塞模式，则为零
	int sockio_value = socket_ioctl(hSock, FIONBIO, (unsigned long*)&arge);

	// 检查错误代码;
	if (Check_WSAError(sockio_value, __FUNCTION__))
	{
		return -1;
	}

	return 0;
}

void OS_Socket::Set_SockOption(int optname, const char* bufsize)
{
	// 设置SendBuf的大小
	int sockopt = socket_setsockopt(hSock, SOL_SOCKET, optname, bufsize, sizeof(int));
	
	// 检查错误代码;
	if (Check_WSAError(sockopt, __FUNCTION__))
	{
		return ;
	}
}

int OS_Socket::SetOpt_ReuseAddr(bool reuse)
{
	int opt = reuse ? 1 : 0;
	socklen_t len = sizeof(opt);
	int sockopt_value = socket_setsockopt(hSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, len);

	// 检查错误代码;
	if (Check_WSAError(sockopt_value, __FUNCTION__))
	{
		return -1;
	}

	return 0;
}

int OS_Socket::GetPeerAddr(OS_SockAddr& addr) const
{
	socklen_t len = sizeof(addr);
	int arge = getpeername(hSock, (sockaddr*)&addr, &len);

	// 检查错误代码;
	if (Check_WSAError(arge, __FUNCTION__))
	{
		return -1;
	}

	return 0;
}

int OS_Socket::GetLocalAddr(OS_SockAddr& addr) const
{
	socklen_t len = sizeof(addr);
	int arge = getsockname(hSock, (sockaddr*)&addr, &len);

	// 检查错误代码;
	if (Check_WSAError(arge, __FUNCTION__))
	{
		return -1;
	}

	return 0;
}

int OS_Socket::Select_ForReading(int timeout)
{
	timeval tm;
	tm.tv_sec = timeout / 1000;
	tm.tv_usec = timeout % 1000;

	fd_set fds;		// 存放套接字数组的结构体;
	FD_ZERO(&fds);		// 初始化设置为空集; 在使用之前, 应始终清除集;
	FD_SET(hSock, &fds);	// 添加要设置的套接字	
	
	// select 函数确定一个或多个套接字的状态,（如有必要）等待执行同步 I/O
	int returned_value = select(hSock+1, &fds, NULL, NULL, &tm);

	// FD_ISSET(hSock, &fds);	// 检查 fd 是否为 set 成员，如果为 TRUE，则返回 TRUE
	FD_CLR(hSock, &fds);		// 从集中删除套接字

	// 检查错误代码;
	if (Check_WSAError(returned_value, __FUNCTION__))
	{
		return -1;
	}

	return returned_value;
}

int OS_Socket::Select_ForWriting(int timeout)
{
	timeval tm;
	tm.tv_sec = timeout / 1000;
	tm.tv_usec = timeout % 1000;

	fd_set fds;		// 存放套接字数组的类
	FD_ZERO(&fds);		// 初始化设置为空集; 在使用之前, 应始终清除集;
	FD_SET(hSock, &fds);	// 添加要设置的套接字

	// select 函数确定一个或多个套接字的状态,（如有必要）等待执行同步 I/O
	int returned_value = select(hSock + 1, NULL, &fds, NULL, &tm);

	// FD_ISSET(hSock, &fds);	// 检查 fd 是否为 set 成员，如果为 TRUE，则返回 TRUE
	FD_CLR(hSock, &fds);		// 从集中删除套接字

	// 检查错误代码;
	if (Check_WSAError(returned_value, __FUNCTION__))
	{
		return -1;
	}

	return returned_value;
}

/*-------------- TCP SOCKET --------------*/

int OS_TcpSocket::Open(bool reuse)
{
	hSock = socket_open(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// 检查错误代码;
	if (Check_WSAError(hSock, __FUNCTION__))
	{
		return -1;
	}

	SetOpt_ReuseAddr(reuse);

	return 0;
}

int OS_TcpSocket::Open(const OS_SockAddr& addr, bool reuse)
{
	if (Open(reuse) < 0)
	{
		return -1;
	}

	int arge = socket_bind(hSock, (const sockaddr*) &addr, sizeof(addr));
	
	// 检查错误代码;
	if (Check_WSAError(hSock, __FUNCTION__))
	{
		// 关闭错误套接字;
		socket_close(hSock);
		hSock = -1;

		return -1;
	}
}

void OS_TcpSocket::Close()
{
	if ((int)hSock >= 0)
	{
		shutdown(hSock, 2);
		socket_close(hSock);
		hSock = -1;
	}
	else if (Check_WSAError(hSock, __FUNCTION__))	//检查错误代码;
	{
		return;
	}
}

// 服务端
int OS_TcpSocket::Listen(int backlog)
{
	int arge = socket_listen(hSock, backlog);

	// 检查错误代码;
	if (Check_WSAError(arge, __FUNCTION__))
	{
		return -1;
	}

	return 0;
}

// 服务端
int OS_TcpSocket::Accept(OS_TcpSocket& peer)
{
	sockaddr_in addr;
	socklen_t addr_len = sizeof(sockaddr_in);
	socket_t peer_handle = socket_accept(hSock, (sockaddr*)&addr, &addr_len);

	// 检查错误代码;
	if (Check_WSAError(peer_handle, __FUNCTION__))
	{
		return -1;
	}

	peer.hSock = peer_handle;
	return 0;
}

int OS_TcpSocket::Connect(const OS_SockAddr& addr)
{
	int arge = socket_connect(hSock, (const sockaddr*)&addr, sizeof(addr));

	// 检查错误代码;
	if (Check_WSAError(arge, __FUNCTION__))
	{
		return -1;
	}

	return 0;
}

int OS_TcpSocket::Send(const void* buf, int len, bool OOB_flag)
{
	// 默认是普通数据的传输;
	int flags = OOB_flag ? MSG_OOB : 0;

	int n = socket_send(hSock, (const char*)buf, len, flags);

	// 检查错误代码;
	if (Check_WSAError(n, __FUNCTION__))
	{
		return -1;
	}

	return n;
}

int OS_TcpSocket::Recv(void* buf, int len, bool OOB_flag)
{
	// 默认是普通数据的接受;
	int flags = OOB_flag ? MSG_OOB : 0;

	int n = socket_recv(hSock, (char*)buf, len, flags);

	// 检查错误代码;
	/* Tcp Recv的错误检测妨碍操作界面的功能简洁;
	if (Check_WSAError(n, __FUNCTION__))
	{
		return -1;
	}
	*/

	return n;
}

int OS_TcpSocket::Recv_OOB(int OOB_flag)
{
	char* buf = new char[1024];
	int len = sizeof(buf);

	// 接受带外数据
	int n = Recv(buf, len, OOB_flag);
	printf("接受的带外数据: %s \n", buf);

	delete[] buf;

	// 检查错误代码;
	if (Check_WSAError(n, __FUNCTION__))
	{
		return -1;
	}

	return n;
}

/*-------------- UDP SOCKET --------------*/

int OS_UdpSocket::Open(bool reuse)
{
	hSock = socket_open(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	// 检查错误代码;
	if (Check_WSAError(hSock, __FUNCTION__))
	{
		return -1;
	}

	SetOpt_ReuseAddr(reuse);
	return 0;
}

int OS_UdpSocket::Open(const OS_SockAddr& addr, bool reuse)
{
	if (Open(reuse) < 0)
	{
		return -1;
	}

	int arge = socket_bind(hSock, (const sockaddr*)&addr, sizeof(addr));

	// 检查错误代码;
	if (Check_WSAError(arge, __FUNCTION__))
	{
		// 关闭错误套接字;
		socket_close(hSock);
		hSock = -1;

		return -1;
	}

	return 0;
}

void OS_UdpSocket::Close()
{
	if ((int)hSock >= 0)
	{
		shutdown(hSock, 2);
		socket_close(hSock);
		hSock = -1;
	}
	else if (Check_WSAError(hSock, __FUNCTION__))	//检查错误代码;
	{
		return;
	}
}

int OS_UdpSocket::SendTo(const char* buf, int len, const OS_SockAddr& peer)
{
	int addr_len = sizeof(sockaddr_in);
	int n = socket_sendto(hSock, buf, len, 0, (const sockaddr*)&peer, addr_len);

	// 检查错误代码;
	if (Check_WSAError(n, __FUNCTION__))
	{
		return -1;
	}

	return n;
}

int OS_UdpSocket::RecvFrom(void* buf, int len, const OS_SockAddr& peer)
{
	socklen_t addr_len = sizeof(sockaddr_in);
	int n = socket_recvfrom(hSock, (char*)buf, len, 0, (sockaddr*)&peer, &addr_len);

	// 检查错误代码;
	if (Check_WSAError(n, __FUNCTION__))
	{
		return -1;
	}

	return n;
}

/*-------------- 组播 McastSock --------------*/

int OS_McastSock::Open(const char* mcast_ip, int port, const char* local_ip)
{
	hSock = socket_open(AF_INET, SOCK_DGRAM, 0);

	// 检查错误代码;
	if (Check_WSAError(hSock, __FUNCTION__))
	{
		return -1;
	}

	// 一定需要;
	if (SetOpt_ReuseAddr(true) < 0)
	{
		socket_close(hSock);
		hSock = -1;
		return 0;
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
#ifdef _WIN32
	addr.sin_addr.s_addr = inet_addr(local_ip);
#else
	addr.sin_addr.s_addr = inet_addr(mcast_ip);
#endif
	addr.sin_port = htons(port);		//大端传输

	int returned_value = socket_bind(hSock, (const sockaddr*)&addr, sizeof(sockaddr));

	// 检查错误代码;
	if (Check_WSAError(returned_value, __FUNCTION__))
	{
		// 关闭错误套接字;
		socket_close(hSock);
		hSock = -1;

		return -1;
	}

	m_McReq.imr_multiaddr.s_addr = inet_addr(mcast_ip);
	m_McReq.imr_interface.s_addr = inet_addr(local_ip);

	int sockopt_value = socket_setsockopt(hSock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&m_McReq, sizeof(ip_mreq));

	// 检查错误代码;
	if (Check_WSAError(sockopt_value, __FUNCTION__))
	{
		return -1;
	}

	return 0;
}

void OS_McastSock::Close()
{
	if ((int)hSock >= 0)
	{
		// shutdown(hSock, SD_BOTH);
		shutdown(hSock, 2);
		socket_close(hSock);
		hSock = -1;
	}
	else if (Check_WSAError(hSock, __FUNCTION__))	//检查错误代码;
	{
		return ;
	}
}

#if 0
int OS_McastSock::SendTo(const char* buf, int len, const OS_SockAddr& peer)
{
	socklen_t addr_len = sizeof(sockaddr_in);
	int n = socket_sendto(hSock, buf, addr_len, 0, (const sockaddr*)&peer, addr_len);

	// 检查错误代码;
	if (Check_WSAError(n, __FUNCTION__))
	{
		return -1;
	}

	return n;
}
#endif

int OS_McastSock::RecvFrom(void* buf, int max_len, const OS_SockAddr& peer)
{
	socklen_t addr_len = sizeof(sockaddr_in);
	int n = socket_recvfrom(hSock, (char*)buf, max_len, 0, (sockaddr*)&peer, &addr_len);

	// 检查错误代码;
	if (Check_WSAError(n, __FUNCTION__))
	{
		return -1;
	}

	return n;
}

/*--------------------------------------------*/

static int Check_WSAError(int sockopt, const char* function)
{
#ifdef _WIN32
	if (sockopt == SOCKET_ERROR || sockopt == INVALID_SOCKET)
	{
		DWORD dwError = WSAGetLastError();	//检查错误代码;
		printf("%s Function Error ! \n", function);
		printf("dwError: %u \n", dwError);

		return -1;
	}
#else
	if (sockopt < 0)
	{
		printf("%s Function Error ! \n", function);

		return -1;
	}
#endif

	return 0;
}

#ifndef _WIN32
void Sleep(int ms)
{
	timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&ts, NULL);
}
#endif

