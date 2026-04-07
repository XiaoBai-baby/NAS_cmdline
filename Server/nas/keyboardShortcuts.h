#ifndef _KEYBOARDSHORTCUTS
#define _KEYBOARDSHORTCUTS

#include "../utility/FileUtils.h"

#include <vector>
using std::vector;

/* keyboardShortcuts
	快捷键, 用于辅助 nasCmdlineEngline 使用; 
*/
class keyboardShortcuts
{
public:
	keyboardShortcuts(string path_command = "");

public:
	// 重置此类;
	void clear();

	// 返回 Tab 命令的原大小;
	int size();

private:
	// 添加相对目录里的文件, 辅助 keyboardShortcuts使用;
	void addSaveFile(string& path_cmdline);

	// 检查命令, 辅助 matchedFiles使用;
	string check(string& cmdline);

	// 二次 Tab 键的使用, 辅助  matchedCommand, onTabButton使用;;
	string nextTab();

	// 匹配命令, 辅助 onTabButton使用;
	string matchedCommand(char* argv);

	// 匹配文件, 辅助 onTabButton使用;
	string matchedFiles(char* argv);

public:
	// Tab 快捷键;
	string onTabButton(string cmdline);

private:
	int space_count;							// 空格次数, 用于补偿 commandSize;

private:
	int tabCount;								// Tab 键最大的有效次数, 用于访问 m_file;
	int commandSize;							// Tab 命令的原大小;
	int byte;									// 遍历 m_file 的位数;

private:
	string m_cmdline;							// 保存命令行的命令;
	vector<string> m_file;						// 存放命令匹配的文件;
};

static vector<string> save_file;				// 命令行相对目录里的文件;

#endif