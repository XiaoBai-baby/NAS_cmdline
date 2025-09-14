#ifndef _MSG_DEFINE_H
#define _MSG_DEFIND_H

#include <string>
using std::string;

/*
	定义数据消息的类型
*/

#define MSG_LOGIN					0x0101							// 登录请求
#define MSG_LOGIN2					0x0102							// 登录请求 2.0, 可支持中文
#define MSG_LIST					0x0103							// 'ls' 指令, 列出 文件名
#define MSG_LIST2					0x0104							// 'll' 指令, 列出 文件信息
#define MSG_CD						0x0105							// 'cd' 指令, 用于 切换目录
#define MSG_REMOVE					0x0106							// 'rm' 指令, 用于 删除目录 或文件
#define MSG_REMOVE2					0x0107							// 'rm' 指令, 用于 离线删除
#define MSG_MAKEDIR					0x0108							// 'mkdir'指令, 用于 创建目录
#define MSG_MOVE					0x0109							// 'mv' 指令, 用于 移动目录 或文件
#define MSG_MOVE2					0x0110							// 'mv' 指令, 用于 离线移动
#define MSG_COPY					0x0111							// 'cp' 指令, 用于 复制目录 或文件
#define MSG_COPY2					0x0112							// 'cp' 指令, 用于 离线复制
#define MSG_PWD						0x0113							// 'pwd' 指令, 用于 打印工作目录
#define MSG_GET						0x0120							// 'get' 指令, 用于 下载文件
#define MSG_PUT						0x0121							// 'put' 指令, 用于 上传文件
#define MSG_HELP					0x0122							// 'help' 指令, 用于 获取指令集
#define MSG_EXIT					0x0123							// 'exit' 指令, 用于 结束会话

/* json code, 判断响应是否成功 */
#define	JSON_CD						0x0201							// 'cd' 指令响应成功
#define JSON_REMOVE					0x0202							// 'rm' 指令响应成功
#define JSON_REMOVE2				0x0203							// 'rm' 指令需要重新响应, 以发送 离线删除 的命令
#define JSON_MOVE					0x0204							// 'mv' 指令响应成功
#define JSON_MOVE2					0x0205							// 'mv' 指令需要重新响应, 以发送 离线移动 的命令
#define JSON_COPY					0x0206							// 'cp' 指令响应成功
#define JSON_COPY2					0x0207							// 'cp' 指令需要重新响应, 以发送 离线复制 的命令
#define JSON_PWD					0x0208							// 'pwd' 指令响应成功
#define JSON_GET					0x0209							// 'get' 指令响应成功
#define JSON_PUT					0x0210							// 'put' 指令响应成功
#define JSON_LOGIN_AND_LIST			0x0211							// 'login', 'ls', 'll' 指令 响应成功
#define JSON_ERROR					0x0222							// 指令响应失败


/* 传输文件的指令集 */
#define ACK_NEW_DIRECTORY			0x0301							// 创建目录
#define ACK_NEW_FILE				0x0302							// 创建文件
#define ACK_NEW_FINISH				0x0303							// 当前目录中, 一个目录分支树创建完成
#define ACK_DATA_SIZE				0x0304							// 单个文件的大小
#define ACK_DATA_CONTINUE			0x0305							// 发送文件数据
#define ACK_DATA_FINISH				0x0306							// 发送文件完成
#define ACK_PATH_END				0x0307							// 一个目录分支树中的全部文件发送完成
#define ACK_FINISH					0x0308							// 全部文件发送完成

#endif