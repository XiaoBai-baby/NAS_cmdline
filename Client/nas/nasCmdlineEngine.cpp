#include "nasCmdlineEngine.h"

// path_cmdline: 命令行相对目录里的文件
// cmdline_prompt: 是否设置命令行提示符, 例如: "C:\> ";
nasCmdlineEngline::nasCmdlineEngline(string path_cmdline, string cmdline_prompt)
	: m_shortcuts(path_cmdline), cmdline_prompt(cmdline_prompt)
{
	history_count = 0;
	cursor_position = 0;
	position_offset = 0;
	cursor_position_backup = 0;

	// GBK 编码, 2个字节;
	isChinese = { false, false };
}

#ifdef _WIN32
string nasCmdlineEngline::start(vector<string>& cmdline_backup)
{
	// HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);				// 控制台标准输入句柄被 SetStdHandle 函数重定向, 则无法使用;
	HANDLE hStdin = CreateFileA("CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, NULL, NULL);

	if (hStdin == INVALID_HANDLE_VALUE)
		return "GetStdHandle error \n";

	const int maxEvents = 128;
	INPUT_RECORD irInBuf[maxEvents];
	DWORD cNumRead;


	// 命令行的全部字符;
	string result;
	while (true)
	{
		// 读取控制台输入
		if (!ReadConsoleInputA(hStdin, irInBuf, maxEvents, &cNumRead))
		{
			printf("Error: ReadConsoleInput error \n");
			return result;
		}

		// 处理每一个事件
		for (DWORD i = 0; i < cNumRead; ++i)
		{
			if (irInBuf[i].EventType == KEY_EVENT)
			{
				// 仅处理按键按下事件
				KEY_EVENT_RECORD& ker = irInBuf[i].Event.KeyEvent;
				if (ker.bKeyDown)
				{
					// 用于遍历字符;
					string character;
					int virtualCode = ker.wVirtualKeyCode;
					if (ker.uChar.UnicodeChar != '\0')
						character = ker.uChar.UnicodeChar;

					// 判断是否输入中文;
					if ((unsigned char)character[0] > 128 && !isChinese.first && !isChinese.second)
						isChinese = { true, true };

					// 重置 光标之后的字符;
					if (cursor_position == 0)
						cursor_string.clear();

					// 重置快捷键;
					if (virtualCode != 9)
						m_shortcuts.clear();

					int position = character.rfind('\r');
					if (position >= 0)
					{
						// 输入 Enter 键;
						if (result.size() > 0)
							return inputEnter(result, character, cmdline_backup);
						else if (cursor_string.size() > 0)						// 光标之前没有字符;
							return inputEnter(cursor_string, character, cmdline_backup);
						else
							printf("\n%s", cmdline_prompt.c_str());
					}
					else if (position >= 0 && character == "\r")
					{
						// 输入 "\n";
						history_count = 0;
						cursor_position = 0;
						position_offset = 0;
						cursor_position_backup = 0;
						printf("\n");
						return "";
					}
					else if (virtualCode == 8)
					{
						// 输入 BackSpace 键;
						if(result.size() > 0)
							backSpace(result);
					}
					else if (virtualCode == 9)
					{
						// 重置光标差值;
						cursor_position = 0;
						position_offset = 0;
						cursor_position_backup = 0;

						// 输入 Tab 键;
						string tab_string = m_shortcuts.onTabButton(result);

						// 删除命令行的字符;
						if (cursor_string.size() > 0)
							removeWindowsCharacter(0, 0, 0 - cursor_string.size());
						removeWindowsCharacter(m_shortcuts.size() + cursor_string.size() + 1, 0, -1);

						// 添加新字符;
						result.erase(result.size() - m_shortcuts.size());
						result.append(tab_string);
						printf("%s", tab_string.c_str());
					}
					else if (character.size() > 0)
					{
						// 处理控制台的字符;
						handleCmdCharacter(result, character);
					}
					else if (virtualCode == 37 || virtualCode == 39)
					{
						// 输入 左右 键;
						inputDirectionKey(result, virtualCode);
					}
					else if (virtualCode == 38 || virtualCode == 40)
					{
						// 输入 上下 键;
						cursor_position = 0;
						position_offset = 0;
						cursor_position_backup = 0;

						if (cursor_string.size() > 0)
							removeWindowsCharacter(0, 0, 0 - cursor_string.size());
						backSpace(result, result.size() + cursor_string.size() - 1);
						result = inputDirectionKey2(cmdline_backup, virtualCode);
					}
				}
			}
		}
		OS_Thread::Msleep(2);
	}

	return result;
}

// number 为删除字符的数量; line_offset 为 命令行 当前行数的偏移量;
// number不为 0, x_offset 为 负数 时, 则函数用于删除命令行的字符;
// number为 0, x_offset 不为 0 时, 则函数仅设置命令行的光标, 不代表删除命令行的字符;
void nasCmdlineEngline::removeWindowsCharacter(short number, short line_offset, short x_offset)
{
	// 获取控制台屏幕缓冲区句柄;
	// HANDLE screen = GetStdHandle(STD_OUTPUT_HANDLE);				// 控制台标准输入句柄被 SetStdHandle 函数重定向, 则无法使用;
	HANDLE screen = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, NULL, NULL);

	// 控制台屏幕缓冲区扩展信息;
	// 不推荐使用, 当 GetConsoleScreenBufferInfoEx检索不到扩展信息时, 则返回 false;
	// _CONSOLE_SCREEN_BUFFER_INFOEX screen_buffer = { 0 };

	// 控制台屏幕缓冲区信息;
	_CONSOLE_SCREEN_BUFFER_INFO screen_buffer = { 0 };

	// 检索控制台屏幕缓冲区的信息;
	bool screen_flag = GetConsoleScreenBufferInfo(screen, &screen_buffer);
	if (screen_flag)
	{
		// 获取光标的位置;
		short x = screen_buffer.dwCursorPosition.X;
		short y = screen_buffer.dwCursorPosition.Y;

		y -= line_offset;

		_COORD cursor_position;
		if (x_offset != 0)
			cursor_position = { x - x_offset, y };
		else
			cursor_position = { number, y };

		// 设置控制台光标的位置;
		SetConsoleCursorPosition(screen, (_COORD)cursor_position);

		// 移动光标后, 函数结束;
		if (x_offset > 0)
			return;

		// 移动光标到起始位置;
		// printf("\x1B[H");

		// 使用ANSI转义的 BackSpace键 删除字符;
		// 注意, 使用 ANSI码 删除时, 删除的范围只有一行;
		for (int j = 0; j < number; j++)
		{
			/*
				\x1B[xA, \x1B[xC 和\x1B[xD 分别为向上、向前和向后移动光标;
				printf("\x1B[%d;%dH", row, col), 可以将光标移动到第 row 行和第 col 列的位置;
			*/
			printf("\b ");
			printf("\x1B[1D");
		}
	}
}


void nasCmdlineEngline::setCursorCharacter(string& result, bool isChinese)
{
	// 删除字符的位数;
	int byte = 1;
	if (isChinese)
		byte = 2;

	// 删除字符并打印光标字符;
	long long length = result.length();
	if (cursor_position <= 0 && byte == 1)
	{
		// 删除字符;
		printf("\b \x1B[1D");
		result.erase(length - byte - 1, byte);
		return;
	}

	// 删除字符;
	if(byte == 1)
		printf("\b \x1B[1D");
	else
		printf("\b\b  \x1B[2D");

	// 光标后的字符一起删除, 并打印 cursor_string;
	removeWindowsCharacter(cursor_string.size() + byte, 0, 0 - cursor_string.size() - byte);
	if (cursor_position > 0)
		printf("%s", cursor_string.c_str());

	// 设置光标;
	if(cursor_string.size() > 0)
		removeWindowsCharacter(0, 0, cursor_string.size());
	result.erase(length - byte, byte);

	// 将左右差值 备份并重置;
	cursor_position_backup = cursor_position;
}

// 如果存在中文字符, 则返回 true;
bool nasCmdlineEngline::checkCmdCharacter(string& result, int& delete_count)
{
	bool isChinese = false;
	int length = result.size() - 1;

	// 第一个字符输入, 跳过检查;
	if (length < 0)
		return isChinese;

	// 字符末尾的'\0'去除, 用于清除命令行界面的字符; 
	while (result[length] == '\0')
	{
		result.erase(length, 1);
		length -= 1;
	}

	// 判断是否为 中文字符;
	for (int i = chinese_position.size() - 1; i >= 0; i--)
	{
		if (chinese_position[i] == length - 1)
		{
			isChinese = true;
			break;
		}
	}

	// 检查字符串是否有多余的字符, 例如: 粘贴后的字符 + 自己敲的字符
	length = result.size() - 1;
	int pos = result.find_first_of('\0');
	while (pos >= 0 && pos != length)
	{
		// 删除多余的'\0';
		result.erase(pos, 1);
		pos = result.find_first_of('\0');
	}


	// 删掉一整行的字符串, 包括命令行提示符;
	// 从而避免 BackSpace 键使用频率过快, 而误删其它字符;
	if (delete_count > 1)
	{
		delete_count += cmdline_prompt.size() + 20;
		result.clear();
	}

	return isChinese;
}

int nasCmdlineEngline::cursorMovementBytes(string& result, int virtualCode)
{
	std::vector<int>::iterator it = chinese_position.end();
	long long length = result.size();

	// 判断光标后的字符是否为中文字符;
	long long offset = 1;
	it = std::find(chinese_position.begin(), chinese_position.end(), length - 2);
	if (it != chinese_position.end())
		offset += 1;

	// 设置 cursor_position 的偏移量;
	if (virtualCode == 37)
	{
		it = std::find(chinese_position.begin(), chinese_position.end(), length - offset);

		if (it != chinese_position.end())
			position_offset++;
	}
	if (virtualCode == 39)
	{
		it = std::find(chinese_position.begin(), chinese_position.end(), length);

		if (it != chinese_position.end())
			position_offset--;
	}

	// 光标移动的位数;
	int byte = 1;
	if (it != chinese_position.end())
		byte++;
	return byte;
}

// byte: 光标移动的位数;
void nasCmdlineEngline::setCursorString(string& result, int byte)
{
	long long length = result.length();

	if (cursor_position_backup != 0)
	{
		// 输入字符后, 第二次移动光标;
		string character = result.substr(length - byte, byte);
		result = result.erase(length - byte);
		cursor_string.insert(0, character);
	}
	else
	{
		// 求前后两次光标移动的差值;
		int difference = cursor_position - cursor_position_backup;

		// 移动字符是否为中文字符;
		vector<int>::iterator it = std::find(chinese_position.begin(), chinese_position.end(), length - 2);
		if (it != chinese_position.end())
			difference++;

		if (difference < 0)
			difference = -difference;

		// 第一次移动光标;
		cursor_string = result.substr(length - difference);
		result = result.erase(length - byte);
	}
}

// backSpace: 是否有使用 BackSpace 按键操作;
void nasCmdlineEngline::resetChinesePosition(string& result, bool backSpace)
{
	for (int i = 0; backSpace || i < cursor_string.size(); i++)
	{
		unsigned char c = 0;
		if(!backSpace)	c = cursor_string[i];

		if (backSpace || c > 128)
		{
			// 清除 chinese_position;
			chinese_position.clear();
			string cmdline_string = result + cursor_string;

			// 重新赋值 chinese_position;
			for (int j = 0; j < cmdline_string.size(); j++)
			{
				unsigned char c = cmdline_string[j];
				if (c > 128)
				{
					chinese_position.push_back(j);
					j++;
				}
			}
			break;
		}
	}
}

void nasCmdlineEngline::resetBeforeCursor(string& result)
{
	if (cursor_position != cursor_position_backup)
	{
		// 求前后两次光标移动的差值;
		long long length = result.length();
		int difference = cursor_position - cursor_position_backup;

		// 移动字符是否为中文字符;
		vector<int>::iterator it = std::find(chinese_position.begin(), chinese_position.end(), length - 2);
		if (it != chinese_position.end())
			difference++;

		if (difference < 0)
			difference = -difference;

		// 获取光标前的字符串;
		result = result.substr(0, length - difference);
		cursor_position_backup = cursor_position;
	}
}

// cmdline_prompt: 命令行提示符;
// delete_count: 删除次数, 如果大于 1, 则表示删除的命令行字符数量;
void nasCmdlineEngline::backSpace(string& result, int delete_count)
{
	// 删除多余'/0';
	int length = result.size() - 1;
	if (length + cursor_string.size() + 1 < cursor_position ||
		result[0] == '\b' || length < 0)
		return;

	// 检查字符串;
	bool isChinese = checkCmdCharacter(result, delete_count);

	// 注意, 使用ANSI转义码 删除字符串, 最多只能删除一行字符;
	for (int j = 0; j < delete_count; j++)
	{
		if (isChinese && length > 0 && delete_count < 2)
		{
			// 删除中文字符;
			setCursorCharacter(result, isChinese);
		}
		else if (cursor_position > 0)
		{
			// 设置命令行字符和光标;
			setCursorCharacter(result, isChinese);
		}
		else if (delete_count > 1)
		{
			// 删除一整行字符串;
			printf("\b \x1B[1D");
		}
		else
		{
			// 删除 ASCII 字符;
			printf("\b \x1B[1D");
			result.erase(length, 1);
		}
	}

	// 重新排列中文字符的位置;
	resetChinesePosition(result, true);

	// 重新打印命令行提示符;
	if (delete_count > 1)
		printf("%s", cmdline_prompt.c_str());
}

string nasCmdlineEngline::inputEnter(string& result, string& character, vector<string>& cmdline_backup)
{
	// 输入换行符, 保存退出;
	printf("%s\n", character.c_str());
	result += character;

	// 将末尾的 '\r' 换成 '\0';
	int length = result.length() - 1;
	result[length] = '\0';

	// 检查字符串是否有多余的字符, 例如: 粘贴后的字符 + 自己敲的字符
	length = result.size() - 1;
	int pos = result.find("\b\0");
	while (pos >= 0 && pos != length)
	{
		// 删除多余的'\0';
		result.erase(pos, 1);
		pos = result.find_first_of('\0');
		length = result.size() - 1;
	}

	// 上下左右的差值清 0;
	history_count = 0;
	cursor_position = 0;
	cursor_position_backup = 0;
	

	// 添加光标后的字符串;
	if (cursor_string.size() > 0)
	{
		// 清除字符串末尾的"\0";
		if(result.size() <= 0) {}
		else if(result[length] == '\0')
			result.erase(length, 1);

		// 添加字符串;
		length = result.size();
		result.append(cursor_string);

		// 如果光标前面没有字符, 则备份 cursor_string;
		if(length == 0)
			cmdline_backup.push_back(result);

		cursor_string.clear();
	}

	// 备份命令, 并将中文字符的位置清 0;
	cmdline_backup.push_back(result);
	chinese_position.clear();

	// 添加'\0'结束符;
	length = result.length();
	if (result[length] != '\0')
		result.append("");

	return result;
}

string nasCmdlineEngline::inputDirectionKey(string& result, int virtualCode)
{
	if (cursor_position < 0)
		return cursor_string;

	long long length = result.size();
	int byte = cursorMovementBytes(result, virtualCode);

	if (virtualCode == 37)
	{
		// 移动光标;
		cursor_position++;
		if (cursor_position + position_offset <= result.size()
			|| cursor_position + position_offset <= (result.size() + cursor_string.size()) && cursor_position_backup != 0)
		{
			removeWindowsCharacter(0, 0, byte);
		}
		else if (cursor_position + position_offset > (result.size() + cursor_string.size()) && cursor_position_backup != 0)
		{
			// 设置 cursor_position 的边界;
			cursor_position--;
			return cursor_string;
		}

		// 设置光标后的字符串;
		setCursorString(result, byte);
	}
	if (virtualCode == 39)
	{
		cursor_position--;

		// 设置光标;
		if (cursor_position < 0)
			cursor_position = 0;
		else
			removeWindowsCharacter(0, 0, -byte);

		// 为 cursor_string 重新赋值;
		if (cursor_string.size() > 0)
		{
			result += cursor_string.substr(0, byte);
			cursor_string = cursor_string.substr(byte, cursor_string.size());
		}
	}

	cursor_position_backup = cursor_position;
 	return cursor_string;
}

string nasCmdlineEngline::inputDirectionKey2(vector<string>& cmdline_backup, int virtualCode)
{
	string result;
	if (cmdline_backup.size() == 0)
		return "";

	// 重新计算 history_count;
	if (virtualCode == 38)
		history_count++;
	else if (virtualCode == 40)
		history_count--;

	// 设置 history_count 的边界;
	if (history_count < 0)
	{
		history_count++;
	}
	else if (history_count > cmdline_backup.size())
	{
		history_count--;
	}

	// 打印历史字符;
	int length = cmdline_backup.size() - history_count;
	if (length >= 0 && length < cmdline_backup.size())
	{
		result = cmdline_backup[length];
		printf("%s", result.c_str());

		// 去除字符末尾的'\0'; 
		length = result.size() - 1;
		if(result[length] == '\0')
			result.erase(length, 1);
	}

	// 重新排列中文字符的位置;
	if(result.size() > 0)
		resetChinesePosition(result, true);

	return result;
}

string nasCmdlineEngline::handleCmdCharacter(string& result, string& character)
{
	// 未输入换行符, 继续读取控制台的输入;
	if (!isChinese.second)
		printf("%s", character.c_str());
	else
		chinese_character += character;

	// 打印中文;
	if (chinese_character.size() > 1)
	{
		printf("%s", chinese_character.c_str());
		chinese_character.clear();
	}

	// 添加中文字符的位置;
	if (isChinese.first)
		chinese_position.push_back(result.length());

	// 重置中文标识符;
	if (isChinese.first)
		isChinese.first = false;
	else
		isChinese.second = false;

	// 检查字符串是否有多余的字符;
	int pos = 0;
	checkCmdCharacter(result, pos);

	// 打印光标后的字符串并移动光标;
	if (cursor_string.size() > 0)
	{
		printf("%s", cursor_string.c_str());
		resetBeforeCursor(result);
		removeWindowsCharacter(0, 0, cursor_string.size());
	}

	// 添加输入字符;
	if (cursor_position == 0)								// 添加字符;
		result += character;
	else
		result.insert(result.size(), character);

	// 重新排列中文字符的位置;
	if (cursor_string.size() > 0 && chinese_character.size() <= 0 || chinese_character.size() > 1)
	{
		resetChinesePosition(result);
	}

	character.clear();
	return result;
}

#endif