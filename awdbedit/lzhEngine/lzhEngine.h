//
// Award BIOS Editor - lzhEngine.h
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
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
//
// Award BIOS Editor's LZH Engine was adapted from "ar" archiver written by 
// Haruhiko Okumura.
// 

#ifndef LZHENGINE_H
#define LZHENGINE_H

#include <limits.h>
#include "../types.h"

//--------- io.cpp ----------
extern ushort bitbuf;
extern bool unpackable;

#define INIT_CRC  0  // CCITT: 0xFFFF
#define BITBUFSIZ (CHAR_BIT * sizeof(bitbuf))

void ioInit(void *instream, ulong instreamsize, void *outstream, ulong outstreamsize);
ushort ioGetCRC(void);
ulong ioGetInSizeUsed(void);
ulong ioGetOutSizeUsed(void);

void make_crctable(void);
void fillbuf(int n);
ushort getbits(int n);
void putbits(int n, ushort x);
int memread_crc(uchar *dest, int n);
void memwrite_crc(uchar *src, int n);
void init_getbits(void);
void init_putbits(void);


//--------- encode.cpp / decode.cpp -----------
#define DICBIT		13    // 12(-lh4-) or 13(-lh5-)
#define DICSIZ		(ushort)(1U << DICBIT)
#define MAXMATCH	256    // formerly F (not more than UCHAR_MAX + 1)
#define THRESHOLD	3    // choose optimal value
#define PERC_FLAG	0x8000U

void encode(void);
void decode_start(void);
void decode(ushort count, uchar *buffer);

//--------- huf.cpp ----------
#define NC (UCHAR_MAX + MAXMATCH + 2 - THRESHOLD)
	// alphabet = {0, 1, 2, ..., NC - 1}
#define CBIT 9  // $\lfloor \log_2 NC \rfloor + 1$
#define CODE_BIT  16  // codeword length

extern ushort left[], right[];

void huf_encode_start(void);
void huf_decode_start(void);
ushort decode_c(void);
ushort decode_p(void);
void output(ushort c, ushort p);
void huf_encode_end(void);

//--------- maketbl.cpp ----------
void make_table(ushort nchar, uchar bitlen[],
				ushort tablebits, ushort table[]);

//--------- maketree.cpp ----------
ushort make_tree(ushort nparm, ushort freqparm[],
				uchar lenparm[], ushort codeparm[]);

#endif
