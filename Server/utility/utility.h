#ifndef _UTILITY_H
#define _UTILITY_H

#ifndef _CRT_SECCUSS_NO_WARNINGS
#define _CRT_SECCUSS_NO_WARNINGS
#endif	// !_CRT_SECURE_NO_WARNINGS

// 禁用Visual C++中的 min/max宏定义
#define NOMINMAX

#include <stdio.h>
#include <string.h>

// STL扩展模板
#include <string>
#include <list>

#include "Endian.h"
#include "FileUtils.h"
#include "ByteBuffer.h"
#include "ThreadTcpRecv.h"
#include "CharacterEncoding.h"

// 在部分项目升级时往往会与rpcndr.h中的byte定义冲突，导致编译失败;
// 最安全的方法是：编写健壮的工业级代码从弃用using namespace语句开始;
// using namespace std;
using std::string;
using std::list;


#endif