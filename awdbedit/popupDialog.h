//
// Award BIOS Editor - popupDialog.h
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

#ifndef POPUPDIALOG_H
#define POPUPDIALOG_H

#include <windows.h>
#include <commctrl.h>
#include "types.h"

#define IDT_POPUPDIALOG			29618			// a magic number :)

#define PDSTYLE_NO_ANIMATION	0
#define PDSTYLE_OPEN_DOWN		1
#define PDSTYLE_OPEN_RIGHT		2


class popupDialog
{
private:
	HINSTANCE	 hInst;
	HWND		 hParentWnd, hDlgWnd;
	HBITMAP		 hDlgBmp;
	int			 baseX, baseY;

	HMENU		 hCurMenu;
	HBITMAP		 hCurBackImg;
	COLORREF	 cCurBackCol, cCurTextCol, cInaTextCol;
	int			 winxsize, winysize;

	int			 curSelItem;

	int			 hTimer;
	int			 timerXSave, timerYSave;

	popupDialog *pChild;

	void drawPopupDialog(HDC dc);
	int  getStringWidth(HDC hdc, char *str);
	void constructDialogBitmap(void);

	int  doHitTest(int ypos, MENUITEMINFO *retmii, RECT *retrc);

public:
	popupDialog(HINSTANCE inst, HWND parentwnd);
	~popupDialog();

	void create(int x, int y, HMENU submenu, int style, HBITMAP backImg, COLORREF backCol, COLORREF textCol, COLORREF inactTextCol);
	void destroy(void);

	void  selectItem(int item);
	int   getSelectedItem(void);
	HMENU getMenu(void);
	HWND  gethWnd(void);

	int  onPaint(HWND hwnd, HDC dc);
	int  onTimer(HWND hwnd, UINT timerID);
	int  onMouseMove(HWND hwnd, int xpos, int ypos, int *retval);
	int  onLButtonDown(HWND hwnd, int xpos, int ypos, int *retval);
};

#endif
