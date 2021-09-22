//
// Award BIOS Editor - io.cpp
// [ input/output ]
//
// $Id: io.cpp,v 1.3 2004/04/11 07:17:16 bpoint Exp $
//
//----------------------------------------------------------------------------
//
// Award BIOS Editor's LZH Engine was adapted from "ar" archiver written by 
// Haruhiko Okumura.
//

#include "lzhEngine.h"
#include <stdlib.h>
#include <string.h>

#define CRCPOLY  0xA001  /* ANSI CRC-16 */
						/* CCITT: 0x8408 */
#define UPDATE_CRC(c) \
	crc = crctable[(crc ^ (c)) & 0xFF] ^ (crc >> CHAR_BIT)

uint16_t bitbuf;
bool unpackable;

static uint16_t crctable[UCHAR_MAX + 1], crc;
static uint16_t subbitbuf;
static int    bitcount;

static uint8_t *inbase, *instreamptr, *outbase, *outstreamptr, *instreammax, *outstreammax;
static uint32_t insize, outsize;

void make_crctable(void)
{
	uint16_t i, j, r;

	for (i = 0; i <= UCHAR_MAX; i++) {
		r = i;
		for (j = 0; j < CHAR_BIT; j++)
			if (r & 1) r = (r >> 1) ^ CRCPOLY;
			else       r >>= 1;
		crctable[i] = r;
	}
}

void fillbuf(int n)  // Shift bitbuf n bits left, read n bits
{
	bitbuf <<= n;
	while (n > bitcount) {
		bitbuf |= subbitbuf << (n -= bitcount);
		if (insize != 0) {
			insize--;
			subbitbuf = (uint8_t) *instreamptr++;
		} else subbitbuf = 0;
		bitcount = CHAR_BIT;
	}
	bitbuf |= subbitbuf >> (bitcount -= n);
}

uint16_t getbits(int n)
{
	uint16_t x;

	x = bitbuf >> (BITBUFSIZ - n);  fillbuf(n);
	return x;
}

void putbits(int n, uint16_t x)  // Write rightmost n bits of x
{
	if (n < bitcount) {
		subbitbuf |= x << (bitcount -= n);
	} else {
		if (outsize < insize) {
			*outstreamptr++ = subbitbuf | (x >> (n -= bitcount));
			outsize++;
		} else unpackable = 1;
		if (n < CHAR_BIT) {
			subbitbuf = x << (bitcount = CHAR_BIT - n);
		} else {
			if (outsize < insize) {
				*outstreamptr++ = x >> (n - CHAR_BIT);
				outsize++;
			} else unpackable = 1;
			subbitbuf = x << (bitcount = 2 * CHAR_BIT - n);
		}
	}
}

/*
int fread_crc(uint8_t *p, int n, FILE *f)
{
	int i;

	i = n = fread(p, 1, n, f);  origsize += n;
	while (--i >= 0) UPDATE_CRC(*p++);
	return n;
}

void fwrite_crc(uint8_t *p, int n, FILE *f)
{
	if (fwrite(p, 1, n, f) < n) error("Unable to write");
	while (--n >= 0) UPDATE_CRC(*p++);
}
*/

int memread_crc(uint8_t *dest, int n)
{
	int i;

	if ((instreamptr + n) > instreammax)
		n = (int)(instreammax - instreamptr);

	memcpy(dest, instreamptr, n);
	instreamptr += n;

	i = n;
	while (--i >= 0)
		UPDATE_CRC(*dest++);

	return n;
}

void memwrite_crc(uint8_t *src, int n)
{
	int i;

	if ((outstreamptr + n) > outstreammax)
		n = (int)(outstreammax - outstreamptr);

	memcpy(outstreamptr, src, n);
	outstreamptr += n;

	i = n;
	while (--i >= 0)
		UPDATE_CRC(*src++);
}

void init_getbits(void)
{
	bitbuf = 0;  subbitbuf = 0;  bitcount = 0;
	fillbuf(BITBUFSIZ);
}

void init_putbits(void)
{
	bitcount = CHAR_BIT;  subbitbuf = 0;
}

//-----------------------------------

void ioInit(void *instream, uint32_t instreamsize, void *outstream, uint32_t outstreamsize)
{
	inbase		 = (uint8_t *)instream;
	outbase		 = (uint8_t *)outstream;
	instreamptr  = (uint8_t *)instream;
	outstreamptr = (uint8_t *)outstream;
	instreammax	 = (uint8_t *)instreamptr + instreamsize;
	outstreammax = (uint8_t *)outstreamptr + outstreamsize;

	insize  = instreamsize;
	outsize = 0; // outstreamsize;		// bad hack, but it *is* what we want... :/

	unpackable = 0;
	crc = INIT_CRC;
}

uint16_t ioGetCRC(void)
{
	return (crc ^ INIT_CRC);
}

uint32_t ioGetInSizeUsed(void)
{
	return (uint32_t) (instreamptr - inbase);
}

uint32_t ioGetOutSizeUsed(void)
{
	return (uint32_t) (outstreamptr - outbase);
}
