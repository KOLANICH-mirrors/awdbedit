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

#define TYPE_MENUITEM		0
#define TYPE_MENUENTRY		1
#define TYPE_MENUHEADER		2

#define TYPE_BIOS			0
#define TYPE_SETUP			1

#pragma pack(push, 1)
typedef struct
{
	ushort	status;
	ushort	pointerToHeader;
	ushort	chipRegIndex;
	ushort	chipRegMask;
	uchar	cmosIndex;
	ushort	cmosMask;
	ushort	pointerToItemTable;
	ushort	itemMinIndex;
	ushort	itemMaxIndex;
	uchar	xPosition;
	uchar	yPosition;
	ushort	biosDefaultIdx;
	ushort	setupDefaultIdx;
	ushort	pointerToHelp;
} sysbiosMenuStruct;

typedef struct
{
	ushort	pointerToPageStart;
	ushort	pointerToPageEnd;
	ushort	pointerToStartupString;
} sysbiosMenuDef;
#pragma pack(pop)


typedef struct menuItem
{
	ulong	  type;

	ushort	  pointerIntoBios;
	ushort	  bestMinIdx;
	ushort	  bestMaxIdx;
	ushort	  itemCount;

	char	**itemText;
	int		 *maxLen;
	bool	 *selectable;

	menuItem *next;
} menuItem;

typedef struct menuEntry
{
	ulong		 type;

	sysbiosMenuStruct *menuPtr;

	char		*headerText;
	int			 headerMaxLen;

	char		*helpText;
	int			 helpMaxLen;

	menuItem	*itemList;

	menuEntry	*next;
} menuEntry;

typedef struct menuHeader
{
	ulong			 type;

	char			*headerText;
	int				 headerMaxLen;

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
	uchar normal;
	uchar hilite;
	uchar reverse;
	uchar warn;
} colorTableStruct;


BOOL CALLBACK sysbiosConfigMenuFunc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);

void sysbiosRefreshMenu(uchar *ptr);
void sysbiosDestroyMenu(void);

void sysbiosReleaseMenuItems(void);

int  sysbiosCMOSRead(uchar index, ushort mask);
void sysbiosCMOSWrite(uchar index, ushort mask, ushort val);
void sysbiosCMOSLoadDefaults(int type);

#endif
