#ifndef _TRANSFER_PARAMETERS_H
#define _TRANSFER_PARAMETERS_H

#include "stdio.h"
#include <string>
using std::string;

/*
	NAS_Cmdline 传输参数;
*/
struct transferParameters
{
	// 设置 NAS 的根目录;
	string homeDir = "";

	// 命令行参数的最大数量;
	int maxCmdlineParameters = 8;

	// 最大文件上传单元;
	long int maxUploadUnit = 1024;

	// 最大文件上传大小;
	long long maxUploadSize = (long long) 8 * 1024 * 1024 * 1024;
};

#endif