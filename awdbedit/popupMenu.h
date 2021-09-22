#pragma once
//
// Award BIOS Editor - popupMenu.h
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: popupMenu.h,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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

#ifndef POPUPMENU_H
#define POPUPMENU_H

#include <windows.h>
#include <commctrl.h>
#include "types.h"
#include "popupdialog.h"

class popupMenu
{
private:
	HINSTANCE	 hInst;
	HWND		 hParentWnd, hToolbarWnd;
	int			 curItem, lowBound, highBound, style;
	HMENU		 hMenu;

	HBITMAP		 hBackBitmap;
	COLORREF	 cBackColor, cTextColor, cInactColor;

	popupDialog *pMenuDlg;

	void drawPopupDialog(void);
	void removePopupDialog(void);

public:
	popupMenu(HINSTANCE inst, HWND parentwnd, HWND toolbarwnd, HMENU menu);
	~popupMenu();

	void closePopup(void);

	void setBounds(int low, int high);
	void setOpenStyle(int style);
	void setBackgroundImage(HBITMAP bmp);
	void setBackgroundColor(COLORREF col);
	void setTextColor(COLORREF col);
	void setInactiveColor(COLORREF col);

	int		 getOpenStyle(void);
	HBITMAP  getBackgroundImage(void);
	COLORREF getBackgroundColor(void);
	COLORREF getTextColor(void);
	COLORREF getInactiveColor(void);

	int  onDropDown(LPNMTOOLBAR lpnmtb);
	int  onHotItemChange(LPNMTBHOTITEM lpnmtbhi);
	int  onPaint(HWND hwnd, HDC dc);
	int  onTimer(HWND hwnd, UINT timerID);

	int  onMouseMove(HWND hwnd, int xpos, int ypos);
	int  onLButtonDown(HWND hwnd, int xpos, int ypos);
};

#endif
