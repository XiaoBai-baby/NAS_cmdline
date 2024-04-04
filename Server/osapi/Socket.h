#ifndef _OSAPI_SOCKET_H
#define _OSAPI_SOCKET_H

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <string>
using std::string;

#ifdef _WIN32

// Windows 下的socket定义
#include <WinSock2.h>
#include <ws2tcpip.h>

typedef SOCKET socket_t;

#define socket_open socket
#define socket_close closesocket
#define socket_ioctl ioctlsocket

#else

// Linux 下的socket定义
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>

typedef int socket_t;

#define socket_open socket
#define socket_close close
#define socket_ioctl ioctl

// 重新定义Linux下的Sleep函数;
void Sleep(int ms);

#endif

// Windows 和 Linux 下的socket定义
#define socket_accept accept
#define socket_bind bind
#define socket_connect connect
#define socket_listen listen
#define socket_send send
#define socket_recv recv
#define socket_sendto sendto
#define socket_recvfrom recvfrom
#define socket_select select
#define socket_getsockopt getsockopt
#define socket_setsockopt setsockopt

// 检查WSA错误代码;
static int Check_WSAError(int sockopt, const char* function);

/* OS_Socket:
	*) 这是一个简单类, 用户可以直接赋值传递;
	*) 子类没有析构函数, 用户需要显示的关闭Close();
	*) 用户可以直接操作Socket：hSock;
*/

// 负责存放地址的类
// 该类可以直接强制转化为sockaddr_in 和 sockaddr结构
class OS_SockAddr
{
public:
	explicit OS_SockAddr();
	explicit OS_SockAddr(const char* ip, unsigned short port);
	explicit OS_SockAddr(const char* ip);	// 默认端口为0;
	explicit OS_SockAddr(unsigned short port);	 // 默认IP地址为0.0.0.0
	explicit OS_SockAddr(sockaddr_in addr);

	// 设置IP地址与端口;
	void SetIp(const char* ip);
	void SetIp(unsigned int ip);
	void SetIp(unsigned short port);

	string GetIp_str() const;
	unsigned int GetIp_n() const;
	unsigned short GetPort() const;

public:
	sockaddr_in iAddr;		// 存放IP地址等网络信息;
};

class OS_Socket
{
public:
	OS_Socket();

	// ms = 0时则永不超时, ms = 1 可以认为是立即返回 (1ms很快完成)
	int SetOpt_RecvTimeout(int ms);
	int SetOpt_SendTimeout(int ms);

	// 返回接收与发送的超时时间; 该选项的默认值为零，表示接收与发送操作都不会超时;
	int GetOpt_RecvTimeout();
	int GetOpt_SendTimeout();

	// 用于设置套接字的I/O模式;
	int Ioctl_SetBlockedIo(bool blocked);

	// 用于设置套接字选项, 一般用来设置套接字接收和发送的系统缓存大小;
	void Set_SockOption(int optname, const char* bufsize);

	// 设置套接字绑定的端口是否重复使用; 一般设置为"否"来防止恶意程序“拒绝服务”攻击或数据盗窃;
	int SetOpt_ReuseAddr(bool reuse = false);

	// 检索到套接字对方的地址;
	int GetPeerAddr(OS_SockAddr& addr) const;

	// 检索到套接字的本地地址;
	int GetLocalAddr(OS_SockAddr& addr) const;

	// select机制：查询读写状态
	// 返回值大于0, 表示可以读或写; 等于0表示超时; 小于0表示Socket不可用或错误.
	int Select_ForReading(int timeout);
	int Select_ForWriting(int timeout);

protected:
	socket_t hSock;		//套接字描述符, 可以直接访问这个socket
};

class OS_TcpSocket : public OS_Socket
{
public:
	// 创建Socket套接字
	int Open(bool reuse = false);
	int Open(const OS_SockAddr& addr, bool reuse = false);

	// 关闭Socket, 需手动关闭;
	void Close();

	// 服务器
	int Listen(int backlog = 100);
	int Accept(OS_TcpSocket& peer);

	// 客户端
	int Connect(const OS_SockAddr& addr);

	// 发送接收
	int Send(const void* buf, int len, bool OOB_flag = false);
	int Recv(void* buf, int len, bool OOB_flag = false);
	int Recv_OOB(int OOB_flag = 1);
};

class OS_UdpSocket : public OS_Socket
{
public:
	// 创建Socket套接字
	int Open(bool reuse = false);
	int Open(const OS_SockAddr& addr, bool reuse = false);

	// 关闭Socket, 需手动关闭;
	void Close();

	// 发送接受
	int SendTo(const char* buf, int len, const OS_SockAddr& peer);
	int RecvFrom(void* buf, int len, const OS_SockAddr& peer);
};

class OS_McastSock :public OS_Socket
{
public:
	// 创建Socket套接字
	int Open(const char* mcast_ip, int port, const char* local_ip);

	// 关闭Socket, 需手动关闭;
	void Close();

	/* 发送多播时, 使用普通UdpSock + 多播路由即可 */
	//int SendTo(const char* buf, int len, const OS_SockAddr& peer);
	int RecvFrom(void* buf, int max_len, const OS_SockAddr& peer);

private:
	ip_mreq m_McReq;	//多播地址
};

#endif