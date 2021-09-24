//
// Award BIOS Editor - sysbios_timing.cpp
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: sysbios_timing.cpp,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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
#include "sysbios_timing.h"
#include "resource.h"


INT_PTR CALLBACK sysbiosBIOSTimingFunc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	return FALSE;
}

void sysbiosRefreshBIOSTiming(uchar *ptr)
{
	ushort *ptr16;
	sysbiosTimingStruct *timingPtr;
	char buf[256];

	// get pointer to pointer to the bios timings
	ptr16 = (ushort *)(ptr + 0x1FFE3);

	// make timing struct pointer to this pointer
	timingPtr = (sysbiosTimingStruct *)((ptr + 0x10000) + *ptr16);

	// fill out the entries...
	sprintf(buf, "%08X", timingPtr->refreshRate);
	SendMessage(GetDlgItem(sysbiosTabList[3].hwnd, IDC_SYSBIOS_REFRESHRATE), EM_SETLIMITTEXT, 8, 0);
	SetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_REFRESHRATE, buf);

	sprintf(buf, "%04X", timingPtr->fddMotorSpinUp);
	SendMessage(GetDlgItem(sysbiosTabList[3].hwnd, IDC_SYSBIOS_FDDMOTORSPINUP), EM_SETLIMITTEXT, 4, 0);
	SetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_FDDMOTORSPINUP, buf);

	sprintf(buf, "%02X", timingPtr->fddHeadSettle);
	SendMessage(GetDlgItem(sysbiosTabList[3].hwnd, IDC_SYSBIOS_FDDHEADSETTLE), EM_SETLIMITTEXT, 2, 0);
	SetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_FDDHEADSETTLE, buf);

	sprintf(buf, "%08X", timingPtr->lptInitialize);
	SendMessage(GetDlgItem(sysbiosTabList[3].hwnd, IDC_SYSBIOS_LPTINITIALIZE), EM_SETLIMITTEXT, 8, 0);
	SetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_LPTINITIALIZE, buf);

	U24_TO_STRING(timingPtr->keyboardCheck, buf);
	SendMessage(GetDlgItem(sysbiosTabList[3].hwnd, IDC_SYSBIOS_KEYBOARDCHECK), EM_SETLIMITTEXT, 6, 0);
	SetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_KEYBOARDCHECK, buf);

	U24_TO_STRING(timingPtr->fdcIRQ6Completion, buf);
	SendMessage(GetDlgItem(sysbiosTabList[3].hwnd, IDC_SYSBIOS_FDCIRQ6COMPLETION), EM_SETLIMITTEXT, 6, 0);
	SetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_FDCIRQ6COMPLETION, buf);

	U24_TO_STRING(timingPtr->fdcStatusByteRead, buf);
	SendMessage(GetDlgItem(sysbiosTabList[3].hwnd, IDC_SYSBIOS_FDCSTATUSBYTEREAD), EM_SETLIMITTEXT, 6, 0);
	SetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_FDCSTATUSBYTEREAD, buf);

	U24_TO_STRING(timingPtr->fdcCommandByteSend, buf);
	SendMessage(GetDlgItem(sysbiosTabList[3].hwnd, IDC_SYSBIOS_FDCCOMMANDBYTESEND), EM_SETLIMITTEXT, 6, 0);
	SetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_FDCCOMMANDBYTESEND, buf);

	U24_TO_STRING(timingPtr->hdcBusyCheck, buf);
	SendMessage(GetDlgItem(sysbiosTabList[3].hwnd, IDC_SYSBIOS_HDCBUSYCHECK), EM_SETLIMITTEXT, 6, 0);
	SetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_HDCBUSYCHECK, buf);

	U24_TO_STRING(timingPtr->hdcIRQ14Completion, buf);
	SendMessage(GetDlgItem(sysbiosTabList[3].hwnd, IDC_SYSBIOS_HDCIRQ14COMPLETION), EM_SETLIMITTEXT, 6, 0);
	SetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_HDCIRQ14COMPLETION, buf);

	U24_TO_STRING(timingPtr->hdcDataRequest, buf);
	SendMessage(GetDlgItem(sysbiosTabList[3].hwnd, IDC_SYSBIOS_HDCDATAREQUEST), EM_SETLIMITTEXT, 6, 0);
	SetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_HDCDATAREQUEST, buf);

	U24_TO_STRING(timingPtr->comPortTimeout, buf);
	SendMessage(GetDlgItem(sysbiosTabList[3].hwnd, IDC_SYSBIOS_COMPORTTIMEOUT), EM_SETLIMITTEXT, 6, 0);
	SetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_COMPORTTIMEOUT, buf);

	U24_TO_STRING(timingPtr->lptBusyCheck, buf);
	SendMessage(GetDlgItem(sysbiosTabList[3].hwnd, IDC_SYSBIOS_LPTBUSYCHECK), EM_SETLIMITTEXT, 6, 0);
	SetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_LPTBUSYCHECK, buf);
}

void sysbiosUpdateBIOSTiming(uchar *ptr, bool *modified, awdbeBIOSVersion vers)
{
	ushort *ptr16, temp16;
	sysbiosTimingStruct *timingPtr;
	ulong temp32;
	uchar temp8;
	char buf[256];

	// get pointer to pointer to the bios timings
	ptr16 = (ushort *)(ptr + 0x1FFE3);

	// make timing struct pointer to this pointer
	timingPtr = (sysbiosTimingStruct *)((ptr + 0x10000) + *ptr16);

	// update the entries...
	temp32 = 0;
	GetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_REFRESHRATE, buf, 256);
	sscanf(buf, "%x", &temp32);
	timingPtr->refreshRate = temp32;

	temp16 = 0;
	GetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_FDDMOTORSPINUP, buf, 256);
	sscanf(buf, "%x", &temp16);
	timingPtr->fddMotorSpinUp = temp16;

	temp8 = 0;
	GetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_FDDHEADSETTLE, buf, 256);
	sscanf(buf, "%x", &temp8);
	timingPtr->fddHeadSettle = temp8;

	temp32 = 0;
	GetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_LPTINITIALIZE, buf, 256);
	sscanf(buf, "%x", &temp32);
	timingPtr->lptInitialize = temp32;

	GetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_KEYBOARDCHECK, buf, 256);
	STRING_TO_U24(buf, timingPtr->keyboardCheck);

	GetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_FDCIRQ6COMPLETION, buf, 256);
	STRING_TO_U24(buf, timingPtr->fdcIRQ6Completion);

	GetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_FDCSTATUSBYTEREAD, buf, 256);
	STRING_TO_U24(buf, timingPtr->fdcStatusByteRead);

	GetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_FDCCOMMANDBYTESEND, buf, 256);
	STRING_TO_U24(buf, timingPtr->fdcCommandByteSend);

	GetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_HDCBUSYCHECK, buf, 256);
	STRING_TO_U24(buf, timingPtr->hdcBusyCheck);

	GetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_HDCIRQ14COMPLETION, buf, 256);
	STRING_TO_U24(buf, timingPtr->hdcIRQ14Completion);

	GetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_HDCDATAREQUEST, buf, 256);
	STRING_TO_U24(buf, timingPtr->hdcDataRequest);

	GetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_COMPORTTIMEOUT, buf, 256);
	STRING_TO_U24(buf, timingPtr->comPortTimeout);

	GetDlgItemText(sysbiosTabList[3].hwnd, IDC_SYSBIOS_LPTBUSYCHECK, buf, 256);
	STRING_TO_U24(buf, timingPtr->lptBusyCheck);
}
