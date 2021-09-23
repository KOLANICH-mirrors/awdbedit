//
// Award BIOS Editor - sysbios_main.cpp
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: sysbios_main.cpp,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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
#include <string.h>
#include <windows.h>
#include <commctrl.h>
#include "types.h"
#include "../awdbedit/awdbe_exports.h"
#include "builtins.h"
#include "sysbios.h"
#include "sysbios_main.h"
#include "resource.h"


INT_PTR CALLBACK sysbiosMainFunc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hwnd;
	uint32_t len, maxlen;
	char buf[256];

	switch (message)
	{
		case WM_INITDIALOG:
			SetDlgItemText(hdlg, IDC_TEXT_SYSBIOS_VERSION, "");
			SetDlgItemText(hdlg, IDC_TEXT_SYSBIOS_INFO,	   "");
			SetDlgItemText(hdlg, IDC_TEXT_SYSBIOS_MESSAGE, "");
			SetDlgItemText(hdlg, IDC_TEXT_SYSBIOS_ID,      "");
			return TRUE;

		case WM_COMMAND:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				hwnd = (HWND)lParam;

				// get the limit of this control we previously set
				maxlen = static_cast<decltype(maxlen)>(SendMessage(hwnd, EM_GETLIMITTEXT, 0, 0));

				// clear our buffer, and set it's length
				memset(buf, 0, 256);
				*(WORD *)buf = sizeof(buf);

				// get the string
				SendMessage(hwnd, EM_GETLINE, 0, (LPARAM)buf);

				// use the string's length to update the amount of bytes used, etc...
				len = strlen(buf);
				sysbiosUpdateLimit(hdlg, LOWORD(wParam), len, maxlen);
			}
			else
			{
				switch (LOWORD(wParam))
				{
					case IDC_CHECKSUM_RECALC:
						sysbiosRecalcChecksum(TRUE);
						break;
				}
			}
			break;
	}

	return FALSE;
}

INT_PTR CALLBACK VerDiffProc(HWND hdlg, UINT message, WPARAM wParam, [[maybe_unused]] LPARAM lParam)
{
	char buf[256];

	switch (message)
	{
		case WM_INITDIALOG:
			switch (sysbiosVersion)
			{
				case awdbeBIOSVer451PG:   strcpy(buf, "v4.51PG"); break;
				case awdbeBIOSVer600PG:   strcpy(buf, "v6.00PG"); break;
				case awdbeBIOSVer60:      strcpy(buf, "v6.0");    break;
				case awdbeBIOSVerUnknown: strcpy(buf, "Unknown"); break;
			}

			SetDlgItemText(hdlg, IDC_VER_PREV, buf);

			switch (sysbiosNowVer)
			{
				case awdbeBIOSVer451PG:   strcpy(buf, "v4.51PG"); break;
				case awdbeBIOSVer600PG:   strcpy(buf, "v6.00PG"); break;
				case awdbeBIOSVer60:      strcpy(buf, "v6.0");    break;
				case awdbeBIOSVerUnknown: strcpy(buf, "Unknown"); break;
			}

			SetDlgItemText(hdlg, IDC_VER_CURR, buf);
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDYES:
				case IDNO:
				case IDCANCEL:
					EndDialog(hdlg, LOWORD(wParam));
					break;
			}
			break;
	}

	return FALSE;
}


void sysbiosRefreshMain(uint8_t *ptr)
{
	uint8_t *sptr, csum;
	unsigned int len;
	char buf[256];

	// get bios version
	sptr = ptr + 0x1E060;
	len  = (*sptr++) - 1;
	SendMessage(GetDlgItem(sysbiosTabList[0].hwnd, IDC_SYSBIOS_VERSION), EM_SETLIMITTEXT, len, 0);

	memcpy(buf, sptr, len);
	buf[len] = 0;
	SetDlgItemText(sysbiosTabList[0].hwnd, IDC_SYSBIOS_VERSION, buf);
	sysbiosUpdateLimit(sysbiosTabList[0].hwnd, IDC_SYSBIOS_VERSION, strlen(buf), len);

	// get bios info string
	sptr = ptr + 0x1EC70;
	len  = (*sptr++) - 1;
	SendMessage(GetDlgItem(sysbiosTabList[0].hwnd, IDC_SYSBIOS_INFO), EM_SETLIMITTEXT, len, 0);

	memcpy(buf, sptr, len);
	buf[len] = 0;
	SetDlgItemText(sysbiosTabList[0].hwnd, IDC_SYSBIOS_INFO, buf);
	sysbiosUpdateLimit(sysbiosTabList[0].hwnd, IDC_SYSBIOS_INFO, strlen(buf), len);

	// get bios message
	sptr = ptr + 0x1E0C0;
	len  = (*sptr++) - 1;
	SendMessage(GetDlgItem(sysbiosTabList[0].hwnd, IDC_SYSBIOS_MESSAGE), EM_SETLIMITTEXT, len, 0);

	memcpy(buf, sptr, len);
	buf[len] = 0;
	SetDlgItemText(sysbiosTabList[0].hwnd, IDC_SYSBIOS_MESSAGE, buf);
	sysbiosUpdateLimit(sysbiosTabList[0].hwnd, IDC_SYSBIOS_MESSAGE, strlen(buf), len);

	// get bios id string
	sptr = ptr + 0x1ECE0;
	len  = sysbiosFindLimit(sptr);
	SendMessage(GetDlgItem(sysbiosTabList[0].hwnd, IDC_SYSBIOS_ID), EM_SETLIMITTEXT, len, 0);

	memcpy(buf, sptr, len);
	buf[len] = 0;
	SetDlgItemText(sysbiosTabList[0].hwnd, IDC_SYSBIOS_ID, buf);
	sysbiosUpdateLimit(sysbiosTabList[0].hwnd, IDC_SYSBIOS_ID, strlen(buf), len);

	// update checksum
	if (sysbiosVersion != awdbeBIOSVer600PG)
	{
		SetDlgItemText(sysbiosTabList[0].hwnd, IDC_CHECKSUM_RANGE_START, "10000");
		SetDlgItemText(sysbiosTabList[0].hwnd, IDC_CHECKSUM_RANGE_END,   "1FFFE");

		CheckDlgButton(sysbiosTabList[0].hwnd, IDC_CHECKSUM_METHOD_ADD, BST_UNCHECKED);
		CheckDlgButton(sysbiosTabList[0].hwnd, IDC_CHECKSUM_METHOD_SUB, BST_CHECKED);
		
		sptr = ptr + 0x1FFFF;
		sprintf(buf, "%02X", *sptr);
		SetDlgItemText(sysbiosTabList[0].hwnd, IDC_CHECKSUM_VALUE, buf);

		csum = sysbiosCalcBiosChecksum(ptr, 0x10000, 0x1FFFE, METHOD_SUB);

		if (csum == *sptr)
			SetDlgItemText(sysbiosTabList[0].hwnd, IDC_CHECKSUM_RESULT, "Pass");
		else
			SetDlgItemText(sysbiosTabList[0].hwnd, IDC_CHECKSUM_RESULT, "FAIL");
	}
	else
	{
		SetDlgItemText(sysbiosTabList[0].hwnd, IDC_CHECKSUM_RANGE_START, "0");
		SetDlgItemText(sysbiosTabList[0].hwnd, IDC_CHECKSUM_RANGE_END,   "0");

		CheckDlgButton(sysbiosTabList[0].hwnd, IDC_CHECKSUM_METHOD_ADD, BST_UNCHECKED);
		CheckDlgButton(sysbiosTabList[0].hwnd, IDC_CHECKSUM_METHOD_SUB, BST_UNCHECKED);
		
		SetDlgItemText(sysbiosTabList[0].hwnd, IDC_CHECKSUM_VALUE, "--");
		SetDlgItemText(sysbiosTabList[0].hwnd, IDC_CHECKSUM_RESULT, "None");
	}
}

bool sysbiosUpdateMain(uint8_t *ptr, bool *modified, awdbeBIOSVersion *vers)
{
	uint8_t *sptr;
	uint8_t len;
	int res;
	char buf[256], tempbuf[256];

	// update bios version
	sptr = ptr + 0x1E060;
	len  = (*sptr++) - 1;
	GetDlgItemText(sysbiosTabList[0].hwnd, IDC_SYSBIOS_VERSION, buf, 256);

	memcpy(tempbuf, sptr, len);
	tempbuf[len] = 0;
	if (strcmp(buf, tempbuf))
	{
		// have to be careful about changes to the bios version...
		sysbiosNowVer = sysbiosGetVersion((uint8_t *)buf, strnlen(buf, sizeof(buf) / sizeof(buf[0])));
		if (sysbiosNowVer != sysbiosVersion)
		{
			res = DialogBox(hinst, MAKEINTRESOURCE(IDD_SYSBIOS_VER_DIFF), sysbiosTabList[0].hwnd, VerDiffProc);
			switch (res)
			{
				case IDYES:
					// do nothing, 'vers' is valid.
					break;

				case IDNO:
					*vers = sysbiosNowVer;
					break;

				case IDCANCEL:
					return FALSE;
			}

			sysbiosVersion = sysbiosNowVer;
		}

		memset(sptr, 0, len);
		memcpy(sptr, buf, strlen(buf));

		*modified = TRUE;
	}

	// update bios info string
	sptr = ptr + 0x1EC70;
	len  = (*sptr++) - 1;
	GetDlgItemText(sysbiosTabList[0].hwnd, IDC_SYSBIOS_INFO, buf, 256);

	memcpy(tempbuf, sptr, len);
	tempbuf[len] = 0;
	if (strcmp(buf, tempbuf))
	{
		memset(sptr, 0, len);
		memcpy(sptr, buf, strlen(buf));

		*modified = TRUE;
	}

	// update bios message
	sptr = ptr + 0x1E0C0;
	len  = (*sptr++) - 1;
	GetDlgItemText(sysbiosTabList[0].hwnd, IDC_SYSBIOS_MESSAGE, buf, 256);

	memcpy(tempbuf, sptr, len);
	tempbuf[len] = 0;
	if (strcmp(buf, tempbuf))
	{
		memset(sptr, 0, len);
		memcpy(sptr, buf, strlen(buf));

		*modified = TRUE;
	}

	// get bios id string
	sptr = ptr + 0x1ECE0;
	len  = sysbiosFindLimit(sptr);
	GetDlgItemText(sysbiosTabList[0].hwnd, IDC_SYSBIOS_ID, buf, 256);

	memcpy(tempbuf, sptr, len);
	tempbuf[len] = 0;
	if (strcmp(buf, tempbuf))
	{
		memset(sptr, 0, len);
		memcpy(sptr, buf, strlen(buf));

		*modified = TRUE;
	}

	return TRUE;
}
