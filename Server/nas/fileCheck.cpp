#include "fileCheck.h"

fileCheck::fileCheck()
{
	local_system = 1;
	peer_system = 1;
	isClient = false;
	m_type = 0;
}

fileCheck::fileCheck(OS_TcpSocket& sock, string homeDir, char system, bool is_client)
{
	local_system = system;
	isClient = is_client;
	
	m_homeDir = homeDir;
	Sock = sock;
}

void fileCheck::operator()(char peerTheSystem)
{
	peer_system = peerTheSystem;
}

void fileCheck::operator()(const char* homeDir)
{
	int home_len = strlen(homeDir);
	if (home_len <= 0)
	{
		// homeDir 为 "" 时, 跳过内存拷贝;
		m_homeDir = "";
		return;
	}

	char* cmdline = new char[(home_len + 1)];

#ifdef _WIN32
	// strcpy_s(cmdline, cmd_len, data);
	memcpy_s(cmdline, home_len, homeDir, home_len);
#else
	memcpy(cmdline, homeDir, strlen(homeDir));
#endif

	m_homeDir = cmdline;
}

void fileCheck::operator()(string homeDir)
{
	m_homeDir = homeDir;
}

void fileCheck::operator()(OS_TcpSocket& sock, string homeDir, char system, bool is_client)
{
	local_system = system;
	isClient = is_client;

	m_homeDir = homeDir;
	Sock = sock;
}


// notDir 为单独文件的处理 (非目录文件);
string fileCheck::on_ls(Json::Value& jresult, string& path, bool notDir)
{
	string result;

	// 解析请求
	// printf("ls %s%s ...\n", m_homeDir.c_str(), m_path.c_str());
	jresult.clear();		// 防止重复显示

#ifdef _WIN32
	// 判断消息
	// msg_list2 为false, 则此函数用于处理 on_ls消息请求; 为true则处理 on_ll消息请求;
	bool msg_list2 = (m_type == MSG_LIST2) ? true : false;

	// 遍历文件信息
	FileEntryList files = FileUtils::List(m_homeDir + path, msg_list2, notDir);
	for (FileEntryList::iterator iter = files.begin();
		iter != files.end(); iter++)
	{
		FileEntry& entry = *iter;
		Json::Value jobj;
		jobj["fileName"] = entry.fileName;
		jobj["isDir"] = entry.isDirectory;
		jobj["fileSize"] = (double)entry.fileSize;				// 在Linux下, jsoncpp不能处理long类型数据;
		jobj["fileTime"] = (double)entry.filetime;
		jobj["fileMode"] = entry.fileMode;
		jresult.append(jobj);
	}

	result = FileUtils::List_Result(jresult, result);
#else
	int type = m_type;
	m_type = MSG_LIST;

	result = ls_linux(m_type, jresult, path);
	m_type = type;
#endif

	return result;
}

// notDir 为单独文件的处理 (非目录文件);
string fileCheck::on_ll(Json::Value& jresult, string& path, bool notDir)
{
	string result;
	jresult.clear();			// 防止重复显示

#ifdef _WIN32
	on_ls(jresult, path, notDir);
	result = FileUtils::List2_Result(jresult, result);
#else
	int type = m_type;
	m_type = MSG_LIST2;

	result = ls_linux(m_type, jresult, path);
	m_type = type;
#endif

	return result;
}

#ifndef _WIN32
string fileCheck::ls_linux(unsigned int type, Json::Value& jresult, string& path)
{
	string result;

	// 获取客户端的IP地址;
	OS_SockAddr SockAddr;
	Sock.GetPeerAddr(SockAddr);

	// 创建一个临时文件, 用来保存用户的操作结果;
	unsigned short pid = SockAddr.GetPort();
	string file = "outcmd" + std::to_string(pid) + ".txt";

	// 临时文件的保护, 防止文件重命名;
	do
	{
		int exist = access(file.c_str(), F_OK);
		if (exist != 0) break;

		file = "outcmd" + std::to_string(rand() % pid) + ".txt";
	} while (true);

	// 用来保存将要执行的命令;
	string cmdline;

	// 执行命令;
	if (type == MSG_LIST2)
		cmdline = "ls -lh \'" + m_homeDir + path + "\' >> " + file;					// '>>'自动创建文件;
	else
		cmdline = "ls \'" + m_homeDir + path + "\' >> " + file;
	system(cmdline.c_str());

	// 打开文件;
	std::ifstream ifs;
	ifs.open(file.c_str());					// 只能打开文件, 无法创建文件;

	// 用来保存命令行输出的结果;
	string line;
	int count = 0;							// 遍历文件的次数;

	// 读取命令行的结果;
	while (getline(ifs, line))
	{
		if (count == 0)
		{
			// 判断'ls'命令是否有效;
			int ls_fail = line.find("ls:");
			if (ls_fail > 0)
			{
				result.append(line + "\n");
				break;
			}
		}

		// 'll'命令的处理;
		if (type == MSG_LIST2)
		{
			// 删除根目录 m_homeDir 有关的信息, 防止根目录泄漏;
			int first_off = line.find('/', 0);
			if (first_off >= 0)
			{
				int end_off = line.rfind('/', line.size() - 1);
				if (first_off < end_off)
					line = line.erase(first_off, end_off + 1);
			}

			// 添加命令行的结果;
			result.append(line + "\n");

			if (count == 0 && peer_system == 0)
				result.append("\n");

			// 保存命令行的结果;
			if (count > 0)
			{
				char* file_info[128];
				FileUtils::Split((char*)line.c_str(), file_info);

				// Json::Value的计数;
				int number = count - 1;
				jresult[number]["fileName"] = string(file_info[8]);
				jresult[number]["isDir"] = (file_info[0][0] == 'd') ? true : false;
				jresult[number]["fileSize"] = (double)atoll(file_info[4]);							// 在Linux下, jsoncpp不能处理long类型数据;

				// 整合文件时间;
				string time = file_info[5];
				time += " ";
				time += file_info[6];
				time += " ";
				time += file_info[7];

				jresult[number]["fileTime"] = time;
				jresult[number]["fileMode"] = string(file_info[0]);					// 以字符串的形式进行保存;
			}
		}
		else						// 'ls'命令的处理;
		{
			// 每遍历5个文件进行一次换行;
			if (count % 5 == 0 && count != 0)
				result.append("\n");

			result.append(line + "   ");
			jresult[count]["fileName"] = line;
		}

		count++;
	}

	// 删除临时文件;
	cmdline = "rm -f " + file;
	system(cmdline.c_str());

	return result;
}
#endif

void fileCheck::checkCdCommand(string& result, string& path, string& part)
{
	// 命令行中 ".."的数量;
	int cd_count = 0;
	// 命令行中 ".."的位置;
	long long cd_pos = 0;					// long long 用于 [] 内相加减, 避免 算数溢出 C26451;

	do
	{
		cd_pos = part.find("..", cd_pos);
		if (cd_pos < 0) break;


		// 防止恶意非法访问, 如: cd ../++../..;
		// 使用两个if 语句来防止内存泄漏;
		if (cd_pos != 0)
		{
			if (part[cd_pos - 1] != '/')
			{
				result = "input bad command ! \n";
				throw result;
			}
		}

		// 防止恶意非法访问, 如: cd ../..+-/..;
		// 使用两个if 语句来防止内存泄漏;
		if (cd_pos + 2 != part.length())
		{
			if (part[cd_pos + 2] != '/')
			{
				result = "input bad command ! \n";
				throw result;
			}
		}

		cd_pos += 2;
		cd_count++;
	} while (true);


	// 防止用户进行越界访问;
	int path_count = std::count(path.begin(), path.end(), '/') - 1;
	if (cd_count > path_count && m_type == MSG_CD)
	{
		result = "The access violation ! \n";
		throw result;
	}
}

void fileCheck::checkCommand(string& result, string& path, string& part)
{
	int count = 0;						// part 迭代器的计数;

	// 文件名不能包含的字符: \ / : * ? " < > |
	for (string::iterator it = part.begin(); it != part.end(); it++)
	{
		if (count == 1 && *it == ':' && m_type > MSG_CD)						// get, put
		{
			// on_get 的 输出命令-o, 需要避开;
		}
		else if (*it == '\"' && count == 0 || count == part.size() - 1)
		{
			// 加上""的文件需要避开;
			// part = part.erase(count, 1);
		}
		else if (*it == '\\' || *it == ':' || *it == '*' || *it == '?'
			|| *it == '"' || *it == '<' || *it == '>' || *it == '|')
		{
			result = "No File or directory ! \n";
			throw result;
		}

		count++;
	}

	//  查找 "//" 字符的位置;
	int find_pos = 0;
	if (part.size() >= 4 && m_type > MSG_CD)								// get, put
		find_pos = 3;


	// 命令行不支持 "//" 字符;
	long long bad_commend = part.find("//", find_pos);					// long long 避免 算数溢出 C26451;
	if (bad_commend >= 0)
	{
		result = "input bad command, cannot input // or \\\\ . \n";
		throw result;
	}

	// 检查 "." 有关的错误命令, 如: ls file./file2
	bad_commend = part.find('.', 0);
	if (bad_commend > 0)				// 使用两个if 语句来防止内存泄漏;
	{
		if (part[bad_commend - 1] != '/' && part[bad_commend + 1] == '/' || 
			bad_commend == part.length() - 2 && m_type <= MSG_CD)
		{
			result = "input bad command ! \n";
			throw result;
		}
	}

	if (m_type >= MSG_CD && path != "/")								// cd, get, put
	{

		checkCdCommand(result, path, part);
	}
	else						// 防止用户跨越根目录进行越界访问;
	{

		int size = part.size();
		if (size <= 2)
			char CanAccess;				// 无不良指令;
		else if (size > 2 && part[0] == part[1] && part[0] == '.'
			|| size > 2 && part[2] == part[3] && part[3] == '.' && part[0] == '.')
		{
			result = "The access violation ! \n";
			throw result;
		}
	}
}

string fileCheck::checkResult(Json::Value& jresult, string path, string part)
{
	string result;			// 返回检查的结果;

	bool is_Dir = isDirectory(path, part);
	// 将文件和目录分开显示;
	//ifstream ifs(m_homeDir + path + part);
	if (!is_Dir)			// 是否为文件
	{
		if (m_type != MSG_LIST2)
		{
			// 增加 "./" 字符串;
			int length = part.find('/', 0);
			if (part[0] != '.' && part[1] != '/' && length > 0)
				part = part.insert(0, "./");

			result = part;
			result.append("\t");
		}
		else
		{
			path += part;

			result = on_ll(jresult, path, true);
			// path = "/";
		}
	}
	else				// 是否为目录
	{
		string directory;			// 单个目录信息;

		// 遍历目录的文件信息
		path += part += "/";
		if (m_type != MSG_LIST)
		{
			result += on_ll(jresult, path).append("\n");
		}
		else
		{
			result = on_ls(jresult, path);
			// result = FileUtils::List_Result(jresult, directory);
		}
		// path = "/";
	}

	return result;
}

// put_flag 为 MSG_PUT指令的 触发信号;
string fileCheck::isExistFile(string path, string part, bool put_flag)
{
	string result;

#ifdef _WIN32
	int exist = _access_s((m_homeDir + path + part).c_str(), 0);

	// 未查找的异常处理;
	if (exist == ENOENT)
	{
		result = "No File or directory ! \n";
		
		// MSG_PUT 命令需要多次检测, 不能立即返回;
		if(m_type != MSG_PUT || isClient || put_flag)
			throw result;
	}
	if (exist == EACCES)
	{
		result = "Access denied ! \n";

		// MSG_PUT 命令需要多次检测, 不能立即返回;
		if (m_type != MSG_PUT || isClient)
			throw result;
	}
#else

	int exist = 0;
	if (isClient)
		exist = access(part.c_str(), F_OK);
	else
		exist = access((m_homeDir + path + part).c_str(), F_OK);

	if (exist < 0)
	{
		result = "No File or directory ! \n";
		
		// MSG_PUT 命令需要多次检测, 不能立即返回;
		if(m_type != MSG_PUT || isClient || put_flag)
			throw result;
	}
#endif

	return result;
}

bool fileCheck::isDirectory(string path, string part)
{
	bool is_Dir = false;
#ifdef _WIN32
	struct _stat infos;
	_stat((m_homeDir + path + part).c_str(), &infos);

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
	stat((m_homeDir + path + part).c_str(), &infos);

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

// direct_convert: 直接转换字符集;
string fileCheck::characterEncoding(const char* src_str, bool direct_convert)
{
	string result;

	if (local_system != peer_system || direct_convert)
	{
	#ifdef _WIN32
		result = Utf8ToGbk(src_str);
	#else
		char cmdline[384] = { 0 };
		GbkToUtf8((char*)src_str, strlen(src_str), cmdline, 384);
		result = cmdline;
	#endif
	}
	else
	{
		result = src_str;
	}

	return result;
}

string fileCheck::Utf8ToGBK(char* src_str, int src_length)
{
	/* Utf8ToGBK
		对 CharacterEncoding.cpp 文件中的原函数进行了重新升级, 使其支持 ASCII和中文字符集 一起使用;
	*/

	string result;					// 返回转换的结果;
	string buffer;					// 需要转换的字符串;
	
	// Utf8ToGbk
	/*	UTF-8使用一至六个字节为每个字符编码:
		1. 128个US-ASCII字符只需一个字节编码（Unicode范围由U+0000至U+007F）;
		2. 带有附加符号的拉丁文、希腊文、西里尔字母、亚美尼亚语、希伯来文、阿拉伯文、叙利亚文及它拿字母则需要两个字节编码（Unicode范围由U+0080至U+07FF）;
		3. 其他基本多文种平面（BMP）中的字符（这包含了大部分常用字，如大部分的汉字）使用三个字节编码（Unicode范围由U+0800至U+FFFF）;
		4. 其他极少使用的Unicode 辅助平面的字符使用四至六字节编码（Unicode范围由U+10000至U+1FFFFF使用四字节，/
		Unicode范围由U+200000至U+3FFFFFF使用五字节，Unicode范围由U+4000000至U+7FFFFFFF使用六字节）;
	*/
	for (int i = 0; i < src_length; i++)
	{
		// 注意, 必须使用 unsigned char表示字节, 因为char的最高位为符号位, 因此表示-127~127;
		// unsigned char没有符号位, 因此能表示0~255, 8个bit, 最多256种情况, 因此无论如何都能表示256个数字;
		// 如果使用char的话, 则需要将下面的判断条件: ch > 127 改为 ch < 0, 因为最高位永远被置为1, 表示负数;
		unsigned char ch = src_str[i];
		
		if (ch < 128)						// 遇到ACSII码;
		{
			// 将之前的Utf8字符集转换;
			if (buffer.size() > 0)
			{
				result += characterEncoding(buffer.c_str(), true);
				buffer.clear();
			}

			// ACSII码;
			result += ch;
		}
		else if(ch > 127)					// 遇到第一个字节;
		{
			// 将字节加起来;
			buffer += ch;

			// 将最后一段字符串转换;
			if (i == src_length - 1 && buffer.size() > 0)
				result += characterEncoding(buffer.c_str(), true);
		}
	}
	
	return result;
}


string fileCheck::GBKToUtf8(char* src_str, int src_length)
{
	/* GBKToUtf8
		对 CharacterEncoding.cpp 文件中的原函数进行了重新升级, 使其支持 ASCII和中文字符集 一起使用;
	*/

	string result;					// 返回转换的结果;
	string buffer;					// 需要转换的字符串;
	
	// GbkToUtf8
	/* 双字节编码：GBK编码采用双字节表示一个字符，其中第一个字节的范围是0x81~0xFE，第二个字节的范围是0x40~0xFE（不包括0x7F） */
	int count = 0;
	while (count < src_length)
	{
		// 注意, 必须使用 unsigned char表示字节, 因为char的最高位为符号位, 因此表示-127~127;
		// unsigned char没有符号位, 因此能表示0~255, 8个bit, 最多256种情况, 因此无论如何都能表示256个数字;
		// 如果使用char的话, 则需要将下面的判断条件: ch > 127 改为 ch < 0, 因为最高位永远被置为1, 表示负数;
		unsigned char ch = src_str[count];

		if (ch > 128)								// 遇到第一个字节;
		{
			// 由第一个字节 确认第二个字节, 并将两位字节加起来;
			if (count + 1 < src_length)
			{
				buffer += src_str[count];
				buffer += src_str[count + 1];
				count++;
			}
			else
			{
				// 将最后一段字符串加起来;
				buffer += ch;
			}

			// 转换 最后一段字符串;
			if (count == src_length - 1 && buffer.size() > 0)
				result += characterEncoding(buffer.c_str(), true);
		}
		else if (buffer.size() > 0)							// 将之前的GBK字符集转换;
		{
			result += characterEncoding(buffer.c_str(), true);
			result += ch;								// ACSII码;

			buffer.clear();
		}
		else
		{
			// ACSII码;
			result += ch;
		}

		count++;
	}

	return result;
}


// convert 为 false 时, 则用于外部函数的封装调用;
// convert 为 true 时, 用于 fileCheck 类的内部调用和计算资源的节约;
string fileCheck::characterEncoding(char* src_str, int src_length, bool convert)
{
	// 操作系统相同, 直接返回原来的字符串;
	if (local_system == peer_system && convert)
		return string(src_str);

	// 客户端上传命令时, 本地主机不进行字符串的转换; 
	if(isClient && m_type == MSG_PUT && convert)
		return string(src_str);

	string result;

#ifdef _WIN32
	result = Utf8ToGBK(src_str, src_length);
#else
	result = GBKToUtf8(src_str, src_length);
#endif

	return result;
}


// 检查文件或目录 是否存在, 若存在则返回相关的文件名称;
string fileCheck::checkFile(string& path, char* data, unsigned int type)
{
	char* argv[64];					// 命令行的子命令;
	string result;					// 检查文件的结果;
	Json::Value jsonResult;			// 文件的详细信息;
	bool put_flag = false;			// 触发上传指令的信号;
	m_type = type;

	int cmd_len = strlen(data) + 1;
	char* cmdline = new char[cmd_len];

#ifdef _WIN32
	// strcpy_s(cmdline, cmd_len, data);
	memcpy_s(cmdline, cmd_len, data, cmd_len);
#else
	// 注意, 在Linux系统中, 使用 memcpy 是无法将原有的内存完全覆盖掉的;
	// memcpy 和 memcpy_s 属于内存拷贝，所以在拷贝过程中即便遇到 \0，也不会结束的;
	// memcpy(cmdline, data, strlen(data));					// 内存错误;
	strcpy(cmdline, data);
#endif


	// 获取命令行的子命令;
	int argc = FileUtils::Split(cmdline, argv);
	if (argc < 0) throw result;
	if (argc > 8)
	{
		result = "Access denied, Because command too long ! \n";
		throw result;
	}

	// 检查文件或目录
	for (int i = 1; i < argc; i++)
	{
		// 获取子命令;
		string part;

		// 不同系统之间的编码转换;
		if (!isClient)
			part = characterEncoding(argv[i], strlen(argv[i]));
		else
			part = argv[i];

		checkCommand(result, path, part);

		if (i == argc - 1 && m_type == MSG_PUT)
			put_flag = true;

		// 判断文件或目录是否存在;
		string check_result = isExistFile(path, part, put_flag);

		// MSG_PUT 命令检查存在文件失败;
		if (check_result.size() <= 0 && m_type == MSG_PUT && !isClient)
			return check_result;

		// 防止访问 非目录文件;
		if (m_type == MSG_CD || m_type == MSG_GET && isClient)
		{
			/* ifstream 判断目录, 只适用于Windows系统;
			ifstream ifs(m_homeDir + path + part);
			if (ifs.is_open())
			{
				result = "The access violation ! \n";
				throw result;
			}*/
			bool is_Dir = isDirectory(path, part);
			if (!is_Dir)
			{
				result = "The access violation ! \n";
				throw result;
			}
		}

		// 返回文件或目录的结果;
		if (m_type == MSG_LIST || m_type == MSG_LIST2)
			result += checkResult(jsonResult, path, part);
	}

#ifdef _WIN32
	delete[] cmdline;		// 使用 memcpy_s 时, 才能删除;
#endif

	return result;			// 返回文件名称;
}