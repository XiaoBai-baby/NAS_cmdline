#include "ByteBuffer.h"

ByteBuffer::ByteBuffer(int bufsize)
{
	m_buffer = new char[bufsize];
	m_offset = 0;
	m_bufsize = bufsize;
	M_sendcount = 0;
	M_total = 0;

	// 设置缓冲区初始位置的索引;
	m_start = (unsigned char*)m_buffer + m_offset;
	m_written = 0;		// 写计数
	m_read = 0;		// 读计数
}

ByteBuffer::ByteBuffer(char* buffer, int offset, int bufsize)
{
	m_buffer = buffer;
	m_offset = offset;
	m_bufsize = bufsize;
	M_sendcount = 0;
	M_total = 0;

	// 设置缓冲区初始位置的索引;
	m_start = (unsigned char*)m_buffer + m_offset;
	m_written = 0;		// 写计数
	m_read = 0;		// 读计数
}

ByteBuffer::~ByteBuffer()
{
	if (m_buffer)
	{
		delete[] m_buffer;
		m_buffer = NULL;
	}
}

unsigned char* ByteBuffer::Position()
{
	return m_start;
}

int ByteBuffer::Written() const
{
	return m_written;
}

int ByteBuffer::Read() const
{
	return m_read;
}

void ByteBuffer::Clear()
{
	M_sendcount = 0;
	M_total = 0;
	
	if (m_read > 0)
	{
		m_start = m_start + m_read;
		m_read = 0;
	}

	if (m_written > 0)
	{
		m_start = m_start + m_written;
		m_written = 0;
	}
}


////////////////////////////////////////

unsigned char* ByteBuffer::Header()
{
	// 获得总数据的大小;
	M_total = m_written;

	return (unsigned char*) &m_header;
}

int ByteBuffer::Headsize() const
{
	return sizeof(m_header);
}

void ByteBuffer::loadHeader(void* buf, int bufsize)
{
	memcpy(&m_header, buf, bufsize);
}

int ByteBuffer::sendCount() const
{
	return M_sendcount;
}

bool* ByteBuffer::isNumber()
{
	return Is_number;
}

unsigned int* ByteBuffer::Individual()
{
	return Individual_datasize;
}

unsigned int ByteBuffer::Total() const
{
	return M_total;
}


////////////////////////////////////////

void ByteBuffer::putUnit8(unsigned char value)
{
	unsigned char* p = m_start + m_written;
	p[0] = value;
	m_written += 1;
	
	Is_number[M_sendcount] = true;
	Individual_datasize[M_sendcount] = 1;
	M_sendcount++;
}

void ByteBuffer::putUnit16(unsigned short value)
{
	unsigned char* p = m_start + m_written;
	p[0] = (unsigned char)(value >> 8);			// 位操作使用unsigned char
	p[1] = (unsigned char)(value);
	m_written += 2;

	Is_number[M_sendcount] = true;
	Individual_datasize[M_sendcount] = 2;
	M_sendcount++;
}

void ByteBuffer::putUnit32(unsigned int value)
{
	unsigned char* p = m_start + m_written;
	p[0] = (unsigned char)(value >> 24);
	p[1] = (unsigned char)(value >> 16);		// 位操作使用unsigned char
	p[2] = (unsigned char)(value >> 8);
	p[3] = (unsigned char)(value);
	m_written += 4;

	Is_number[M_sendcount] = true;
	Individual_datasize[M_sendcount] = 4;
	M_sendcount++;
}

void ByteBuffer::putUnit64(unsigned long long value)
{
	unsigned char* p = m_start + m_written;
	p[0] = (unsigned char)(value >> 56);
	p[1] = (unsigned char)(value >> 48);
	p[2] = (unsigned char)(value >> 40);
	p[3] = (unsigned char)(value >> 32);		// 位操作使用unsigned char
	p[4] = (unsigned char)(value >> 24);
	p[5] = (unsigned char)(value >> 16);
	p[6] = (unsigned char)(value >> 8);
	p[7] = (unsigned char)(value);
	m_written += 8;

	Is_number[M_sendcount] = true;
	Individual_datasize[M_sendcount] = 8;
	M_sendcount++;
}

void ByteBuffer::putString(const string& value)
{
	unsigned int length = value.length();
	putUnit32(length);

	unsigned char* p = m_start + m_written;
	memcpy(p, value.c_str(), length);
	m_written += length;

	Is_number[M_sendcount - 1] = false;
	Individual_datasize[M_sendcount - 1] = length;
}

// 也可以放“不以0结束的无规则数据”
void ByteBuffer::putBytes(const char* data, int length)
{
	putUnit32(length);

	unsigned char* p = m_start + m_written;
	memcpy(p, data, length);
	m_written += length;

	Is_number[M_sendcount - 1] = false;
	Individual_datasize[M_sendcount - 1] = length;
}


////////////////////////////////////////

unsigned char ByteBuffer::getUnit8()
{
	unsigned char* p = m_start + m_read;
	unsigned char result = p[0];

	m_read += 1;
	return result;
}

unsigned short ByteBuffer::getUnit16()
{
	unsigned char* p = m_start + m_read;

	unsigned short result = 0;
	result += (p[0] << 8);
	result += (p[1]);

	m_read += 2;
	return result;
}

unsigned int ByteBuffer::getUnit32()
{
	unsigned char* p = m_start + m_read;

	unsigned int result = 0;
	result += (p[0] << 24);
	result += (p[1] << 16);
	result += (p[2] << 8);
	result += (p[3]);

	m_read += 4;
	return result;
}

unsigned long long ByteBuffer::getUnit64()
{
	unsigned char* p = m_start + m_read;

	unsigned long long result = 0;
	result += (p[0] << 56);
	result += (p[1] << 48);
	result += (p[2] << 40);
	result += (p[3] << 32);
	result += (p[4] << 24);
	result += (p[5] << 16);
	result += (p[6] << 8);
	result += (p[7]);

	m_read += 8;
	return result;
}

string ByteBuffer::getString()
{
	unsigned int length = getUnit32();

	unsigned char* p = m_start + m_read;
	// string result((const char*)p, length);
	string result;
	result.append((const char*)p, length);

	m_read += length;
	return result;
}

// 也可以放“不以0结束的无规则数据”
int ByteBuffer::getBytes(char* buf)
{
	unsigned int length = getUnit32();

	unsigned char* p = m_start + m_read;
	memcpy(buf, p, length);

	m_read += length;
	return length;
}