#ifndef _FTP_FILE_HANDLER
#define _FTP_FILE_HANDLER

#include "MsgDefine.h"
#include "../osapi/osapi.h"
#include "../utility/FileUtils.h"
#include "../jsoncpp/include/json.h"
#include "../utility/CharacterEncoding.h"

// STL 模板
#include <string>
#include <fstream>
#include <algorithm>
using namespace std;

#ifdef _WIN32

#include <io.h>
#include <direct.h>

#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#endif

/* FileHandler
	文件处理器, 用于处理文件操作;
*/

class FileHandler
{
public:
	FileHandler() {};

	FileHandler(OS_TcpSocket RecvSock, string homeDir, bool is_win, int size = 1024 * 1024);

	// 该模板函数 用于类的二次初始化, 已确认客户端的操作系统;
	void operator()(bool is_windows);

	// 该模板函数 用于类的二次初始化, 已指定下载文件的输出目录;
	void operator()(const char* homeDir);

	// 该模板函数 用于类的二次初始化, 已指定下载文件的输出目录;
	void operator()(string homeDir);

	// 该模板函数 用于类的初始化;
	void operator()(OS_TcpSocket RecvSock, string homeDir, bool is_win, int size = 1024 * 1024);

private:
	// 处理MSG_LIST 消息请求, 辅助 CheckResult使用;
	string on_ls(Json::Value& jresult, string& path, bool notDir = false);

	// 处理MSG_LIST2 消息请求, 辅助 CheckResult使用;
	string on_ll(Json::Value& jresult, string& path, bool notDir = false);

public:
	// 在Linux下, 处理MSG_LIST, MSG_LIST2 消息请求, 辅助 on_ls使用;
	string ls_linux(unsigned int type, Json::Value& jresult, string& path);

private:
	// 检查 cd 命令, 辅助 CheckCommand使用;
	void CheckCdCommand(string& result, string& path, string& part);

	// 检查命令行的命令, 辅助 CheckFile使用;
	void CheckCommand(string& result, string& path, string& part);

	// 显示检查文件或目录的结果, 辅助 CheckFile使用;
	string CheckResult(Json::Value& jresult, string path, string part);
	
	bool IsDirectory(string path, string part);

public:
	// 发送 ACK 序号, 用来标识 向目的端发送文件 的消息请求和字节流;
	int ACK_Send(unsigned short type, const void* data = 0, unsigned int length = 0);

	// 接收 ACK 序号, 用来接收 向目的端接收文件 的消息请求和字节流;
	int ACK_Recv();

private:
	// 接收文件的消息处理, 辅助 RecvFile使用;
	void RecvHandler(string& path, string& part);

public:
	// 检查文件或目录 是否存在;
	string CheckFile(string& path, char* data, unsigned int type, bool isClient = false);

	// 发送文件;
	void SendFile(string path, char** argv, int argc, OS_Mutex& Mutex);

	// 接收文件;
	void RecvFile(string path, OS_Mutex& Mutex);

private:
	// 接收数据
	int ReceiveN(void* buf, int count, int timeout = 0);
	
private:
	FILE* fp;							// 文件指针
	long long fileSize;					// 文件大小
	string m_homeDir;					// 根目录所在的位置

private:
	bool isWin;							// 判断服务端的系统
	bool isWindows;						// 判断客户端的系统
	
	char* m_data;						// 接收数据的缓冲区
	int m_size;							// 接收数据的缓冲区大小
	unsigned int m_type;				// 接收消息的类型
	unsigned int m_length;				// 接收消息的长度

private:
	OS_TcpSocket Sock;					// 连接客户端的socket地址
};

#endif