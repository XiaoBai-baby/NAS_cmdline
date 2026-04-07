#include "keyboardShortcuts.h"

keyboardShortcuts::keyboardShortcuts(string path_cmdline)
	: byte(0), tabCount(0), commandSize(0)
{
	space_count = 0;
	addSaveFile(path_cmdline);
}

void keyboardShortcuts::clear()
{
	tabCount = 0;
	commandSize = 0;
	space_count = 0;
	m_file.clear();
}

int keyboardShortcuts::size()
{
	commandSize += space_count;
	space_count = 0;
	return commandSize;
}

void keyboardShortcuts::addSaveFile(string& path_cmdline)
{
	if (save_file.size() <= 0)
	{
		// 第一次 "ls"命令;
		save_file.push_back(path_cmdline);
	}
	else
	{
		// 二次 "ls"命令;
		bool flag = false;
		for (int i = 0; i < save_file.size(); i++)
		{
			// 判断是否存在相同;
			int same = save_file[i].compare(path_cmdline.c_str());
			if (same == 0)
			{
				// 存在相同的目录;
				flag = true;
				break;
			}
		}

		// 添加文件;
		if (!flag)
			save_file.push_back(path_cmdline);
	}
}

string keyboardShortcuts::check(string& cmdline)
{
	string result;

	if (cmdline.size() <= 2)
		return result;
	if (cmdline[0] != '.')
		return result;

	// 重置 commandSize;
	commandSize = 0;

	// 检查是否有"./";
	if (cmdline[1] == '/')
	{
		cmdline.erase(0, 2);
		commandSize += 2;
		return "./";
	}

	// 检查是否有".\\";
	if (cmdline[1] == '\\' && cmdline[2] == '\\')
	{
		cmdline.erase(0, 3);
		commandSize += 3;
		return ".\\\\";
	}
	
	return result;
}

string keyboardShortcuts::matchedFiles(char* argv)
{
	// 文件参数;
	string cmdline2;

	string result = argv;
	string symbol = check(result);
	commandSize += result.length();

	for (int i = 0; i < save_file.size(); i++)
	{
		string cmdline2 = save_file[i];

		// 命令行相对目录里的文件;
		char* argv2[64] = { 0 };
		int argc2 = FileUtils::Split((char*)cmdline2.c_str(), argv2);

		// 搜索参数匹配的文件;
		for (int j = 0; j < argc2 && argv2[j] != NULL; j++)
		{
			// path 目录下的文件;
			string file = argv2[j];

			// 去除目录标识符;
			int directory_flag = file.find("[+]", file.length() - 3);
			if (directory_flag > 0)
				file = file.substr(0, directory_flag);

			// 识别命令行的文件;
			int flag = file.compare(0, result.size(), result);
			if (flag == 0)
			{
				// 找到文件;
				m_file.push_back(file);
				tabCount++;
			}
		}
	}

	return symbol;
}

string keyboardShortcuts::nextTab()
{
	// 设置 m_file 位数;
	byte++;
	if (m_file.size() < byte)
		byte = m_file.size();

	// 重置 commandSize;
	char* argv[64] = { 0 };
	string cmdline = m_cmdline;
	int argc = FileUtils::Split((char*)cmdline.c_str(), argv);

	// 获取 symbol;
	string symbol;
	if (argv[argc - 1] != NULL)
	{
		cmdline = argv[argc - 1];
		symbol = check(cmdline);
	}

	if (argc > 1)
		commandSize = cmdline.size();

	// 重置 byte;
	if (byte >= tabCount)
		byte = 0;

	// 返回下一个匹配文件;
	string next_string;
	if (symbol.size() <= 0)
	{
		next_string = m_file[byte];
	}
	else
	{
		// 添加 symbol;
		next_string = symbol + m_file[byte];
		commandSize += symbol.length();
	}

	return next_string;
}

string keyboardShortcuts::matchedCommand(char* argv)
{
	string result = argv;

	if (tabCount > 0)
		return nextTab();

	commandSize = strlen(argv);

	// 单命令匹配;
	if (result == "r")
		return "rm";
	if (result == "pw")
		return "pwd";
	if (result == "g" || result == "ge")
		return "get";
	if (result == "mk" || result == "mkd" || result == "mkdi")
		return "mkdir";
	if (result == "h" || result == "he" || result == "hel")
		return "help";
	if (result == "e" || result == "ex" || result == "exi")
		return "exit";

	// 多命令匹配;
	if (result == "l")
	{
		tabCount += 2;
		m_file.push_back("ls");
		m_file.push_back("ll");
		return "ls";
	}
	if (result == "c")
	{
		tabCount += 2;
		m_file.push_back("cd");
		m_file.push_back("cp");
		return "cd";
	}
	if (result == "m")
	{
		tabCount += 2;
		m_file.push_back("mkdir");
		m_file.push_back("mv");
		return "mkdir";
	}
	if (result == "p")
	{
		tabCount += 2;
		m_file.push_back("put");
		m_file.push_back("pwd");
		return "put";
	}

	return "";
}

string keyboardShortcuts::onTabButton(string cmdline)
{
	m_cmdline = cmdline;
	int length = cmdline.length() - 1;
	while (cmdline[length] == ' ')
	{
		space_count++;
		length--;
	}

	string result;
	char* argv[64] = { 0 };

	int argc = FileUtils::Split((char*)cmdline.c_str(), argv);
	if (argc == 1 && argv[0] != NULL)
	{
		// 命令参数;
		return matchedCommand(argv[0]);
	}
	else if (tabCount > 0)
	{
		// 二次使用 Tab 键;
		return nextTab();
	}
	else if (argc > 1)
	{
		// 匹配文件;
		string symbol = matchedFiles(argv[argc - 1]);

		// 返回文件名;
		if (m_file.size() > 0 && symbol.size() > 0)
			return symbol + m_file[0];

		if(m_file.size() > 0)
			return m_file[0];
	}
	
	// 重置参数;
	tabCount = 0;
	commandSize = 0;
	m_file.clear();

	return "";
}