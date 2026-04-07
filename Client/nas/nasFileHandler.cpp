#include "nasFileHandler.h"

FileHandler::FileHandler() : windows_transfer(false)
{
#ifdef _WIN32
	FileUtils::CMD_GBK();
#endif
}

FileHandler::FileHandler(OS_TcpSocket sock, string homeDir, char local_system, bool isClient, int size)
	: m_fileCheck(Sock, homeDir, maxCmdlineParameters, local_system, isClient)
{
	FileHandler(sock, homeDir, size);
#ifdef _WIN32
	FileUtils::CMD_GBK();
#endif
}

void FileHandler::operator()(char peerTheSystem)
{
	m_fileCheck(peerTheSystem);
}

void FileHandler::operator()(const char* homeDir)
{ 
	int home_len = strlen(homeDir);
	if(home_len <= 0)
	{
		// homeDir 为 "" 时, 跳过内存拷贝;
		m_homeDir = "";
		m_fileCheck("");
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
	m_fileCheck(m_homeDir);
	offLine(Sock, m_homeDir);
}

void FileHandler::operator()(string homeDir)
{
	m_homeDir = homeDir;
	m_fileCheck(m_homeDir);
	offLine(Sock, m_homeDir);
}

void FileHandler::operator()(OS_TcpSocket sock, string homeDir, char local_system, bool isClient, int size)
{
	m_type = 0;
	m_homeDir = homeDir;

	fp = NULL;
	fileSize = 0;
	
	offLine(sock, homeDir);
	m_fileCheck(sock, homeDir, maxCmdlineParameters, local_system, isClient);

	cmdline_count = 0;
	Sock = sock;
	windows_transfer = false;

	m_size = size;
	m_data = new char[m_size];
}


string FileHandler::destinationFile(vector<string>& print_file, string& file, string& path, unsigned int type, int argc, int i)
{
	// 返回错误的提示信息;
	char errorBuffer[128] = { 0 };

	// 文件不为目标文件则返回;
	string complete_file;
	if (i != argc - 1)
		return complete_file;

	char* argv[64] = { 0 };
	string destination = file;
	int argc2 = FileUtils::Split((char*)destination.c_str(), argv, "/");

	string argv_file;					// 存放 destination 的"../"字符;
	int argc3 = argc2;					// argc2 排除 "./"与 "../"之后的数量;

	// 判断 destination 的根目录是否存在;
	for (int k = 0; k < argc2; k++)
	{
		if (argv[k] == NULL)
			break;
		if (strcmp(".", (const char*)argv[k]) == 0)
		{
			argc3--;
			continue;
		}

		// 将 destination加上 "../", 来防止错误;
		if (strcmp("..", (const char*)argv[k]) == 0)
		{
			argv_file += (const char*)argv[k];
			argv_file += "/";
			argc3--;
			continue;
		}

		// 存在 destination和 destination的子目录;
		if (argc3 > 1)
		{
			complete_file = m_homeDir + path + argv_file + (const char*)argv[k];
			int isDirectory = offLine.isExistFile(complete_file);
			if (isDirectory != 0)
			{
			#ifdef _WIN32
				sprintf_s(errorBuffer, "the \'%s\' destination file does not exist .", (const char*)argv[k]);
			#else
				sprintf(errorBuffer, "the \'%s\' destination file does not exist .", (const char*)argv[k]);
			#endif
				throw string(errorBuffer);
			}

			// 只判断首个目录即可, 后面的子目录在 move_copy_file函数会自动创建;
			break;
		}
	}

	// 第一个文件是否为目录, 用于处理 重命名的操作;
	bool firstDirectory = false;
	if (i == 1)
		firstDirectory = isDirectory(path, file);

	// 删除 destination 末尾的 "/";
	int file_length = file.length() - 1;
	if (file[file_length] == '/')
		file.erase(file_length, 1);

	// 完整文件路径;
	complete_file = m_homeDir + path + file;

	// 判断目标是否为目录 或 重命名的操作;
	bool error = false;
	int exist = offLine.isExistFile(complete_file);
	bool isDirectory = this->isDirectory(path, file);
	if ((type == MSG_COPY || type == MSG_COPY2) && !isDirectory && exist == 0)
	{
		error = true;
	}
	else if (argc == 3 && !isDirectory && firstDirectory != isDirectory
		|| argc > 3 && !isDirectory && exist == 0)
	{
		error = true;
	}

	// 目标文件错误;
	if (error)
	{
	#ifdef _WIN32
		sprintf_s(errorBuffer, "the \'%s\' destination file is not a directory .", file.c_str());
	#else
		sprintf(errorBuffer, "the \'%s\' destination file is not a directory .", file.c_str());
	#endif
		throw string(errorBuffer);
	}

	for (int j = 0; j < print_file.size(); j++)
	{
		string end_file = complete_file;
		end_file += "/" + print_file[j];

		// 如果 destination 存在相同的文件, 则返回错误;
		exist = offLine.isExistFile(end_file);
		if (exist == 0)
		{
		#ifdef _WIN32
			sprintf_s(errorBuffer, "the destination file exist duplicate files of \'%s\' file.", print_file[j].c_str());
		#else
			sprintf(errorBuffer, "the destination file exist duplicate files of \'%s\' file.", print_file[j].c_str());
		#endif
			throw string(errorBuffer);
		}
	}

	return complete_file;
}

void FileHandler::add_fileList(vector<string>& total_file, vector<string>& print_file, string& path, string& file, int end_len, int i)
{
	// 返回错误的提示信息;
	char errorBuffer[128] = { 0 };

	if(file == "." || file == "..")
		throw string("The '.' or '..' operator is not allowed .");

	// 删除 file 末尾的 "/";
	int file_length = file.length() - 1;
	if (file[file_length] == '/')
		file.erase(file_length, 1);

	// 完整文件路径;
	string complete_file = m_homeDir + path + file;

	// 如果文件不存在, 则返回错误;
	int exist = offLine.isExistFile(complete_file);
	if (exist != 0 && i != end_len)
	{
	#ifdef _WIN32
		sprintf_s(errorBuffer, "the \'%s\' directory or file does not exist .", file.c_str());
	#else
		sprintf(errorBuffer, "the \'%s\' directory or file does not exist .", file.c_str());
	#endif
		throw string(errorBuffer);
	}

	// 将完整的文件加到 total_file 命令列表;;
	char* buf[32] = { 0 };
	if (i != end_len)
		total_file.push_back(complete_file);

	// 将含有"../"的文件加到 print_file 打印列表;
	int buf_size = file.find("..");
	if (buf_size >= 0)
	{
		file = file.substr(buf_size);
		print_file.push_back(file);
		return;
	}

	// 将 destination 的子文件加到 print_file 打印列表;
	buf_size = FileUtils::Split((char*)file.c_str(), buf, "/") - 1;
	if (buf_size >= 0 && buf[buf_size] != NULL)
		print_file.push_back(buf[buf_size]);
}

// off_line 为是否离线操作;
vector<string> FileHandler::move_copy_file(vector<string>& total_file, vector<string>& print_file, string& destination_file, unsigned int type, bool off_line)
{
	// 存放 命令行的子目录, 用于标记命令行的参数 是否为目录;
	vector<string> subdirectory;
	string cmdout_file = offLine.temporaryFile();

	// 判断 destination 的子目录是否存在;
	string destination = destination_file;
	int exist = offLine.isExistFile(destination);

	// 创建目录;
	if (exist != 0 && (total_file.size() > 1 || type == MSG_COPY) && !off_line)				// total_file > 1, destination_file 为目录;
	{
		/*
			创建 destination_file 目录;
			注意, 当 move 命令拷贝多个目录时, 如果 destination_file 的子目录不存在, /
			move 命令会跳过第一个目录的创建, 并为 destination_file 创建子目录;
		*/

		string directory = destination_file;

	#ifdef _WIN32
		directory = FileUtils::BackSlash(directory);
		directory.insert(0, "mkdir ");
	#else
		directory.insert(0, "mkdir -p ");
	#endif

		system(directory.c_str());
	}

	// 添加命令行的子目录, 用于离线删除;
	// xcopy 命令无法复制命令行上的目录; 需要手动创建;
	if (type == MSG_MOVE || type == MSG_MOVE2 || type == MSG_COPY || type == MSG_COPY2)
	{
		
		for (int j = 0; j < print_file.size() - 1; j++)
		{
			char* argv[32] = { 0 };
			string file = print_file[j];
			string first_file = total_file[j];

			// 判断命令行上的文件是否为目录;
			FileUtils::Split((char*)first_file.c_str(), argv);
			bool isDirectory = offLine.isDirectory(first_file.c_str());

			first_file = " ";
			if (type == MSG_MOVE || type == MSG_MOVE2 || type == MSG_COPY || type == MSG_COPY2)
			{
				// 加入目录参数, 用于删除用户 重复的输入;
				first_file += file;
			}

			// 如果命令行上的文件非目录, 则多加一个文件参数;
			if (!isDirectory && (type == MSG_MOVE || type == MSG_MOVE2))
			{
				// 加入文件参数;
				first_file += " <movingFile>";
			}
			else if (!isDirectory && (type == MSG_COPY || type == MSG_COPY2))
			{
				// 加入文件参数;
				first_file += " <copyingFile>";
			}
			subdirectory.push_back(first_file);
		}
	}

	// 移动文件;
	vector<string> off_line_service;
	for (int i = 0; i < total_file.size(); i++)
	{
		// 注意, move 命令无法移动多个命令行文件;
		if (off_line)
		{
			// 离线操作;
			total_file[i] += " ";
			total_file[i] += destination_file;

			// 添加第一个参数, 用于添加命令行上的文件或目录;
			// 添加第二个参数, 表示该操作为移动文件, 而非重命名;
			if (total_file.size() > 1 && type == MSG_MOVE || type == MSG_MOVE2)
				total_file[i] += subdirectory[i];
			else if(type == MSG_COPY || type == MSG_COPY2)
				total_file[i] += subdirectory[i];

			off_line_service.push_back(total_file[i]);
		}
		else
		{
			// 加入 destination 文件;
			string cmdline_system = total_file[i] + " " + destination_file;

			if (type == MSG_MOVE || type == MSG_MOVE2)
			{
			#ifdef _WIN32
				cmdline_system = FileUtils::BackSlash(cmdline_system);
				cmdline_system.insert(0, "move /Y ");
			#else
				cmdline_system.insert(0, "mv -f ");
			#endif
			}
			else if (type == MSG_COPY || type == MSG_COPY2)
			{
				// 判断是否为目录;
				bool is_directory = offLine.isDirectory(total_file[i]);

			#ifdef _WIN32
				// 加上命令行的目录;
				if (is_directory)
				{
					cmdline_system += "/";
					subdirectory[i].erase(0, 1);
					cmdline_system += subdirectory[i];
				}

				// 插入CMD命令;
				cmdline_system = FileUtils::BackSlash(cmdline_system);
				if(is_directory)
					cmdline_system.insert(0, "xcopy /E /Y /G /H /I /K ");				// 复制目录;
				else
					cmdline_system.insert(0, "xcopy /Y /G /H /I /K ");					// 复制文件;
			#else
				cmdline_system.insert(0, "cp -f -a ");
			#endif
			}
			else
			{
				break;
			}

			// 执行命令;
			cmdline_system = cmdline_system + " > " + cmdout_file;
			system(cmdline_system.c_str());
		}
	}

	// 删除临时文件;
	offLine.temporaryFile(cmdout_file);
	return off_line_service;
}

#ifndef _WIN32
string FileHandler::add_directory(Json::Value& jresult, char* file_info[], int argc, int& directory_path, int& count)
{
	string Directory;
	char* file_info2[384];

	// 文件路径存在空格形式;
	string file2(file_info[0]);
	for (int i = 1; i < argc; i++)
	{
		file2 += " ";
		file2 += file_info[i];
	}
	
	// 查找'/'的数量, '/'数量等于目录数量;
	int argc2 = FileUtils::Split((char*)file2.c_str(), file_info2, "/");
	if (count == 0) directory_path = argc2 - 1;				// directory_path 永远小于 argc2;

	Directory.clear();
	for (int j = directory_path; j < argc2; j++)
	{
		// 添加目录;
		Directory += file_info2[j];

		// 清除末尾的 ':', 方便后续操作;
		if (j == argc2 - 1)
		{
			int length = Directory.length() - 1;
			if (Directory[length] == ':')
				Directory = Directory.erase(length, 1);
		}

		// 预先导入 需要下载的根目录;
		if (count == 0)
		{
			jresult[count]["fileName"] = Directory;
			jresult[count]["isDir"] = true;
			count++;
		}

		Directory += "/";
	}

	return Directory;
}

string FileHandler::ls_linux(unsigned int type, Json::Value& jresult, string& path)
{
	return m_fileCheck.ls_linux(type, jresult, path);
}

void FileHandler::ls_linux2(unsigned int type, Json::Value& jresult, string& path)
{
	// 创建一个临时文件, 用来保存用户的操作结果;
	string file = offLine.temporaryFile();

	// 用来保存将要执行的命令;
	string cmdline;

	// 执行命令;
	cmdline = "ls -lR \'" + m_homeDir + path + "\' >> " + file;					// '>>'自动创建文件;
	system(cmdline.c_str());

	// 打开文件;
	std::ifstream ifs;
	ifs.open(file.c_str());					// 只能打开文件, 无法创建文件;

	// 用来保存命令行输出的结果;
	string line;
	int count = 0;							// 遍历文件的次数;
	int directory_path = 0;					// 保存根目录的下标;
	
	string fileName;						// 文件名;
	string Directory;						// 文件位置;
	
	// 读取命令行的结果;
	while (getline(ifs, line))
	{
		// 保存命令行的结果;
		char* file_info[384];
		int argc = FileUtils::Split((char*)line.c_str(), file_info);
		if (argc <= 0)	continue;						// 换行符

		// 总用量 436
		if (argc == 2)
		{
			string total = file_info[1];
			int total_length = total.size();

			if (total[total_length - 1] != ':')
				continue;
		}
		
		// 处理目录下的文件;
		if (file_info[0][0] == '/' || file_info[0][0] == '.' && file_info[0][1] == '/' || 
			file_info[0][0] == '.' && file_info[0][1] == '.' && file_info[0][2] == '/')
		{
			// 添加文件的路径;
			Directory = add_directory(jresult, file_info, argc, directory_path, count);
			continue;
		}
		else if(argc > 8)			// 读到文件或目录
		{
				
				// 文件存在空格形式;
				fileName = Directory;
				for (int i = 8; i < argc; i++)
				{
					fileName += string(file_info[i]);
					if (i != argc - 1)
						fileName += " ";
				}
		}
		else			// 普通文件
		{
			fileName = Directory + string(file_info[8]);
		}
			

		// Json::Value的计数;
		jresult[count]["fileName"] = fileName;
		jresult[count]["isDir"] = (file_info[0][0] == 'd') ? true : false;
		jresult[count]["fileSize"] = (double)atoll(file_info[4]);							// 在Linux下, jsoncpp不能处理long类型数据;

		// 整合文件时间;
		string time = file_info[5];
		time += " ";
		time += file_info[6];
		time += " ";
		time += file_info[7];

		jresult[count]["fileTime"] = time;
		jresult[count]["fileMode"] = string(file_info[0]);					// 以字符串的形式进行保存;

		count++;
	}

	// 删除临时文件;
	offLine.temporaryFile(file);
}
#endif


int FileHandler::ACK_Send(unsigned short type, const void* data, unsigned int length)
{
	// 发送消息类型
	Sock.Send(&type, 2);
	Sock.Send(&length, 4);

	// 数据部分是0个字节则退出
	if (length <= 0) return -1;

	// 发送数据
	return Sock.Send(data, length, false);
}

int FileHandler::ACK_Recv()
{
	// 接收消息类型
	Sock.Recv(&m_type, 2);
	Sock.Recv(&m_length, 4);

	// 数据部分是0个字节则退出
	if (m_length <= 0) return 0;

	// 接收数据
	return ReceiveN(m_data, m_length);
}

// linux_notDir: Linux命令行中, 子命令是否为单个文件;  
void FileHandler::sendFile(string& path, string& part, bool linux_notDir)
{
	// 打开文件;
	FILE* fp = NULL;
#ifdef _WIN32
	fopen_s(&fp, (m_homeDir + path + part).c_str(), "rb");
#else
	fp = fopen((m_homeDir + path + part).c_str(), "rb");
#endif
	if (fp == NULL)
	{
		directory_distances == 0;
		fileError = "Error: failed to upload file, unable to create file . \n";
		return;
	}

	// Linux_notDir 为 true 时, 备份 part;
	string part_backup;

	// 在Linux系统中, 将命令行的文件去掉 "/", 为下面发送 ACK_NEW_FILE 指令使用;
	if (linux_notDir)
	{
		part_backup = part;
		long long length = part.find_last_of('/', part.length() - 2);
		part = part.erase(0, length + 1);
	}
	ACK_Send(ACK_NEW_FILE, part.c_str(), part.length());
	
	// 还原 part;
	if(part_backup.size() > 0)
		part = part_backup;

	Json::Value jsonFile;
	string file_path = path + part;

	m_fileCheck.on_ll(jsonFile, file_path, true);
	// printf("path2: %s \n", file_path.c_str());

	long long fileSize = (double)jsonFile[0]["fileSize"].asInt64();
	ACK_Send(ACK_DATA_SIZE, &fileSize, 8);

	int bufsize = 1024 * 24;					// 24KB
	if (fileSize > 1024 * 1024 * 800)
		bufsize = 1024 * 1024 * 240;			// 240MB
	else if (fileSize > 1024 * 1024 * 60)
		bufsize = 1024 * 1024 * 24;				// 24MB

	char* buf = new char[bufsize];

	/* 发送文件数据 */
	while (!feof(fp))
	{
		int n;
	#ifdef _WIN32
		n = fread_s(buf, bufsize, 1, bufsize, fp);
	#else
		n = fread(buf, 1, bufsize, fp);
	#endif

		if (n < 0) break;
		if (n == 0) continue;
		if (ACK_Send(ACK_DATA_CONTINUE, buf, n) < 0)
		{
			// 网络已中断
			break;
		}

		if (m_fileCheck.isClient) printf(".");

		// 可以适当sleep,减少发送速率
		if (n > 1000000 * 5)
		{
			OS_Thread::Msleep(20);
		}
		else if(n > 1000)
		{
			// 必须加上几毫秒的延迟, 否则在局域网中, 会因为传输过快而导致接收数据出错;
			OS_Thread::Msleep(10);
		}
		else
		{
			OS_Thread::Msleep(5);
		}
	}

	ACK_Send(ACK_DATA_FINISH);
	fclose(fp);
}

void FileHandler::sendDirectory(Json::Value& jresult, string& path, string& part, char* argv2[], OS_Mutex& Mutex)
{
	ACK_Send(ACK_NEW_DIRECTORY, part.c_str(), part.length());

	// 将 part 加到 path 中, 方便后续调用;
	jresult.clear();
	path += part;
	if (part[part.length() - 1] != '/')						// path 尾数加上 '/';
		path += "/";

	m_fileCheck.on_ls(jresult, path);
	// printf("path: %s \n", path.c_str());

	Mutex.Unlock();

	// 防止文件过多, 超过存放文件的缓冲区;
	if (jresult.size() > maxUploadUnit)
	{
		directory_distances == 0;
		fileError = "Failed to upload file, too many files have been uploaded or downloaded, \
				the number of files cannot be greater than 1024 . \n";
		return;
	}

	argv2[0] = 0;
	for (int i = 1; i <= jresult.size(); i++)
	{
		argv2[i] = (char*)jresult[i - 1]["fileName"].asCString();
	}

	directory_distances++;
	// 使用递归 来加载一个目录中的所有目录;
	if (jresult.size() > 0)
		uploadFile(path, argv2, jresult.size() + 1, Mutex);

	directory_distances--;

	long long dir_pos = path.find_last_of('/', path.length() - 2);
	path.erase(dir_pos + 1);

	ACK_Send(ACK_NEW_FINISH);
}

void FileHandler::acquireDirectory(string& path)
{
	char buf[128] = { 0 };
#ifdef WIN32
	char* String = _getcwd(buf, 128);
	if (String != NULL)
	{
		path = String;
		path += '/';
	}
#else
	getcwd(buf, 128);
	if (buf[0] != 0)
	{
		path += buf;
		path += '/';
	}
#endif
}

// argv_count > 0 时, 为处理文件的前置操作;
void FileHandler::uploadHandler(string& path, char* argv[], int argv_count)
{
	/* 发送子命令的前置处理 */
	if (argv[0] != 0 && argv_count > 0)					// argv[0] != 0, argv 为客户端传输文件的参数; 
	{
		// 客户端上传当前目录文件时, 需要取得当前的工作目录;
	#ifdef _WIN32
		if(argv[argv_count][1] != ':' && m_fileCheck.isClient)
			acquireDirectory(path);
	#else
		// 保存用户的相对目录;
		m_path = path;
		if(argv[argv_count][0] != '/' && m_fileCheck.isClient)
			acquireDirectory(path);
	#endif

		// 计算命令行多余 "/"的数量;
		// 为后续重置 path参数使用, 如: get a/b/c => get c
		string cmdline = argv[argv_count];
		cmdline_count = std::count(cmdline.begin() + 1, cmdline.end() - 1, '/');
		
		
		// 不能使用'.'进行传输;
		long long length = cmdline.length() - 1;
		if (length > 0 && cmdline[length - 1] == '/' && cmdline[length] == '.')
			throw("Cannot use '.' or '..' for transmission . \n");
		if (length > 0 && cmdline[length - 1] == '.' && cmdline[length] == '/')
			throw("Cannot use '.' or '..' for transmission . \n");

		return ;
	}


	/* 发送完子命令的后置处理 */
#ifdef _WIN32
	if (directory_distances == 0)
	{
		ACK_Send(ACK_PATH_END);

		// 防止 put 命令上传出错;
		if (m_fileCheck.isClient)
			path.clear();
	}

	// 删除 path 多余的 "/", 以便下一个子命令通过, 如:  get a/b/c => get c
	for (int i = 0; i < cmdline_count && argv[0] != 0; i++)
	{
		long long dir_pos = path.find_last_of('/', path.length() - 2);
		path.erase(dir_pos + 1);
	}

	// 将 "/"的计数重置为 0, 为 下个子命令使用;
	if (cmdline_count > 0 && argv[0] != 0)
		cmdline_count = 0;
#else
	// 还原相对目录;
	if (m_fileCheck.isClient)
		path = m_path;
#endif
}

bool FileHandler::isDirectory(string path, string part)
{
	return m_fileCheck.isDirectory(path, part);
}

// 检查文件或目录 是否存在;
string FileHandler::checkFile(string& path, char* data, unsigned int type)
{
	return m_fileCheck.checkFile(path, data, type);
}

// 中文字符集转换, 用于外部函数封装调用;
string FileHandler::characterEncoding(char* src_str, int src_length)
{
	// convert 为 false 时, 则用于外部函数的调用;
	return m_fileCheck.characterEncoding(src_str, src_length, false);
}

void FileHandler::checkSingleFile(string path, string part)
{
	// 打开文件;
	FILE* fp = NULL;
#ifdef _WIN32
	fopen_s(&fp, (m_homeDir + path + part).c_str(), "rb");
#else
	fp = fopen((m_homeDir + path + part).c_str(), "rb");
#endif

	if (fp == NULL)
	{
		fileError = "reason: failed to upload file, unable to create file . \n";
		return;
	}

	fseek(fp, 0, SEEK_END);
	unsigned int size = ftell(fp);
	fclose(fp);

	if (size > maxUploadSize)
		fileError = "reason: failed to upload file, unable to create file . \n";
}

void FileHandler::checkDirectory(string path, string part, bool isDirectory)
{
	// 将 part 加到 path 中, 方便后续调用;
	Json::Value jresult;
	path += part;
	if (part[part.length() - 1] != '/')						// path 尾数加上 '/';
		path += "/";

	m_fileCheck.on_ls(jresult, path, isDirectory);

	// 防止文件过多, 超过存放文件的缓冲区;
	if (jresult.size() > maxUploadUnit)
	{
		fileError = "reason: failed to upload file, too many files have been uploaded or downloaded, \
			the number of files cannot be greater than 1024 . \n";
		return;
	}
	
	Json::Value jresult2;
	for (int i = 0; i < jresult.size(); i++)
	{
		// 判断是否为目录;
		bool isDirectory = jresult[i]["isDir"].asBool();
		
		if (!isDirectory)
			checkSingleFile(path, jresult[i]["fileName"].asCString());
		else
			checkDirectory(path, jresult[i]["fileName"].asCString(), isDirectory);
	}
}

void FileHandler::checkFileSize(OS_Mutex& Mutex, char** argv, int argc)
{
	// 客户端使用;
	if (!m_fileCheck.isClient)
		return;

	string path;
	for (int i = 1; i < argc; i++)
	{
		// 文件大小超标, 结束传输;
		if (fileError.size() > 0)
			return;

		string part;

		// 获取文件;
		if (argv[0] != 0)
			part = m_fileCheck.characterEncoding(argv[i], strlen(argv[i]), true);
		else
			part = argv[i];					// 注意, 只有命令行的子命令才可以进行 字符串的转换;

		Mutex.Lock();
		// 判断是否为目录;
		bool isDir = m_fileCheck.isDirectory(path, part);
		if (isDir)
			checkDirectory(path, part);
		else
			checkSingleFile(path, part);
		Mutex.Unlock();
	}
}

void FileHandler::uploadFile(string path, char** argv, int argc, OS_Mutex& Mutex)
{
	// 单个子命令;
	string part;

	int i = 1;
	char* argv2[1024];						// 存放文件, 注意 argv2不可以小于文件的数量;
	Json::Value jresult;

	fileError.clear();
	checkFileSize(Mutex, argv, argc);

	// 文件大小超标, 结束传输;
	if (fileError.size() > 0)
		i = argc;

	for (; i < argc; i++)
	{
		// 上传文件之前的处理;
		uploadHandler(path, argv, i);

		// 获取文件;
		if (argv[0] != 0)
			part = m_fileCheck.characterEncoding(argv[i], strlen(argv[i]), true);
		else
			part = argv[i];					// 注意, 只有命令行的子命令才可以进行 字符串的转换;

	#ifdef _WIN32
		Mutex.Lock();
		
		// 判断是否为目录;
		bool isDir = m_fileCheck.isDirectory(path, part);
		if (isDir)
			sendDirectory(jresult, path, part, argv2, Mutex);
		else							// 判断为文件处理;
			sendFile(path, part);
		
		// 上传文件错误, 本次传输结束;
		if (fileError.size() > 0)
			i = argc;

		Mutex.Unlock();
	#else
		linux_SendFile(jresult, part, Mutex);
	#endif

		// 上传文件之后的处理;
		uploadHandler(path, argv);
	}
}

void FileHandler::downloadFile(string path, OS_Mutex& Mutex)
{
	// 保存用户的相对目录;
	m_path = path;

	string part;
	int recv_length = 0;
	
	// 对端为 Windows系统 的附加处理;
	if (m_fileCheck.peer_system == 1)
		windows_transfer = true;

	while (recv_length >= 0)
	{
		Mutex.Lock();
		recv_length = ACK_Recv();

		// 下载完成
		if (m_type == ACK_FINISH)
			break;

		if (recv_length >= 0)
		{
			m_data[recv_length] = 0;
			part = m_data;
			recvHandler(path, part);
		}

		// 下载文件错误, 终止下载
		if (fileError.size() > 0)
			break;
		
		Mutex.Unlock();
	}
	Mutex.Unlock();
}

#ifndef _WIN32
bool FileHandler::linux_PathHandler(Json::Value& jresult, string& path)
{
	// 返回值, 判断命令行是否为单个文件参数;
	bool linux_notDir = false;
	
	
	// 清除 path末尾的 '/', 方便后续操作;
	long long length = path.length() - 1;
	if (path[length] == '/')
		path = path.erase(length, 1);


	// 调用 ls_linux2函数时, 需要 在子命令前 添加相对目录;
	if (!m_fileCheck.isClient)		// 服务端
	{
		if (path[0] == '/')
			path = path.erase(0, 1);
		path = m_path + path;
	}
	else if(path[0] != '/')			// 客户端
	{
		// 上传相对目录里的文件时, 需要加上 "./"以辅助 ls_linux2使用;
		path.insert(0, "./");
	}
	
	// 判断命令行的子命令是否为目录;
	// 如果为目录, 则 需要将相对目录处理, 为 sendFile函数传参使用;
	bool isDirectory = m_fileCheck.isDirectory("", path);
	if (!isDirectory)
		linux_notDir = true;

	// 注意, 必须将 jresult 清空才可以存储文件信息;
	jresult.clear();
	ls_linux2(m_type, jresult, path);

	// 将上传或下载的根目录进行处理, 为 sendFile函数传参使用;
	length = path.find_last_of('/', path.length() - 1);
	if (length > 0 && isDirectory)
		path = path.substr(0, length + 1);				// path = 相对目录 + 命令行上的路径;
	else
		path = m_path;							// path = 相对目录;
	
	return linux_notDir;
}

void FileHandler::linux_SendFile(Json::Value& jresult, string& path, OS_Mutex& Mutex)
{	
	// 处理发送文件的相对目录, 并对 jresult 进行文件加载;
	bool linux_notDir = linux_PathHandler(jresult, path);
	
	for (int i = 0; i < jresult.size(); i++)
	{
		Mutex.Lock();
		
		bool isDirectory = jresult[i]["isDir"].asBool();
		string part = jresult[i]["fileName"].asString();
		
		if (isDirectory)
		{
			ACK_Send(ACK_NEW_DIRECTORY, part.c_str(), part.length());
		}
		else if(i == 0)				// 命令行选项为单个文件;
		{
			// 将 m_homeDir 与 path 暂时置为0;
			// 命令行选项为单个文件时, part为全路径(包含m_homeDir 与 path);
			string homeDir = m_homeDir;
			m_homeDir.clear();
			
			string path_backup = path;
			path.clear();
			
			sendFile(path, part, linux_notDir);
			
			// 还原 m_homeDir 与 path;
			m_homeDir = homeDir;
			path = path_backup;
		}
		else
		{
			sendFile(path, part, linux_notDir);
		}
		
		// 将命令行选项重置为0;
		if(linux_notDir)
			linux_notDir = false;
		
		Mutex.Unlock();
	}

	// 单个子命令发送完成;
	ACK_Send(ACK_PATH_END);
}
#endif

void FileHandler::recvHandler(string& path, string& part)
{
	if (m_type == ACK_NEW_DIRECTORY || m_type == ACK_NEW_FILE)
	{
		bool Win32 = false;
	#ifdef _WIN32
		Win32 = true;
	#endif
		
		// 不同系统之间的编码转换;
		part = m_fileCheck.characterEncoding((char*)part.c_str(), part.length(), true);

		// 对端为 Windows系统 的额外操作;
		if (windows_transfer)
		{
			// 带有 '/'符号的路径进行上传和下载时, 需要删除 '/'之前的路径;
			long long pos_len = part.find_last_of('/', part.length() - 2);				// long long 避免 算数溢出;				
			if (pos_len >= 0)
				part = part.substr(pos_len + 1);						// 只保留文件名;
		}
	}

	if (m_type == ACK_NEW_DIRECTORY)
	{
	#ifdef _WIN32
		int dir = _mkdir((m_homeDir + path + part).c_str());
	#else
		int dir = mkdir((m_homeDir + path + part).c_str(), S_IRWXU);
	#endif
		if (dir < 0)
		{
			// 必须关闭文件, 防止段错误;
			if (fp != NULL)	
			{
				fclose(fp);
				fp = NULL;
			}
			
			fileError = "Error: unable to create directory . \n";
			return;
		}


		bool Win32 = false;
	#ifdef _WIN32
		Win32 = true;
	#endif
		
		// 对端为 Windows系统 的额外操作;
		if (windows_transfer)
			path += part += "/";
	}

	if (m_type == ACK_NEW_FINISH)
	{
		long long cd_pos = path.find_last_of('/', path.length() - 2);				// long long 避免 算数溢出;
		if(cd_pos > 0)
			path.erase(cd_pos + 1);
	}

	if (m_type == ACK_NEW_FILE)
	{
	#ifdef _WIN32
		fopen_s(&fp, (m_homeDir + path + part).c_str(), "ab+");
	#else
		fp = fopen((m_homeDir + path + part).c_str(), "ab+");
	#endif
		if (fp == NULL)
		{
			fileError = "Error: unable to create files . \n";
			return;
		}
	}

	if (m_type == ACK_DATA_SIZE)
	{
	#ifdef _WIN32
		memcpy_s(&fileSize, 8, m_data, sizeof(fileSize));
	#else
		memcpy(&fileSize, m_data, sizeof(fileSize));
	#endif
	}

	if (m_type == ACK_DATA_CONTINUE)
	{
		int numOfBytes = 0;

		// 写入文件
		int n = fwrite(m_data, 1, m_length, fp);
		numOfBytes += n;

		if(m_fileCheck.isClient) printf(".");
	}

	if (m_type == ACK_PATH_END)
	{
		// 返回到原来的目录;
		path = m_path;
	}

	if (m_type == ACK_DATA_FINISH)
	{
		fclose(fp);
		fp = NULL;
	}
}


int FileHandler::ReceiveN(void* buf, int count, int timeout)
{
	// 设置超时
	if (timeout > 0)
	{
		Sock.SetOpt_RecvTimeout(timeout);
	}

	// 接收数据过长的异常处理
	if (count > m_size)
	{
		printf("ReceiveN function error, the received data is too long ! \n");
		return -12;
	}

	// 反复接收数据, 直到接满指定的字节数;
	int bytes_got = 0;
	while (bytes_got < count)
	{
		int n = Sock.Recv((char*)buf + bytes_got, count - bytes_got, false);
		if (n <= 0)
		{
			continue;
		}

		bytes_got += n;
	}

	return bytes_got;	// 返回接收数据的大小;
}