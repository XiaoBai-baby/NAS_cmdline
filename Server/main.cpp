#include "osapi/osapi.h"
#include "tcpserver/TcpServer.h"

int main()
{
	TcpServer tcpServer;
	tcpServer.startService(8888);

	while (1) { OS_Thread::Msleep(1); }

	return 0;
}