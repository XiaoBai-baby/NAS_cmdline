#ifndef _OFF_LINE_SERVICE_H
#define _OFF_LINE_SERVICE_H

#include <stdio.h>
#include "../osapi/osapi.h"
#include "../nas/MsgDefine.h"
#include "../nas/fileCheck.h"
#include "../utility/FileUtils.h"

// STL 模板
#include <vector>
#include <string>

using std::vector;
using std::string;

/* offLineService
	离线服务器, 必须定义在全局变量上, 并配合二次初始化使用;
	注意, 不可以将其声明为 static类型, 这将使变量的定义范围限制为当前对象文件, 并允许多个文件副本同时存在;
*/
class offLineService
{
public:
	offLineService();
	offLineService(OS_TcpSocket sock, string homeDir);
	~offLineService();

public:
	// 类的二次初始化, 用于解锁全部的离线服务;
	void operator()(OS_TcpSocket sock, string homeDir);

public:
	// 类的算数运算符;
	void operator++();
	void operator--();

	void operator+=(int i);
	void operator-=(int i);

	int operator+(int i);
	int operator-(int i);

public:
	// 添加离线移动文件;
	void offLineMove(vector<string>& move);

	// 添加离线拷贝文件;
	void offLineCopy(vector<string>& copy);

	// 添加离线删除文件;
	void offLineRemove(string complete_path);

public:
	// 返回离线服务处理的数量;
	int size();

	// 返回在线服务处理的数量;
	int user_size();

public:
	// 判断是否为目录, 辅助 offLineHandler, move_copy_file使用;
	bool isDirectory(string complete_path);

	// 判断文件或目录是否存在, 辅助 destinationFile, on_mkdir, on_mv, on_cp使用;
	int isExistFile(string complete_path);

	// 创建或 删除临时文件, 用来保存命令行的操作结果, 辅助 offLineHandler, MoveDirectory使用;
	string temporaryFile(string rm_cmdout = "");

public:
	// 所有离线服务的处理, 辅助 serviceHandler使用;
	int offLineHandler();

public:
	int is_using;							// 在线服务处理的数量;
	int off_line_number;					// 离线服务处理的数量;

private:
	vector<string> off_line_remove;			// 用于离线删除文件;
	vector<string> off_line_move;			// 用于离线移动文件;
	vector<string> off_line_copy;			// 用于离线复制文件;
	
private:
	string m_homeDir;						// 根目录所在的位置

private:
	OS_TcpSocket Sock;						// 连接客户端的socket地址
	fileCheck m_fileCheck;					// 检查文件或目录
};

#endif