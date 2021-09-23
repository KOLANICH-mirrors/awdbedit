#pragma once
//
// Award BIOS Editor - sysbios_menu.h
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: sysbios_menu.h,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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

#ifndef SYSBIOS_MENU_H
#define SYSBIOS_MENU_H

#include <stdint.h>
#include <windows.h>

#define TYPE_MENUITEM		0
#define TYPE_MENUENTRY		1
#define TYPE_MENUHEADER		2

#define TYPE_BIOS			0
#define TYPE_SETUP			1

#if defined(__WINE__)
#include <pshpack1.h>
#else
#pragma pack(push, 1)
#endif
typedef struct
{
	uint16_t	status;
	uint16_t	pointerToHeader;
	uint16_t	chipRegIndex;
	uint16_t	chipRegMask;
	uint8_t	cmosIndex;
	uint16_t	cmosMask;
	uint16_t	pointerToItemTable;
	uint16_t	itemMinIndex;
	uint16_t	itemMaxIndex;
	uint8_t	xPosition;
	uint8_t	yPosition;
	uint16_t	biosDefaultIdx;
	uint16_t	setupDefaultIdx;
	uint16_t	pointerToHelp;
} sysbiosMenuStruct;

typedef struct
{
	uint16_t	pointerToPageStart;
	uint16_t	pointerToPageEnd;
	uint16_t	pointerToStartupString;
} sysbiosMenuDef;
#if defined(__WINE__)
#include <poppack.h>
#else
#pragma pack(pop)
#endif


typedef struct menuItem
{
	uint32_t	  type;

	uint16_t	  pointerIntoBios;
	uint16_t	  bestMinIdx;
	uint16_t	  bestMaxIdx;
	uint16_t	  itemCount;

	char	**itemText;
	uint32_t *maxLen;
	bool	 *selectable;

	menuItem *next;
} menuItem;

typedef struct menuEntry
{
	uint32_t		 type;

	sysbiosMenuStruct *menuPtr;

	char		*headerText;
	uint32_t			 headerMaxLen;

	char		*helpText;
	uint32_t			 helpMaxLen;

	menuItem	*itemList;

	menuEntry	*next;
} menuEntry;

typedef struct menuHeader
{
	uint32_t			 type;

	char			*headerText;
	uint32_t		 headerMaxLen;

	int				 entryCount;
	menuEntry		*firstEntry;
	sysbiosMenuDef	*menuDef;

	menuHeader		*next;
} menuHeader;

typedef struct menuDefinition
{
	int			 pageCount;
	menuHeader	*pageList;
} menuDefinition;

typedef struct
{
	uint8_t normal;
	uint8_t hilite;
	uint8_t reverse;
	uint8_t warn;
} colorTableStruct;


INT_PTR CALLBACK sysbiosConfigMenuFunc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);

void sysbiosRefreshMenu(uint8_t *ptr);
void sysbiosDestroyMenu(void);

void sysbiosReleaseMenuItems(void);

uint16_t sysbiosCMOSRead(uint8_t index, uint16_t mask);
void sysbiosCMOSWrite(uint8_t index, uint16_t mask, uint16_t val);
void sysbiosCMOSLoadDefaults(int type);

#endif
