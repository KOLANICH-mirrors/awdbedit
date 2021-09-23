//
// Award BIOS Editor - sysbios_menu.cpp
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: sysbios_menu.cpp,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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
#include "../awdbedit/awdbe_exports.h"
#include "builtins.h"
#include "sysbios.h"
#include "sysbios_menu.h"
#include "resource.h"
#include "console.h"

#include "modbin.h"


static menuItem   *menuItemList   = nullptr;
static menuEntry  *menuEntryList  = nullptr;
static menuHeader *menuHeaderList = nullptr;
static int		   menuHeaderCount = 0;

static HANDLE hMenuThread = nullptr;
static HANDLE hCon = nullptr;
static bool   gExitMenu = FALSE;

static uint8_t cmosTable[0x80];


uint8_t sysbiosMenuStrParse(uint8_t *ptr)
{
	uint8_t len = 0;

	switch (*ptr)
	{
		case MENU_DONE:
		case MENU_DONE_1:
			return 0;

		case MENU_BORDER:
			return 6;

		case MENU_CLEAR:
			return 5;

		case MENU_DISP_STRING:
			return 5;

		case MENU_POS:
			return 3;

		case MENU_HILITE:
		case MENU_REVERSE:
		case MENU_NORMAL:
		case MENU_BLINK:
			return 1;

		case MENU_NEWLINE:
			return 1;

		case MENU_ADDX:
			return 2;

		case MENU_ADDY:
			return 2;

		case MENU_SUBY:
			return 2;

		case MENU_CALL_ROUTINE:
			return 3;

		case MENU_CHANGE_COLOR:
			return 1;			// unknown!!!

		case MENU_WARN:		// also V6_LF
			if (sysbiosVersion != awdbeBIOSVer600PG)
				return 1;
			
			return 1;

		case MENU_SUBX:		// also V6_CR
			if (sysbiosVersion != awdbeBIOSVer600PG)
				return 2;
			
			return 1;

		case MENU_WARN_V6:
			return 1;

		case MENU_SUBX_V6:
			return 2;

		case MENU_JMPSTRING_V6:
			return 3;
	}

	// this is a printable character.  find it's length...
	while (*ptr > MENU_MAXTYPE)
	{
		ptr++;
		len++;
	}

	return len;
}

uint32_t sysbiosMenuStrFindLen(uint8_t *ptr)
{
	uint32_t len = 0, res;

	while ( (res = sysbiosMenuStrParse(ptr)) != 0 )
	{
		ptr += res;
		len += res;
	}

	return len;
}

menuEntry *sysbiosAddToMenuEntryList(sysbiosMenuStruct *pMenu, uint8_t *biosPtr, uint32_t biosBaseOffset)
{
	menuEntry *me, *mz;
	uint16_t *ptr16;
	uint8_t *ptr;

	// create new menu entry
	me = new menuEntry;
	me->type	= TYPE_MENUENTRY;
	me->menuPtr = pMenu;

	// allocate/copy strings
	ptr16 = (uint16_t *)&pMenu->pointerToHeader;
	ptr   = (uint8_t *)((biosPtr + biosBaseOffset) + *ptr16);

	me->headerMaxLen = sysbiosMenuStrFindLen(ptr);
	me->headerText   = new char[me->headerMaxLen + 1];
	memset(me->headerText,   0, me->headerMaxLen + 1);
	memcpy(me->headerText, ptr, me->headerMaxLen);

	ptr16 = (uint16_t *)&pMenu->pointerToHelp;
	ptr   = (uint8_t *)((biosPtr + biosBaseOffset) + *ptr16);

	me->helpMaxLen = sysbiosMenuStrFindLen(ptr);
	me->helpText   = new char[me->helpMaxLen + 1];
	memset(me->helpText,   0, me->helpMaxLen + 1);
	memcpy(me->helpText, ptr, me->helpMaxLen);

	// zap item list and next pointers for now
	me->itemList = nullptr;
	me->next	 = nullptr;

	// insert this entry into the end of our list
	if (menuEntryList == nullptr)
	{
		menuEntryList = me;
	}
	else
	{
		mz = menuEntryList;
		while (mz->next != nullptr)
			mz = mz->next;

		mz->next = me;
	}

	return me;
}

menuItem *sysbiosAddToMenuItemList(sysbiosMenuStruct *pMenu)
{
	menuItem *mi, *mz;

	// create a new menu item
	mi = new menuItem;
	mi->type			= TYPE_MENUITEM;
	mi->pointerIntoBios = pMenu->pointerToItemTable;
	mi->bestMinIdx		= pMenu->itemMinIndex;
	mi->bestMaxIdx		= pMenu->itemMaxIndex;
	mi->itemCount		= 0;

	// zap the pointers for now
	mi->itemText	= nullptr;
	mi->maxLen		= nullptr;
	mi->selectable	= nullptr;
	mi->next		= nullptr;

	// insert this entry into the end of our list
	if (menuItemList == nullptr)
	{
		menuItemList = mi;
	}
	else
	{
		mz = menuItemList;
		while (mz->next != nullptr)
			mz = mz->next;

		mz->next = mi;
	}

	return mi;
}

menuItem *sysbiosFindMenuItem(uint16_t pointerToItem)
{
	menuItem *mi = menuItemList;
	
	while (mi != nullptr)
	{
		if (mi->pointerIntoBios == pointerToItem)
			return mi;

		mi = mi->next;
	}

	return nullptr;
}

void sysbiosCreateMenuItems(uint8_t *biosPtr, uint32_t biosBaseOffset)
{
	menuEntry *me = menuEntryList;
	menuItem *mi;
	uint16_t *ptr16;
	uint8_t *ptr, *baseptr;
	int t;

	while (me != nullptr)
	{
		// lookup this item based on the pointer value stored in the BIOS
		mi = sysbiosFindMenuItem(me->menuPtr->pointerToItemTable);
		if (mi == nullptr)
		{
			// not found, create a new one
			me->itemList = sysbiosAddToMenuItemList(me->menuPtr);
		}
		else
		{
			// found, simply pass the pointer over
			me->itemList = mi;

			// update the best min and max indices, too
			if (me->menuPtr->itemMinIndex < mi->bestMinIdx)
				mi->bestMinIdx = me->menuPtr->itemMinIndex;

			if (me->menuPtr->itemMaxIndex > mi->bestMaxIdx)
				mi->bestMaxIdx = me->menuPtr->itemMaxIndex;
		}

		me = me->next;
	}

	// now, go through all item lists, calculate counts, allocate item arrays and copy in data
	mi = menuItemList;
	while (mi != nullptr)
	{
		if (mi->bestMaxIdx != 0xFFFF)
		{
			mi->bestMaxIdx -= mi->bestMinIdx;
			mi->bestMinIdx  = 0;
			mi->itemCount   = mi->bestMaxIdx + 1;

			mi->itemText	= new char *[mi->itemCount];
			mi->maxLen		= new uint32_t[mi->itemCount];
			mi->selectable  = new bool[mi->itemCount];

			// lookup item pointer
			ptr16 = (uint16_t *)&mi->pointerIntoBios;
			ptr   = (uint8_t *)((biosPtr + biosBaseOffset) + *ptr16);

			for (t = 0; t < mi->itemCount; t++)
			{
				// keep a temp base pointer
				baseptr = ptr;

				// advance until we reach a terminator
				while ((*ptr != MENU_DONE) && (*ptr != MENU_DONE_1))
					ptr++;

				// this is the maximum length of this item
				mi->maxLen[t] = static_cast<uint32_t>(ptr - baseptr);

				// allocate/copy the data
				mi->itemText[t] = new char[mi->maxLen[t] + 1];
				memset(mi->itemText[t], 0, mi->maxLen[t] + 1);
				memcpy(mi->itemText[t], baseptr, mi->maxLen[t]);

				// set selectable flag
				if ((*ptr) == 0x00)
					mi->selectable[t] = TRUE;
				else
					mi->selectable[t] = FALSE;

				// advance to next item
				ptr++;
			}
		}

		mi = mi->next;
	}
}

void sysbiosAddToMenuHeaderList(char *text, int entryCount, menuEntry *firstEntry, sysbiosMenuDef *menuDef)
{
	menuHeader *mh, *mz;

	// create a new header
	mh = new menuHeader;
	mh->type		= TYPE_MENUHEADER;
	mh->entryCount  = entryCount;
	mh->firstEntry  = firstEntry;
	mh->menuDef		= menuDef;

	// allocate/copy strings
	mh->headerMaxLen = strlen(text);
	mh->headerText   = new char[mh->headerMaxLen + 1];
	memset(mh->headerText, 0, mh->headerMaxLen + 1);
	memcpy(mh->headerText, text, mh->headerMaxLen);

	// zap next pointer
	mh->next = nullptr;

	// insert this entry into the end of our list
	if (menuHeaderList == nullptr)
	{
		menuHeaderList = mh;
	}
	else
	{
		mz = menuHeaderList;
		while (mz->next != nullptr)
			mz = mz->next;

		mz->next = mh;
	}

	menuHeaderCount++;
}

void sysbiosReleaseMenuItems(void)
{
	menuItem   *mi = menuItemList, *min;
	menuEntry  *me = menuEntryList, *men;
	menuHeader *mh = menuHeaderList, *mhn;
	int t;

	while (mi != nullptr)
	{
		for (t = 0; t < mi->itemCount; t++)
			delete []mi->itemText[t];

		delete []mi->itemText;
		delete []mi->maxLen;
		delete []mi->selectable;

		min = mi->next;

		delete mi;
		mi = min;
	}

	while (me != nullptr)
	{
		delete []me->headerText;
		delete []me->helpText;

		men = me->next;

		delete me;
		me = men;
	}

	while (mh != nullptr)
	{
		delete []mh->headerText;

		mhn = mh->next;

		delete mh;
		mh = mhn;
	}

	menuItemList    = nullptr;
	menuEntryList   = nullptr;
	menuHeaderList  = nullptr;
	menuHeaderCount = 0;
}

void sysbiosMenuDrawHorizLine(uint8_t *borderPtr, SHORT x1, SHORT x2, SHORT y1, SHORT y2)
{
	char borderBuf[1024], *bptr;
	int len, tlen;

	// make top and bottom lines of border
	bptr = borderBuf;
	len  = (x2 - x1) + 1;

	if (len > 0)
	{
		tlen = len;

		while (tlen--)
			*bptr++ = borderPtr[1];

		// draw them
		gotoxy(x1, y1);
		c_rawputblk((char *)borderBuf, len);

		gotoxy(x1, y2);
		c_rawputblk((char *)borderBuf, len);
	}
}

void sysbiosMenuDrawVertLine(uint8_t *borderPtr, SHORT x1, SHORT x2, SHORT y1, SHORT y2)
{
	SHORT y;

	for (y = y1; y <= y2; y++)
	{
		gotoxy(x1, y);
		c_rawputch(borderPtr[3]);

		gotoxy(x2, y);
		c_rawputch(borderPtr[3]);
	}
}

void sysbiosMenuStrDrawBorder(int x1, int y1, int x2, int y2, int type)
{
	//						  +     -     +     |     +     +	
	//                        UL   Hstr   UR   Vstr   BL    BR
	uint8_t borderType1[6] = { 0xDA, 0xC4, 0xBF, 0xB3, 0xC0, 0xD9 };
	uint8_t borderType2[6] = { 0xC9, 0xCD, 0xBB, 0xBA, 0xC8, 0xBC };
	uint8_t *borderPtr;

	// setup border type
	if (type == 0)
		borderPtr = borderType1;
	else
		borderPtr = borderType2;

	// draw horizontal portion
	if (y1 != y2)
		sysbiosMenuDrawHorizLine(borderPtr, x1 + 1, x2 - 1, y1, y2);
	else
	{
		sysbiosMenuDrawHorizLine(borderPtr, x1, x2, y1, y1);

		// set cursor position to bottom-right corner of border window
		gotoxy(x2, y2);
		return;
	}

	// draw vertical portion
	if (x1 != x2)
		sysbiosMenuDrawVertLine(borderPtr, x1, x2, y1 + 1, y2 - 1);
	else
	{
		sysbiosMenuDrawVertLine(borderPtr, x1, x2, y1, y2);

		// set cursor position to bottom-right corner of border window
		gotoxy(x2, y2);
		return;
	}

	// draw corners
	gotoxy(x1, y1);
	c_rawputch(borderPtr[0]);

	gotoxy(x2, y1);
	c_rawputch(borderPtr[2]);

	gotoxy(x1, y2);
	c_rawputch(borderPtr[4]);

	gotoxy(x2, y2);
	c_rawputch(borderPtr[5]);

	// set cursor position to bottom-right corner of border window
	gotoxy(x2, y2);
}

void sysbiosMenuStrDraw(uint8_t *ptr, colorTableStruct *colTbl)
{
	uint32_t len;
	SHORT cx, cy;
	uint8_t *str;
	uint16_t *ptr16;

	while ((*ptr != MENU_DONE) && (*ptr != MENU_DONE_1))
	{
		cx = wherex();
		cy = wherey();

		len = sysbiosMenuStrParse(ptr);

		switch (*ptr)
		{
			case MENU_DONE:
			case MENU_DONE_1:
				break;

			case MENU_BORDER:
				sysbiosMenuStrDrawBorder(*(ptr + 1), *(ptr + 2), *(ptr + 3), *(ptr + 4), *(ptr + 5));
				break;

			case MENU_CLEAR:
				len = 0;
				break;

			case MENU_DISP_STRING:
				// update x/y position
				if (*(ptr + 1) != 0xFF) cx = *(ptr + 1);
				if (*(ptr + 2) != 0xFF) cy = *(ptr + 2);

				gotoxy(cx, cy);

				// get pointer to string to show
				ptr16 = (uint16_t *)(ptr + 3);
				str   = (uint8_t *)((sysbiosBasePtr + 0x10000) + (*ptr16));
				
				// recursively call ourselves to draw the string
				sysbiosMenuStrDraw(str, colTbl);
				break;

			case MENU_POS:
				if (*(ptr + 1) != 0xFF) cx = *(ptr + 1);
				if (*(ptr + 2) != 0xFF) cy = *(ptr + 2);

				gotoxy(cx, cy);
				break;

			case MENU_HILITE:
				textattr(colTbl->hilite);
				break;

			case MENU_REVERSE:
				textattr(colTbl->reverse);
				break;

			case MENU_NORMAL:
				textattr(colTbl->normal);
				break;

			case MENU_BLINK:
				break;

			case MENU_WARN:		// also V6_LF
				if (sysbiosVersion != awdbeBIOSVer600PG)
				{
					// do warn stuff
					textattr(colTbl->warn);
				}
				else
				{
					// linefeed?
				}
				break;

			case MENU_ADDX:
				gotoxy(cx + *(ptr + 1), cy);
				break;

			case MENU_SUBX:		// also V6_CR
				if (sysbiosVersion != awdbeBIOSVer600PG)
				{
					// do subx
					gotoxy(cx - *(ptr + 1), cy);
				}
				else
				{
					// do cr
				}
				break;

			case MENU_ADDY:
				gotoxy(cx, cy + *(ptr + 1));
				break;

			case MENU_SUBY:
				gotoxy(cx, cy - *(ptr + 1));
				break;

			default:
				c_rawputblk((char *)ptr, len);
				break;
		}

		ptr += len;
	}
}


menuEntry *sysbiosMenuLookupEntry(menuHeader *mh, int itemIndex)
{
	menuEntry *me;

	me = mh->firstEntry;
	while (itemIndex--)
		me = me->next;

	return me;
}

void sysbiosMenuDrawItem(menuHeader *mh, int itemIdx, int attr, colorTableStruct *colTbl)
{
	menuEntry *me;
	int idx, val;
	char fmtbuf[16];

	me = sysbiosMenuLookupEntry(mh, itemIdx);

	if ((me->menuPtr->status & STATUS_ITEMDISABLE) || (me->menuPtr->status & STATUS_HIDDEN))
		return;

	// draw the header text
	textattr(colTbl->normal);
	gotoxy(me->menuPtr->xPosition, me->menuPtr->yPosition);
	sysbiosMenuStrDraw((uint8_t *)me->headerText, colTbl);

	// now draw the item
	if (!(me->menuPtr->status & STATUS_NOMODBIN))
	{
		// check if this item is an input type or not
		if (!(me->menuPtr->status & STATUS_INPUT))
		{
			if (me->itemList->itemCount > 0)
			{
				idx = sysbiosCMOSRead(me->menuPtr->cmosIndex, me->menuPtr->cmosMask);

				if (!(me->menuPtr->status & STATUS_SHOWONLY))
					textattr(attr);

				sysbiosMenuStrDraw((uint8_t *)me->itemList->itemText[idx], colTbl);
			}
		}
		else
		{
		}
	}
	else
	{
		// check if there are any items associated with entry
		if (me->itemList->itemCount > 0)
		{
			idx = sysbiosCMOSRead(me->menuPtr->cmosIndex, me->menuPtr->cmosMask);

			if (!(me->menuPtr->status & STATUS_SHOWONLY))
				textattr(attr);

			sysbiosMenuStrDraw((uint8_t *)me->itemList->itemText[idx], colTbl);
		}
		else
		{
			// create a format to display a number
			fmtbuf[0] = '%';

			switch (me->menuPtr->status & STATUS_DIGIT_MASK)
			{
				case STATUS_DIGIT2: fmtbuf[1] = '2'; break;
				case STATUS_DIGIT3: fmtbuf[1] = '3'; break;
				case STATUS_DIGIT4: fmtbuf[1] = '4'; break;
				case STATUS_DIGIT5: fmtbuf[1] = '5'; break;
			}

			if (me->menuPtr->status & STATUS_TYPENUM)
				fmtbuf[2] = 'd';
			else if (me->menuPtr->status & STATUS_TYPEHEX)
				fmtbuf[2] = 'X';

			fmtbuf[3] = 0;

			val = sysbiosCMOSRead(me->menuPtr->cmosIndex, me->menuPtr->cmosMask);

			if (!(me->menuPtr->status & STATUS_SHOWONLY))
				textattr(attr);

			c_printf(fmtbuf, val);
		}
	}
}

void drawSetupMenuPage(colorTableStruct *colTbl, menuHeader *mh)
{
	uint8_t *ptr;
	int cnt;

	// clear screen
	textattr(colTbl->normal);
	clrscr();

	// draw startup string
	ptr = (uint8_t *)((sysbiosBasePtr + 0x10000) + mh->menuDef->pointerToStartupString);
	sysbiosMenuStrDraw(ptr, colTbl);
	flip_to_console();

	// draw the items
	for (cnt = 0; cnt < mh->entryCount; cnt++)
		sysbiosMenuDrawItem(mh, cnt, colTbl->hilite, colTbl);

	flip_to_console();
}

void runSetupMenuPage(colorTableStruct *colTbl, menuHeader *mh)
{
	bool done;
	uint32_t val;
	uint32_t itemIdx, newIdx;
	bool needUpd;
	menuEntry *me;
	menuHeader *mhx;
	WORD kc;

	drawSetupMenuPage(colTbl, mh);

	// run menu
	itemIdx = 0;
	newIdx  = 0;
	needUpd = TRUE;
	done    = FALSE;

	while (!done)
	{
		if (gExitMenu == TRUE)
			done = TRUE;
		else
		{
			if (c_kbhit())
			{
				kc = c_getkeycode();
				switch (kc)
				{
					case 0x0026:			// up
						newIdx = itemIdx;

						do
						{
							newIdx--;

							if (newIdx < 0)
								newIdx = static_cast<uint32_t>((mh->entryCount) - 1);

							me = sysbiosMenuLookupEntry(mh, newIdx);
						} while ((me->menuPtr->status & STATUS_SHOWONLY) || (me->menuPtr->status & STATUS_HIDDEN) || (me->menuPtr->status & STATUS_ITEMDISABLE));

						needUpd = TRUE;
						break;

					case 0x0028:			// down
						newIdx = itemIdx;

						do
						{
							newIdx++;

							if (newIdx >= mh->entryCount)
								newIdx = 0;

							me = sysbiosMenuLookupEntry(mh, newIdx);
						} while ((me->menuPtr->status & STATUS_SHOWONLY) || (me->menuPtr->status & STATUS_HIDDEN) || (me->menuPtr->status & STATUS_ITEMDISABLE));

						needUpd = TRUE;
						break;

					case 0x0021:			// pgup
						me = sysbiosMenuLookupEntry(mh, itemIdx);

						val = sysbiosCMOSRead(me->menuPtr->cmosIndex, me->menuPtr->cmosMask);
						val--;

						if (val < me->itemList->bestMinIdx)
							val = me->itemList->bestMaxIdx;

						sysbiosCMOSWrite(me->menuPtr->cmosIndex, me->menuPtr->cmosMask, val);
						needUpd = TRUE;
						break;

					case 0x0022:			// pgdn
						me = sysbiosMenuLookupEntry(mh, itemIdx);

						val = sysbiosCMOSRead(me->menuPtr->cmosIndex, me->menuPtr->cmosMask);
						val++;

						if (val > me->itemList->bestMaxIdx)
							val = me->itemList->bestMinIdx;

						sysbiosCMOSWrite(me->menuPtr->cmosIndex, me->menuPtr->cmosMask, val);
						needUpd = TRUE;
						break;

					case 0x0025:			// left
						break;

					case 0x0027:			// right
						break;

					case 13:			// enter
						if (mh == menuHeaderList)		// top menu check
						{
							if (itemIdx < (menuHeaderCount - 1))		// -1, the top page doesn't count
							{
								// find the selected page
								val = itemIdx;
								mhx = mh->next;

								while (val--)
									mhx = mhx->next;

								// run it
								runSetupMenuPage(colTbl, mhx);

								// redisplay our page
								drawSetupMenuPage(colTbl, mh);

								newIdx  = itemIdx;
								needUpd = TRUE;
							}
						}
						break;

					case 27:			// esc
						if (mh != menuHeaderList)		// not top menu check
						{
							done = TRUE;
						}
						else
						{
							// ask if want to quit
						}
						break;

					default:
						textattr(0x07);
						c_printf("%04X ", kc);
						flip_to_console();
						break;
				}
			}

			if (needUpd)
			{
				// redraw the old item in plain color
				sysbiosMenuDrawItem(mh, itemIdx, colTbl->hilite, colTbl);

				// update the item index
				itemIdx = newIdx;

				// draw new item in reverse color
				sysbiosMenuDrawItem(mh, itemIdx, colTbl->reverse, colTbl);

				flip_to_console();
				needUpd = FALSE;
			}
		}

		Sleep(50);
	}
}

DWORD WINAPI runSetupMenu([[maybe_unused]] LPVOID unused)
{
	uint8_t *ptr, monoTbl[8] = { 0x07, 0x0F, 0x70, 0x07, 0x70, 0x78, 0x07, 0x70 };
	colorTableStruct *colTbl;
	int cnt;

	// clear exit flag
	gExitMenu = FALSE;

	// initialize console
	if (hCon == nullptr)
	{
		init_console(FALSE, MODE_80x25, nullptr);
		cursor_off();

		hCon = getConsoleHandle();
		SetConsoleTitle("BIOS Setup Menu");
	}

	// search for color table
	colTbl = nullptr;
	ptr    = (uint8_t *)(sysbiosBasePtr + 0x10000);
	cnt    = 0xFFF0;

	while (cnt--)
	{
		if (!memcmp(ptr, monoTbl, 8))
		{
			colTbl = (colorTableStruct *)(ptr + 8);
			cnt    = 0;
		}

		ptr++;
	}

	// use monochrome if none found
	if (colTbl == nullptr)
		colTbl = (colorTableStruct *)monoTbl;

	// run the root menu page
	runSetupMenuPage(colTbl, menuHeaderList);

	hMenuThread = nullptr;
	return 0;
}

INT_PTR CALLBACK sysbiosConfigMenuFunc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	LPNMHDR	lpNM = (LPNMHDR)lParam;
	LPNMTREEVIEW lpnmtv;
	uint32_t *type;
	menuHeader *mh;
	menuEntry *me;
	menuItem *mi;
	char buf[256];
	int t;
	HWND hItemBios, hItemSetup;
	DWORD foo;

	switch (message)
	{
		case WM_INITDIALOG:
			return TRUE;

		case WM_NOTIFY:
			switch (lpNM->code)
			{
				case TVN_SELCHANGED:
					lpnmtv = (LPNMTREEVIEW)lParam;

					// ignore nullptr selects
					if (lpnmtv->itemNew.hItem == nullptr)
						return FALSE;

					// make pointer to newly selected item's handle
					type = (uint32_t *)lpnmtv->itemNew.lParam;

					// lookup type
					switch (*type)
					{
						case TYPE_MENUHEADER:
							mh = (menuHeader *)type;
							break;

						case TYPE_MENUENTRY:
							me = (menuEntry *)type;

							// update header/help text
							SetDlgItemText(hdlg, IDC_MENU_HEADER, me->headerText);
							SetDlgItemText(hdlg, IDC_MENU_HELP, me->helpText);

				// ***START DEBUG CODE***
							sprintf(buf, "%04X", me->menuPtr->status);
							SetDlgItemText(hdlg, IDC_MENU_STATUS, buf);
				// ***END DEBUG CODE***

							// select appropriate status
							CheckDlgButton(hdlg, IDC_MENU_ACTIVE,   BST_UNCHECKED);
							CheckDlgButton(hdlg, IDC_MENU_SHOWONLY, BST_UNCHECKED);
							CheckDlgButton(hdlg, IDC_MENU_DISABLED, BST_UNCHECKED);

							// mode -> normal[00] | disabled[08] | show-only[04]
							if (me->menuPtr->status & 0x04)
								CheckDlgButton(hdlg, IDC_MENU_SHOWONLY, BST_CHECKED);
							else if (me->menuPtr->status & 0x08)
								CheckDlgButton(hdlg, IDC_MENU_DISABLED, BST_CHECKED);
							else
								CheckDlgButton(hdlg, IDC_MENU_ACTIVE, BST_CHECKED);

							// set x/y position
							sprintf(buf, "%d", me->menuPtr->xPosition);
							SetDlgItemText(hdlg, IDC_MENU_POSITION_X, buf);

							sprintf(buf, "%d", me->menuPtr->yPosition);
							SetDlgItemText(hdlg, IDC_MENU_POSITION_Y, buf);

							// nuke all items in *both* combo boxes
							hItemBios  = GetDlgItem(hdlg, IDC_MENU_ITEMBIOS);
							hItemSetup = GetDlgItem(hdlg, IDC_MENU_ITEMSETUP);
							SendMessage(hItemBios,  CB_RESETCONTENT, 0, 0);
							SendMessage(hItemSetup, CB_RESETCONTENT, 0, 0);

							// add the items to *both* combo boxes
							mi = me->itemList;
							for (t = 0; t < mi->itemCount; t++)
							{
								SendMessage(hItemBios,  CB_ADDSTRING, 0, (LPARAM)mi->itemText[t]);
								SendMessage(hItemSetup, CB_ADDSTRING, 0, (LPARAM)mi->itemText[t]);
							}

							// set the current selector in the appropriate combo box
							SendMessage(hItemBios,  CB_SETCURSEL, me->menuPtr->biosDefaultIdx,  0);
							SendMessage(hItemSetup, CB_SETCURSEL, me->menuPtr->setupDefaultIdx, 0);

							// show CMOS data
							sprintf(buf, "%02X", me->menuPtr->cmosIndex);
							SetDlgItemText(hdlg, IDC_MENU_CMOS_INDEX, buf);

							sprintf(buf, "%04X", me->menuPtr->cmosMask);
							SetDlgItemText(hdlg, IDC_MENU_CMOS_MASK, buf);

							// show chipset register data
							sprintf(buf, "%04X", me->menuPtr->chipRegIndex);
							SetDlgItemText(hdlg, IDC_MENU_CHIPREG_INDEX, buf);

							sprintf(buf, "%04X", me->menuPtr->chipRegMask);
							SetDlgItemText(hdlg, IDC_MENU_CHIPREG_MASK, buf);
							break;
					}
					break;
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_MENU_RUN:
					if (sysbiosVersion != awdbeBIOSVer451PG)
					{
						if (MessageBox(hdlg, "This function has not fully been implemented for BIOSes other than version 4.51PG.\n\n"
							"Of course, you can try it anyway and see if it does anything, but at worst case\n"
							"the Award BIOS Editor might crash.  Click OK to try it, Cancel to abort.", "Warning", MB_OKCANCEL) == IDCANCEL)
						{
							return FALSE;
						}
					}

					if (hMenuThread != nullptr)
						return FALSE;

					hMenuThread = CreateThread(nullptr, 0, runSetupMenu, 0, 0, &foo);
					break;
			}
			break;
	}

	return FALSE;
}

const char SYSBIOS_MENU_NAME_ALL_ITEMS[] = "All Items";

void sysbiosRefreshMenu(uint8_t *ptr)
{
	uint8_t *sptr, *mptr;
	uint16_t *ptr16;
	char buf[256];
	int len, t, count, pg;
	HWND htree;
	sysbiosMenuStruct *menuPtr, *menuEndPtr;
	menuEntry *me, *keepentry;
	menuHeader *mh;
	TVINSERTSTRUCT tvis;
	HTREEITEM hitem;
	sysbiosMenuDef *menuDefPtr;
	fileEntry *setupFE;
	uint8_t *setupPtr, *menuStrPtr;
	uint32_t baseOffset;
	bool done;

	// call our own destroy function to kill any leftover threads and free memory
	sysbiosDestroyMenu();

	// setup default menu pointers/offsets to point within this component
	setupPtr   = ptr;
	baseOffset = 0x10000;

	// lookup menu definition struct
	sptr  = ptr + 0x1F85D;
	ptr16 = (uint16_t *)sptr;
	menuDefPtr = (sysbiosMenuDef *)((ptr + 0x10000) + (*ptr16));

	if (sysbiosVersion == awdbeBIOSVer60)
	{
		setupFE = awdbeSearchForID(myID, 0x6000);
		if (setupFE != nullptr)
		{
			setupPtr   = (uint8_t *)setupFE->data;
			baseOffset = 0;
		}
		else
		{
			MessageBox(sysbiosTabList[5].hwnd, "BIOS Setup Menu component was not found.  Strings will most likely be displayed as garbage... please notify bpoint.", "Warning", MB_OK);
		}

		menuPtr    = (sysbiosMenuStruct *)((ptr + 0x10000) + menuDefPtr->pointerToPageStart);
		menuEndPtr = (sysbiosMenuStruct *)((ptr + 0x10000) + menuDefPtr->pointerToPageEnd);

		count = (int)(menuEndPtr - menuPtr);

		for (t = 0; t < count; t++)
		{
			me = sysbiosAddToMenuEntryList(menuPtr, setupPtr, baseOffset);
			if (t == 0)
				keepentry = me;

			menuPtr++;
		}

		sysbiosAddToMenuHeaderList(const_cast<char*>(SYSBIOS_MENU_NAME_ALL_ITEMS), count, keepentry, menuDefPtr);
	}
	else
	{
		pg   = 1;
		done = FALSE;

		while (!done)
		{
			if ((menuDefPtr->pointerToPageStart == 0xFFFF) || (menuDefPtr->pointerToStartupString == 0xFFFF))
				done = TRUE;
			else
			{
				menuPtr    = (sysbiosMenuStruct *)((ptr + 0x10000) + menuDefPtr->pointerToPageStart);
				menuEndPtr = (sysbiosMenuStruct *)((ptr + 0x10000) + menuDefPtr->pointerToPageEnd);
				menuStrPtr = (uint8_t *)            ((ptr + 0x10000) + menuDefPtr->pointerToStartupString);

				count = (int)(menuEndPtr - menuPtr);

				for (t = 0; t < count; t++)
				{
					me = sysbiosAddToMenuEntryList(menuPtr, ptr, 0x10000);
					if (t == 0)
						keepentry = me;

					menuPtr++;
				}

				// assign a default name, just in case we don't find anything..
				sprintf(buf, "Page %d", pg);

				// iterate through the menu string until we reach a POS byte, or a terminator
				done = FALSE;
				mptr = menuStrPtr;

				while (!done)
				{
					len = sysbiosMenuStrParse(mptr);

					if (*mptr == MENU_POS)
					{
						// found a POS byte -- skip this section
						mptr += len;

						// check to see if the next few bytes are "printable"
						if (isprint(*mptr) && isprint(*(mptr + 1)) && isprint(*(mptr + 2)))
						{
							// use this string
							len = sysbiosMenuStrParse(mptr);

							memcpy(buf, mptr, len);
							buf[len] = 0;

							done = TRUE;
						}
					}
					else if ((*mptr == MENU_DONE) || (*mptr == MENU_DONE_1))
					{
						done = TRUE;
					}
					else
					{
						// advance to next token
						mptr += len;
					}
				}

				// clear done flag for outer loop
				done = FALSE;

				sysbiosAddToMenuHeaderList(buf, count, keepentry, menuDefPtr);

				menuDefPtr++;
				pg++;
			}
		}
	}

	sysbiosCreateMenuItems(setupPtr, baseOffset);

	// remove all items from the tree
	htree = GetDlgItem(sysbiosTabList[5].hwnd, IDC_MENU_TREE);
	SendMessage(htree, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT);

	// add the goodies to the tree
	mh = menuHeaderList;
	while (mh != nullptr)
	{
		// create a root entry
		tvis.hParent		= TVI_ROOT;
		tvis.hInsertAfter	= TVI_LAST;
		tvis.itemex.mask	= TVIF_TEXT | TVIF_PARAM;
		tvis.itemex.pszText	= mh->headerText;
		tvis.itemex.lParam	= (LPARAM)mh;
		hitem = (HTREEITEM)SendMessage(htree, TVM_INSERTITEM, 0, (LPARAM)&tvis);

		// add the entries below the header
		me    = mh->firstEntry;
		count = mh->entryCount;
		while (count--)
		{
			tvis.hParent		= hitem;
			tvis.hInsertAfter	= TVI_LAST;
			tvis.itemex.mask	= TVIF_TEXT | TVIF_PARAM;
			tvis.itemex.pszText	= me->headerText;
			tvis.itemex.lParam	= (LPARAM)me;
			SendMessage(htree, TVM_INSERTITEM, 0, (LPARAM)&tvis);

			me = me->next;
		}

		mh = mh->next;
	}

	// finally, clear cmos and load BIOS defaults
	sysbiosCMOSLoadDefaults(TYPE_BIOS);
}

void sysbiosDestroyMenu(void)
{
	// kill the setup menu thread if it is still running
	if (hMenuThread != nullptr)
	{
		gExitMenu = TRUE;

		if (WaitForSingleObject(hMenuThread, 1000) == WAIT_TIMEOUT)
		{
			TerminateThread(hMenuThread, 0);
			hMenuThread = nullptr;
		}

		gExitMenu = FALSE;
	}

	// kill the console, if it's still around
	if (hCon != nullptr)
	{
		free_console();
		hCon = nullptr;
	}

	// free any existing memory
	sysbiosReleaseMenuItems();
}

int sysbiosGetMaskShift(int mask)
{
	int count = 0;

	if (mask == 0)
		return 0;

	while ((mask & 1) == 0)
	{
		mask >>= 1;
		count++;
	}

	return count;
}

uint16_t sysbiosCMOSRead(uint8_t index, uint16_t mask)
{
	uint16_t val;

	// check for 16-bit reads
	if (mask & 0xFF00)
	{
		val   = (cmosTable[index] << 8) | (cmosTable[index + 1]);
		val  &= mask;
		val >>= sysbiosGetMaskShift(mask);
	}
	else
	{
		val   = cmosTable[index] & mask;
		val >>= sysbiosGetMaskShift(mask);
	}

	return val;
}

void sysbiosCMOSWrite(uint8_t index, uint16_t mask, uint16_t val)
{
	uint8_t mh, ml, vh, vl;

	// check for 16-bit writes
	if (mask & 0xFF00)
	{
		mh = (mask & 0xFF00) >> 8;
		ml = (mask & 0x00FF);

		vh = (val & 0xFF00) >> 8;
		vl = (val & 0x00FF);

		cmosTable[index] &= ~mh;
		cmosTable[index] |= (vh << sysbiosGetMaskShift(mh)) & mh;

		cmosTable[index + 1] &= ~ml;
		cmosTable[index + 1] |= (vl << sysbiosGetMaskShift(ml)) & ml;
	}
	else
	{
		cmosTable[index] &= ~mask;
		cmosTable[index] |= (val << sysbiosGetMaskShift(mask)) & mask;
	}
}

void sysbiosCMOSLoadDefaults(int type)
{
	menuEntry *me = menuEntryList;
	uint8_t idx;
	uint16_t val;

	// first, zap the table
	memset(cmosTable, 0, 0x80);

	// iterate through all menu entries and store the default values
	while (me != nullptr)
	{
		idx = me->menuPtr->cmosIndex;
		if (idx < 0x80)
		{
			if (type == TYPE_BIOS)
				val = me->menuPtr->biosDefaultIdx;
			else
				val = me->menuPtr->setupDefaultIdx;

			// ensure value is within limits (should I really have to do this?)
			if ( (!(me->menuPtr->status & STATUS_NOMODBIN)) && (!(me->menuPtr->status & STATUS_INPUT)) )
			{
				if (val >= me->itemList->itemCount)
					val = (me->itemList->itemCount) - 1;
			}

			sysbiosCMOSWrite(idx, me->menuPtr->cmosMask, val);
		}

		me = me->next;
	}
}
