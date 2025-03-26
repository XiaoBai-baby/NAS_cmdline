#include "FtpUser.h"

FtpUser::FtpUser()
{
	UserManagement();
}

void FtpUser::UserDetection(string& username, string& password)
{
	if (AdminDetection(username, password))	return;
	
	int count = 0;
	for (list<string>::iterator it = m_User.begin(); it != m_User.end(); it++)
	{
		count++;
		if (count <= m_Root.size())
			continue;
		
		if (*it == username)
		{
			it++;

			if (*it == password)
				return ;
			else
				throw string("bad password!");
		}

		it++;
	}

	throw string("bad username!");
}

bool FtpUser::AdminDetection(string& username, string& password)
{
	for (list<string>::iterator it = m_Root.begin(); it != m_Root.end(); it++)
	{
		if (*it == username)
		{
			it++;

			if (*it == password)
				return true;
			else
				throw string("bad password!");
		}

		it++;
	}

	return false;
}


void FtpUser::UserManagement()
{
	FtpGroup1();

	FtpGroup2();
}

void FtpUser::FtpGroup1()
{
	m_Root.push_back("root");
	m_Root.push_back("123456789");

	m_Root.push_back("user01");				// username
	m_Root.push_back("123456");				// password

	m_Root.push_back("user02");
	m_Root.push_back("123456");
}

void FtpUser::FtpGroup2()
{
	m_User = m_Root;

	m_User.push_back("我ss1_");				// username
	m_User.push_back("123456");				// password
}