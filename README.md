# NAS cmdline
Perform NAS operations through the command line.

## Features
Use common Linux commands to download and upload files, support TCP/IP protocol transmission, and simultaneous access files by multiple users. it can be run on both Windows and Linux systems, also data can be shared between them and can be operated in Chinese.

## Init
In the "nas/nasServer.cpp" file, locate the "Init" function and set the "homeDir" string, that is the root directory of NAS.
```
void nasServer::Init()
{
	// 设置 NAS 的根目录;
	string homeDir = "";

	......
}
```

## Compile
**Linux**: Run MakeFile file, then run "*.out" file.  
**Window**: Run "*.sln" file, then run "*.exe" file.

## Options
* the Windows system supports C++14, C++17, C++20 and C++23.  
* the Linux system supports C++11.


