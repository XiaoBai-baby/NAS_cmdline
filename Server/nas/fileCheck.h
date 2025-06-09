#ifndef _FILE_CHECK_H
#define _FILE_CHECK_H

#include "MsgDefine.h"
#include "../osapi/osapi.h"
#include "../utility/FileUtils.h"
#include "../jsoncpp/include/json.h"
#include "../utility/CharacterEncoding.h"

// STL 模板
#include <string>
#include <fstream>
#include <algorithm>
using std::string;

#ifdef _WIN32

#include <io.h>
#include <direct.h>

#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#endif

/* fileCheck
	文件检查器, 用于辅助 FileHandler类 处理文件;
*/
class fileCheck
{
public:
	fileCheck();

	fileCheck(OS_TcpSocket& Sock, string homeDir, char local_system, bool isClient = false);

public:
	// 该模板函数 用于类的二次初始化, 以确认客户端的操作系统;
	void operator()(char peerTheSystem);

	// 该模板函数 用于类的二次初始化, 以指定下载文件的输出目录;
	void operator()(const char* homeDir);

	// 该模板函数 用于类的二次初始化, 以指定下载文件的输出目录;
	void operator()(string homeDir);

	// 该模板函数 用于类的初始化;
	void operator()(OS_TcpSocket& Sock, string homeDir, char local_system, bool isClient);

public:
	// 处理MSG_LIST 消息请求, 辅助 checkResult使用;
	string on_ls(Json::Value& jresult, string& path, bool notDir = false);

	// 处理MSG_LIST2 消息请求, 辅助 checkResult使checkCdCommand用;
	string on_ll(Json::Value& jresult, string& path, bool notDir = false);

	// 在Linux下, 处理MSG_LIST, MSG_LIST2 消息请求, 辅助 on_ls使用;
	string ls_linux(unsigned int type, Json::Value& jresult, string& path);

private:
	// 检查 cd 命令, 辅助 checkCommand使用;
	void checkCdCommand(string& result, string& path, string& part);

	// 检查命令行的命令, 辅助 checkFile使用;
	void checkCommand(string& result, string& path, string& part);

	// 显示检查文件或目录的结果, 辅助 checkFile使用;
	string checkResult(Json::Value& jresult, string path, string part);

	// 判断文件或目录是否存在, 辅助 checkFile使用;
	string isExistFile(string path, string part, bool put_flag = false);

	// 中文字符集转换, 辅助 Utf8ToGBK, GBKToUtf8使用;
	string characterEncoding(const char* src_str, bool direct_convert = false);

	// 中文字符集转换, 辅助 characterEncoding使用;
	string Utf8ToGBK(char* src_str, int src_length);

	// 中文字符集转换, 辅助 characterEncoding使用;
	string GBKToUtf8(char* src_str, int src_length);

public:
	// 判断是否为目录, 辅助 checkFile uploadFile使用;
	bool isDirectory(string path, string part);

	// 中文字符串转换, convert 为调用标识, 用于 fileCheck 类内部计算资源的节约;
	string characterEncoding(char* src_str, int src_length, bool convert = true);

public:
	// 检查文件或目录 是否存在;
	string checkFile(string& path, char* data, unsigned int type);

public:
	char local_system;					// 本地的操作系统
	char peer_system;					// 对端的操作系统

	bool isClient;						// 判断是否为客户端

public:
	string m_homeDir;					// 根目录所在的位置
	unsigned int m_type;				// 接收消息的类型

private:
	OS_TcpSocket Sock;					// 连接客户端的socket地址
};

#endif