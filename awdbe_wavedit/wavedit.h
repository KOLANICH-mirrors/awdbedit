//
// Award BIOS Editor - wavedit.h
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

#ifndef WAVEDIT_H
#define WAVEDIT_H

char *waveditDescription(void);
void waveditAboutBox(HWND parentWnd);

void waveditInit(ulong pluginID);
void waveditOnLoad(fileEntry *fe, int count);
bool waveditDetect(fileEntry *fe);

HWND waveditCreateDialog(awdbeItem *item, fileEntry *fe, HWND parentWnd, RECT *rc);
bool waveditUpdateDialog(awdbeItem *item, fileEntry *fe, HWND dialogWnd);
void waveditRefreshDialog(awdbeItem *item, fileEntry *fe, HWND dialogWnd);
void waveditOnDestroyDialog(awdbeItem *item, HWND dialogWnd);

void waveditOnResizeDialog(awdbeItem *item, HWND dialogWnd, RECT *rc);

#endif
