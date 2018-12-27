/*
 * XPUtils.h
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

#ifndef XPLANEUDPCLIENT_SRC_XPUTILS_H_
#define XPLANEUDPCLIENT_SRC_XPUTILS_H_

 //#include <cstdint>
#include <stdlib.h>

#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#include <wiring.h>
#include <iostream>
#endif

	// typedefs for the xplane specific types
typedef uint32_t xint;
typedef char xchr;
typedef float xflt;
typedef double xdob;

xint uint2xint(uint32_t input);

uint32_t xint2uint(xint input);

xflt float2xflt(float input);

float xflt2float(xflt input);

//Use this to set ANY data-ref by UDP! With this power, you can send in any floating-point value to any data-ref in the entire sim!
//Just look up the datarefs at http://www.xsquawkbox.net/.
//Easy!
//	DREF0+(4byte byte value)+dref_path+0+spaces to complete the whole message to 509 bytes
struct _Xp_dref_struct {
	xflt var;
	xchr dref_path[500];
};

//uint32_t xint2uint32(uint8_t * buf);

//
#endif /* XPLANEUDPCLIENT_SRC_XPUTILS_H_ */
