#pragma once
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

#include <windows.h>

#if defined(__WINE__)
#include <pshpack1.h>
#else
#pragma pack(push, 1)
#endif
typedef struct
{
	uint8_t inta;
	uint8_t intb;
	uint8_t intc;
	uint8_t intd;
} sysbiosPCIRoutingEntry;
#if defined(__WINE__)
#include <poppack.h>
#else
#pragma pack(pop)
#endif

INT_PTR CALLBACK sysbiosIRQRoutingFunc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);

void sysbiosRefreshIRQRouting(uint8_t *ptr);

#endif
