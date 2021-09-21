//
// Award BIOS Editor - wavedit.cpp
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: wavedit.cpp,v 1.3 2004/04/11 07:17:16 bpoint Exp $
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
#include "wavedit.h"
#include "resource.h"

// plugin necessities
static ulong myID, timerID;
static HINSTANCE hinst;

// structures required to playback the wave data
static WAVEFORMATEX *wfex = NULL;
static HWAVEOUT hWaveOut;
static WAVEHDR wh;

// all GDI objects required for drawing the wave data
static HDC hdc = NULL;
static HBITMAP waveBitmap;
static HBRUSH hDarkGreyBrush;
static HPEN hOrangePen, hCyanPen, hWhitePen;
static RECT area;
static SIZE areasz;


BOOL CALLBACK waveditDialogProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK aboutBoxProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void updateControls(HWND hdlg, fileEntry *fe);

void allocateGDIObjects(HWND hdlg);
void buildWaveBitmap(bool showPosMarker);
void blitWaveBitmap(HDC blitToDC);
void freeGDIObjects(void);


awdbeFuncTable waveditTable = {
	waveditDescription,
	waveditAboutBox,

	waveditInit,
	waveditOnLoad,
	waveditDetect,
	
	waveditCreateDialog,
	waveditUpdateDialog,
	waveditRefreshDialog,
	waveditOnDestroyDialog,

	waveditOnResizeDialog
};

awdbeItem waveditItemList[] = {
	{ 0, AWDBE_ITEM,	0x4012, "Startup WAVE sound[0]",	0 },
	{ 0, AWDBE_ITEM,	0x4013, "Startup WAVE sound[1]",	0 },
	{ 0, AWDBE_ITEM,	0x4014, "Startup WAVE sound[2]",	0 },
	{ 0, AWDBE_ITEM,	0x4015, "Startup WAVE sound[3]",	0 },
	{ 0, AWDBE_ITEM,	0x4016, "Startup WAVE sound[4]",	0 },
	{ 0, AWDBE_ITEM,	0x4017, "Startup WAVE sound[5]",	0 },
	{ 0, AWDBE_ITEM,	0x4018, "Startup WAVE sound[6]",	0 },
	{ 0, AWDBE_ITEM,	0x4019, "Startup WAVE sound[7]",	0 }
};


BOOL WINAPI DllMain(HINSTANCE hModule, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
			hinst = hModule;
			break;

		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;

		case DLL_PROCESS_DETACH:
			// on deinitialization, release our GDI objects
			freeGDIObjects();
			break;
	}

	return TRUE;
}


extern "C" __declspec(dllexport) awdbeFuncTable *awdbeRegisterPlugin(void)
{
	return &waveditTable;
}

char *waveditDescription(void)
{
	return "Wave Editor";
}

void waveditAboutBox(HWND parentWnd)
{
	DialogBox(hinst, MAKEINTRESOURCE(IDD_ABOUT), parentWnd, aboutBoxProc);
}

void waveditInit(ulong pluginID)
{
	// store our plugin ID
	myID = pluginID;

	// pass our supported component item list to the editor
	awdbeAddToItemList(myID, waveditItemList, sizeof(waveditItemList) / sizeof(awdbeItem));
}

void waveditOnLoad(fileEntry *fe, int count)
{
	// we're not worried about changing our item table based on any content in the BIOS image, so we do nothing here...
}

bool waveditDetect(fileEntry *fe)
{
	uchar *dptr = (uchar *)fe->data;
	ulong size;

	// check first for a RIFF tag
	if ((dptr[0] != 'R') || (dptr[1] != 'I') || (dptr[2] != 'F') || (dptr[3] != 'F'))
		return FALSE;

	dptr += 8;

	// check next for a WAVE chunk
	if ((dptr[0] != 'W') || (dptr[1] != 'A') || (dptr[2] != 'V') || (dptr[3] != 'E'))
		return FALSE;

	dptr += 4;

	// check for the 'fmt ' subchunk
	if ((dptr[0] != 'f') || (dptr[1] != 'm') || (dptr[2] != 't') || (dptr[3] != ' '))
		return FALSE;

	dptr += 4;

	// get the size of the format subchunk and advance
	size  = *(ulong *)dptr;
	dptr += (size + 4);

	// lastly, check for a 'data' subchunk
	if ((dptr[0] != 'd') || (dptr[1] != 'a') || (dptr[2] != 't') || (dptr[3] != 'a'))
		return FALSE;

	return TRUE;
}

HWND waveditCreateDialog(awdbeItem *item, fileEntry *fe, HWND parentWnd, RECT *rc)
{
	HWND hwnd;

	hwnd = CreateDialog(hinst, MAKEINTRESOURCE(IDD_WAVEDIT), parentWnd, waveditDialogProc);

	// allocate the GDI objects we need to draw the wave data, if we haven't yet done so...
	if (hdc == NULL)
		allocateGDIObjects(hwnd);

	// update the controls with the file data passed to us
	updateControls(hwnd, fe);

	return hwnd;
}

bool waveditUpdateDialog(awdbeItem *item, fileEntry *fe, HWND dialogWnd)
{
	char buf[256];
	ulong temp;
	bool modified;

	// assume no components have been modified...
	modified = FALSE;

	// get the data from our controls and update them in our file's data
	GetDlgItemText(dialogWnd, IDC_WAVE_FORMAT, buf, 256);
	sscanf(buf, "%04X", &temp);
	if ((ushort)temp != wfex->wFormatTag)
	{
		wfex->wFormatTag = (ushort)temp;
		modified = TRUE;
	}

	GetDlgItemText(dialogWnd, IDC_WAVE_SAMPLERATE, buf, 256);
	sscanf(buf, "%d", &temp);
	if (temp != wfex->nSamplesPerSec)
	{
		wfex->nSamplesPerSec = temp;
		modified = TRUE;
	}
	
	GetDlgItemText(dialogWnd, IDC_WAVE_CHANNELS, buf, 256);
	sscanf(buf, "%d", &temp);
	if ((ushort)temp != wfex->nChannels)
	{
		wfex->nChannels = (ushort)temp;
		modified = TRUE;
	}

	GetDlgItemText(dialogWnd, IDC_WAVE_BITSPERSAMPLE, buf, 256);
	sscanf(buf, "%d", &temp);
	if ((ushort)temp != wfex->wBitsPerSample)
	{
		wfex->wBitsPerSample = (ushort)temp;
		modified = TRUE;
	}

	// return the modified flag
	return modified;
}

void waveditRefreshDialog(awdbeItem *item, fileEntry *fe, HWND dialogWnd)
{
	updateControls(dialogWnd, fe);
}

void waveditOnDestroyDialog(awdbeItem *item, HWND dialogWnd)
{
}

void waveditOnResizeDialog(awdbeItem *item, HWND dialogWnd, RECT *rc)
{
}


//-----------------------------------------------------------------------

void CALLBACK timerFunc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	HDC dc;
	HWND hwnd = (HWND)dwUser;

	// get a dc to our dialog
	dc = GetDC(hwnd);

	// build the bitmap with a line marker
	buildWaveBitmap(TRUE);

	// blit it...
	blitWaveBitmap(dc);

	// and release the obtained dc
	ReleaseDC(hwnd, dc);
}

BOOL CALLBACK waveditDialogProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HICON hicon;
	HDC dc;
	PAINTSTRUCT ps;

	switch (message)
	{
		case WM_INITDIALOG:			
			// apply our play and stop icons to the buttons in our dialog
			hicon = LoadIcon(hinst, MAKEINTRESOURCE(IDI_PLAY));
			SendMessage(GetDlgItem(hdlg, IDC_WAVE_PLAY), BM_SETIMAGE, IMAGE_ICON, (LPARAM)hicon);
			DestroyIcon(hicon);

			hicon = LoadIcon(hinst, MAKEINTRESOURCE(IDI_STOP));
			SendMessage(GetDlgItem(hdlg, IDC_WAVE_STOP), BM_SETIMAGE, IMAGE_ICON, (LPARAM)hicon);
			DestroyIcon(hicon);

			// enable the play button and disable the stop button
			EnableWindow(GetDlgItem(hdlg, IDC_WAVE_PLAY), TRUE);
			EnableWindow(GetDlgItem(hdlg, IDC_WAVE_STOP), FALSE);
			return TRUE;

		case WM_PAINT:
			dc = BeginPaint(hdlg, &ps);

			// only reblit our wave bitmap...
			blitWaveBitmap(dc);

			EndPaint(hdlg, &ps);
			return 0;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_WAVE_PLAY:
					// have the bios editor call us to update our controls.  we have to do this because we want any changes
					// made to the controls be reflected when we playback our wave...
					awdbeUpdateSelf(myID);

					// open the default audio device
					if (waveOutOpen(&hWaveOut, WAVE_MAPPER, wfex, (DWORD)hdlg, 0, CALLBACK_WINDOW) != MMSYSERR_NOERROR)
					{
						MessageBox(hdlg, "Unable to open output device!", "Error", MB_OK);
						return FALSE;
					}

					// prepare our header
					if (waveOutPrepareHeader(hWaveOut, &wh, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
					{
						MessageBox(hdlg, "Unable to open output device!", "Error", MB_OK);
						return FALSE;
					}

					// play the output
					if (waveOutWrite(hWaveOut, &wh, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
					{
						MessageBox(hdlg, "Unable to play wave data!", "Error", MB_OK);
						return FALSE;
					}

					// setup a timer to show graphically what's playing (update every 15ms)
					timerID = timeSetEvent(15, 0, timerFunc, (DWORD)hdlg, TIME_PERIODIC);

					// all okay, disable the play button and enable the stop button
					EnableWindow(GetDlgItem(hdlg, IDC_WAVE_PLAY), FALSE);
					EnableWindow(GetDlgItem(hdlg, IDC_WAVE_STOP), TRUE);
					break;

				case IDC_WAVE_STOP:
					// reset the output device... this will also send a MM_WOM_DONE message to us which will close
					// the device and reset our buttons.
					waveOutReset(hWaveOut);
					break;
			}
			break;

		case MM_WOM_DONE:
			// kill our timer
			timeKillEvent(timerID);

			// unprepare our wave header
			waveOutUnprepareHeader((HWAVEOUT)wParam, (LPWAVEHDR)lParam, sizeof(WAVEHDR));

			// close the output device
			waveOutClose((HWAVEOUT)wParam);

			// reblit our wave data without a position marker
			dc = GetDC(hdlg);
			buildWaveBitmap(FALSE);
			blitWaveBitmap(dc);
			ReleaseDC(hdlg, dc);

			// and reset our buttons
			EnableWindow(GetDlgItem(hdlg, IDC_WAVE_PLAY), TRUE);
			EnableWindow(GetDlgItem(hdlg, IDC_WAVE_STOP), FALSE);
			break; 
	}

	return FALSE;
}

void updateControls(HWND hdlg, fileEntry *fe)
{
	uchar *dptr = (uchar *)fe->data;
	char buf[256];
	ulong fmtSize, wavSize;
	void *wavData;
	HDC dc;

	// make a pointer to the format subchunk
	wfex = (WAVEFORMATEX *)(dptr + 20);

	// make a pointer to the actual wave data and get its size
	fmtSize = *(ulong *)(dptr + 16);
	wavSize = *(ulong *)(dptr + 20 + fmtSize + 4);
	wavData = dptr + (20 + fmtSize + 8);

	// make sure wavSize isn't too big
	if (((uchar *)wavData + wavSize) > ((uchar *)fe->data + fe->size))
		wavSize = (ulong)((uchar *)fe->data + fe->size) - (ulong)(dptr + 20 + fmtSize + 8);

	// store this data in a WAVEHDR, which we'll use to playback the data
	ZeroMemory(&wh, sizeof(WAVEHDR));
	wh.lpData		  = (LPSTR)wavData;
	wh.dwBufferLength = wavSize;

	// update our controls
	sprintf(buf, "%04X", wfex->wFormatTag);
	SetDlgItemText(hdlg, IDC_WAVE_FORMAT, buf);

	sprintf(buf, "%d", wfex->nSamplesPerSec);
	SetDlgItemText(hdlg, IDC_WAVE_SAMPLERATE, buf);

	sprintf(buf, "%d", wfex->nChannels);
	SetDlgItemText(hdlg, IDC_WAVE_CHANNELS, buf);

	sprintf(buf, "%d", wfex->wBitsPerSample);
	SetDlgItemText(hdlg, IDC_WAVE_BITSPERSAMPLE, buf);

	// rebuild the wave bitmap and blit it
	dc = GetDC(hdlg);
	buildWaveBitmap(FALSE);
	blitWaveBitmap(dc);
	ReleaseDC(hdlg, dc);
}

void allocateGDIObjects(HWND hdlg)
{
	BITMAPINFO bmi;
	VOID *pvBits;

	// create a rectangle which will encompass the entire area in which we want to draw
	area.left   = 16;
	area.top    = 141;
	area.right  = 224;
	area.bottom = 181;

	// map these coordinates into dialog space
	MapDialogRect(hdlg, &area);

	// make a SIZE struct of the area
	areasz.cx = (area.right - area.left) + 1;
	areasz.cy = (area.bottom - area.top) + 1;

	// allocate our GDI objects (we have to do this because older versions of Windows [95/98/ME] don't release the
	// memory resources used by GDI objects when created/deleted dynamically... what a pain.)
	hdc = CreateCompatibleDC(NULL);
	hDarkGreyBrush	= CreateSolidBrush(RGB(96, 96, 96));
	hOrangePen		= CreatePen(PS_SOLID, 1, RGB(230, 127, 40));
	hCyanPen		= CreatePen(PS_SOLID, 1, RGB(40, 180, 255));
	hWhitePen		= CreatePen(PS_SOLID, 1, RGB(255, 255, 255));

	// create a bitmap which we'll use to draw our waveform data
	ZeroMemory(&bmi, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize		= sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth		= areasz.cx;
	bmi.bmiHeader.biHeight		= areasz.cy;
	bmi.bmiHeader.biPlanes		= 1;
	bmi.bmiHeader.biBitCount	= 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage	= bmi.bmiHeader.biWidth * bmi.bmiHeader.biHeight * 4;

	waveBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);

	// select some default objects and modes into our DC
	SelectObject(hdc, waveBitmap);
	SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));

	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, RGB(255, 255, 255));
}

void buildWaveBitmap(bool showPosMarker)
{
	RECT rc;
	POINT pt[5];
	TEXTMETRIC tm;
	int heightOffset, samplesPerPixel, count, yOffset, heightScale, pos;
	uchar *ptr8;
	signed char s8;
	signed short *ptr16;
	MMTIME mmt;

	// exit out if we don't have a valid pointer to the wave data
	if (wfex == NULL)
		return;

	// build a rectangle that we'll use for various drawing things...
	rc.left   = 0;
	rc.top    = 0;
	rc.right  = areasz.cx;
	rc.bottom = areasz.cy;

	// fill the inside with a dark grey color
	FillRect(hdc, &rc, hDarkGreyBrush);

	// make sure this data is in PCM format, and it is 8 or 16 bits...
	if ((wfex->wFormatTag != WAVE_FORMAT_PCM) || ((wfex->wBitsPerSample != 8) && (wfex->wBitsPerSample != 16)))
	{
		// determine the height offset of our current font
		GetTextMetrics(hdc, &tm);
		heightOffset = 15 - tm.tmHeight;

		// draw some notification text
		rc.left += 130;
		rc.top  += 20 + heightOffset;
		DrawText(hdc, "- Not PCM format -", -1, &rc, DT_NOCLIP | DT_TOP | DT_LEFT);
	}
	else
	{
		// calculate some vars needed to draw our waveform
		count   = areasz.cx - 2;
		yOffset = areasz.cy / 2;

		if (wfex->wBitsPerSample <= 8)
		{
			if (wfex->nChannels == 1)
				samplesPerPixel = wh.dwBufferLength / count;
			else
			{
				samplesPerPixel  = (wh.dwBufferLength / 2) / count;
				samplesPerPixel *= 2;
			}
		}
		else
		{
			if (wfex->nChannels == 1)
				samplesPerPixel = (wh.dwBufferLength / 2) / count;
			else
			{
				samplesPerPixel  = ((wh.dwBufferLength / 2) / 2) / count;
				samplesPerPixel *= 2;
			}
		}

		// draw a slightly red/orangish horizontal line through the middle
		SelectObject(hdc, hOrangePen);

		pt[0].x = 1;				pt[0].y = yOffset;
		pt[1].x = areasz.cx - 1;	pt[1].y = pt[0].y;
		Polyline(hdc, pt, 2);

		// use our cyan-like pen for drawing the waveform
		SelectObject(hdc, hCyanPen);

		// pick a draw function based on the bits per sample
		if (wfex->wBitsPerSample <= 8)
		{
			// calculate height scale (we're not using floating point, so it's not going to be precise... a little +1 will help)
			heightScale = (256 / ((area.bottom - area.top) - 2)) + 1;

			// setup initial pointer and position
			ptr8  = (uchar *)wh.lpData;
			s8    = (signed char)((*ptr8) - 128);
			ptr8 += samplesPerPixel;

			rc.left = 1;
			MoveToEx(hdc, rc.left, (s8 / heightScale) + yOffset, NULL);

			// draw it!
			while (count--)
			{
				rc.left++;

				s8 = (signed char)((*ptr8) - 128);

				LineTo(hdc, rc.left, (s8 / heightScale) + yOffset);
				ptr8 += samplesPerPixel;
			}
		}
		else
		{
			// calculate height scale (again, a little adjustment)
			heightScale = (32768 / ((area.bottom - area.top) - 2)) + 1;

			// setup initial pointer and position
			ptr16 = (signed short *)wh.lpData;

			rc.left = 1;
			MoveToEx(hdc, rc.left, (*ptr16 / heightScale) + yOffset, NULL);
			ptr16 += samplesPerPixel;

			// draw it!
			while (count--)
			{
				rc.left++;

				LineTo(hdc, rc.left, (*ptr16 / heightScale) + yOffset);
				ptr16 += samplesPerPixel;
			}
		}

		// lastly, draw the position marker, if we need to...
		if (showPosMarker)
		{
			// get the current position in the playback buffer
			mmt.wType = TIME_SAMPLES;
			waveOutGetPosition(hWaveOut, &mmt, sizeof(MMTIME));

			// calculate the horizontal position
			pos = mmt.u.sample / samplesPerPixel;

			if (wfex->nChannels > 1)
				pos *= 2;

			if (pos <= areasz.cx)
			{
				SelectObject(hdc, hWhitePen);

				pt[0].x = pos;	pt[0].y = 1;
				pt[1].x = pos;	pt[1].y = areasz.cy - 1;
				Polyline(hdc, pt, 2);
			}
		}
	}

	// finally, draw a border around the wave in black
	SelectObject(hdc, GetStockObject(BLACK_PEN));

	pt[0].x = 0;				pt[0].y = 0;
	pt[1].x = areasz.cx - 1;	pt[1].y = 0;
	pt[2].x = areasz.cx - 1;	pt[2].y = areasz.cy - 1;
	pt[3].x = 0;				pt[3].y = areasz.cy - 1;
	pt[4].x = 0;				pt[4].y = 0;
	Polyline(hdc, pt, 5);
}

void blitWaveBitmap(HDC blitToDC)
{
	// simply blit our bitmap into the specified DC
	BitBlt(blitToDC, area.left, area.top, (area.right - area.left) + 1, (area.bottom - area.top) + 1, hdc, 0, 0, SRCCOPY);
}

void freeGDIObjects(void)
{
	if (hdc != NULL)
	{
		// delete our GDI objects
		DeleteObject(waveBitmap);
		DeleteObject(hDarkGreyBrush);
		DeleteObject(hOrangePen);
		DeleteObject(hCyanPen);
		DeleteObject(hWhitePen);

		// now delete our DC
		DeleteDC(hdc);
		hdc = NULL;
	}
}

BOOL CALLBACK aboutBoxProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char buf[256];

	switch (message)
	{
		case WM_INITDIALOG:
			sprintf(buf, "Compiled on %s at %s", __DATE__, __TIME__);
			SetDlgItemText(hDlg, IDC_TEXT_COMPILETIME, buf);
			return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK)
				EndDialog(hDlg, TRUE);
			return TRUE;
	}

	return FALSE;
}
