//
// Award BIOS Editor - epa.cpp
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: epa.cpp,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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

#include <stdio.h>
#include <windows.h>
#include "types.h"
#include "../awdbedit/awdbe_exports.h"
#include "builtins.h"
#include "epa.h"
#include "resource.h"

#define MAKERGB(r, g, b)		((r << 16) | (g << 8) | (b))

static HWND hGroupBox = NULL;
static HBITMAP hBitmap = NULL;
static HDC hBitmapDC = NULL;
static RECT blitrc;
static BITMAPINFO bmi;
static VOID *pvBits;


uint32_t egaPalette[16] = {
	MAKERGB(  0,   0,   0), MAKERGB(0, 0, 128), MAKERGB(0, 128, 0), MAKERGB(0, 128, 128), MAKERGB(128, 0, 0), MAKERGB(128, 0, 128), MAKERGB(128, 128, 0), MAKERGB(192, 192, 192),
	MAKERGB(128, 128, 128), MAKERGB(0, 0, 255), MAKERGB(0, 255, 0), MAKERGB(0, 255, 255), MAKERGB(255, 0, 0), MAKERGB(255, 0, 255), MAKERGB(255, 255, 0), MAKERGB(255, 255, 255)
};


uint16_t getEPALogoVersion(uint8_t *data, uint32_t size)
{
	int xs, ys;

	// check first for version 1 type formats...
	xs = data[0];
	ys = data[1];

	if ((2 + (xs * ys) + (xs * ys * 14) + 70) == size)
		return 1;

	// now check for version 2's header
	if ((data[0] == 'A') && (data[1] == 'W') && (data[2] == 'B') && (data[3] == 'M'))
		return 2;

	return 0;
}

bool isEPALogo(uint8_t *data, uint32_t size)
{
	if (getEPALogoVersion(data, size) != 0)
		return TRUE;

	return FALSE;
}

INT_PTR CALLBACK epaLogoFunc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC dc;
	PAINTSTRUCT ps;
	OPENFILENAME ofn;
	char buf[256], fname[256];
	FILE *fp;
	BITMAPFILEHEADER bmfh;
	BITMAPV4HEADER bh;
	uint32_t count;
	uint8_t *tempbuf, *sptr, *dptr;

	switch (message)
	{
		case WM_INITDIALOG:
			return TRUE;

		case WM_PAINT:
			dc = BeginPaint(hdlg, &ps);

			// simply blit our preview bitmap...
			epaBlitBitmap(dc);

			EndPaint(hdlg, &ps);
			return 0;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_EPA_EXPORT:
					GetDlgItemText(hdlg, IDC_FILENAME, buf, 256);
					_splitpath(buf, NULL, NULL, fname, NULL);
					strcat(fname, ".bmp");

					// display the save dialog
					ZeroMemory(&ofn, sizeof(OPENFILENAME));
					ofn.lStructSize			= sizeof(OPENFILENAME);
					ofn.hwndOwner			= hdlg;
					ofn.hInstance			= hinst;
					ofn.lpstrFilter			= "Windows BMP (*.bmp)\0*.bmp\0All Files (*.*)\0*.*\0\0";
					ofn.lpstrCustomFilter	= NULL;
					ofn.nMaxCustFilter		= 0;
					ofn.nFilterIndex		= 1;
					ofn.lpstrFile			= fname;
					ofn.nMaxFile			= 256;
					ofn.lpstrFileTitle		= NULL;
					ofn.nMaxFileTitle		= 0;
					ofn.lpstrInitialDir		= NULL;
					ofn.lpstrTitle			= NULL;
					ofn.Flags				= OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_ENABLESIZING | OFN_EXPLORER;
					ofn.nFileOffset			= 0;
					ofn.nFileExtension		= 0;
					ofn.lpstrDefExt			= "bmp";
					ofn.lCustData			= NULL;
					ofn.lpfnHook			= NULL;
					ofn.lpTemplateName		= NULL;

					if (GetSaveFileName(&ofn) == FALSE)
						return FALSE;

					// make some structures
					bmfh.bfType		 = 0x4D42;
					bmfh.bfSize		 = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPV4HEADER) + (bmi.bmiHeader.biWidth * bmi.bmiHeader.biHeight * 3);
					bmfh.bfReserved1 = 0;
					bmfh.bfReserved2 = 0;
					bmfh.bfOffBits	 = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPV4HEADER);

					ZeroMemory(&bh, sizeof(BITMAPV4HEADER));
					bh.bV4Size			= sizeof(BITMAPV4HEADER);
					bh.bV4Width			= bmi.bmiHeader.biWidth;
					bh.bV4Height		= bmi.bmiHeader.biHeight;
					bh.bV4Planes		= 1;
					bh.bV4BitCount		= 24;
					bh.bV4V4Compression	= bmi.bmiHeader.biCompression;
					bh.bV4SizeImage		= bmi.bmiHeader.biWidth * bmi.bmiHeader.biHeight * 3;
					bh.bV4CSType		= LCS_sRGB;

					// downconvert to 24-bit
					count	= bh.bV4Width * bh.bV4Height;
					tempbuf	= new uint8_t[count * 3];
					
					sptr = (uint8_t *)pvBits;
					dptr = tempbuf;

					while (count--)
					{
						*dptr++ = *sptr++;
						*dptr++ = *sptr++;
						*dptr++ = *sptr++;
						sptr++;
					}

					// write out a BMP
					fp = fopen(fname, "wb");
					fwrite(&bmfh, 1, sizeof(BITMAPFILEHEADER), fp);
					fwrite(&bh, 1, sizeof(BITMAPV4HEADER), fp);
					fwrite(tempbuf, bh.bV4Width * 3, bh.bV4Height, fp);
					fclose(fp);

					// free our downconverted buffer
					delete []tempbuf;
					break;
			}
			break;
	}

	return FALSE;
}

HWND epaCreateDialog(HWND parentWnd)
{
	return CreateDialog(hinst, MAKEINTRESOURCE(IDD_EPALOGO), parentWnd, epaLogoFunc);
}

void epaRefreshDialog(HWND hwnd, fileEntry *fe)
{
	uint8_t *data8 = (uint8_t *)fe->data;
	uint16_t *data16 = (uint16_t *)fe->data;
	int width, height;
	char buf[256];
	RECT rc;
	HDC hdc;
	int ver;
	SIZE sz;

	ver = getEPALogoVersion(data8, fe->size);
	switch (ver)
	{
		case 1:
			SetDlgItemText(hwnd, IDC_EPA_VERSION, "1.0");

			width  = data8[0] * 8;
			height = data8[1] * 14;
			break;

		case 2:
			SetDlgItemText(hwnd, IDC_EPA_VERSION, "2.0");

			width  = data16[2];
			height = data16[3];
			break;

		default:
			SetDlgItemText(hwnd, IDC_EPA_VERSION, "Unknown");

			width  = 0;
			height = 0;
			break;
	}

	sprintf(buf, "%d", width);
	SetDlgItemText(hwnd, IDC_EPA_WIDTH, buf);

	sprintf(buf, "%d", height);
	SetDlgItemText(hwnd, IDC_EPA_HEIGHT, buf);

	// delete the current groupbox, preview bitmap, and DC... if they exist
	epaOnDestroyDialog(hwnd);

	// exit here if invalid (shouldn't happen, but...)
	if ((width == 0) || (height == 0))
		return;

	// get a DC to our dialog
	hdc = GetDC(hwnd);

	// create a rect to display our preview
	rc.left   = 7;
	rc.top    = 125;
	rc.right  = 0;
	rc.bottom = 0;

	// convert position into dialog units
	MapDialogRect(hwnd, &rc);

	// create group box and set the font, then make it visible
	hGroupBox = CreateWindow("BUTTON", "Preview", BS_GROUPBOX | WS_CHILD | WS_GROUP, rc.left, rc.top, width + 20,
		height + 30, hwnd, NULL, hinst, NULL);

	SendMessage(hGroupBox, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(FALSE, 0));
	ShowWindow(hGroupBox, SW_SHOWNORMAL);

	// create a DC for our bitmap
	hBitmapDC = CreateCompatibleDC(NULL);

	// create a 32-bit bitmap of needed size
	ZeroMemory(&bmi, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize		= sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth		= width;
	bmi.bmiHeader.biHeight		= height;
	bmi.bmiHeader.biPlanes		= 1;
	bmi.bmiHeader.biBitCount	= 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage	= bmi.bmiHeader.biWidth * bmi.bmiHeader.biHeight * 4;

	hBitmap = CreateDIBSection(hBitmapDC, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
	SelectObject(hBitmapDC, hBitmap);

	// use the pvBits pointer to render the preview based on its version...
	switch (ver)
	{
		case 1:
			epaMakeV1Bitmap((uint8_t *)fe->data, (uint8_t *)pvBits);
			break;

		case 2:
			// check for VGA format
			if ((width == 640) && (height == 480))
				epaMakeV2VGABitmap((uint8_t *)fe->data, fe->size, (uint8_t *)pvBits);
			else
				epaMakeV2Bitmap((uint8_t *)fe->data, fe->size, (uint8_t *)pvBits);
			break;
	}

	// flip the bitmap (because Windows is silly)
	epaFlipBitmap((uint8_t *)pvBits, width, height);

	// setup our blit position
	blitrc.left   = rc.left + 10;
	blitrc.top    = rc.top + 18;
	blitrc.right  = width;
	blitrc.bottom = height;

	// make sure our client area is large enough...
	awdbeGetDialogSize(myID, &sz);

	if ((blitrc.left + width + 20) > sz.cx)
		sz.cx = blitrc.left + width + 20;

	if ((blitrc.top + height + 20) > sz.cy)
		sz.cy = blitrc.top + height + 20;

	awdbeResizeDialog(myID, sz);

	// blit the bitmap
	epaBlitBitmap(hdc);

	// release our DC and we're done!
	ReleaseDC(hwnd, hdc);
}

void epaOnDestroyDialog(HWND hwnd)
{
	// release all of our objects
	if (hGroupBox != NULL)
	{
		DestroyWindow(hGroupBox);
		hGroupBox = NULL;
	}

	if (hBitmap != NULL)
	{
		DeleteObject(hBitmap);
		hBitmap = NULL;
	}

	if (hBitmapDC != NULL)
	{
		DeleteDC(hBitmapDC);
		hBitmapDC = NULL;
	}
}

void epaMakeV1Bitmap(uint8_t *data, uint8_t *outmap)
{
	int xs, ys, x, y, span, bc;
	uint8_t *bptr, *aptr, b, a;
	uint32_t *dptr, **rowptrs;

	// get x/y size
	xs = data[0];
	ys = data[1];

	// make row pointers into the output map for easy lookup
	rowptrs = new uint32_t *[ys];
	for (y = 0; y < ys; y++)
		rowptrs[y] = ((uint32_t *)outmap) + ((14 * y) * (8 * xs));

	// setup attribute and bitmap pointers
	aptr = data + 2;
	bptr = data + (2 + (xs * ys));	

	// make the bitmap
	for (y = 0; y < ys; y++)
	{
		for (x = 0; x < xs; x++)
		{
			a    = *aptr++;
			dptr = rowptrs[y] + (8 * x);

			span = 14;
			while (span--)
			{
				b = *bptr++;
				
				bc = 8;
				while (bc--)
				{
					if (b & 0x80)
						*dptr = egaPalette[a & 0x0F];
					else
						*dptr = egaPalette[a >> 4];

					dptr++;
					b <<= 1;
				}

				dptr += 8 * (xs - 1);
			}
		}
	}

	// cleanup
	delete []rowptrs;
}

void epaMakeV2Bitmap(uint8_t *data, uint32_t size, uint8_t *outmap)
{
	int xs, ys, x, y, bc, ci;
	uint8_t *pptr, *blay, *glay, *rlay, *ilay, bbyte, gbyte, rbyte, ibyte;
	uint16_t *szptr;
	uint32_t customPalette[16], *curPal, *dptr;

	// get x/y size
	szptr = (uint16_t *)data;
	xs    = szptr[2] / 8;
	ys    = szptr[3];

	// assign current palette as default EGA, but check for custom palette too...
	curPal = egaPalette;
	pptr   = data + (8 + (xs * ys * 4));

	if (pptr < (data + size))
	{
		if ((pptr[0] == 'R') && (pptr[1] == 'G') && (pptr[2] == 'B') && (pptr[3] == ' '))
		{
			// set palette pointer to custom palette
			curPal = customPalette;

			// read custom palette data
			pptr += 4;

			for (x = 0; x < 16; x++)
			{
				customPalette[x] = MAKERGB(pptr[0] * 4, pptr[1] * 4, pptr[2] * 4);
				pptr += 3;
			}
		}
	}

	// calculate our RGBI layer pointers
	rlay = (uint8_t *)data + 8;
	glay = rlay + (xs * 1);
	blay = rlay + (xs * 2);
	ilay = rlay + (xs * 3);
	dptr = (uint32_t *)outmap;

	// make the bitmap!
	for (y = 0; y < ys; y++)
	{
		for (x = 0; x < xs; x++)
		{
			rbyte = *rlay++;
			gbyte = *glay++;
			bbyte = *blay++;
			ibyte = *ilay++;

			bc = 8;
			while (bc--)
			{
				// calculate a color index
				ci = 0;
				if (bbyte & 0x80) ci |= 1;
				if (gbyte & 0x80) ci |= 2;
				if (rbyte & 0x80) ci |= 4;
				if (ibyte & 0x80) ci |= 8;

				// use this index to draw a pixel
				*dptr++ = curPal[ci];

				// shift bitmasks
				rbyte <<= 1;
				gbyte <<= 1;
				bbyte <<= 1;
				ibyte <<= 1;
			}
		}

		// advance layer pointers
		rlay += (xs * 3);
		glay += (xs * 3);
		blay += (xs * 3);
		ilay += (xs * 3);
	}
}

void epaMakeV2VGABitmap(uint8_t *data, uint32_t size, uint8_t *outmap)
{
	int xs, ys, x, y, count;
	uint8_t *sptr, *pptr;
	uint16_t *szptr;
	uint32_t customPalette[256], *curPal, *dptr;

	// get x/y size
	szptr = (uint16_t *)data;
	xs    = szptr[2];
	ys    = szptr[3];

	// assign current palette as default EGA, but check for custom palette too...
	curPal = egaPalette;
	pptr   = data + (8 + (xs * ys));

	if (pptr < (data + size))
	{
		if ((pptr[0] == 'R') && (pptr[1] == 'G') && (pptr[2] == 'B') && (pptr[3] == ' '))
		{
			// set palette pointer to custom palette
			curPal = customPalette;
			pptr += 4;

			// calculate count of RGB entries
			count = (size - (pptr - data)) / 3;
			if (count > 256)
				count = 256;

			// read custom palette data
			for (x = 0; x < count; x++)
			{
				customPalette[x] = MAKERGB(pptr[0] * 4, pptr[1] * 4, pptr[2] * 4);
				pptr += 3;
			}
		}
	}

	// calculate our RGBI layer pointers
	sptr = (uint8_t *)data + 8;
	dptr = (uint32_t *)outmap;

	// make the bitmap!
	for (y = 0; y < ys; y++)
	{
		for (x = 0; x < xs; x++)
		{
			// use this index to draw a 32-bit RGBA pixel
			*dptr++ = curPal[*sptr++];
		}
	}
}


void epaFlipBitmap(uint8_t *outmap, int width, int height)
{
	uint8_t *strip, *tptr, *bptr;
	int y, wx4, hd2;

	wx4 = width * 4;
	hd2 = height / 2;

	// allocate a strip buffer for flipping
	strip = new uint8_t[wx4];

	// setup pointers
	tptr = outmap;
	bptr = outmap + (wx4 * (height - 1));

	// flip it
	for (y = 0; y < hd2; y++)
	{
		// flip top and bottom strips
		memcpy(strip, tptr, wx4);
		memcpy(tptr, bptr, wx4);
		memcpy(bptr, strip, wx4);

		// adjust pointers
		tptr += wx4;
		bptr -= wx4;
	}

	// free strip buffer
	delete []strip;
}

void epaBlitBitmap(HDC blitToDC)
{
	BitBlt(blitToDC, blitrc.left, blitrc.top, blitrc.right, blitrc.bottom, hBitmapDC, 0, 0, SRCCOPY);
}
