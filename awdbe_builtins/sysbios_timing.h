#pragma once
//
// Award BIOS Editor - sysbios_timing.h
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: sysbios_timing.h,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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

#ifndef SYSBIOS_TIMING_H
#define SYSBIOS_TIMING_H

#include <windows.h>

#if defined(__WINE__)
#include <pshpack1.h>
#else
#pragma pack(push, 1)
#endif
typedef struct
{
	uint32_t	refreshRate;
	uint16_t	fddMotorSpinUp;
	uint8_t	fddHeadSettle;
	uint32_t	lptInitialize;
	u24		keyboardCheck;
	u24		fdcIRQ6Completion;
	u24		fdcStatusByteRead;
	u24		fdcCommandByteSend;
	u24		hdcBusyCheck;
	u24		hdcIRQ14Completion;
	u24		hdcDataRequest;
	u24		comPortTimeout;
	u24		lptBusyCheck;
} sysbiosTimingStruct;
#if defined(__WINE__)
#include <poppack.h>
#else
#pragma pack(pop)
#endif

INT_PTR CALLBACK sysbiosBIOSTimingFunc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);

void sysbiosRefreshBIOSTiming(uint8_t *ptr);
void sysbiosUpdateBIOSTiming(uint8_t *ptr, bool *modified, awdbeBIOSVersion vers);

#endif
