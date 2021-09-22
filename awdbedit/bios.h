#pragma once
//
// Award BIOS Editor - bios.h
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: bios.h,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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

#ifndef BIOS_H
#define BIOS_H

#include <stdint.h>

extern const char APP_NAME[];
extern const char APP_REV[];
extern const char APP_VERSION[];

#define HASH_UNKNOWN_ITEM_MAX		0x000000F8

#define HASH_SUBMENU_ITEM			0x000000FB
#define HASH_RECOGNIZED_ROOT		0x000000FC
#define HASH_UNKNOWN_ROOT			0x000000FD
#define HASH_INCLUDABLE_ROOT		0x000000FE

#define HASH_RESERVED				0x000000FF


void biosUpdateCurrentDialog(void);
void biosItemChanged(LPNMTREEVIEW lpnmtv);
void biosRefreshCurrentDialog(void);
bool biosHandleModified(char *text);

void biosInit(HINSTANCE hi, HWND hw, HWND statwnd, HWND treewnd, HTREEITEM recgitem, HTREEITEM inclitem, HTREEITEM unkitem, RECT *dlgrect);
void biosFreeMemory(void);
void biosTitleUpdate(void);

char *biosGetFilename(void);
HWND biosGetDialog(void);

bool biosOpenFile(char *fname);
bool biosOpen(void);
bool biosSave(void);
bool biosSaveAs(void);
void biosProperties(void);
void biosRevert(void);

void biosInsert(uint16_t typeID);
void biosReplace(void);
void biosExtract(void);
void biosExtractAll(void);
void biosRemove(void);
void biosHexEdit(void);

fileEntry *biosScanForID(uint16_t id);

void biosSetModified(bool val);
awdbeBIOSVersion biosGetVersion(void);

void biosResizeCurrentDialog(HWND hwnd, RECT *rc);

void biosGetDialogSize(SIZE *sz);
void biosResizeDialog(SIZE sz);

#endif
