//
// Award BIOS Editor - main.cpp
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: main.cpp,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_GLOB_H
#include <glob.h>
#endif
#include <windows.h>
#include <commctrl.h>
#include "types.h"
#include "resource.h"
#include "main.h"
#include "popupMenu.h"
#include "awdbedit_ids.h"
#include "config.h"
#include "lzh.h"
#define AWDBE_EXPORT_FUNCS
#include "awdbe_exports.h"
#include "bios.h"
#include "plugin.h"


#define MAX_SKINS			20

struct
{
	int		id;
	char	fname[256];
} skinList[MAX_SKINS];

globalsStruct globals;
char exePath[256], fullTempPath[256];

static char mainChangedText[] = "This BIOS image has been changed.  Do you want to save your changes before exiting?";

#define SPLITTER_WINDOW_SIZE		3


INT_PTR APIENTRY ConfigBoxProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	OPENFILENAME ofn;
	char fname[256];

	switch (message)
	{
		case WM_INITDIALOG:
			SetWindowText(GetDlgItem(hDlg, IDC_HEXEDIT_PATH), config.hexEditor);
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_HEXEDIT_SEARCH:
					// display an open dialog
					fname[0] = 0;
					ZeroMemory(&ofn, sizeof(OPENFILENAME));
					ofn.lStructSize			= sizeof(OPENFILENAME);
					ofn.hwndOwner			= hDlg;
					ofn.hInstance			= globals.MainhInst;
					ofn.lpstrFilter			= "Executable Files (*.exe)\0*.exe\0All Files (*.*)\0*.*\0\0";
					ofn.lpstrCustomFilter	= nullptr;
					ofn.nMaxCustFilter		= 0;
					ofn.nFilterIndex		= 1;
					ofn.lpstrFile			= fname;
					ofn.nMaxFile			= 256;
					ofn.lpstrFileTitle		= nullptr;
					ofn.nMaxFileTitle		= 0;
					ofn.lpstrInitialDir		= nullptr;
					ofn.lpstrTitle			= nullptr;
					ofn.Flags				= OFN_FILEMUSTEXIST | OFN_ENABLESIZING | OFN_EXPLORER;
					ofn.nFileOffset			= 0;
					ofn.nFileExtension		= 0;
					ofn.lpstrDefExt			= nullptr;
					ofn.lCustData			= 0;
					ofn.lpfnHook			= nullptr;
					ofn.lpTemplateName		= nullptr;

					if (GetOpenFileName(&ofn) == FALSE)
						return TRUE;

					SetWindowText(GetDlgItem(hDlg, IDC_HEXEDIT_PATH), fname);
					break;

				case IDC_RESET_ASK:
					config.removeNoAsk	 = FALSE;
					config.removeBDNoAsk = FALSE;

					MessageBox(hDlg, "All \"Don\'t ask me again\" dialogs have been reset.", "Notice", MB_OK);
					break;

				case IDOK:
					GetWindowText(GetDlgItem(hDlg, IDC_HEXEDIT_PATH), config.hexEditor, 256);
					
					// fall through to end the dialog...

				case IDCANCEL:
					EndDialog(hDlg, TRUE);
					break;					
			}
			return TRUE;
	}

	return FALSE;
}

const char APP_NAME[] = "Award BIOS Editor";
const char APP_REV[] = "1.0";
const char APP_VERSION[] = "Award BIOS Editor 1.0";

INT_PTR APIENTRY AboutBoxProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	LPDRAWITEMSTRUCT lpdis;
	COLORREF oldcol;
	char buf[256], buf2[264];
	POINT pt[2];
	HPEN hpen, oldpen;

	switch (message)
	{
		case WM_INITDIALOG:
			SetDlgItemText(hDlg, IDC_TEXT_APPVERSION, APP_VERSION);

			sprintf(buf, "Compiled on %s at %s", __DATE__, __TIME__);
			SetDlgItemText(hDlg, IDC_TEXT_COMPILETIME, buf);
			return TRUE;

		case WM_DRAWITEM:
			lpdis = (LPDRAWITEMSTRUCT)lParam;

			switch (wParam)
			{
				case IDC_TEXT_SOURCEFORGE_WEB:
				case IDC_TEXT_SOURCEFORGE_MAIL:
				case IDC_TEXT_HENDRIK_MAIL:
				case IDC_TEXT_TMOD_WEB:
					oldcol = SetTextColor(lpdis->hDC, RGB(0, 0, 255));
					GetWindowText(lpdis->hwndItem, buf, 256);

					// draw the text
					TextOut(lpdis->hDC, 1, 0, buf, strlen(buf));

					// now draw the underline
					hpen   = CreatePen(PS_SOLID, 1, RGB(0, 0, 255));
					oldpen = (HPEN)SelectObject(lpdis->hDC, hpen);

					pt[0].x = 0;					   pt[0].y = lpdis->rcItem.bottom - 1;
					pt[1].x = lpdis->rcItem.right - 1; pt[1].y = pt[0].y;
					Polyline(lpdis->hDC, pt, 2);

					SelectObject(lpdis->hDC, oldpen);
					DeleteObject(hpen);

					SetTextColor(lpdis->hDC, oldcol);
					return TRUE;
			}
			break;

		case WM_COMMAND:
			if (HIWORD(wParam) == STN_CLICKED)
			{
				switch (LOWORD(wParam))
				{
					case IDC_TEXT_SOURCEFORGE_WEB:
					case IDC_TEXT_TMOD_WEB:
						GetWindowText(GetDlgItem(hDlg, LOWORD(wParam)), buf, 256);
						ShellExecute(globals.MainhWnd, "open", buf, nullptr, nullptr, SW_SHOWNORMAL);
						return 0;

					case IDC_TEXT_SOURCEFORGE_MAIL:
					case IDC_TEXT_HENDRIK_MAIL:
						GetWindowText(GetDlgItem(hDlg, LOWORD(wParam)), buf, 256);
						sprintf(buf2, "mailto:%s", buf);

						ShellExecute(globals.MainhWnd, "open", buf2, nullptr, nullptr, SW_SHOWNORMAL);
						return 0;
				}
			}

			if (LOWORD(wParam) == IDOK)
			{
				EndDialog(hDlg, TRUE);
			}
			return TRUE;
	}

	return FALSE;
}

void enableControls(bool fileLoaded, bool selIsValid)
{
	int t;
	int idLoadTable[6] = { ID_ACTION_SAVE, ID_ACTION_SAVE_AS, ID_FILE_REVERT, ID_FILE_PROPERTIES, ID_ACTION_INSERT, ID_ACTION_EXTRACT_ALL };
	int idSelVTable[4] = { ID_ACTION_REPLACE, ID_ACTION_EXTRACT, ID_ACTION_REMOVE, ID_ACTION_HEXEDIT };
	LPARAM lflag;
	UINT uflag;

	if (fileLoaded == TRUE)
	{
		lflag = MAKELONG(TBSTATE_ENABLED, 0);
		uflag = MF_ENABLED;
	}
	else
	{
		lflag = MAKELONG(0, 0);
		uflag = MF_GRAYED;
	}

	for (t = 0; t < 6; t++)
	{
		SendMessage(globals.hToolBar, TB_SETSTATE, idLoadTable[t], lflag);
		EnableMenuItem(globals.MainhMenu, idLoadTable[t], uflag);
	}

	if (selIsValid == TRUE)
	{
		lflag = MAKELONG(TBSTATE_ENABLED, 0);
		uflag = MF_ENABLED;
	}
	else
	{
		lflag = MAKELONG(0, 0);
		uflag = MF_GRAYED;
	}

	for (t = 0; t < 4; t++)
	{
		SendMessage(globals.hToolBar, TB_SETSTATE, idSelVTable[t], lflag);
		EnableMenuItem(globals.MainhMenu, idSelVTable[t], uflag);
	}
}

void insertRebarControls(HWND hwnd)
{
	REBARBANDINFO rbbi;
	RECT rc; //, rc2;
	SIZE size;

	// first, tell our rebar to use the default GUI font for drawing text
	SendMessage(globals.hRebar, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(FALSE, 0));

	// Encapsulate the menu in the rebar
	ZeroMemory(&rbbi, sizeof(rbbi));
	GetWindowRect(globals.hMenuBar, &rc);
//	GetWindowRect(hwnd, &rc2);					// use the main window to max out the size of the menu bar
	rbbi.cbSize		= sizeof(REBARBANDINFO);
	rbbi.fMask		= RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_BACKGROUND;
	rbbi.fStyle		= RBBS_FIXEDBMP | RBBS_NOGRIPPER | RBBS_FIXEDSIZE | RBBS_BREAK;
	rbbi.hbmBack	= globals.hSkin;
	rbbi.hwndChild	= globals.hMenuBar;
	rbbi.cxMinChild	= 32760; //rc2.right - rc2.left;
	rbbi.cyMinChild	= (rc.bottom - rc.top) - 3;

	// Add the menu band
	SendMessage(globals.hRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbbi);

	// Get the size of the toolbar.
	SendMessage(globals.hToolBar, TB_GETMAXSIZE, 0, (LPARAM)&size);

	// Encapsulate the toolbar in the rebar
	ZeroMemory(&rbbi, sizeof(rbbi));
	GetWindowRect(globals.hToolBar, &rc);
	rbbi.cbSize		= sizeof(REBARBANDINFO);
	rbbi.fMask		= RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_BACKGROUND | RBBIM_SIZE | RBBIM_HEADERSIZE;
	rbbi.fStyle		= RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBBS_NOGRIPPER;
	rbbi.hbmBack	= globals.hSkin;
	rbbi.hwndChild  = globals.hToolBar;
	rbbi.cxMinChild = size.cx;
	rbbi.cyMinChild = size.cy;
	rbbi.cx			= size.cx;
	rbbi.cxHeader	= 3;		// just some space before the button

	// Add the toolbar band
	SendMessage(globals.hRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbbi);
}

void resizeControlsFromRoot(SIZE rootsz)
{
	RECT rc, rcx;
	int sbarHeight, rbarHeight;
	HWND hwnd;

	// from the root, subtract the size used by our rebar
	GetClientRect(globals.hRebar, &rc);
	rbarHeight = (rc.bottom - rc.top);
	rootsz.cy -= rbarHeight;

	// and subtract the size used by our status bar
	GetClientRect(globals.hStatusBar, &rc);
	sbarHeight = (rc.bottom - rc.top);
	rootsz.cy -= sbarHeight;

	// now, create a rect and reposition our treeview
	rc.left   = 0;
	rc.top    = rbarHeight + 2;
	rc.right  = config.treeViewWidth;
	rc.bottom = rootsz.cy - 2;

	if (globals.hTreeView != nullptr)
		SetWindowPos(globals.hTreeView, 0, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER);

	// subtract the size of the treeview, and reposition our splitter bar
	rc.left += config.treeViewWidth;
	rc.right = rc.left + SPLITTER_WINDOW_SIZE;

	if (globals.hSplitter != nullptr)
		SetWindowPos(globals.hSplitter, 0, rc.left, rc.top, rc.right, rc.bottom + 2, SWP_NOZORDER);

	// subtract the size of the splitter bar, and reposition our dialog
	rc.left  = rc.right;
	rc.right = rootsz.cx - (config.treeViewWidth + SPLITTER_WINDOW_SIZE);

	hwnd = biosGetDialog();
	if (hwnd != nullptr)
	{
		SetWindowPos(hwnd, 0, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER);

		GetClientRect(hwnd, &rcx);
		biosResizeCurrentDialog(hwnd, &rcx);
	}

	// store this rect in our dialog rect struct
	memcpy(&globals.dialogrc, &rc, sizeof(RECT));
}

void resizeControlsFromDialog(SIZE dialogsz)
{
	RECT rc, rc2;
	SIZE border;

	// from the size of the dialog rectangle, we need to resize our main window.  start by adding the width of the treeview...
	dialogsz.cx += config.treeViewWidth + SPLITTER_WINDOW_SIZE;

	// get the size of the status bar and add it to our height
	GetClientRect(globals.hStatusBar, &rc);
	dialogsz.cy += (rc.bottom - rc.top);

	// get the size of our rebar and add that to our height
	GetClientRect(globals.hRebar, &rc);
	dialogsz.cy += (rc.bottom - rc.top);

	// magic borders
	dialogsz.cy += 2;

	// adjust for window borders and titlebar by taking the size of the total window, minus the client rectangle
	GetWindowRect(globals.MainhWnd, &rc);
	GetClientRect(globals.MainhWnd, &rc2);

	border.cx = ((rc.right - rc.left) - rc2.right);
	border.cy = ((rc.bottom - rc.top) - rc2.bottom);

	// use this rectangle to resize our main window
	SetWindowPos(globals.MainhWnd, 0, 0, 0, dialogsz.cx + border.cx, dialogsz.cy + border.cy, SWP_NOMOVE | SWP_NOZORDER);

	// and resize our other controls from here
	resizeControlsFromRoot(dialogsz);
}

void resizeTreeView(int newWidth)
{
	SIZE sz, border;
	RECT rc, rc2;

	config.treeViewWidth = newWidth;

	// adjust for window borders and titlebar by taking the size of the total window, minus the client rectangle
	GetWindowRect(globals.MainhWnd, &rc);
	GetClientRect(globals.MainhWnd, &rc2);

	border.cx = ((rc.right - rc.left) - rc2.right);
	border.cy = ((rc.bottom - rc.top) - rc2.bottom);

	// use this rectangle to resize our main window
	sz.cx = config.rootXSize - border.cx;
	sz.cy = config.rootYSize - border.cy;

	resizeControlsFromRoot(sz);
}

const char SOME_NAME_Recognized_Items[] = "Recognized Items";
const char SOME_NAME_Unknown_Items[] = "Unknown Items";
const char SOME_NAME_Includable_Items[] = "Includable Items";

void createControls(HWND hwnd)
{
	REBARINFO rbi;
	HICON iFileOpen, iFileSave, iFileProperties, iFileExit, iActionInsert, iActionReplace, iActionExtract;	
	HICON iActionExtractAll, iActionRemove, iActionHexEdit, iOptionConfig;
	TVINSERTSTRUCT tvis;
	SIZE sz;

	TBBUTTON menuList[] = {
		{ .iBitmap=I_IMAGENONE, .idCommand=ID_MENU_FILE,	.fsState=TBSTATE_ENABLED, .fsStyle=TBSTYLE_AUTOSIZE | TBSTYLE_DROPDOWN, .dwData=0, .iString=0 },
		{ .iBitmap=I_IMAGENONE, .idCommand=ID_MENU_ACTIONS,	.fsState=TBSTATE_ENABLED, .fsStyle=TBSTYLE_AUTOSIZE | TBSTYLE_DROPDOWN, .dwData=0, .iString=0 },
		{ .iBitmap=I_IMAGENONE, .idCommand=ID_MENU_OPTIONS,	.fsState=TBSTATE_ENABLED, .fsStyle=TBSTYLE_AUTOSIZE | TBSTYLE_DROPDOWN, .dwData=0, .iString=0 },
		{ I_IMAGENONE, ID_MENU_HELP,	TBSTATE_ENABLED, TBSTYLE_AUTOSIZE | TBSTYLE_DROPDOWN, 0, 0 }
	};
	
	TBBUTTON toolList[] = {
		{ .iBitmap=0, .idCommand=ID_ACTION_OPEN,		.fsState=TBSTATE_ENABLED,	.fsStyle=TBSTYLE_BUTTON,.dwData=0, .iString=0 },
		{ .iBitmap=0, .idCommand=ID_ACTION_SAVE,		.fsState=0,					.fsStyle=TBSTYLE_BUTTON,.dwData=0, .iString=0 },
		{ .iBitmap=0, .idCommand=ID_FILE_PROPERTIES,	.fsState=0,					.fsStyle=TBSTYLE_BUTTON,.dwData=0, .iString=0 },
		{ .iBitmap=0, .idCommand=0,						.fsState=TBSTATE_ENABLED,	.fsStyle=TBSTYLE_SEP,	.dwData=0, .iString=-1 },
		{ .iBitmap=0, .idCommand=ID_ACTION_INSERT,		.fsState=0,					.fsStyle=TBSTYLE_BUTTON,.dwData=0, .iString=0 },
		{ .iBitmap=0, .idCommand=ID_ACTION_REPLACE,		.fsState=0,					.fsStyle=TBSTYLE_BUTTON,.dwData=0, .iString=0 },
		{ .iBitmap=0, .idCommand=ID_ACTION_EXTRACT,		.fsState=0,					.fsStyle=TBSTYLE_BUTTON,.dwData=0, .iString=0 },
		{ .iBitmap=0, .idCommand=ID_ACTION_EXTRACT_ALL,	.fsState=0,					.fsStyle=TBSTYLE_BUTTON,.dwData=0, .iString=0 },
		{ .iBitmap=0, .idCommand=ID_ACTION_REMOVE,		.fsState=0,					.fsStyle=TBSTYLE_BUTTON,.dwData=0, .iString=0 },
		{ .iBitmap=0, .idCommand=0,						.fsState=TBSTATE_ENABLED,	.fsStyle=TBSTYLE_SEP,	.dwData=0, .iString=-1 },
		{ .iBitmap=0, .idCommand=ID_ACTION_HEXEDIT,		.fsState=0,					.fsStyle=TBSTYLE_BUTTON,.dwData=0, .iString=0 },
		{ .iBitmap=0, .idCommand=0,						.fsState=TBSTATE_ENABLED,	.fsStyle=TBSTYLE_SEP,	.dwData=0, .iString=-1 },
		{ .iBitmap=0, .idCommand=ID_OPTION_CONFIG,		.fsState=TBSTATE_ENABLED,	.fsStyle=TBSTYLE_BUTTON,.dwData=0, .iString=0 },
		{ .iBitmap=0, .idCommand=0,						.fsState=TBSTATE_ENABLED,	.fsStyle=TBSTYLE_SEP,	.dwData=0, .iString=-1 },
		{ .iBitmap=0, .idCommand=ID_FILE_EXIT,			.fsState=TBSTATE_ENABLED,	.fsStyle=TBSTYLE_BUTTON,.dwData=0, .iString=0 }
	};

	// create a rebar
	globals.hRebar = CreateWindowEx(WS_EX_CLIENTEDGE, REBARCLASSNAME, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
		WS_CLIPCHILDREN | RBS_VARHEIGHT | RBS_BANDBORDERS | CCS_NODIVIDER, 0, 0, 0, 0, hwnd, 
		nullptr, globals.MainhInst, nullptr);

	// initialize and send the REBARINFO structure.
	rbi.cbSize = sizeof(REBARINFO);
	rbi.fMask  = 0;
	rbi.himl   = (HIMAGELIST)nullptr;
	SendMessage(globals.hRebar, RB_SETBARINFO, 0, (LPARAM)&rbi);

	// create a toolbar for our menu
	globals.hMenuBar = CreateWindowEx(0, TOOLBARCLASSNAME, nullptr, WS_CHILD | TBSTYLE_FLAT | TBSTYLE_LIST, 0, 0,
		0, 0, globals.hRebar, nullptr, globals.MainhInst, nullptr);

	SendMessage(globals.hMenuBar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

	menuList[0].iString = (INT_PTR)"&File";
	menuList[1].iString = (INT_PTR)"&Actions";
	menuList[2].iString = (INT_PTR)"&Options";
	menuList[3].iString = (INT_PTR)"&Help";

	SendMessage(globals.hMenuBar, TB_ADDBUTTONS, 4, (LPARAM)&menuList);
	SendMessage(globals.hMenuBar, TB_AUTOSIZE, 0, 0); 

	// create our toolbar
	globals.hToolBar = CreateWindowEx(0, TOOLBARCLASSNAME, nullptr, WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | CCS_NOMOVEY |
		CCS_NOPARENTALIGN | CCS_NODIVIDER | CCS_NORESIZE, 0, 0, 0, 0, hwnd, nullptr, globals.MainhInst, nullptr);

	SendMessage(globals.hToolBar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	SendMessage(globals.hToolBar, TB_SETBITMAPSIZE, 0, MAKELONG(16, 16));

	// get handles to the icons we're going to use on our toolbar
	iFileOpen			= LoadIcon(globals.MainhInst, MAKEINTRESOURCE(IDI_FILE_OPEN));
	iFileSave			= LoadIcon(globals.MainhInst, MAKEINTRESOURCE(IDI_FILE_SAVE));
	iFileProperties		= LoadIcon(globals.MainhInst, MAKEINTRESOURCE(IDI_FILE_PROPERTIES));
	iFileExit			= LoadIcon(globals.MainhInst, MAKEINTRESOURCE(IDI_FILE_EXIT));
	iActionInsert		= LoadIcon(globals.MainhInst, MAKEINTRESOURCE(IDI_ACTION_INSERT));
	iActionReplace		= LoadIcon(globals.MainhInst, MAKEINTRESOURCE(IDI_ACTION_REPLACE));
	iActionExtract		= LoadIcon(globals.MainhInst, MAKEINTRESOURCE(IDI_ACTION_EXTRACT));
	iActionExtractAll	= LoadIcon(globals.MainhInst, MAKEINTRESOURCE(IDI_ACTION_EXTRACT_ALL));
	iActionRemove		= LoadIcon(globals.MainhInst, MAKEINTRESOURCE(IDI_ACTION_REMOVE));
	iActionHexEdit		= LoadIcon(globals.MainhInst, MAKEINTRESOURCE(IDI_ACTION_HEXEDIT));
	iOptionConfig		= LoadIcon(globals.MainhInst, MAKEINTRESOURCE(IDI_OPTION_CONFIG));

	// create an imagelist
	globals.hToolImageList = ImageList_Create(16, 16, ILC_COLOR16 | ILC_MASK, 7, 0);
	ImageList_SetBkColor(globals.hToolImageList, CLR_NONE);

	// fill out our menu's bitmaps...
	toolList[0].iBitmap		= ImageList_AddIcon(globals.hToolImageList, iFileOpen);
	toolList[1].iBitmap		= ImageList_AddIcon(globals.hToolImageList, iFileSave);
	toolList[2].iBitmap		= ImageList_AddIcon(globals.hToolImageList, iFileProperties);

	toolList[4].iBitmap		= ImageList_AddIcon(globals.hToolImageList, iActionInsert);
	toolList[5].iBitmap		= ImageList_AddIcon(globals.hToolImageList, iActionReplace);
	toolList[6].iBitmap		= ImageList_AddIcon(globals.hToolImageList, iActionExtract);
	toolList[7].iBitmap		= ImageList_AddIcon(globals.hToolImageList, iActionExtractAll);
	toolList[8].iBitmap		= ImageList_AddIcon(globals.hToolImageList, iActionRemove);

	toolList[10].iBitmap	= ImageList_AddIcon(globals.hToolImageList, iActionHexEdit);
	toolList[12].iBitmap	= ImageList_AddIcon(globals.hToolImageList, iOptionConfig);
	toolList[14].iBitmap	= ImageList_AddIcon(globals.hToolImageList, iFileExit);

	// set the toolbar's imagelist first...
	SendMessage(globals.hToolBar, TB_SETIMAGELIST, 0, (LPARAM)globals.hToolImageList);

	// then add the buttons and resize.
	SendMessage(globals.hToolBar, TB_ADDBUTTONS, 15, (LPARAM)&toolList);
	SendMessage(globals.hToolBar, TB_AUTOSIZE, 0, 0);

	// destroy our no-longer needed icons
	DestroyIcon(iFileOpen);
	DestroyIcon(iFileSave);
	DestroyIcon(iFileProperties);
	DestroyIcon(iFileExit);
	DestroyIcon(iActionInsert);
	DestroyIcon(iActionReplace);
	DestroyIcon(iActionExtract);
	DestroyIcon(iActionExtractAll);
	DestroyIcon(iActionRemove);
	DestroyIcon(iActionHexEdit);
	DestroyIcon(iOptionConfig);

	// insert the controls into the rebar...
	insertRebarControls(hwnd);

	// now, create our tree view (for the left-side pane), but hidden for now
	globals.hTreeView = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, nullptr, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
		TVS_HASLINES | TVS_HASBUTTONS | TVS_SHOWSELALWAYS | TVS_LINESATROOT, 0, 0, 0, 0, hwnd, nullptr, globals.MainhInst, nullptr);

	// add the root items to the tree...
	tvis.hParent			= TVI_ROOT;
	tvis.hInsertAfter		= TVI_LAST;
	tvis.itemex.mask		= TVIF_TEXT | TVIF_PARAM;
	tvis.itemex.pszText		= const_cast<char *>(SOME_NAME_Recognized_Items);
	tvis.itemex.lParam		= HASH_RECOGNIZED_ROOT;
	globals.hTreeRecgItem	= (HTREEITEM)SendMessage(globals.hTreeView, TVM_INSERTITEM, 0, (LPARAM)&tvis);

	tvis.hParent			= TVI_ROOT;
	tvis.hInsertAfter		= TVI_LAST;
	tvis.itemex.mask		= TVIF_TEXT | TVIF_PARAM;
	tvis.itemex.pszText		= const_cast<char *>(SOME_NAME_Unknown_Items);
	tvis.itemex.lParam		= HASH_UNKNOWN_ROOT;
	globals.hTreeUnkItem	= (HTREEITEM)SendMessage(globals.hTreeView, TVM_INSERTITEM, 0, (LPARAM)&tvis);

	tvis.hParent			= TVI_ROOT;
	tvis.hInsertAfter		= TVI_LAST;
	tvis.itemex.mask		= TVIF_TEXT | TVIF_PARAM;
	tvis.itemex.pszText		= const_cast<char *>(SOME_NAME_Includable_Items);
	tvis.itemex.lParam		= HASH_INCLUDABLE_ROOT;
	globals.hTreeInclItem	= (HTREEITEM)SendMessage(globals.hTreeView, TVM_INSERTITEM, 0, (LPARAM)&tvis);

	// create our tiny splitter window, but also hidden for now
	globals.hSplitter = CreateWindowEx(0, AWDBEDIT_SPLITTER_CLASSNAME, nullptr, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 0, 0,
		hwnd, nullptr, globals.MainhInst, nullptr);

	// resize our controls, and we're done!
	sz.cx = config.rootXSize;
	sz.cy = config.rootYSize;
	resizeControlsFromRoot(sz);
}

LRESULT DoNotify(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	LPNMHDR	lpNM = (LPNMHDR)lParam;
	LPNMTOOLBAR lpnmtb;
	LPNMTBHOTITEM lpnmtbhi;
	LPNMTREEVIEW lpnmtv;

	switch (lpNM->code)
	{
		case TBN_DROPDOWN:
			lpnmtb = (LPNMTOOLBAR)lParam;

			if (lpnmtb->hdr.hwndFrom == globals.hMenuBar)
			{
				// pass through to the popup menu handler...
				return globals.pMenu->onDropDown(lpnmtb);
			}
			break;

		case TBN_HOTITEMCHANGE:
			lpnmtbhi = (LPNMTBHOTITEM)lParam;

			if (lpnmtbhi->hdr.hwndFrom == globals.hMenuBar)
			{
				// pass through to the popup menu handler...
				return globals.pMenu->onHotItemChange(lpnmtbhi);
			}
			else if (lpnmtbhi->hdr.hwndFrom == globals.hToolBar)
			{
				// close any popup window
				globals.pMenu->closePopup();

				// send this ID so we can update the status bar
				SendMessage(globals.MainhWnd, WM_MENUSELECT, lpnmtbhi->idNew, 0);
			}
			break;

		case TVN_SELCHANGED:
			lpnmtv = (LPNMTREEVIEW)lParam;

			biosItemChanged(lpnmtv);
			break;
	}

	return FALSE;
}

void resizeStatusBar(HWND hwnd)
{
	RECT rc;
	int parts[2] = { 0, -1 };

	// get the size of our main window
	GetClientRect(hwnd, &rc);

	// use this size to calculate the part to split off on the status bar
	parts[0] = rc.right - 150;
	if (parts[0] < 0)
		parts[0] = 0;

	// Tell the status bar to create the window parts.
	SendMessage(globals.hStatusBar, SB_SETPARTS, 2, (LPARAM)&parts);

	// show the status bar
	ShowWindow(globals.hStatusBar, TRUE);
	UpdateWindow(globals.hStatusBar);
}

void createStatusBar(HWND hwnd)
{
	// Create the status bar. 
	globals.hStatusBar = CreateWindowEx(0, STATUSCLASSNAME, nullptr, WS_CHILD | SBARS_SIZEGRIP, 0, 0, 0, 0, hwnd, nullptr, globals.MainhInst, nullptr);

	// and resize it
	resizeStatusBar(hwnd);
}

int handleMenuPopup(HMENU menu)
{
	int count, t;
	char cwd[256], skindir[262];
	#if !defined(HAVE_GLOB_H)
	struct _finddata_t fd;
	#else
	glob_t fd;
	memset(&fd, 0, sizeof(fd));
	#endif
	long hFile;

	// figure out which menu this is...
	if (menu == globals.hRecentFilesMenu)
	{
		// first, delete all items from the menu
		count = GetMenuItemCount(menu);

		while (count--)
			DeleteMenu(menu, 0, MF_BYPOSITION);

		// append all recent files to the menu
		if (config.recentFile[0][0] == 0)
			AppendMenu(menu, MF_GRAYED, ID_NOTHING, "No recent files");
		else
		{
			for (t = 0; t < 4; t++)
			{
				if (config.recentFile[t][0] != 0)
					AppendMenu(menu, MF_ENABLED, ID_FILE_MRU_FILE1 + t, config.recentFile[t]);
			}
		}
		return 0;
	}
	else if (menu == globals.hSkinsMenu)
	{
		// first, delete all items from the menu
		count = GetMenuItemCount(menu);

		while (count--)
			DeleteMenu(menu, 0, MF_BYPOSITION);

		// save off the current dir
		getcwd(cwd, 256);

		// change to the "Skins" subdirectory
		sprintf(skindir, "%sSkins", exePath);
		chdir(skindir);

		// look for all bmp files and add them to the menu
		#if !defined(HAVE_GLOB_H)
		hFile = _findfirst("*.bmp", &fd);
		#else
		hFile = glob("*.bmp", GLOB_TILDE, nullptr, &fd);
		#endif

		if (hFile == -1){
			AppendMenu(menu, MF_GRAYED, ID_NOTHING, "No skins found");
			#if defined(HAVE_GLOB_H)
			globfree(&fd);
			#endif
		}else
		{
			t = 0;

			#if !defined(HAVE_GLOB_H)
			do
			{
			#else
			for(size_t i = 0; i < fd.gl_pathc && i < MAX_SKINS; ++i){
			#endif
				// store the skin in the list
				skinList[t].id = ID_SELECT_SKIN0 + t;
				#if !defined(HAVE_GLOB_H)
				strcpy(skinList[t].fname, fd.name);
				#else
				strcpy(skinList[t].fname, fd.gl_pathv[i]);
				#endif

				// add it to the menu
				if (!stricmp(config.lastSkin, skinList[t].fname))
					AppendMenu(menu, MF_ENABLED | MF_CHECKED, skinList[t].id, skinList[t].fname);
				else
					AppendMenu(menu, MF_ENABLED, skinList[t].id, skinList[t].fname);

				// next...
				t++;
			#if !defined(HAVE_GLOB_H)
			} while ((_findnext(hFile, &fd) == 0) && (t < MAX_SKINS));

			_findclose(hFile);
			#else
			}
			globfree(&fd);
			#endif
		}

		// return back to the stored dir
		chdir(cwd);
		return 0;
	}
	else if (menu == globals.hAboutPluginsMenu)
	{
		// first, delete all items from the menu
		count = GetMenuItemCount(menu);

		while (count--)
			DeleteMenu(menu, 0, MF_BYPOSITION);

		// call our plugin manager to add the items to the menu
		pluginAddToMenu(menu, ID_HELP_PLUGIN0);
	}

	return 1;
}

BOOL LoadBitmapFromBMPFile(LPTSTR szFileName, HBITMAP *phBitmap, HPALETTE *phPalette)
{
	BITMAP bm;

	*phBitmap = nullptr;
	
	if (phPalette != nullptr)
		*phPalette = nullptr;

	// Use LoadImage() to get the image loaded into a DIBSection
	*phBitmap = (HBITMAP)LoadImage(nullptr, szFileName, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_LOADFROMFILE);
	if (*phBitmap == nullptr)
		return FALSE;

	// Get the color depth of the DIBSection
	GetObject(*phBitmap, sizeof(BITMAP), &bm);

	if (phPalette != nullptr)
	{
		// If the DIBSection is 256 color or less, it has a color table
		if ((bm.bmBitsPixel * bm.bmPlanes) <= 8)
		{
			HDC           hMemDC;
			HBITMAP       hOldBitmap;
			RGBQUAD       rgb[256];
			LPLOGPALETTE  pLogPal;
			WORD          i;

			// Create a memory DC and select the DIBSection into it
			hMemDC = CreateCompatibleDC(nullptr);
			hOldBitmap = (HBITMAP)SelectObject(hMemDC, *phBitmap);

			// Get the DIBSection's color table
			GetDIBColorTable(hMemDC, 0, 256, rgb);

			// Create a palette from the color table
			pLogPal = (LOGPALETTE *)malloc(sizeof(LOGPALETTE) + (256 * sizeof(PALETTEENTRY)));
			pLogPal->palVersion = 0x300;
			pLogPal->palNumEntries = 256;
			
			for (i = 0; i < 256; i++)
			{
				pLogPal->palPalEntry[i].peRed = rgb[i].rgbRed;
				pLogPal->palPalEntry[i].peGreen = rgb[i].rgbGreen;
				pLogPal->palPalEntry[i].peBlue = rgb[i].rgbBlue;
				pLogPal->palPalEntry[i].peFlags = 0;
			}

			*phPalette = CreatePalette(pLogPal);

			// Clean up
			free(pLogPal);
			SelectObject(hMemDC, hOldBitmap);
			DeleteDC(hMemDC);
		}
		else   // It has no color table, so use a halftone palette
		{
			HDC    hRefDC;

			hRefDC = GetDC(nullptr);
			*phPalette = CreateHalftonePalette(hRefDC);
			ReleaseDC(nullptr, hRefDC);
		}
	}

	return TRUE;
}

bool selectSkin(char *fname)
{
	char cwd[256], skindir[262];
	HBITMAP hbmp;

	// save off the current dir
	_getcwd(cwd, 256);

	// change to the "Skins" subdirectory
	sprintf(skindir, "%sSkins", exePath);
	chdir(skindir);

	// load the bitmap
	if (LoadBitmapFromBMPFile(fname, &hbmp, nullptr) == FALSE)
		return FALSE;

	// return to the previous dir
	chdir(cwd);

	// nuke any current skin
	if (globals.hSkin != nullptr)
		DeleteObject(globals.hSkin);

	// store the handle to the new skin
	globals.hSkin = hbmp;

	// save the name too
	strcpy(config.lastSkin, fname);

	return TRUE;
}

void applySkin(void)
{
	int count;

	// pass the hbitmap down to our skinnable objects...
	globals.pMenu->setBackgroundImage(globals.hSkin);

	// get the count of bands in our rebar (should be 3, but what the heck...)
	count = SendMessage(globals.hRebar, RB_GETBANDCOUNT, (WPARAM)0, (LPARAM)0);

	// remove each band
	while (count--)
		SendMessage(globals.hRebar, RB_DELETEBAND, (WPARAM)0, (LPARAM)0);

	// re-insert the controls back into the rebar
	insertRebarControls(globals.MainhWnd);
}

LRESULT APIENTRY SplitterWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool isDown = FALSE;
	static signed short startX;
	int diffX, newSize;

	switch (message)
	{
		case WM_LBUTTONDOWN:
			if (wParam & MK_LBUTTON)
			{
				isDown = TRUE;
				startX = (signed short)LOWORD(lParam);

				SetCapture(hWnd);
			}
			break;

		case WM_LBUTTONUP:
			isDown = FALSE;
			ReleaseCapture();
			break;

		case WM_MOUSEMOVE:
			if (isDown)
			{
				diffX = ((signed short)LOWORD(lParam)) - startX;
				newSize = config.treeViewWidth + diffX;

				if (newSize < 20)
					newSize = 20;
				else if (newSize > (config.rootXSize - 20))
					newSize = config.rootXSize - 20;

				resizeTreeView(newSize);
			}
			break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT APIENTRY PopupWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC dc;
	int res;

	switch (message)
	{
		case WM_PAINT:
			dc  = BeginPaint(hWnd, &ps);
			res = globals.pMenu->onPaint(hWnd, dc);
			EndPaint(hWnd, &ps);
			return res;

		case WM_MOUSEMOVE:
			return globals.pMenu->onMouseMove(hWnd, LOWORD(lParam), HIWORD(lParam));

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
			return globals.pMenu->onLButtonDown(hWnd, LOWORD(lParam), HIWORD(lParam));

		case WM_TIMER:
			return globals.pMenu->onTimer(hWnd, wParam);
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void updateMRU(void)
{
	int t, x;

	// first, see if the currently opened bios is in the mru list
	for (t = 0; t < 4; t++)
	{
		if (!stricmp(biosGetFilename(), config.recentFile[t]))
		{
			// it is, so move the remaining items up the list
			for (x = t; x < 3; x++)
				strcpy(config.recentFile[x], config.recentFile[x + 1]);
		}
	}

	// now, move all the items down one
	for (t = 3; t >= 1; t--)
		strcpy(config.recentFile[t], config.recentFile[t - 1]);

	// copy the current one into the top
	strcpy(config.recentFile[0], biosGetFilename());
}

LRESULT APIENTRY WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	UINT uItem;
	char statusBuf[256];
	SIZE sz;

	switch (message)
	{
		case WM_ACTIVATEAPP:
			if (wParam == FALSE)
			{
				// we're being deactivated.  close any popup window which might be open
				if (globals.pMenu != nullptr)
					globals.pMenu->closePopup();
			}
			break;

		case WM_CREATE:
			// create our status bar and controls (this must be here, or things won't size right!)
			createStatusBar(hWnd);
			createControls(hWnd);
			break;

		case WM_SIZE:
			sz.cx = LOWORD(lParam); // + 8;
			sz.cy = HIWORD(lParam); // + 29;

			if (globals.hRebar != nullptr)
				SendMessage(globals.hRebar, message, wParam, lParam);

			if (globals.hStatusBar != nullptr)
			{
				SendMessage(globals.hStatusBar, message, wParam, lParam);
				resizeStatusBar(hWnd);
			}

			resizeControlsFromRoot(sz);

			config.rootXSize = sz.cx + 8;
			config.rootYSize = sz.cy + 29;
			break;

		case WM_PAINT:
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_QUIT:
			break;

		case WM_CLOSE:
			if (biosHandleModified(mainChangedText) == FALSE)
				return 0;
			break;

		case WM_NOTIFY:
			return DoNotify(hWnd, wParam, lParam);

		case WM_INITMENUPOPUP:
			if (handleMenuPopup((HMENU)wParam) == 0)
				return 0;
			break;

		case WM_MENUSELECT:
			// update the text in the status bar
			uItem = (UINT)LOWORD(wParam);

			if ((uItem > 1) && (uItem != 65535))
			{
				LoadString(globals.MainhInst, uItem, statusBuf, 255);
				SendMessage(globals.hStatusBar, SB_SETTEXT, 0, (LPARAM)statusBuf);
			}
			else
			{
				SendMessage(globals.hStatusBar, SB_SETTEXT, 0, (LPARAM)"");
			}
			return 0;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case ID_FILE_MRU_FILE1:
				case ID_FILE_MRU_FILE2:
				case ID_FILE_MRU_FILE3:
				case ID_FILE_MRU_FILE4:
					if (biosOpenFile(config.recentFile[LOWORD(wParam) - ID_FILE_MRU_FILE1]) == TRUE)
					{
						// update the mru list
						updateMRU();

						// show the splitter window
						if (globals.hSplitter != nullptr)
							ShowWindow(globals.hSplitter, SW_SHOW);
					}
					return 0;

				case ID_ACTION_OPEN:
					if (biosOpen() == TRUE)
					{
						// update the mru list
						updateMRU();

						// show the splitter window
						if (globals.hSplitter != nullptr)
							ShowWindow(globals.hSplitter, SW_SHOW);
					}
					return 0;

				case ID_ACTION_SAVE:
					biosSave();
					return 0;

				case ID_ACTION_SAVE_AS:
					biosSaveAs();
					return 0;

				case ID_FILE_REVERT:
					if (MessageBox(globals.MainhWnd, "Lose changes and revert to previously saved BIOS?", "Notice", MB_YESNO) == IDYES)
						biosRevert();
					return 0;

				case ID_FILE_PROPERTIES:
					biosProperties();
					return 0;

				case ID_FILE_EXIT:
					if (biosHandleModified(mainChangedText) == TRUE)
						PostQuitMessage(0);
					return 0;

				case ID_ACTION_INSERT:
					biosInsert(0);
					return 0;

				case ID_ACTION_REPLACE:
					biosReplace();
					return 0;

				case ID_ACTION_EXTRACT:
					biosExtract();
					return 0;

				case ID_ACTION_EXTRACT_ALL:
					biosExtractAll();
					return 0;

				case ID_ACTION_REMOVE:
					biosRemove();
					break;

				case ID_ACTION_HEXEDIT:
					if (config.hexEditor[0] != 0)
						biosHexEdit();
					else
						MessageBox(globals.MainhWnd, "No external hex editor is currently configured.  You can set one up by choosing Options->Configuration from the menu.", "Notice", MB_OK);
					break;

				case ID_OPTION_CONFIG:
					DialogBox(globals.MainhInst, MAKEINTRESOURCE(IDD_CONFIG), globals.MainhWnd, ConfigBoxProc);
					return 0;

				case ID_SELECT_SKIN0:
				case ID_SELECT_SKIN1:
				case ID_SELECT_SKIN2:
				case ID_SELECT_SKIN3:
				case ID_SELECT_SKIN4:
				case ID_SELECT_SKIN5:
				case ID_SELECT_SKIN6:
				case ID_SELECT_SKIN7:
				case ID_SELECT_SKIN8:
				case ID_SELECT_SKIN9:
				case ID_SELECT_SKIN10:
				case ID_SELECT_SKIN11:
				case ID_SELECT_SKIN12:
				case ID_SELECT_SKIN13:
				case ID_SELECT_SKIN14:
				case ID_SELECT_SKIN15:
				case ID_SELECT_SKIN16:
				case ID_SELECT_SKIN17:
				case ID_SELECT_SKIN18:
				case ID_SELECT_SKIN19:
					if (selectSkin(skinList[LOWORD(wParam) - ID_SELECT_SKIN0].fname) == FALSE)
					{
						MessageBox(hWnd, "Error loading skin", "Error", MB_OK);
					}
					else
					{
						applySkin();
					}
					return 0;

				case ID_HELP_ABOUT:
					DialogBox(globals.MainhInst, MAKEINTRESOURCE(IDD_ABOUT), globals.MainhWnd, AboutBoxProc);
					return 0;

				case ID_HELP_PLUGIN0:
				case ID_HELP_PLUGIN1:
				case ID_HELP_PLUGIN2:
				case ID_HELP_PLUGIN3:
				case ID_HELP_PLUGIN4:
				case ID_HELP_PLUGIN5:
				case ID_HELP_PLUGIN6:
				case ID_HELP_PLUGIN7:
				case ID_HELP_PLUGIN8:
				case ID_HELP_PLUGIN9:
				case ID_HELP_PLUGIN10:
				case ID_HELP_PLUGIN11:
				case ID_HELP_PLUGIN12:
				case ID_HELP_PLUGIN13:
				case ID_HELP_PLUGIN14:
				case ID_HELP_PLUGIN15:
				case ID_HELP_PLUGIN16:
				case ID_HELP_PLUGIN17:
				case ID_HELP_PLUGIN18:
				case ID_HELP_PLUGIN19:
					pluginShowAboutBox(LOWORD(wParam) - ID_HELP_PLUGIN0, globals.MainhWnd);
					break;

				case ID_HELP_HOMEPAGE:
					ShellExecute(globals.MainhWnd, "open", "http://awdbedit.sourceforge.net", nullptr, nullptr, SW_SHOWNORMAL);
					break;
			}
			break;

		case WM_LBUTTONDOWN:
			return globals.pMenu->onLButtonDown(hWnd, LOWORD(lParam), HIWORD(lParam));
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

HWND CreateMainWindow(uint xsize, uint ysize, HINSTANCE hInstance, int nCmdShow, char *name)
{
	HWND hwnd;
	WNDCLASSEX wc;
	HBRUSH hSplitterBrush;

	// Create a popup window class...
	wc.cbSize			= sizeof(WNDCLASSEX);
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
	wc.lpfnWndProc		= PopupWindowProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= hInstance;
	wc.hIcon			= nullptr;
	wc.hCursor			= LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName		= nullptr;
	wc.lpszClassName	= AWDBEDIT_POPUP_CLASSNAME;
	wc.hIconSm			= nullptr;

	if (!RegisterClassEx(&wc)) return FALSE;

	// create the splitter window's brush
	hSplitterBrush = CreateSolidBrush(RGB(224, 223, 227));

	// Create a splitter window class...
	wc.cbSize			= sizeof(WNDCLASSEX);
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
	wc.lpfnWndProc		= SplitterWindowProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= hInstance;
	wc.hIcon			= nullptr;
	wc.hCursor			= LoadCursor(nullptr, IDC_SIZEWE);
	wc.hbrBackground	= hSplitterBrush;
	wc.lpszMenuName		= nullptr;
	wc.lpszClassName	= AWDBEDIT_SPLITTER_CLASSNAME;
	wc.hIconSm			= nullptr;

	if (!RegisterClassEx(&wc)) return FALSE;

	// Finally, create the window class and the window...
	wc.cbSize			= sizeof(WNDCLASSEX);
	wc.style			= CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc		= WindowProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= hInstance;
	wc.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
	wc.hCursor			= LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground	= (HBRUSH)GetStockObject(GRAY_BRUSH);
	wc.lpszMenuName		= nullptr;
	wc.lpszClassName	= AWDBEDIT_MAIN_CLASSNAME;
	wc.hIconSm			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
	if (!RegisterClassEx(&wc)) return FALSE;

	hwnd = CreateWindowEx(0, AWDBEDIT_MAIN_CLASSNAME, name, WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_THICKFRAME, CW_USEDEFAULT, 
		CW_USEDEFAULT, xsize, ysize, nullptr, nullptr, hInstance, nullptr);

	if (hwnd == nullptr)
		return 0;

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);
	SetFocus(hwnd);

	return hwnd;
}

void mapMenuIcon(int cmdId, int iconId)
{
	HICON htmp;
	ICONINFO ii;

	htmp = LoadIcon(globals.MainhInst, MAKEINTRESOURCE(iconId));
	GetIconInfo(htmp, &ii);

	SetMenuItemBitmaps(globals.MainhMenu, cmdId, MF_BYCOMMAND, ii.hbmMask, ii.hbmColor);
}

bool getExePath(char *cmdline, char *path, int len)
{
	char *ptr;

	// check if path has enough space
	if ((int)strlen(cmdline) >= len)
		return FALSE;

	// check for a quote (") and copy the command line into path
	if (cmdline[0] == '\"')
		strcpy(path, cmdline + 1);
	else
		strcpy(path, cmdline);

	// backtrace path until we hit a backslash (\)
	ptr = path + strlen(path);
	while ((ptr >= path) && (*ptr != '\\'))
		ptr--;

	// if we found a backslash, terminate the string following it
	if (*ptr == '\\')
		*(ptr + 1) = 0;

	return TRUE;
}

void cleanTempPath(void)
{
	char cwd[295];
	#if !defined(DIRENT_IS_OK_FOR_OUR_PURPOSES)
	struct _finddata_t fd;
	long hFile;
	#else
	DIR *hFile;
	struct dirent *fd;
	#endif

	// save the current path
	getcwd(cwd, 256);

	// change into the temp dir
	if (chdir(fullTempPath) < 0)
	{
		// some error occured changing into our temp dir.  we don't want to destroy any files here!
		sprintf(cwd, "Unable to clean temporary files dir: [%s]", fullTempPath);
		MessageBox(globals.MainhWnd, cwd, "Internal Error", MB_OK);
		return;
	}

	// iterate through all files, and delete them
	#if !defined(DIRENT_IS_OK_FOR_OUR_PURPOSES)
	hFile = _findfirst("*.*", &fd);
	if (hFile != -1)
	#else
	hFile = opendir(".");
	fd = readdir(hFile);
	if (fd)
	#endif
	{
		do
		{
			// ignore directories
			#if !defined(DIRENT_IS_OK_FOR_OUR_PURPOSES)
			if (fd.attrib == _A_SUBDIR){
			#else
			if (fd->d_type == DT_DIR){
			#endif
				unlink(
					#if !defined(DIRENT_IS_OK_FOR_OUR_PURPOSES)
					fd.name
					#else
					fd->d_name
					#endif
				);
			}
		#if !defined(DIRENT_IS_OK_FOR_OUR_PURPOSES)
		} while (_findnext(hFile, &fd) == 0);

		_findclose(hFile);
		#else
		} while ((fd = readdir(hFile)));

		closedir(hFile);
		#endif
	}

	// return back
	chdir(cwd);
}

void makeTempPath(void)
{
	char *tmp;

	// try to get the system temp path
	tmp = getenv("Temp");
	if (tmp == nullptr)
		tmp = "C:\\TEMP";

	// make sure it exists (it really should...)
	#if !defined(HAVE_IO_H)
	mkdir(tmp, 0664);
	#else
	mkdir(tmp);
	#endif

	// split off a directory for us
	sprintf(fullTempPath, "%s\\Award BIOS Editor Temp Files", tmp);

	// make sure this exists too
	#if !defined(HAVE_IO_H)
	mkdir(fullTempPath, 0664);
	#else
	mkdir(tmp);
	#endif

	// now cleanup the temp path...
	cleanTempPath();
}

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR cmdline, int nCmdShow)
{
	MSG msg;
	bool done;
	INITCOMMONCONTROLSEX icex;

	// Begin basic initialization...
	getExePath(GetCommandLine(), exePath, 256);
	globals.MainhWnd  = nullptr;
	globals.MainhInst = hInstance;

	icex.dwSize	= sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC	= ICC_WIN95_CLASSES | ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES;
	if (InitCommonControlsEx(&icex) == FALSE)
	{
		MessageBox(nullptr, "Unable to init common controls!", "Error", MB_OK);
		return 0;
	}

	// make temporary path and cleanup any leftover files
	makeTempPath();

	// load configuration from registry
	configInit(APP_NAME, APP_REV);
	configLoad();

	// init plugin system
	pluginInit();

	// init LZH engine
	lzhInit();

	// apply configured skin
	if (selectSkin(config.lastSkin) == FALSE)
	{
		globals.hSkin = LoadBitmap(globals.MainhInst, MAKEINTRESOURCE(IDB_BACKGROUND));
		config.lastSkin[0] = 0;
	}

	// load the menu data first.
	globals.MainhMenu = LoadMenu(globals.MainhInst, MAKEINTRESOURCE(IDR_MENU));

	// next, create the main window...
	globals.MainhWnd = CreateMainWindow(config.rootXSize, config.rootYSize, globals.MainhInst, nCmdShow, const_cast<char*>(APP_VERSION));
	if (globals.MainhWnd == nullptr)
	{
		MessageBox(nullptr, "Unable to create main window!", "Error", MB_OK);
		return 0;
	}

	// create the popup menu class
	globals.pMenu = new popupMenu(globals.MainhInst, globals.MainhWnd, globals.hMenuBar, globals.MainhMenu);
	globals.pMenu->setBounds(ID_MENU_FILE, ID_MENU_HELP);
	globals.pMenu->setBackgroundImage(globals.hSkin);
	globals.pMenu->setBackgroundColor(RGB(255, 255, 255));
	globals.pMenu->setTextColor(RGB(0, 0, 0));
	globals.pMenu->setInactiveColor(RGB(128, 128, 128));

	// setup icons
	mapMenuIcon(ID_ACTION_OPEN,			IDI_FILE_OPEN);
	mapMenuIcon(ID_ACTION_SAVE,			IDI_FILE_SAVE);
	mapMenuIcon(ID_FILE_PROPERTIES,		IDI_FILE_PROPERTIES);
	mapMenuIcon(ID_FILE_EXIT,			IDI_FILE_EXIT);
	mapMenuIcon(ID_ACTION_INSERT,		IDI_ACTION_INSERT);
	mapMenuIcon(ID_ACTION_REPLACE,		IDI_ACTION_REPLACE);
	mapMenuIcon(ID_ACTION_EXTRACT,		IDI_ACTION_EXTRACT);
	mapMenuIcon(ID_ACTION_EXTRACT_ALL,	IDI_ACTION_EXTRACT_ALL);
	mapMenuIcon(ID_ACTION_REMOVE,		IDI_ACTION_REMOVE);
	mapMenuIcon(ID_ACTION_HEXEDIT,		IDI_ACTION_HEXEDIT);
	mapMenuIcon(ID_OPTION_CONFIG,		IDI_OPTION_CONFIG);
	mapMenuIcon(ID_HELP_ABOUT,			IDI_HELP_ABOUT);

	// get pointers to our menus which we change dynamically
	globals.hRecentFilesMenu	= GetSubMenu(GetSubMenu(globals.MainhMenu, 0), 5);
	globals.hSkinsMenu			= GetSubMenu(GetSubMenu(globals.MainhMenu, 2), 1);
	globals.hAboutPluginsMenu	= GetSubMenu(GetSubMenu(globals.MainhMenu, 3), 1);

	// Invalidate the windows's rectangle to paint the window.
	InvalidateRect(globals.MainhWnd, nullptr, FALSE);
	UpdateWindow(globals.MainhWnd);

	// Initialize some text in our status bar
	SendMessage(globals.hStatusBar, SB_SETTEXT, 1, (LPARAM)"No BIOS loaded");

	// initialize bios system
	biosInit(globals.MainhInst, globals.MainhWnd, globals.hStatusBar, globals.hTreeView, globals.hTreeRecgItem, globals.hTreeInclItem, globals.hTreeUnkItem, &globals.dialogrc);

	// Begin main message loop...
	done = FALSE;

	while (!done)
	{
		if (!GetMessage(&msg, nullptr, 0, 0))
		{
			done = TRUE;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// save config to registry
	configSave();

	// kill everything...
	biosFreeMemory();

	delete globals.pMenu;
	globals.pMenu = nullptr;

	DestroyWindow(globals.hToolBar);
	DestroyWindow(globals.hStatusBar);
	DestroyWindow(globals.hMenuBar);
	DestroyWindow(globals.hRebar);

	ImageList_Destroy(globals.hToolImageList);

	DestroyMenu(globals.MainhMenu);
	DestroyWindow(globals.MainhWnd);

	// unload plugins
	pluginShutdown();

	// remove any temporary files
	cleanTempPath();

	return 0;
}
