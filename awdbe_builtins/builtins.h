//
// Award BIOS Editor - builtins.h
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

#ifndef BUILTINS_H
#define BUILTINS_H

extern ulong myID;
extern HINSTANCE hinst;


char *builtinsDescription(void);
void builtinsAboutBox(HWND parentWnd);

void builtinsInit(ulong pluginID);
void builtinsOnLoad(fileEntry *fe, int count);
bool builtinsDetect(fileEntry *fe);

HWND builtinsCreateDialog(awdbeItem *item, fileEntry *fe, HWND parentWnd, RECT *rc);
bool builtinsUpdateDialog(awdbeItem *item, fileEntry *fe, HWND dialogWnd);
void builtinsRefreshDialog(awdbeItem *item, fileEntry *fe, HWND dialogWnd);
void builtinsOnDestroyDialog(awdbeItem *item, HWND dialogWnd);

void builtinsOnResizeDialog(awdbeItem *item, HWND dialogWnd, RECT *rc);

#endif
