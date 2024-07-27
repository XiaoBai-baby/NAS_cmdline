#ifndef _MSG_DEFINE_H
#define _MSG_DEFIND_H

#include <string>
using namespace std;

/*
	定义数据消息的类型
*/

#define MSG_LOGIN				0x0101
#define MSG_LOGIN2				0x0102
#define MSG_LIST				0x0103
#define MSG_LIST2				0x0104
#define MSG_CD					0x0105
#define MSG_GET					0x0106
#define MSG_HELP				0x0108
#define MSG_EXIT				0x0109

#define ACK_NEW_DIRECTORY		0x0201
#define ACK_NEW_FILE			0x0202
#define ACK_NEW_FINISH			0x0203
#define ACK_DATA_SIZE			0x0204
#define ACK_DATA_CONTINUE		0x0205
#define ACK_DATA_FINISH			0x0206
#define ACK_PATH_END			0x0207
#define ACK_FINISH				0x0208

#endif