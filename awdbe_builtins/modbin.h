//
// Award BIOS Editor - modbin.h
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

#ifndef MODBIN_H
#define MODBIN_H

// menu string types
#define	MENU_DONE				0		// terminate string
#define MENU_DONE_1				1		// item is invalid
#define	MENU_BORDER				2		// draw border
#define	MENU_CLEAR				3
#define	MENU_DISP_STRING		4
#define	MENU_POS				5
#define	MENU_HILITE				6		// set attribute to hilight
#define	MENU_REVERSE			7		// set attribute to reverse
#define	MENU_NORMAL				8		// set attribute to normal
#define	MENU_BLINK				9		// set attribute to blink 
#define	MENU_WARN				10		// set attribute to warning
#define	MENU_LF_V6				10		// 6.0 only
#define	MENU_NEWLINE			11
#define	MENU_ADDX				12
#define	MENU_SUBX				13
#define	MENU_CR_V6				13		// 6.0 only
#define	MENU_ADDY				14
#define	MENU_SUBY				15
#define	MENU_CALL_ROUTINE		16
#define	MENU_CHANGE_COLOR		17
#define	MENU_WARN_V6			18		// 6.0 only
#define	MENU_SUBX_V6			19		// 6.0 only
#define	MENU_JMPSTRING_V6		20		// 6.0 only

#define MENU_MAXTYPE			MENU_JMPSTRING_V6

// status flags
#define STATUS_SHOWONLY			0x0004
#define STATUS_ITEMDISABLE		0x0008

#define STATUS_NOMODBIN			0x0080		// disallow item editing by modbin
#define STATUS_INPUT			0x0100
#define STATUS_TYPENUM			0x0200
#define STATUS_TYPEHEX			0x0400

#define STATUS_DIGIT2			0x0000
#define STATUS_DIGIT3			0x0800
#define STATUS_DIGIT4			0x1000
#define STATUS_DIGIT5			0x1800
#define STATUS_DIGIT_MASK		0x1800

#define STATUS_HIDDEN			0x8000

#endif
