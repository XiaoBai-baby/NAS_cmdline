# NAS cmdline API
The NAS cmdline API can be utilized to achieve the functions of NAS.

## Init
In the "Server/nas/transferParameter.h" file, locate the "transferParameter" structure and set the "homeDir" string, that is the root directory of NAS.
```
struct transferParameters
{
	// 设置 NAS 的根目录;
	string homeDir = "";

	......
};
```

## Operate
**Sever: Define the "nasServer.h" file, define a class name of nasSever and use the start() function.
**Client: Define the "nasClient.h" file, define a class name of nasClient and use the start() function.

## Options
* the Windows system supports C++14, C++17, C++20 and C++23.  
* the Linux system supports C++11.

## Author
author: 小白 <br>
code link: [https://github.com/XiaoBai-baby/NAS_cmdline](https://github.com/XiaoBai-baby/NAS_cmdline)