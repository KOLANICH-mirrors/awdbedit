//
// Award BIOS Editor - popupDialog.cpp
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: popupDialog.cpp,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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
#include "popupDialog.h"


popupDialog::popupDialog(HINSTANCE inst, HWND parentwnd)
{
	hInst		= inst;
	hParentWnd	= parentwnd;

	hDlgWnd		= nullptr;
	hDlgBmp		= nullptr;
	pChild		= nullptr;

	winxsize	= 0;
	winysize	= 0;
	hTimer		= 0;

	curSelItem	= -1;
}

popupDialog::~popupDialog()
{
	destroy();

	// delete any existing bitmap
	if (hDlgBmp != nullptr)
	{
		DeleteObject(hDlgBmp);
		hDlgBmp = nullptr;
	}
}

void popupDialog::drawPopupDialog(HDC dc)
{
	HDC bmdc;

	if ((hDlgWnd == nullptr) || (hDlgBmp == nullptr))
		return;

	// ensure we have a valid device context...
	if (dc == nullptr)
	{
		dc = GetDC(hDlgWnd);

		if (dc == nullptr)
			return;
	}

	// get a DC of the bitmap
	bmdc = CreateCompatibleDC(nullptr);
	SelectObject(bmdc, hDlgBmp);

	// bit the bitmap
	BitBlt(dc, 0, 0, winxsize, winysize, bmdc, 0, 0, SRCCOPY);

	DeleteDC(bmdc);
}

int popupDialog::getStringWidth(HDC hdc, char *str)
{
	int len = 0;
	char *ptr;
	ABC abc;

// maybe optimize this later... somehow?  also GetCharABCWidths only works with truetype fonts... but GetCharWidth32 isn't supported under windows?!
	
	ptr = str;
	while (*ptr != 0)
	{
		GetCharABCWidths(hdc, *ptr, *ptr, &abc);
		len += (abc.abcA + abc.abcB + abc.abcC);

		ptr++;
	}

	return len;
}

void popupDialog::constructDialogBitmap(void)
{
	int t, x, y, items, sepitems = 0, nonsepitems = 0, maxleft = 0, maxright = 0, len, heightOffset;
	char buf[256], *ptr;
	MENUITEMINFO mii;
	HDC hdc, hbackdc;
	HFONT hfnt;
	HBRUSH backBrush;
	RECT rc;
	BITMAP bm;
	BITMAPINFO bmi;
	VOID *pvBits;
	POINT pt[5];
	HPEN hpen;
	HICON hicon;
	ICONINFO ii;
	TEXTMETRIC tm;

	// create a DC we can use to draw into
	hdc = CreateCompatibleDC(nullptr);

	// setup some defaults into our DC...
	hfnt = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SelectObject(hdc, hfnt);
	SetBkMode(hdc, TRANSPARENT);

	// get some information about our font (since different platforms use varying fonts...)
	GetTextMetrics(hdc, &tm);
	heightOffset = 15 - tm.tmHeight;

	// determine number of items in this menu
	items = GetMenuItemCount(hCurMenu);

	// go through each item and get the length of each string to calculate the max size of a window to create
	for (t = 0; t < items; t++)
	{
		mii.cbSize		= sizeof(MENUITEMINFO);
		mii.fMask		= MIIM_TYPE;
		mii.fType		= MFT_STRING;
		mii.dwTypeData	= buf;
		mii.cch			= 256;
		GetMenuItemInfo(hCurMenu, t, TRUE, &mii);

		if (!(mii.fType & MFT_SEPARATOR))
		{
			nonsepitems++;
			ptr = buf;

			// stop when we hit a 0x09 (for tab) or a nullptr
			while ((*ptr != 0x09) && (*ptr != 0))
				ptr++;

			if (*ptr == 0)
			{
				// we hit a nullptr, so just test the buffer as the left side portion of the menu
				len = getStringWidth(hdc, buf);
				if (len > maxleft)
					maxleft = len;
			}
			else
			{
				// we hit a tab, so zap the tab and test both sides
				*ptr++ = 0;

				len = getStringWidth(hdc, buf);
				if (len > maxleft)
					maxleft = len;

				len = getStringWidth(hdc, ptr);
				if (len > maxright)
					maxright = len;
			}
		}
		else
		{
			sepitems++;
		}
	}

	// now calculate the size required for the window
	winxsize =	1 +			// 1 pixel for the border
				4 +			// 4 pixels for padding
				16 +		// 16 pixels for the icon (if any)
				3 +			// 3 pixels to the edge of the dark area
				4 +			// 4 pixels from the edge of the light area
				maxleft +	// the largest pixel size required by the left text elements
				16 +		// 16 pixels for padding till the right text
				maxright +	// the largest pixel size required by the right text elements
				12 +		// 12 pixels of padding until the right edge
				1;			// 1 pixel for the right border

	winysize =	1 +			// 1 pixel for the border
				3 +			// 3 pixels for padding
				(20 * nonsepitems) +	// 20 pixels for each non-separator item
				(6 * sepitems) +		// 6 pixels for each separator item
				(2 * (items - 1)) +		// 2 pixels of padding _between_ each item (hence, the -1)
				5 +			// 5 pixels for padding
				1;			// 1 pixel for the border

	// delete any existing bitmap
	if (hDlgBmp != nullptr)
	{
		DeleteObject(hDlgBmp);
		hDlgBmp = nullptr;
	}

	// create a bitmap of required size and select it into the dc
	ZeroMemory(&bmi, sizeof(BITMAPINFO));

	// setup bitmap info 
	bmi.bmiHeader.biSize		= sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth		= winxsize;
	bmi.bmiHeader.biHeight		= winysize;
	bmi.bmiHeader.biPlanes		= 1;
	bmi.bmiHeader.biBitCount	= 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage	= winxsize * winysize * 4;

	// create our DIB section and select the bitmap into the dc
	hDlgBmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pvBits, nullptr, 0);
	SelectObject(hdc, hDlgBmp);

	// if we have a background bitmap, tile blit it repeatedly...
	if (hCurBackImg != nullptr)
	{
		hbackdc = CreateCompatibleDC(hdc);
		SelectObject(hbackdc, hCurBackImg);

		GetObject(hCurBackImg, sizeof(bm), (LPSTR)&bm);

		// initialize rect which points inside of background
		y = 0;
		rc.top = baseY;

		while (y < winysize)
		{
			x = 0;
			rc.left = baseX;

			while (x < winxsize)
			{
				len = bm.bmWidth - rc.left;
				BitBlt(hdc, x, y, len, bm.bmHeight, hbackdc, rc.left, rc.top, SRCCOPY);

				x += len;
				rc.left = 0;
			}

			y += (bm.bmHeight - rc.top);
			rc.top = 0;
		}

		DeleteDC(hbackdc);
	}
	else
	{
		// no background, so fill in the specified background color
		backBrush = CreateSolidBrush(cCurBackCol);

		rc.top = 0;
		rc.left = 0;
		rc.right = winxsize + 1;
		rc.bottom = winysize + 1;
		FillRect(hdc, &rc, backBrush);
		DeleteObject(backBrush);
	}

	// draw the border of the menu in black
	SelectObject(hdc, GetStockObject(BLACK_PEN));

	pt[0].x = 0;			pt[0].y = 0;
	pt[1].x = winxsize - 1; pt[1].y = 0;
	pt[2].x = winxsize - 1; pt[2].y = winysize - 1;
	pt[3].x = 0;			pt[3].y = winysize - 1;
	pt[4].x = 0;			pt[4].y = 0;
	Polyline(hdc, pt, 5);

	// fill in a light grey bar on the left
	backBrush = CreateSolidBrush(RGB(192, 192, 192));

	rc.top = 1;
	rc.left = 1;
	rc.right = (1 + 4 + 16 + 3) + 1;
	rc.bottom = winysize - 1;
	FillRect(hdc, &rc, backBrush);
	DeleteObject(backBrush);

	// if there's a selection element, draw a light blue bar and border
	if ((curSelItem != -1) && (curSelItem < items))
	{
		// check and see if this element is a separator
		mii.cbSize		= sizeof(MENUITEMINFO);
		mii.fMask		= MIIM_TYPE;
		mii.fType		= MFT_STRING;
		mii.dwTypeData	= buf;
		mii.cch			= 256;
		GetMenuItemInfo(hCurMenu, curSelItem, TRUE, &mii);

		if (!(mii.fType & MFT_SEPARATOR))
		{
			// start at the top
			rc.top = 1 + 3;
			rc.left = 1 + 2;
			rc.right = (1 + 4 + 16 + 3 + 4 + maxleft + 16 + maxright + 12) - 3;				

			// iterate through each item, offsetting by the correct amount
			for (t = 0; t < curSelItem; t++)
			{
				mii.cbSize		= sizeof(MENUITEMINFO);
				mii.fMask		= MIIM_TYPE;
				mii.fType		= MFT_STRING;
				mii.dwTypeData	= buf;
				mii.cch			= 256;
				GetMenuItemInfo(hCurMenu, t, TRUE, &mii);

				if (!(mii.fType & MFT_SEPARATOR))
					rc.top += (20 + 2);
				else
					rc.top += (6 + 2);
			}

			// update rect
			rc.bottom = rc.top + 20;

			// draw our border first
			hpen = CreatePen(PS_SOLID, 1, RGB(124, 127, 142));
			SelectObject(hdc, hpen);

			pt[0].x = rc.left;	pt[0].y = rc.top;
			pt[1].x = rc.right;	pt[1].y = rc.top;
			pt[2].x = rc.right; pt[2].y = rc.bottom;
			pt[3].x = rc.left;	pt[3].y = rc.bottom;
			pt[4].x = rc.left;	pt[4].y = rc.top;
			Polyline(hdc, pt, 5);
			DeleteObject(hpen);

			// now draw the bar
			rc.top++;
			rc.left++;

			backBrush = CreateSolidBrush(RGB(188, 199, 227));
			FillRect(hdc, &rc, backBrush);
			DeleteObject(backBrush);
		}
	}

	// initialize our rectangle and start drawing the text elements!
	rc.top = (1 + 3 + 6) - 4;
	rc.left = (1 + 4 + 16 + 3 + 4) + 1;
	rc.right = winxsize;
	rc.bottom = winysize;

	for (t = 0; t < items; t++)
	{
		mii.cbSize		= sizeof(MENUITEMINFO);
		mii.fMask		= MIIM_TYPE | MIIM_CHECKMARKS | MIIM_SUBMENU | MIIM_STATE;
		mii.fType		= MFT_STRING;
		mii.dwTypeData	= buf;
		mii.cch			= 256;
		GetMenuItemInfo(hCurMenu, t, TRUE, &mii);

		if (!(mii.fType & MFT_SEPARATOR))
		{
			// check if there is an icon attached to this item
			if (mii.hbmpChecked != nullptr)
			{
				// create a structure to rebuild the icon from two bitmaps
				ii.fIcon	= TRUE;
				ii.hbmMask	= mii.hbmpUnchecked;
				ii.hbmColor	= mii.hbmpChecked;

				// draw the icon
				hicon = CreateIconIndirect(&ii);
				DrawIconEx(hdc, (1 + 4) + 1, rc.top, hicon, 16, 16, 0, nullptr, DI_NORMAL);
				DestroyIcon(hicon);
			}
			else
			{
				// no icon, so check if this item is "checked".
				if (mii.fState & MFS_CHECKED)
				{
					// draw a check mark in the left bar
					SelectObject(hdc, GetStockObject(BLACK_PEN));

					pt[0].x = (1 + 4) + 5; pt[0].y = rc.top + 8;
					pt[1].x = (1 + 4) + 5; pt[1].y = rc.top + 10;

					for (x = 0; x < 7; x++)
					{
						Polyline(hdc, pt, 2);

						pt[0].x++;
						pt[1].x++;

						if (x < 2)
						{
							pt[0].y++;
							pt[1].y++;
						}
						else
						{
							pt[0].y--;
							pt[1].y--;
						}
					}
				}
			}

			// check if this item has a submenu
			if (mii.hSubMenu != nullptr)
			{
				// draw a little arrow to the right
				SelectObject(hdc, GetStockObject(BLACK_PEN));

				pt[0].x = rc.right - 8; pt[0].y = rc.top + 8;
				pt[1].x = rc.right - 8; pt[1].y = rc.top + 9;

				for (x = 0; x < 4; x++)
				{
					Polyline(hdc, pt, 2);

					pt[0].x--;
					pt[1].x--;

					pt[0].y--;
					pt[1].y++;
				}
			}

			// start parsing the menu text
			ptr = buf;

			// stop when we hit a 0x09 (for tab) or a nullptr
			while ((*ptr != 0x09) && (*ptr != 0))
				ptr++;

			// offset the base of the font
			rc.top += heightOffset;

			// check if this item is grayed
			if (mii.fState & MF_GRAYED)			// yes, MF_GRAYED as it's what we really want to check
			{
				// draw in the grayed text color
				SetTextColor(hdc, cInaTextCol);
			}
			else
			{
				// draw in the normal text color
				SetTextColor(hdc, cCurTextCol);
			}

			if (*ptr == 0)
			{
				// we hit a nullptr, so just draw the text on the left side
				DrawText(hdc, buf, -1, &rc, DT_NOCLIP | DT_TOP | DT_LEFT);
			}
			else
			{
				// we hit a tab, so zap the tab and draw the left side first
				*ptr++ = 0;

				DrawText(hdc, buf, -1, &rc, DT_NOCLIP | DT_TOP | DT_LEFT);

				// adjust the left coord and draw the right
				rc.left += (maxleft + 16);

				DrawText(hdc, ptr, -1, &rc, DT_NOCLIP | DT_TOP | DT_LEFT);

				// reset the left coord
				rc.left -= (maxleft + 16);
			}

			// reset the base of the font
			rc.top -= heightOffset;

			// advance to next item
			rc.top += (20 + 2);
		}
		else
		{
			// draw a separator (line 1)
			hpen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
			SelectObject(hdc, hpen);

			pt[0].x = rc.left - 2;							pt[0].y = rc.top + 1;
			pt[1].x = pt[0].x + maxleft + 16 + maxright;	pt[1].y = pt[0].y;
			Polyline(hdc, pt, 2);
			DeleteObject(hpen);

			// (line 2)
			hpen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
			SelectObject(hdc, hpen);

			pt[0].y += 1;
			pt[1].y += 1;
			Polyline(hdc, pt, 2);
			DeleteObject(hpen);

			rc.top += (6 + 2);
		}
	}

	// remove the no longer needed DC
	DeleteDC(hdc);
}

void popupDialog::create(int x, int y, HMENU submenu, int style, HBITMAP backImg, COLORREF backCol, COLORREF textCol, COLORREF inactTextCol)
{
	// init stuff
	hTimer = 0;

	// store away vital data
	baseX		= x;
	baseY		= y;

	hCurMenu	= submenu;
	hCurBackImg	= backImg;
	cCurBackCol	= backCol;
	cCurTextCol	= textCol;
	cInaTextCol = inactTextCol;

	// build our dialog bitmap
	curSelItem = -1;
	constructDialogBitmap();

	// create the window
	hDlgWnd = CreateWindowEx(0, AWDBEDIT_POPUP_CLASSNAME, nullptr, WS_CHILD | WS_VISIBLE, baseX, baseY, winxsize, winysize, hParentWnd, nullptr, hInst, nullptr);

	// move it in Z-order above all windows
	BringWindowToTop(hDlgWnd);

	// and blit the bitmap
	drawPopupDialog(nullptr);
}

void popupDialog::destroy(void)
{
	if (hTimer != 0)
	{
		KillTimer(hDlgWnd, hTimer);
		hTimer = 0;
	}

	if (pChild != nullptr)
	{
		delete pChild;
		pChild = nullptr;
	}

	if (hDlgWnd != nullptr)
	{
		DestroyWindow(hDlgWnd);
		hDlgWnd = nullptr;
	}
}

void popupDialog::selectItem(int item)
{
	curSelItem = item;

	constructDialogBitmap();
	drawPopupDialog(nullptr);
}

int popupDialog::getSelectedItem(void)
{
	return curSelItem;
}

HMENU popupDialog::getMenu(void)
{
	return hCurMenu;
}

HWND popupDialog::gethWnd(void)
{
	return hDlgWnd;
}

int popupDialog::onPaint(HWND hwnd, HDC dc)
{
	// draw ourselves first...
	if (hDlgWnd == hwnd)
		drawPopupDialog(dc);

	// if we have any children, draw them too.
	if (pChild != nullptr)
		pChild->onPaint(hwnd, dc);

	return 0;
}

int popupDialog::onTimer(HWND hwnd, UINT timerID)
{
	// check if this window handle is ours...
	if (hDlgWnd != hwnd)
	{
		// nope, so send it to any children.
		if (pChild != nullptr)
			return pChild->onTimer(hwnd, timerID);

		return 0;
	}

	// make sure this timer ID is ours
	if (timerID != IDT_POPUPDIALOG)
		return 0;

	// call the "mouse left button down" handler as if it was pressed by the user...
	return onLButtonDown(hwnd, timerXSave, timerYSave, nullptr);
}

int popupDialog::doHitTest(int ypos, MENUITEMINFO *retmii, RECT *retrc)
{
	int t, items;
	MENUITEMINFO mii;
	RECT rc;
	char buf[256];

	// determine number of items in this menu
	items = GetMenuItemCount(hCurMenu);

	// iterate through each item, and build a rect for it
	rc.top = 1 + 3;
	rc.left = 0;
	rc.right = winxsize;

	for (t = 0; t < items; t++)
	{
		mii.cbSize		= sizeof(MENUITEMINFO);
		mii.fMask		= MIIM_TYPE | MIIM_ID | MIIM_SUBMENU | MIIM_STATE;
		mii.fType		= MFT_STRING;
		mii.dwTypeData	= buf;
		mii.cch			= 256;
		GetMenuItemInfo(hCurMenu, t, TRUE, &mii);

		// construct bottom of rect
		if (!(mii.fType & MFT_SEPARATOR))
			rc.bottom = rc.top + 22;
		else
			rc.bottom = rc.top + 8;

		// perform hit test
		if ((ypos >= rc.top) && (ypos <= rc.bottom))
		{
			// copy the returned menu data, if requested
			if (retmii != nullptr)
				memcpy(retmii, &mii, sizeof(MENUITEMINFO));

			if (retrc != nullptr)
				memcpy(retrc, &rc, sizeof(RECT));

			return t;
		}

		// advance top
		if (!(mii.fType & MFT_SEPARATOR))
			rc.top += (20 + 2);
		else
			rc.top += (6 + 2);
	}

	return -1;
}

int popupDialog::onMouseMove(HWND hwnd, int xpos, int ypos, int *retval)
{
	int curitem;
	MENUITEMINFO mii;

	// check if this window handle is ours...
	if (hDlgWnd != hwnd)
	{
		// nope, so send it to any children.
		if (pChild != nullptr)
			return pChild->onMouseMove(hwnd, xpos, ypos, nullptr);

		return 0;
	}

	// find the current item the cursor is over
	curitem = doHitTest(ypos, &mii, nullptr);

	// update only if the hit item is not the same as the currently selected item
	if (curitem == curSelItem)
		return 0;

	// cancel any existing timers
	if (hTimer != 0)
	{
		KillTimer(hDlgWnd, hTimer);
		hTimer = 0;
	}

	// update the current item
	curSelItem = curitem;

	// send this menu's command ID as a "select" to the main window, if retval is nullptr
	if (retval == nullptr)
		SendMessage(hParentWnd, WM_MENUSELECT, mii.wID, 0);
	else
		*retval = mii.wID;

	// rebuild the bitmap, and redraw it
	constructDialogBitmap();
	drawPopupDialog(nullptr);

	// if this item does not have a submenu, then kill off any existing child
	if ((mii.hSubMenu == nullptr) && (pChild != nullptr))
	{
		delete pChild;
		pChild = nullptr;
	}

	// if this item has a submenu, create a timer to popup the child automatically after 800 milliseconds
	if (mii.hSubMenu != nullptr)
	{
		// create a timer id/handle
		hTimer = IDT_POPUPDIALOG;

		// save the x and y position to determine what to popup
		timerXSave = xpos;
		timerYSave = ypos;
		
		// setup the timer proc
		SetTimer(hDlgWnd, hTimer, 800, nullptr);
	}

	return 0;
}

int popupDialog::onLButtonDown(HWND hwnd, int xpos, int ypos, int *retval)
{
	int curitem;
	MENUITEMINFO mii;
	RECT rc;

	// check if this window handle is ours...
	if (hDlgWnd != hwnd)
	{
/*
		// is this handle the main window?
		if (hwnd == hParentWnd)
		{
			// yes, so kill off all popups!
			if (pChild != nullptr)
			{
				delete pChild;
				pChild = nullptr;
			}

			// and clear out the "select" item
			SendMessage(hParentWnd, WM_MENUSELECT, 0, 0);
			return TRUE;
		}
*/
		// nope, so send it to any children.
		if (pChild != nullptr)
			return pChild->onLButtonDown(hwnd, xpos, ypos, nullptr);

		// return -1 for "not our window"...
		return -1;
	}

	// cancel any existing timers
	if (hTimer != 0)
	{
		KillTimer(hDlgWnd, hTimer);
		hTimer = 0;
	}

	// find the current item the cursor is over
	curitem = doHitTest(ypos, &mii, &rc);

	// if this item is disabled or grayed, end here
	if ((mii.fState & MF_DISABLED) || (mii.fState & MF_GRAYED))		// again, these are the proper flags (see winuser.h for the confusion)
		return FALSE;

	// see if this menu item has a submenu
	if (mii.hSubMenu != nullptr)
	{
		// cleanup any existing child
		if (pChild != nullptr)
		{
			// if the existing child has the same submenu, and this menu item is the same as the currently selected item, then we can end here.  (*whew*)
			if ((pChild->getMenu() == mii.hSubMenu) && (curitem == curSelItem))
				return FALSE;

			delete pChild;
			pChild = nullptr;
		}

		// send a message to the main window that we're about to create a submenu
		SendMessage(hParentWnd, WM_INITMENUPOPUP, (WPARAM)mii.hSubMenu, MAKELPARAM(curitem, FALSE));

		// create a new child
		pChild = new popupDialog(hInst, hParentWnd);
		pChild->create(baseX + rc.right, (baseY + rc.top) - 3, mii.hSubMenu, 0, hCurBackImg, cCurBackCol, cCurTextCol, cInaTextCol);
		return FALSE;
	}

	// clear out the "select" item
	SendMessage(hParentWnd, WM_MENUSELECT, 0, 0);

	// post this menu item's command to the main window, if the return value pointer is nullptr
	if (retval == nullptr)
		PostMessage(hParentWnd, WM_COMMAND, mii.wID, 0);
	else
		*retval = mii.wID;

	return TRUE;
}
