#ifndef _OSAPI_H
#define _OSAPI_H

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <string>
#include <iostream>

#include "Socket.h"
#include "Thread.h"
#include "Mutex.h"
#include "Semaphore.h"

#include "TcpConnect.h"

// �ڲ�����Ŀ����ʱ��������rpcndr.h�е�byte�����ͻ�����±���ʧ��;
// �ȫ�ķ����ǣ���д��׳�Ĺ�ҵ�����������using namespace��俪ʼ;
// using namespace std;
using std::string;
using std::iostream;

#endif