#include "TcpServiceTask.h"
#include "../nas/nasServer.h"
#include "../utility/ByteBuffer.h"

/*
	假设有一个业务服务函数;
*/

int TcpServiceTask::BusinessService()
{	
	nasServer nas(m_clientSock, (char*)m_buf, 0, m_bufsize);
	nas.startReceiveData();

	return 0;
}