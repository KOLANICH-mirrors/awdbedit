//
// Award BIOS Editor - main.h
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: main.h,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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

#ifndef MAIN_H
#define MAIN_H

class popupMenu;

typedef struct
{
	HINSTANCE	MainhInst;
	HWND		MainhWnd;
	HMENU		MainhMenu;

	HWND		hStatusBar;
	HWND		hRebar, hMenuBar, hToolBar, hTreeView, hSplitter;
	HIMAGELIST	hToolImageList;
	HTREEITEM	hTreeRecgItem, hTreeInclItem, hTreeUnkItem;
	RECT		dialogrc;

	HBITMAP		hSkin;

	popupMenu	*pMenu;

	HMENU		hRecentFilesMenu, hSkinsMenu, hAboutPluginsMenu;
} globalsStruct;

extern globalsStruct globals;
extern char exePath[256], fullTempPath[256];

extern void cleanTempPath(void);
extern void enableControls(bool fileLoaded, bool selIsValid);
extern void resizeControlsFromDialog(SIZE dialogsz);

#endif
