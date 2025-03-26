#ifndef _FTP_USER_H
#define _FTP_USER_H

#include <stdio.h>
#include <list>
#include <string>
#include <algorithm>

using std::list;
using std::string;

/*
	FtpUser
	存放所有用户的信息
*/
class FtpUser
{
public:
	FtpUser();

public:
	// 用户的登录检查;
	void UserDetection(string& username, string& password);

	// 检查是否为管理员权限;
	bool AdminDetection(string& username, string& password);

private:
	// 所有用户组的合集;
	void UserManagement();

	// 用户组1, 用于存放管理员信息;
	void FtpGroup1();

	// 用户组2, 用于存放所有用户信息;
	void FtpGroup2();

private:
	list<string> m_Root;			// 管理员信息
	list<string> m_User;			// 所有用户信息
};

#endif