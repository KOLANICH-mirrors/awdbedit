//
// Award BIOS Editor - sysbios_options.cpp
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: sysbios_options.cpp,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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
#include <commctrl.h>
#include "types.h"
#include "../awdbedit/awdbe_exports.h"
#include "builtins.h"
#include "sysbios.h"
#include "sysbios_options.h"
#include "resource.h"


BOOL CALLBACK sysbiosBIOSOptionsFunc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	return FALSE;
}

void sysbiosRefreshBIOSOptions(uchar *ptr)
{
	uchar *sptr;
	ushort *ptr16;
	char buf[256], tempbuf[16];
	int t;

	// numlock state, bit 0 clear to enable
	sptr = ptr + 0x1FFE2;
	if ((*sptr) & 0x01)
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_BOOTUP_NUMLOCK, BST_UNCHECKED);
	else
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_BOOTUP_NUMLOCK, BST_CHECKED);

	// display pci configuration, bit 1 clear to enable
	if (sysbiosVersion != awdbeBIOSVer600PG)
		sptr = ptr + 0x1FFE9;
	else
		sptr = ptr + 0x1FFEC;

	if ((*sptr) & 0x02)
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_BOOTUP_DISPPCICONF, BST_UNCHECKED);
	else
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_BOOTUP_DISPPCICONF, BST_CHECKED);
	
	// power on delay count
	ptr16 = (ushort *)(ptr + 0x1FEA8);
	sprintf(buf, "%d", *ptr16);
	SetDlgItemText(sysbiosTabList[4].hwnd, IDC_SYSBIOS_BOOTUP_POWERONDELAY, buf);

	// setup default color value, lower 4 bits
	sptr = ptr + 0x1FEAA;
	sprintf(buf, "%d", (*sptr) & 0x0F);
	SetDlgItemText(sysbiosTabList[4].hwnd, IDC_SYSBIOS_COLOR_SETUPDEFAULT, buf);

	// post default color value, upper 4 bits
	sptr = ptr + 0x1FEAA;
	sprintf(buf, "%d", ((*sptr) & 0xF0) >> 4);
	SetDlgItemText(sysbiosTabList[4].hwnd, IDC_SYSBIOS_COLOR_POSTDEFAULT, buf);
	
	// post color option stored in bit 0
	sptr = ptr + 0x1FEAB;
	if ((*sptr) & 0x01)
	{
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_COLOR_OPTSETUP, BST_CHECKED);
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_COLOR_OPTPOST, BST_UNCHECKED);
	}
	else
	{
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_COLOR_OPTPOST, BST_CHECKED);
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_COLOR_OPTSETUP, BST_UNCHECKED);
	}

	// security password (hash only displayed for now)
	sptr = ptr + 0x1EC60;

	if (sysbiosVersion != awdbeBIOSVer600PG)
	{
		sprintf(buf, "%02X %02X", *sptr, *(sptr + 1));
	}
	else
	{
		buf[0] = 0;

		for (t = 0; t < 7; t++)
		{
			sprintf(tempbuf, "%02X ", *sptr++);
			strcat(buf, tempbuf);
		}

		sprintf(tempbuf, "%02X", *sptr);
		strcat(buf, tempbuf);
	}

	SetDlgItemText(sysbiosTabList[4].hwnd, IDC_SYSBIOS_SECURITY_PASSWORD, buf);

	// security number of retries
	if (sysbiosVersion != awdbeBIOSVer600PG)
		sptr = ptr + 0x1EC62;
	else
		sptr = ptr + 0x1EC68;

	sprintf(buf, "%d", *sptr);
	SetDlgItemText(sysbiosTabList[4].hwnd, IDC_SYSBIOS_SECURITY_NUMRETRIES, buf);
	
	// floppy speed switching, bit 1 clear to enable
	sptr = ptr + 0x1FFE8;
	if ((*sptr) & 0x02)
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_FLOPPY_SPEEDSWITCH, BST_UNCHECKED);
	else
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_FLOPPY_SPEEDSWITCH, BST_CHECKED);

	// PIE wait for floppy, bit 7 clear to enable
	sptr = ptr + 0x1FFEA;
	if ((*sptr) & 0x80)
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_FLOPPY_PIEWAIT, BST_UNCHECKED);
	else
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_FLOPPY_PIEWAIT, BST_CHECKED);

	// DRAM parity check, bit 2 clear to enable
	sptr = ptr + 0x1FFEC;
	if ((*sptr) & 0x04)
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_MISC_DRAMPARITY, BST_UNCHECKED);
	else
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_MISC_DRAMPARITY, BST_CHECKED);

	// keylock check, bit 3 clear to enable
	if ((*sptr) & 0x08)
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_BOOTUP_KEYLOCK, BST_UNCHECKED);
	else
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_BOOTUP_KEYLOCK, BST_CHECKED);

	// onboard E000h ROM check, bit 4 set to enable
	if ((*sptr) & 0x10)
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_VIDEO_ROME000, BST_CHECKED);
	else
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_VIDEO_ROME000, BST_UNCHECKED);

	// onboard C000h ROM check, bit 5 set to enable
	if ((*sptr) & 0x20)
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_VIDEO_ROMC000, BST_CHECKED);
	else
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_VIDEO_ROMC000, BST_UNCHECKED);

	// manufacture loop check, bit 6 clear to enable
	if ((*sptr) & 0x40)
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_MISC_LOOPCHECK, BST_UNCHECKED);
	else
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_MISC_LOOPCHECK, BST_CHECKED);

	// ps/2 mouse support, 0x1FFEC bit 7 set (if not v6.00PG, 0x1E6FA bit 2 is also set) to enable
	if ((*sptr) & 0x80)
	{
		if (sysbiosVersion != awdbeBIOSVer600PG)
		{
			sptr = ptr + 0x1E6FA;
			if ((*sptr) & 0x04)
				CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_BOOTUP_PS2MOUSE, BST_CHECKED);
			else
				CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_BOOTUP_PS2MOUSE, BST_UNCHECKED);
		}
		else
		{
			CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_BOOTUP_PS2MOUSE, BST_CHECKED);
		}
	}
	else
	{
		CheckDlgButton(sysbiosTabList[4].hwnd, IDC_SYSBIOS_BOOTUP_PS2MOUSE, BST_UNCHECKED);
	}
}

void sysbiosUpdateBIOSOptions(uchar *ptr, bool *modified, awdbeBIOSVersion vers)
{
	uchar *sptr;
	ushort *ptr16;
	char buf[256];
	ulong temp32;

	// numlock state, bit 0 clear to enable
	sptr = ptr + 0x1FFE2;
	if (IsDlgButtonChecked(sysbiosTabList[4].hwnd, IDC_SYSBIOS_BOOTUP_NUMLOCK) == BST_CHECKED)
		*sptr &= ~0x01;
	else
		*sptr |= 0x01;

	// display pci configuration, bit 1 clear to enable
	if (sysbiosVersion != awdbeBIOSVer600PG)
		sptr = ptr + 0x1FFE9;
	else
		sptr = ptr + 0x1FFEC;

	if (IsDlgButtonChecked(sysbiosTabList[4].hwnd, IDC_SYSBIOS_BOOTUP_DISPPCICONF) == BST_CHECKED)
		*sptr &= ~0x02;
	else
		*sptr |= 0x02;
	
	// power on delay count
	temp32 = 0;
	ptr16  = (ushort *)(ptr + 0x1FEA8);
	GetDlgItemText(sysbiosTabList[4].hwnd, IDC_SYSBIOS_BOOTUP_POWERONDELAY, buf, 256);
	sscanf(buf, "%d", &temp32);

	if (temp32 > 65535)
		temp32 = 65535;

	*ptr16 = (ushort)temp32;

	// setup default color value, lower 4 bits
	temp32 = 0;
	sptr   = ptr + 0x1FEAA;
	GetDlgItemText(sysbiosTabList[4].hwnd, IDC_SYSBIOS_COLOR_SETUPDEFAULT, buf, 256);
	sscanf(buf, "%d", &temp32);

	if (temp32 > 15)
		temp32 = 15;

	*sptr = ((*sptr) & 0xF0) | ((uchar)temp32 & 0x0F);

	// post default color value, upper 4 bits
	temp32 = 0;
	sptr   = ptr + 0x1FEAA;
	GetDlgItemText(sysbiosTabList[4].hwnd, IDC_SYSBIOS_COLOR_POSTDEFAULT, buf, 256);
	sscanf(buf, "%d", &temp32);

	if (temp32 > 15)
		temp32 = 15;

	*sptr = ((*sptr) & 0x0F) | (((uchar)temp32 & 0x0F) << 4);
	
	// post color option stored in bit 0
	sptr = ptr + 0x1FEAB;
	if (IsDlgButtonChecked(sysbiosTabList[4].hwnd, IDC_SYSBIOS_COLOR_OPTSETUP) == BST_CHECKED)
		*sptr |= 0x01;
	else
		*sptr &= ~0x01;

	// security number of retries
	if (sysbiosVersion != awdbeBIOSVer600PG)
		sptr = ptr + 0x1EC62;
	else
		sptr = ptr + 0x1EC68;

	temp32 = 0;
	GetDlgItemText(sysbiosTabList[4].hwnd, IDC_SYSBIOS_SECURITY_NUMRETRIES, buf, 256);
	sscanf(buf, "%d", &temp32);

	if (temp32 > 255)
		temp32 = 255;

	*sptr = (uchar)temp32;
	
	// floppy speed switching, bit 1 clear to enable
	sptr = ptr + 0x1FFE8;
	if (IsDlgButtonChecked(sysbiosTabList[4].hwnd, IDC_SYSBIOS_FLOPPY_SPEEDSWITCH) == BST_CHECKED)
		*sptr &= ~0x02;
	else
		*sptr |= 0x02;

	// PIE wait for floppy, bit 7 clear to enable
	sptr = ptr + 0x1FFEA;
	if (IsDlgButtonChecked(sysbiosTabList[4].hwnd, IDC_SYSBIOS_FLOPPY_PIEWAIT) == BST_CHECKED)
		*sptr &= ~0x80;
	else
		*sptr |= 0x80;

	// DRAM parity check, bit 2 clear to enable
	sptr = ptr + 0x1FFEC;
	if (IsDlgButtonChecked(sysbiosTabList[4].hwnd, IDC_SYSBIOS_MISC_DRAMPARITY) == BST_CHECKED)
		*sptr &= ~0x04;
	else
		*sptr |= 0x04;

	// keylock check, bit 3 clear to enable
	if (IsDlgButtonChecked(sysbiosTabList[4].hwnd, IDC_SYSBIOS_BOOTUP_KEYLOCK) == BST_CHECKED)
		*sptr &= ~0x08;
	else
		*sptr |= 0x08;

	// onboard E000h ROM check, bit 4 set to enable
	if (IsDlgButtonChecked(sysbiosTabList[4].hwnd, IDC_SYSBIOS_VIDEO_ROME000) == BST_CHECKED)
		*sptr |= 0x10;
	else
		*sptr &= ~0x10;

	// onboard C000h ROM check, bit 5 set to enable
	if (IsDlgButtonChecked(sysbiosTabList[4].hwnd, IDC_SYSBIOS_VIDEO_ROMC000) == BST_CHECKED)
		*sptr |= 0x20;
	else
		*sptr &= ~0x20;

	// manufacture loop check, bit 6 clear to enable
	if (IsDlgButtonChecked(sysbiosTabList[4].hwnd, IDC_SYSBIOS_MISC_LOOPCHECK) == BST_CHECKED)
		*sptr &= ~0x40;
	else
		*sptr |= 0x40;

	// ps/2 mouse support, 0x1FFEC bit 7 set (if not v6.00PG, 0x1E6FA bit 2 is also set) to enable
	if (IsDlgButtonChecked(sysbiosTabList[4].hwnd, IDC_SYSBIOS_BOOTUP_PS2MOUSE) == BST_CHECKED)
	{
		*sptr |= 0x80;

		if (vers != awdbeBIOSVer600PG)
		{
			sptr  = ptr + 0x1E6FA;
			*sptr |= 0x04;
		}
	}
	else
	{
		*sptr &= ~0x80;

		if (vers != awdbeBIOSVer600PG)
		{
			sptr  = ptr + 0x1E6FA;
			*sptr &= ~0x04;
		}
	}
}

