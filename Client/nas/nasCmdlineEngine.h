#ifndef _NAS_CMDLINE_ENGINE_H
#define _NAS_CMDLINE_ENGINE_H

#include <stdio.h>
#include "../osapi/osapi.h"

#include "keyboardShortcuts.h"

#include <string>
#include <vector>
#include <algorithm>
using std::string;
using std::vector;

#ifdef _WIN32
#include <Windows.h>
#endif

/* nasCmdlineEngline
	NAS_cmdline 的键盘引擎;
*/
class nasCmdlineEngline
{
public:
	nasCmdlineEngline(string path_cmdline = "", string cmdline_prompt = "");

public:
	// 打印 Windows 命令行的字符;
	string start(vector<string>& cmdline_backup);

	// 删除 Windows 命令行的字符;
	void removeWindowsCharacter(short number = 0, short line_offset = 0, short x_offset = 0);

private:
	// 设置命令行的光标字符, 用于辅助 backSpace使用;
	void setCursorCharacter(string& result, bool isChinese);

	// 检查命令行的字符串, 用于辅助 backSpace, handleCmdCharacter使用;
	bool checkCmdCharacter(string& result, int& delete_count);
	
	// 返回光标移动的位数, 用于辅助 inputDirectionKey使用;
	int cursorMovementBytes(string& result, int virtualCode);

private:
	// 设置光标后的字符串, 用于辅助 inputDirectionKey使用;
	void setCursorString(string& result, int byte);

	// 重新排列存放 中文字符位置的数组, 用于辅助 handleCmdCharacter, inputDirectionKey2使用;
	void resetChinesePosition(string& result, bool backSpace = false);

	// 重新设置光标之前的字符串, 用于辅助 handleCmdCharacter使用;
	void resetBeforeCursor(string& result);

private:
	// 输入 BackSpace 键的处理, 用于辅助 start使用;
	void backSpace(string& result, int delete_count = 1);

	// 输入 Enter 键的处理, 用于辅助 start使用;
	string inputEnter(string& result, string& character, vector<string>& cmdline_backup);

	// 输入 左右方向 键的处理, 用于辅助 start使用;
	string inputDirectionKey(string& result, int virtualCode);

	// 输入 上下方向 键的处理, 用于辅助 start使用;
	string inputDirectionKey2(vector<string>& cmdline_backup, int virtualCode);

	// 处理控制台的字符, 用于辅助 start使用;
	string handleCmdCharacter(string& result, string& character);

private:
	int history_count;								// 上下键次数的差值, 用于遍历命令;
	int cursor_position;							// 左右键次数的差值, 用于设置光标;
	long long position_offset;						// cursor_position 的偏移量, 用于移动光标;
	int cursor_position_backup;						// 备份 cursor_position 值, 用于在光标后输入或 删除字符;

private:
	string cmdline_prompt;							// 命令行提示符;
	string cursor_string;							// 光标之后的字符串;
	
private:
	string chinese_character;						// 单个中文字符串, 用于命令行打印;
	vector<int> chinese_position;					// 中文字符的位置;
	std::pair<bool, bool> isChinese;				// 判断是否为中文字符编码;

private:
	keyboardShortcuts m_shortcuts;					// 命令行快捷键;
};

#endif