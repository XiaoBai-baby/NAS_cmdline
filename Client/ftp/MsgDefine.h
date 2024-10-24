#ifndef _MSG_DEFINE_H
#define _MSG_DEFIND_H

#include <string>
using std::string;

/*
	定义数据消息的类型
*/

#define MSG_LOGIN				0x0101								// 登录请求
#define MSG_LOGIN2				0x0102								// 登录请求 2.0, 可支持中文
#define MSG_LIST				0x0103								// 'ls' 指令, 列出 文件名
#define MSG_LIST2				0x0104								// 'll' 指令, 列出 文件信息
#define MSG_CD					0x0105								// 'cd' 指令, 用于 切换目录
#define MSG_GET					0x0106								// 'get' 指令, 用于 下载文件
#define MSG_HELP				0x0108								// 'help' 指令, 用于 获取指令集
#define MSG_EXIT				0x0109								// 'exit' 指令, 用于 结束会话


/* 传输文件的指令集 */
#define ACK_NEW_DIRECTORY		0x0201								// 创建目录
#define ACK_NEW_FILE			0x0202								// 创建文件
#define ACK_NEW_FINISH			0x0203								// 当前目录中, 一个目录分支树创建完成
#define ACK_DATA_SIZE			0x0204								// 单个文件的大小
#define ACK_DATA_CONTINUE		0x0205								// 发送文件数据
#define ACK_DATA_FINISH			0x0206								// 发送文件完成
#define ACK_PATH_END			0x0207								// 一个目录分支树中的所有文件创建完成
#define ACK_FINISH				0x0208								// 单个文件创建完成

#endif