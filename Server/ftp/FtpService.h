#ifndef _FTP_SERVICE_H
#define _FTP_SERVICE_H

#include "./FtpUser.h"
#include "./MsgDefine.h"
#include "./FtpFileHandler.h"

#include "../osapi/osapi.h"
#include "../utility/utility.h"
#include "../jsoncpp/include/json.h"

#include <sys/stat.h>

#include <fstream>
#include <string>
#include <algorithm>

using std::fstream;
using std::string;

#ifdef _WIN32

#include <io.h>
#include <direct.h>

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

	// 检查ftp目录是否存在;
	void CheckDirectory();

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
	
	// 处理MSG_GET 消息请求;
	string on_get();

	// 处理MSG_PUT 消息请求;
	string on_put();

private:

	// 检查 m_path 中的字符, 辅助 on_cd使用;
	void CheckCd();

	// 处理 CD 命令, 辅助 on_cd使用;
	string HandleCd(string& directory, int& find_pos, int& count, int& position, int (*argv)[128]);				// int(*p)[n] 为数组指针(也称为行指针)

	// 求数组的最小差值, 辅助 HandlerDir使用;
	int Difference_mininum(int arr[], int count);

	// 处理 用户的目录操作, 续 HandleCd 之后使用;
	void HandlerDir(int& count, bool& isII, int (*argv)[128]);

	// 返回 on_cd 消息请求的结果, 辅助 on_cd使用;
	string HandleCdResult(string directory, string last_directory, int position, bool isII);
	
	// 用于处理 Linux 系统的消息, 辅助 MessageHandler使用;
	string LinuxHandler();
	
private:
	// 所有消息类型的处理;
	int MessageHandler(string& result, string& reason, Json::Value file_block);
	
	// 所有服务类型的处理;
	int ServiceHandler();
	
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
	
	char client_system;				// 判断客户端的系统
	char server_system;				// 判断服务端的系统

private:
	char* m_data;					// 接收消息的数据
	unsigned int m_type;			// 接收消息的类型
	unsigned int m_length;			// 接收消息的数据长度

private:
	int m_bufsize;					// 接收缓冲区大小
	ByteBuffer m_buffer;			// 缓存数据的字节编码器
	FtpUser m_user;					// 存放所有用户的信息
	FileHandler m_file;				// 操作文件的文件处理器

private:
	OS_Mutex m_Mutex;				// 控制接收数据的互斥锁
	OS_TcpSocket m_RecvSock;		// 连接客户端的socket地址
	OS_SockAddr m_SockAddr;			// 客户端的IP地址
};

#endif