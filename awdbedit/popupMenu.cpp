//
// Award BIOS Editor - popupMenu.cpp
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: popupMenu.cpp,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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

#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include "types.h"
#include "popupmenu.h"
#include "popupdialog.h"


popupMenu::popupMenu(HINSTANCE inst, HWND parentwnd, HWND toolbarwnd, HMENU menu)
{
	hInst		= inst;
	hParentWnd	= parentwnd;
	hToolbarWnd	= toolbarwnd;
	hMenu		= menu;

	curItem		= -1;
	lowBound	= -1;
	highBound	= -1;
	style		= PDSTYLE_NO_ANIMATION;
	hBackBitmap	= NULL;
	cBackColor	= RGB(255, 255, 255);
	cTextColor	= RGB(0, 0, 0);
	cInactColor	= RGB(128, 128, 128);

	pMenuDlg	= new popupDialog(hInst, hParentWnd);
}

popupMenu::~popupMenu()
{
	delete pMenuDlg;
	pMenuDlg = NULL;
}

void popupMenu::drawPopupDialog(void)
{
	RECT rc, rc2;
	int index = curItem - lowBound;

	// get the rect for this menu button
	SendMessage(hToolbarWnd, TB_GETITEMRECT, index, (LPARAM)&rc);

	// "press" the correct button on the bar
	SendMessage(hToolbarWnd, TB_SETSTATE, curItem, (LPARAM)MAKELONG(TBSTATE_PRESSED | TBSTATE_ENABLED, 0));

	// send a message to the main window that we're about to create a menu
	SendMessage(hParentWnd, WM_INITMENUPOPUP, (WPARAM)GetSubMenu(hMenu, index), MAKELPARAM(index, FALSE));

	// adjust the coords a bit, then create the popup menu window
/*
	GetWindowRect(hToolbarWnd, &rc2);
	rc.left	  += rc2.left;
	rc.bottom += rc2.top;
*/
	pMenuDlg->create(rc.left, rc.bottom + 2, GetSubMenu(hMenu, index), style, hBackBitmap, cBackColor, cTextColor, cInactColor);
}

void popupMenu::removePopupDialog(void)
{
	if (curItem == -1)
		return;

	// remove any existing dialog
	pMenuDlg->destroy();

	// reset the pressed state on the bar
	SendMessage(hToolbarWnd, TB_SETSTATE, curItem, (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
}

void popupMenu::closePopup(void)
{
	removePopupDialog();
	curItem = -1;
}

void popupMenu::setBounds(int low, int high)
{
	lowBound  = low;
	highBound = high;
}

void popupMenu::setOpenStyle(int style)
{
	this->style = style;
}

void popupMenu::setBackgroundImage(HBITMAP bmp)
{
	hBackBitmap = bmp;
}

void popupMenu::setBackgroundColor(COLORREF col)
{
	cBackColor = col;
}

void popupMenu::setTextColor(COLORREF col)
{
	cTextColor = col;
}

void popupMenu::setInactiveColor(COLORREF col)
{
	cInactColor = col;
}

int popupMenu::getOpenStyle(void)
{
	return style;
}

HBITMAP popupMenu::getBackgroundImage(void)
{
	return hBackBitmap;
}

COLORREF popupMenu::getBackgroundColor(void)
{
	return cBackColor;
}

COLORREF popupMenu::getTextColor(void)
{
	return cTextColor;
}

COLORREF popupMenu::getInactiveColor(void)
{
	return cInactColor;
}

int popupMenu::onDropDown(LPNMTOOLBAR lpnmtb)
{
	// check bounds on item
	if ((lpnmtb->iItem < lowBound) || (lpnmtb->iItem > highBound))
		return TBDDRET_NODEFAULT;

	// if there's an existing popup menu window, close it, and remove the pressed state on the toolbar
	if (curItem != -1)
	{
		removePopupDialog();

		// if this item is the same as the one last selected, simply end here.
		if (lpnmtb->iItem == curItem)
		{
			curItem = -1;
			return TBDDRET_DEFAULT;
		}
	}

	// update the current menu index
	curItem = lpnmtb->iItem;

	// display the menu
	drawPopupDialog();

	return TBDDRET_DEFAULT;
}

int popupMenu::onHotItemChange(LPNMTBHOTITEM lpnmtbhi)
{
	// if there is no menu displayed now, do nothing...
	if (curItem == -1)
		return 0;

	// check bounds on item
	if ((lpnmtbhi->idNew < lowBound) || (lpnmtbhi->idNew > highBound))
		return 0;

	// if this item is the same as the one we're displaying, select a "no item" in our popup
	if (curItem == lpnmtbhi->idNew)
	{
		pMenuDlg->selectItem(-1);
		return 0;
	}

	// there must an existing popup menu window -- so close it, and remove the pressed state on the toolbar
	removePopupDialog();

	// update the current menu index
	curItem = lpnmtbhi->idNew;

	// display the menu
	drawPopupDialog();

	return 0;
}

int popupMenu::onPaint(HWND hwnd, HDC dc)
{
	// if there is no menu displayed now, do nothing...
	if (curItem == -1)
		return 0;

	return pMenuDlg->onPaint(hwnd, dc);
}

int popupMenu::onTimer(HWND hwnd, UINT timerID)
{
	// if there is no menu displayed now, do nothing...
	if (curItem == -1)
		return 0;

	return pMenuDlg->onTimer(hwnd, timerID);
}

int popupMenu::onMouseMove(HWND hwnd, int xpos, int ypos)
{
	// if there is no menu displayed now, do nothing...
	if (curItem == -1)
		return 0;

	// pass the mouse move to our dialog.
	return pMenuDlg->onMouseMove(hwnd, xpos, ypos, NULL);
}

int popupMenu::onLButtonDown(HWND hwnd, int xpos, int ypos)
{
	int res;

	// if there is no menu displayed now, do nothing...
	if (curItem == -1)
		return 0;

	// pass the button press to our dialog, and check for a child popup
	res = pMenuDlg->onLButtonDown(hwnd, xpos, ypos, NULL);

	if ((res == TRUE) || (res == -1))
	{
		// either an item was selected (res == TRUE) or the click was outside any currently displayed menu, and
		// the popup needs to be removed...
		closePopup();
	}

	return 0;
}
