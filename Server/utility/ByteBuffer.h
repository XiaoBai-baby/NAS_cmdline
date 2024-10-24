#ifndef _BYTE_BUFFER_H
#define _BYTE_BUFFER_H

// #include "utility.h"		// C3646: 未知重写说明符, 类的顺序错误

#include <string>
#include <string.h>

using std::string;

/* ByteBuffer 字节编码器。
	用于缓存和编码连续的 无规则数据或汉字等。
*/

class ByteBuffer
{
public:
	ByteBuffer(int bufsize);
	ByteBuffer(char* buffer, int offset, int bufsize);

	~ByteBuffer();

public:
	// 返回缓冲区初始位置的索引;
	unsigned char* Position();

	// 返回写入数据个数;
	int Written() const;

	// 返回读取数据个数;
	int Read() const;

	// 重置缓冲区的数据;
	// 这里的重置非真正清空, 而是为了让 ByteBuffer二次发送和接收时使用;
	// 即二次使用时修改 Position{ }函数 和标头信息等;
	void Clear();

public:
	// 返回整个缓存区的数据报头;
	unsigned char* Header();

	// 返回数据报头的大小;
	int Headsize() const;

public:
	// 加载数据报头;
	void loadHeader(void* buf, int bufsize);

	// 发送的数据段个数;
	int sendCount() const;

	// 检索是否为单个整数数据;
	bool* isNumber();

	// 单个数据的大小;
	unsigned int* Individual();

	// 总数据的大小;
	unsigned int Total() const;

public:
	// 推入单个整数数据
	void putUnit8(unsigned char value);
	void putUnit16(unsigned short value);
	void putUnit32(unsigned int value);
	void putUnit64(unsigned long long value);
	
	// 推入多个字节数据
	void putString(const string& value);
	void putBytes(const char* data, int length);

public:
	// 推出单个整数数据
	unsigned char getUnit8();
	unsigned short getUnit16();
	unsigned int getUnit32();
	unsigned long long getUnit64();

	// 推出多个字节数据
	string getString();
	int getBytes(char* buf);

private:
	char* m_buffer;		// 存放数据的缓冲区
	int m_bufsize;		// 缓冲区的大小
	int m_offset;	// 手动设置 m_buffer的偏离量

	unsigned char* m_start;		// 缓冲区初始位置的索引;
	int m_written;	// 已经写入的数据个数;
	int m_read;		// 已经读取的数据个数;
	

private:
	// 用于编码缓存区数据的数据报头;
	struct m_header
	{
		// 注意, 在结构体中没有深度拷贝函数, 所以不要放任何数据类型的指针;
		unsigned int m_sendcount;		// 发送次数, 也是 is_number 和 individual_datasize 的下标大小;
		unsigned int m_total;			// 总数据的大小;
		bool is_number[1024];			// 判断每个数据是否为数字;
		unsigned int individual_datasize[1024];		// 每个数据段的大小;
	}m_header;

	// m_header 结构体中数据的简写;
	#define M_sendcount m_header.m_sendcount
	#define M_total m_header.m_total
	#define Is_number m_header.is_number
	#define Individual_datasize m_header.individual_datasize
};

#endif