#ifndef _UTILITY_H
#define _UTILITY_H

#ifndef _CRT_SECCUSS_NO_WARNINGS
#define _CRT_SECCUSS_NO_WARNINGS
#endif	// !_CRT_SECURE_NO_WARNINGS

// 禁用Visual C++中的 min/max宏定义
#define NOMINMAX

#include <cstdio>
#include <cstring>

// STL扩展模板
#include <string>
#include <list>

#include "Endian.h"
#include "FileUtils.h"
#include "ByteBuffer.h"
#include "ThreadTcpRecv.h"
#include "CharacterEncoding.h"
using namespace std;


#endif