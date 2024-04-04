#ifndef _FTP_SERVICE_H
#define _FTP_SERVICE_H

#include "./MsgDefine.h"
#include "../osapi/osapi.h"
#include "../utility/utility.h"
#include "../jsoncpp/include/json.h"

#include <fstream>
#include <algorithm>
using namespace std;

#ifdef _WIN32
#include <io.h>

#else
#include <unistd.h>
#include <fcntl.h>

#endif

/*	FtpService
	实现类似 FTP的服务
*/

class FtpService
{
public:
	FtpService(OS_TcpSocket& recv_sock, int bufsize = 8192);

	FtpService(OS_TcpSocket& recv_sock, char* buffer, int offset, int bufsize);

	~FtpService();

public:
	// 开始接收任务;
	void startReceiveData();

	// 清空数据;
	void Clear();

private:
	// 接收消息类型;
	int ReceiveMessages();

	// 处理MSG_LOGIN 消息请求;
	int on_Login(const string& jsonreq);

	// 处理MSG_LOGIN2 消息请求, 接受中文传输;
	int on_Login2();

	// 处理MSG_LIST 消息请求;
	string on_ls(Json::Value& jresult, bool notDir = false);

	// 处理MSG_LIST2 消息请求;
	string on_ll(Json::Value& jresult, bool notDir = false);

	// 处理MSG_CD 消息请求;
	string on_cd(Json::Value& jresult);

private:
	// 显示检查文件或目录的结果, 辅助 CheckFile使用;
	string CheckResult(Json::Value& jresult, string& path);

	// 检查文件或目录 是否存在;
	string CheckFile();

	string LinuxHandler();

	// 所有消息类型的处理;
	int MessageHandler(string& result, string& reason, Json::Value file_block);

	// 响应客户端操作;
	int ResponseClient();

private:
	// 接收数据
	int ReceiveN(void* buf, int count, int timeout = 0);

private:
	string m_homeDir;				// 根目录所在的位置
	string m_path;					// 用户的相对目录

	string username;				// 客户端的用户名
	string password;				// 客户端的密码

private:
	bool login_OK;					// 登录是否成功
	bool exit_OK;					// 退出是否成功
	
	bool isWindows;					// 判断客户端的系统

private:
	char* m_data;					// 接收消息的数据
	unsigned int m_type;			// 接收消息的类型
	unsigned int m_length;			// 接收消息的数据长度

private:
	int m_bufsize;					// 接收缓冲区大小
	ByteBuffer m_buffer;			// 缓存数据的字节编码器

private:
	OS_Mutex m_Mutex;				// 控制接收数据的互斥锁
	OS_TcpSocket m_RecvSock;		// 连接客户端的socket地址
	OS_SockAddr m_SockAddr;			// 客户端的IP地址
};

#endif