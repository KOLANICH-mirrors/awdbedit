//
// Award BIOS Editor - sysbios_chipset.h
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

#ifndef SYSBIOS_CHIPSET_H
#define SYSBIOS_CHIPSET_H

#pragma pack(push, 1)
 typedef struct
 {
	uchar	type;
	uchar	index;
	uchar	function:3;
	uchar	device:5;
	uchar	mask;
	uchar	_0x00_2;
	uchar	value;
	uchar	_0x00_3;
 } sysbiosChipRegEntry;
#pragma pack(pop)

BOOL CALLBACK sysbiosChipsetRegsFunc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);

void sysbiosRefreshChipsetRegs(uchar *ptr);

#endif
