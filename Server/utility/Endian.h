#ifndef _ENDIAN_H
#define _ENDIAN_H

#include <cstdio>
using namespace std;

/* Endian.h
    大小端与字节数组之间的转换函数
*/

/////////////// big-endian functions ////////////

// int -> bytes, 32bits , big-endidan
void itob_32be(unsigned int a, unsigned char bytes[]);


// int -> bytes, 32bits , big-endidan
unsigned int btoi_32be(unsigned char bytes[]);


// short -> bytes, 16bits , big-endidan
void itob_16be(unsigned short a, unsigned char bytes[]);


// short -> bytes, 16bits , big-endidan
unsigned short btoi_16be(unsigned char bytes[]);


/////////////// little-endian functions ////////////


// int -> bytes, 32bits , little-endidan
void itob_32le(unsigned int b, unsigned char bytes[]);


// int -> bytes, 32bits , little-endidan
unsigned int btoi_32le(unsigned char bytes[]);


// short -> bytes, 16bits , little-endidan
void itob_16le(unsigned short b, unsigned char bytes[]);


// short -> bytes, 16bits , little-endidan
unsigned short btoi_16le(unsigned char bytes[]);


#endif