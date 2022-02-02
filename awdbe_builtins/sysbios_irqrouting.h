//
// Award BIOS Editor - sysbios_irqrouting.h
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: sysbios_irqrouting.h,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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

#ifndef SYSBIOS_IRQROUTING_H
#define SYSBIOS_IRQROUTING_H

#pragma pack(push, 1)
typedef struct
{
	uchar inta;
	uchar intb;
	uchar intc;
	uchar intd;
} sysbiosPCIRoutingEntry;
#pragma pack(pop)

BOOL CALLBACK sysbiosIRQRoutingFunc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);

void sysbiosRefreshIRQRouting(uchar *ptr);

#endif
