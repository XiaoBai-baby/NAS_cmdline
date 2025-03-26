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

// 在部分项目升级时往往会与rpcndr.h中的byte定义冲突，导致编译失败;
// 最安全的方法是：编写健壮的工业级代码从弃用using namespace语句开始;
// using namespace std;
using std::string;
using std::iostream;

#endif