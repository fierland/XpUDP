/*
 * XPUtils.cpp
 *
 *  Created on: Jul 27, 2017
 *      Author: shahada
 *
 *  Copyright (c) 2017-2018 Shahada Abubakar.
 *
 *  This file is part of libXPlane-UDP-Client.
 *
 *  This program is free software: you can redistribute it and/or
 *  modify it under the terms of the Lesser GNU General Public
 *  License as  published by the Free Software Foundation, either
 *  version 3 of the  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  See the GNU General Public License for more details.
 *
 */

#include "XPUtils.h"
#include "xpudp_debug.h"
#include <endian.h>

using namespace std;

//#ifdef ___BIG_ENDIAN

// byteorder or running Xplane on PC is litte endian

#define  __BYTE_ORDER__ __LITTLE_ENDIAN

#if __BYTE_ORDER__ == __LITTLE_ENDIAN

#define SWITCH(x)

#else

static inline void _switchByteOrder(void* ptr, int len)
{
	DPRINTLN("Swapping bytes");
	uint8_t tmp[4], *pbyte = (uint8_t*)ptr;
	if (len == 2)
	{
		memcpy(tmp, ptr, 2);
		pbyte[0] = tmp[1];
		pbyte[1] = tmp[0];
	}
	else if (len == 4)
	{
		memcpy(tmp, ptr, 4);
		pbyte[0] = tmp[3];
		pbyte[1] = tmp[2];
		pbyte[2] = tmp[1];
		pbyte[3] = tmp[0];
	}
}
#  define SWITCH(x) _switchByteOrder(&(x), sizeof(x))
#  define SWITCHP(x) _switchByteOrder((x), sizeof(x))
#endif

xint uint2xint(uint32_t input)
{
	xint res = (xint)input;
	SWITCH(res);
	return (res);
};

uint32_t xint2uint(xint input)
{
	uint32_t res = (uint32_t)input;
	SWITCH(res);
	return (res);
};

xflt float2xflt(float input)
{
	xflt res = (xflt)input;
	SWITCH(res);
	return (res);
};

float xflt2float(xflt input)
{
	float res = (float)input;
	SWITCH(res);
	return (res);
};

/*
uint32_t xint2uint32(uint8_t * buf)
{
	// cerr <<
	//	" buf[0] is " << (int) buf[0] <<
	//	" buf[1] is " << (int) buf[1] <<
	//	" buf[2] is " << (int) buf[2] <<
	//	" buf[3] is " << (int) buf[3] <<
	//	endl;

		//	return buf[3] << 24 | buf[2] << 16 | buf[1] << 8 | buf[0];
	uint32_t res = (uint32_t)input;
	SWITCH(res);
	return (res);
	return (uint32_t)SWITCHP(buf);
};

//float xflt2float(uint8_t * buf)
{
	// treat it as a uint32_t first, so we can flip the bits based on
	// local endianness. Then union it into a float.

	union {
		float f;
		uint32_t i;
	} v;

	v.i = xint2uint32(buf);
	return v.f;
}
*/