#ifndef _CHARACTER_ENCODING_H
#define _CHARACTER_ENCODING_H

#include <iostream>
#include <string>
#include <string.h>
#include <stdlib.h>

using namespace std;

/* CharacterEncoding 字符集编码转换器
		用于实现utf8和gbk编码字符串互相转换
*/

#ifdef _WIN32
#include <windows.h>

string GbkToUtf8(const char* src_str);

string Utf8ToGbk(const char* src_str);

#else

#include <iconv.h>

int GbkToUtf8(char* str_str, size_t src_len, char* dst_str, size_t dst_len);

int Utf8ToGbk(char* src_str, size_t src_len, char* dst_str, size_t dst_len);

#endif

#endif