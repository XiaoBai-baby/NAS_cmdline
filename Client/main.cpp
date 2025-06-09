#include "osapi/osapi.h"
#include "nas/nasClient.h"
#include "utility/utility.h"

int main()
{
	// 创建socket
	OS_TcpSocket client_sock;
	client_sock.Open();

	// 服务器的IP地址
	const char* server_ip = "127.0.0.1";
	int port = 8888;

	// 连接服务器
	if (client_sock.Connect(OS_SockAddr(server_ip, port)) < 0)
	{
		printf("Cannot connect to the server (%s: %d) !", server_ip, port);
		return -1;
	}
	
	// 启动服务;
	nasClient nas(client_sock, 1024);
	nas.start("我ss1_ ", "123456");

	// 关闭socket
	client_sock.Close();

	return 0;
}
