#include "osapi/osapi.h"
#include "ftp/FtpClient.h"
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
	
	// 用封装好的数据包发送数据
	FtpClient ftp(client_sock, 1024);
	// ftp.SendData(MSG_LOGIN, "root", "123456");

	#ifndef _WIN32
		system("clear");
	#endif
	
	string user;
	user.append("我ss1_ ");
	user.append("123456");
	
	#ifndef _WIN32
		system("clear");
	#endif
	
	ftp.SendData(MSG_LOGIN2, user.c_str());

	ftp.Start();

	// 关闭socket
	client_sock.Close();

	return 0;
}
