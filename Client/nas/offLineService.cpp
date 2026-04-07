#include "offLineService.h"

offLineService::offLineService()
{
	is_using = 0;
	off_line_number = 0;
}

offLineService::offLineService(OS_TcpSocket sock, string homeDir) : Sock(sock), m_homeDir(homeDir)
{
	is_using = 0;
	off_line_number = 0;

	m_fileCheck(homeDir);
}

offLineService::~offLineService()
{
	off_line_remove.clear();
	off_line_move.clear();
	off_line_copy.clear();
}

void offLineService::operator()(OS_TcpSocket sock, string homeDir)
{
	is_using = 0;
	off_line_number = 0;

	Sock = sock;
	m_homeDir = homeDir;

	m_fileCheck(homeDir);
}

void offLineService::operator++()
{
	is_using++;
}

void offLineService::operator--()
{
	is_using--;
}

void offLineService::operator+=(int i)
{
	is_using += i;
}

void offLineService::operator-=(int i)
{
	is_using -= i;
}

int offLineService::operator+(int i)
{
	is_using += i;
	return is_using;
}

int offLineService::operator-(int i)
{
	is_using -= i;
	return is_using;
}


void offLineService::offLineMove(vector<string>& move)
{
	if (move.size() <= 0)
		return;

	off_line_move.insert(off_line_move.end(), move.begin(), move.end());
	off_line_number++;
}

void offLineService::offLineCopy(vector<string>& copy)
{
	if (copy.size() <= 0)
		return;

	off_line_copy.insert(off_line_copy.end(), copy.begin(), copy.end());
	off_line_number++;
}

void offLineService::offLineRemove(string complete_path)
{
	off_line_remove.push_back(complete_path);
	off_line_number++;
}

int offLineService::size()
{
	return off_line_number;
}

int offLineService::user_size()
{
	return is_using;
}

bool offLineService::isDirectory(string complete_path)
{
	bool is_Dir = false;
#ifdef _WIN32
	struct _stat infos;
	_stat(complete_path.c_str(), &infos);

	if (infos.st_mode & _S_IFDIR)
	{
		is_Dir = true;    			//目录
	}
	else if (infos.st_mode & _S_IFREG)
	{
		is_Dir = false;				//文件
	}
#else
	struct stat infos;
	stat(complete_path.c_str(), &infos);

	if (infos.st_mode & S_IFDIR)
	{
		is_Dir = true;    			//目录
	}
	else if (infos.st_mode & S_IFREG)
	{
		is_Dir = false;				//文件
	}
#endif

	return is_Dir;
}

int offLineService::isExistFile(string complete_path)
{
	int exist = 0;
#ifdef _WIN32
	exist = _access_s(complete_path.c_str(), 0);
#else
	exist = access(complete_path.c_str(), F_OK);
#endif
	return exist;
}

// cmdout_file 为临时文件, 不为空时, 则删除;
string offLineService::temporaryFile(string cmdout_file)
{
	// 删除临时文件;
	if (cmdout_file.size() > 0)
	{
	#if _WIN32
		DeleteFileA(cmdout_file.c_str());
	#else
		remove(cmdout_file.c_str());
	#endif
		cmdout_file.clear();
		return cmdout_file;
	}

	// 获取客户端的IP地址;
	OS_SockAddr SockAddr;
	Sock.GetPeerAddr(SockAddr);

	// 创建一个临时文件, 用来保存用户的操作结果;
	unsigned short pid = SockAddr.GetPort();
	string file = "outcmd" + std::to_string(pid) + ".txt";

	// 临时文件的保护, 防止文件重命名;
	do
	{
		int exist = 0;
	#if _WIN32
		exist = _access_s(file.c_str(), 0);
	#else
		exist = access(file.c_str(), F_OK);
	#endif
		if (exist != 0) break;

		file = "outcmd" + std::to_string(rand() % pid) + ".txt";
	} while (true);

	return file;
}

int offLineService::offLineHandler()
{
	while (is_using == 0 && off_line_number > 0)
	{
		int type;
		string path;
		string cmdline;
		string cmdline_path;
		if (off_line_remove.size() > 0)
		{
			cmdline_path = off_line_remove[0];
			type = MSG_REMOVE;
		}
		else if (off_line_move.size() > 0)
		{
			cmdline_path = off_line_move[0];
			type = MSG_MOVE;
		}
		else if(off_line_copy.size() > 0)
		{
			cmdline_path = off_line_copy[0];
			type = MSG_COPY;
		}
		else
		{
			break;
		}
		
		int result = 0;
		char* argv[64] = { 0 };
		string first_cmdline = cmdline_path;
		int argc = FileUtils::Split((char*)first_cmdline.c_str(), argv);

		if (type == MSG_MOVE || type == MSG_COPY)
		{
			/*
				创建 destination 目录;
				注意, 当 move 命令拷贝多个目录时, 如果 destination 的子目录不存在, /
				move 命令会跳过第一个目录的创建, 并为 destination 创建子目录;
			*/

			// argv[1] 为 destination 目录;
			string directory = (const char*)argv[1];
			result = isExistFile(directory);
			if (result != 0)
			{
			#ifdef _WIN32
				directory = FileUtils::BackSlash(directory);
				directory.insert(0, "mkdir ");
			#else
				directory.insert(0, "mkdir -p ");
			#endif
				system(directory.c_str());
			}
		}

		// 删除重复出现的文件, 防止文件出现错误;
		if (argv[0] != NULL && type == MSG_REMOVE)
		{
			result = isExistFile(string(argv[0]));
		}
		else if (argc > 2)						// MSG_MOVE, MSG_COPY
		{
			// 用于检查 destination_file 的子文件是否重复;
			string complete_cmdline = argv[1];
			complete_cmdline += "/";
			complete_cmdline += argv[2];

			result = isExistFile(complete_cmdline);

			// 文件不存在, 置为 0; 存在, 置为 1;
			if (result != 0)
				result = 0;
			else
				result = 1;
		}

		if (result != 0 && argv[0] != NULL)
		{
			if (type == MSG_REMOVE)
				off_line_remove.erase(off_line_remove.begin(), off_line_remove.begin() + 1);
			else if (type == MSG_MOVE)
				off_line_move.erase(off_line_move.begin(), off_line_move.begin() + 1);
			else if (type == MSG_COPY)
				off_line_copy.erase(off_line_copy.begin(), off_line_copy.begin() + 1);
			continue;
		}

		string outcmd = temporaryFile();

		// 执行服务;
		if(argv[0] == NULL)
		{
			continue;
		}
		if (type == MSG_REMOVE)
		{
		#ifdef _WIN32
			if (m_fileCheck.isDirectory("", cmdline_path))
				FileUtils::rmdir(cmdline_path);
			else
				DeleteFileA(cmdline_path.c_str());
		#else
			// 执行命令;
			string cmdline2 = "rm -Rf ";
			cmdline2 += cmdline_path;
			cmdline2 += " > " + outcmd;
			system(cmdline2.c_str());
		#endif
			off_line_remove.erase(off_line_remove.begin(), off_line_remove.begin() + 1);
		}
		else if (type == MSG_MOVE || type == MSG_COPY)
		{
			if(argc > 2)
				result = isExistFile(string(argv[1]));

			// 添加命令行的命令;
			cmdline = (const char*)argv[0];
			cmdline += " ";
			cmdline += (const char*)argv[1];

			if (type == MSG_MOVE)
			{
			#ifdef _WIN32
				cmdline = FileUtils::BackSlash(cmdline);
				cmdline.insert(0, "move /Y ");
			#else
				cmdline.insert(0, "mv -f ");
			#endif
			}
			else if (type == MSG_COPY)
			{
				// 判断是否为目录;
				bool is_directory = isDirectory((const char*)argv[0]);

			#ifdef _WIN32
				// 加上命令行的目录;
				if (is_directory)
				{
					cmdline += "/";
					cmdline += (const char*)argv[2];
				}

				cmdline = FileUtils::BackSlash(cmdline);
				if (is_directory)
					cmdline.insert(0, "xcopy /E /Y /G /H /I /K ");				// 复制目录;
				else
					cmdline.insert(0, "xcopy /Y /G /H /I /K ");					// 复制文件;
			#else
				cmdline.insert(0, "cp -f -a ");
			#endif
			}
			// 执行命令;
			cmdline += " > " + outcmd;
			system(cmdline.c_str());
			cmdline.clear();

			if(type == MSG_MOVE)
				off_line_move.erase(off_line_move.begin(), off_line_move.begin() + 1);
			else if(type == MSG_COPY)
				off_line_copy.erase(off_line_copy.begin(), off_line_copy.begin() + 1);
		}
		else
		{

		}

		temporaryFile(outcmd);
		off_line_number--;
	}

	return 0;
}