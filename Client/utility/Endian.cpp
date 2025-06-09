#include "Endian.h"

/////////////// big-endian functions ////////////

// int -> bytes, 32bits , big-endidan
void itob_32be(unsigned int a, unsigned char bytes[])
{
    bytes[0] = (unsigned char)(a >> 24);
    bytes[1] = (unsigned char)(a >> 16);
    bytes[2] = (unsigned char)(a >> 8);
    bytes[3] = (unsigned char)a;
}

// int -> bytes, 32bits , big-endidan
unsigned int btoi_32be(unsigned char bytes[])
{
    unsigned int a = 0;
    a += (bytes[0] << 24);
    a += (bytes[1] << 16);
    a += (bytes[2] << 8);
    a += (bytes[3]);
    return a;
}

// short -> bytes, 16bits , big-endidan
void itob_16be(unsigned short a, unsigned char bytes[])
{
    bytes[0] = (unsigned char)(a >> 8);
    bytes[1] = (unsigned char)a;
}

// short -> bytes, 16bits , big-endidan
unsigned short btoi_16be(unsigned char bytes[])
{
    unsigned short a = 0;
    a += (bytes[0] << 8);
    a += bytes[1];
    return a;
}

/////////////// little-endian functions ////////////

// int -> bytes, 32bits , little-endidan
void itob_32le(unsigned int b, unsigned char bytes[])
{
    bytes[0] = (unsigned char)b;
    bytes[1] = (unsigned char)(b >> 8);
    bytes[2] = (unsigned char)(b >> 16);
    bytes[3] = (unsigned char)(b >> 24);
}

// int -> bytes, 32bits , little-endidan
unsigned int btoi_32le(unsigned char bytes[])
{
    unsigned int b = 0;
    b += (bytes[0]);
    b += (bytes[1] << 8);
    b += (bytes[2] << 16);
    b += (bytes[3] << 24);
    return b;
}

// short -> bytes, 16bits , little-endidan
void itob_16le(unsigned short b, unsigned char bytes[])
{
    bytes[0] = (unsigned char)b;
    bytes[1] = (unsigned char)(b >> 8);
}

// short -> bytes, 16bits , little-endidan
unsigned short btoi_16le(unsigned char bytes[])
{
    unsigned short b = 0;
    b += (bytes[0]);
    b += (bytes[1] << 8);
    return b;
}

