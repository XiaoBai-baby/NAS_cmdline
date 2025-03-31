#ifndef _FTP_FILE_HANDLER
#define _FTP_FILE_HANDLER

// osapi.h 必须写在所有头文件的前面, 否则会出现编译冲突 (如: E0040, E0338, E1389, C2011 等等);
#include "../osapi/osapi.h"

// 目录文件操作;
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "MsgDefine.h"
#include "FileCheck.h"
#include "../utility/FileUtils.h"
#include "../jsoncpp/include/json.h"

// STL 模板
#include <string>
#include <fstream>
#include <algorithm>

using std::string;
using std::fstream;

/* FileHandler
	文件处理器, 用于处理文件操作;
*/
class FileHandler
{
public:
	FileHandler();

	FileHandler(OS_TcpSocket RecvSock, string homeDir, char local_system, bool isClient = false, int size = 1024 * 1024 * 300);

	// 该模板函数 用于类的二次初始化, 以确认对端的操作系统;
	void operator()(char peerTheSystem);

	// 该模板函数 用于类的二次初始化, 以指定下载文件的输出目录;
	void operator()(const char* homeDir);

	// 该模板函数 用于类的二次初始化, 以指定下载文件的输出目录;
	void operator()(string homeDir);

	// 该模板函数 用于类的初始化;
	void operator()(OS_TcpSocket RecvSock, string homeDir, char local_system, bool isClient = false, int size = 1024 * 1024 * 300);

public:
	// 在Linux下, 添加文件的路径, 辅助 ls_linux2使用;
	string add_directory(Json::Value& jresult, char* file_info[], int argc, int& directory_path, int& count);

	// 在Linux下, 处理MSG_LIST, MSG_LIST2 消息请求, 辅助 on_ls使用;
	string ls_linux(unsigned int type, Json::Value& jresult, string& path);

	// 在Linux下, 遍历全部文件, 辅助 Linux_PathHandler使用;
	void ls_linux2(unsigned int type, Json::Value& jresult, string& path);

public:
	// 发送 ACK 序号, 用来标识 向目的端发送文件 的消息请求和字节流;
	int ACK_Send(unsigned short type, const void* data = 0, unsigned int length = 0);

	// 接收 ACK 序号, 用来接收 向目的端接收文件 的消息请求和字节流;
	int ACK_Recv();

private:
	// 接收文件的消息处理, 辅助 DownloadFile使用;
	void RecvHandler(string& path, string& part);

private:
	// 发送单个文件, 辅助 UploadFile使用;
	void SendFile(string& path, string& part, bool linux_notDir = false);

	// 发送单个指令下的全部目录, 辅助 UploadFile使用;
	void SendDirectory(Json::Value& jresult, string& path, string& part, char* argv2[], OS_Mutex& Mutex);

	// 处理发送文件的相对目录, 辅助 Linux_SendFile使用;
	bool Linux_PathHandler(Json::Value& jresult, string& path);

	// 以 Linux 的方式, 发送全部文件, 辅助 UploadFile使用;
	void Linux_SendFile(Json::Value& jresult, string& part, OS_Mutex& Mutex);

	// 获取当前的工作目录, 辅助  UploadHandler使用;
	void AcquireDirectory(string& path);

	// 上传文件的前后处理, 辅助 UploadFile使用;
	void UploadHandler(string& path, char* argv[], int argv_count = 0);

public:
	// 检查文件或目录 是否存在;
	string CheckFile(string& path, char* data, unsigned int type);

	// 中文字符集转换, 用于外部函数调用;
	string CharacterEncoding(char* src_str, int src_length);

	// 上传文件;
	void UploadFile(string path, char** argv, int argc, OS_Mutex& Mutex);

	// 下载文件;
	void DownloadFile(string path, OS_Mutex& Mutex);

private:
	// 接收数据
	int ReceiveN(void* buf, int count, int timeout = 0);

private:
	FILE* fp;							// 文件指针
	long long fileSize;					// 文件大小

	string m_homeDir;					// 根目录所在的位置
	string m_path;						// 用户的相对目录

private:
	char* m_data;						// 接收数据的缓冲区
	int m_size;							// 接收数据的缓冲区大小
	unsigned int m_type;				// 接收消息的类型
	unsigned int m_length;				// 接收消息的长度

public:
	string fileError;					// 判断传输文件是否错误;

private:
	bool windows_transfer;				// 判断是否需要以 Windows方式进行文件传输;
	int directory_distances;			// 发送目录与相对目录之间的关系系数, 即目录的距离;
	int cmdline_count;					// 客户端输入子命令时, 返回上一级目录的数量, 即 "/" 的数量;

private:
	OS_TcpSocket Sock;					// 连接客户端的socket地址
	FileCheck fileCheck;				// 检查文件或目录
};

#endif