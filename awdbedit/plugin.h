#pragma once
//
// Award BIOS Editor - plugin.h
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: plugin.h,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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

#ifndef PLUGIN_H
#define PLUGIN_H

typedef struct pluginFuncEntry
{
	char			*fname;
	HINSTANCE		 hInst;
	awdbeFuncTable	*functbl;

	pluginFuncEntry	*next;
} pluginFuncEntry;

typedef struct awdbeItemEntry
{
	awdbeItem		*list;
	int				 count;
	pluginFuncEntry *plugin;

	awdbeItemEntry	*next;
} awdbeItemEntry;


void pluginInit(void);
void pluginShutdown(void);

void pluginInitScan(HWND text, HWND prog);
uint32_t pluginScan(char *dir, bool doLoad);

void pluginAdd(char *fname, HINSTANCE hInst, awdbeFuncTable *ftbl);

awdbeItemEntry *pluginGetItemList(void);
awdbeItem *pluginFindResponder(fileEntry *fe);
awdbeItem *pluginFindSubMenu(awdbeItem *item);
awdbeItem *pluginFindHash(uint32_t hash);

void pluginCallOnLoad(fileEntry *fe, int count);
HWND pluginCallCreateDialog(awdbeItem *item, fileEntry *curFileEntry, HWND parentWnd, RECT *dlgrc);
bool pluginCallUpdateDialog(awdbeItem *item, fileEntry *curFileEntry, HWND dialogWnd);
bool pluginCallRefreshDialog(awdbeItem *item, fileEntry *fe, HWND hwnd);
void pluginCallOnDestroyDialog(awdbeItem *item, HWND hwnd);
void pluginCallOnResizeDialog(awdbeItem *item, HWND hwnd, RECT *rc);

void pluginAddToMenu(HMENU menu, uint32_t fromID);
void pluginShowAboutBox(uint32_t index, HWND parentWnd);

#endif
