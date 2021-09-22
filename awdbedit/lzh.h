#pragma once
//
// Award BIOS Editor - lzh.h
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: lzh.h,v 1.3 2004/04/11 07:17:15 bpoint Exp $
//
//----------------------------------------------------------------------------
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
//---------------------------------------------------------------------------------------------------------------------

#ifndef LZH_H
#define LZH_H

#include "types.h"

#pragma warning(disable: 4200)			// zero-sized array warning

#pragma pack(push, 1)
typedef struct
{
	uint8_t	headerSize;
	uint8_t	headerSum;

	uint8_t	method[5];
	uint32_t	compressedSize;
	uint32_t	originalSize;
	uint16_t	_unknown;
	uint16_t	fileType;
	uint8_t	_0x20;
	uint8_t	_0x01;
	uint8_t	filenameLen;
	uint8_t	filename[0];
} lzhHeader;

typedef struct
{
	uint16_t	crc;
	uint8_t	_0x20;
	uint16_t	extendedHeaderSize;
	uint8_t	extendedHeader[0];
} lzhHeaderAfterFilename;
#pragma pack(pop)

typedef enum
{
	LZHERR_OK = 0,
	LZHERR_UNKNOWN_METHOD,
	LZHERR_EXTHEADER_EXISTS,
} lzhErr;


void	lzhInit(void);
uint8_t	lzhCalcSum(uint8_t *ptr, uint32_t len);
lzhErr	lzhCompress(void *fname, uint32_t fnamelen, void *inbuf, uint32_t inbufsize, void *outbuf, uint32_t outbufsize, uint32_t *usedsize);
lzhErr	lzhExpand(lzhHeader *lzhptr, void *outbuf, uint32_t outbufsize, uint16_t *crc);

#endif
