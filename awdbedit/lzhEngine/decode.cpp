//
// Award BIOS Editor - decode.cpp
//
// $Id: decode.cpp,v 1.3 2004/04/11 07:17:16 bpoint Exp $
//
//----------------------------------------------------------------------------
//
// Award BIOS Editor's LZH Engine was adapted from "ar" archiver written by 
// Haruhiko Okumura.
// 

#include "lzhEngine.h"

static int j;  /* remaining bytes to copy */

void decode_start(void)
{
	huf_decode_start();
	j = 0;
}

void decode(ushort count, uchar *buffer)
	/* The calling function must keep the number of
	   bytes to be processed.  This function decodes
	   either 'count' bytes or 'DICSIZ' bytes, whichever
	   is smaller, into the array 'buffer[]' of size
	   'DICSIZ' or more.
	   Call decode_start() once for each new file
	   before calling this function. */
{
	static ushort i;
	ushort r, c;

	r = 0;
	while (--j >= 0) {
		buffer[r] = buffer[i];
		i = (i + 1) & (DICSIZ - 1);
		if (++r == count) return;
	}
	for ( ; ; ) {
		c = decode_c();
		if (c <= UCHAR_MAX) {
			buffer[r] = (uchar)c;
			if (++r == count) return;
		} else {
			j = c - (UCHAR_MAX + 1 - THRESHOLD);
			i = (r - decode_p() - 1) & (DICSIZ - 1);
			while (--j >= 0) {
				buffer[r] = buffer[i];
				i = (i + 1) & (DICSIZ - 1);
				if (++r == count) return;
			}
		}
	}
}
