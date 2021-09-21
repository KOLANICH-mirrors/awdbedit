//
// Award BIOS Editor - sysbios.cpp
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: sysbios.cpp,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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
#include "sysbios_main.h"
#include "sysbios_drive.h"
#include "sysbios_chipset.h"
#include "sysbios_timing.h"
#include "sysbios_options.h"
#include "sysbios_menu.h"
#include "sysbios_irqrouting.h"
#include "resource.h"


uchar *sysbiosBasePtr = NULL;
awdbeBIOSVersion sysbiosVersion = awdbeBIOSVerUnknown;
awdbeBIOSVersion sysbiosNowVer  = awdbeBIOSVerUnknown;


sysbiosTabEntry sysbiosTabList[] = {
	{ 0, "BIOS ID / Versions",	IDD_SYSBIOS_MAIN,				sysbiosMainFunc,				NULL },
	{ 1, "Drive Table",			IDD_SYSBIOS_DRIVE_TABLE,		sysbiosDriveTableFunc,			NULL },
	{ 2, "Chipset Registers",	IDD_SYSBIOS_CHIPSET_REGS,		sysbiosChipsetRegsFunc,			NULL },
	{ 3, "BIOS Timings",		IDD_SYSBIOS_TIMINGS,			sysbiosBIOSTimingFunc,			NULL },
	{ 4, "BIOS Options",		IDD_SYSBIOS_BIOS_OPTIONS,		sysbiosBIOSOptionsFunc,			NULL },
	{ 5, "Setup Menu",			IDD_SYSBIOS_CONFIG_MENU,		sysbiosConfigMenuFunc,			NULL },
	{ 6, "IRQ Routing",			IDD_SYSBIOS_IRQ_ROUTING,		sysbiosIRQRoutingFunc,			NULL }
};



static bool ignoreUpdate = FALSE;

int  sysbiosFindLimit(uchar *ptr);
void sysbiosUpdateLimit(HWND hdlg, int id, int curlen, int maxlen);


uchar sysbiosCalcBiosChecksum(uchar *ptr, ulong start, ulong end, int method)
{
	uchar csum = 0x00;
	int count;

	ptr  += start;
	count = (end - start) + 1;

	if (method == METHOD_ADD)
	{
		while (count--)
			csum += *ptr++;
	}
	else if (method == METHOD_SUB)
	{
		while (count--)
			csum -= *ptr++;
	}

	return csum;
}

void sysbiosRecalcChecksum(bool showErr)
{
	ulong from, to;
	uchar csum, *sptr;
	char buf[256];

	// can't do anything without a window...
	if (sysbiosTabList[0].hwnd == NULL)
		return;

	// and unknown versions are bad.
	if (sysbiosVersion == awdbeBIOSVerUnknown)
		return;

	if (sysbiosVersion != awdbeBIOSVer600PG)
	{
		GetDlgItemText(sysbiosTabList[0].hwnd, IDC_CHECKSUM_RANGE_START, buf, 256);
		sscanf(buf, "%x", &from);

		GetDlgItemText(sysbiosTabList[0].hwnd, IDC_CHECKSUM_RANGE_END, buf, 256);
		sscanf(buf, "%x", &to);

		if (IsDlgButtonChecked(sysbiosTabList[0].hwnd, IDC_CHECKSUM_METHOD_ADD) == BST_CHECKED)
			csum = sysbiosCalcBiosChecksum(sysbiosBasePtr, from, to, METHOD_ADD);
		else
			csum = sysbiosCalcBiosChecksum(sysbiosBasePtr, from, to, METHOD_SUB);

		sptr = sysbiosBasePtr + 0x1FFFF;
		if (*sptr != csum)
		{
			// update checksum with valid one
			*sptr = csum;
		
			sprintf(buf, "%02X", csum);
			SetDlgItemText(sysbiosTabList[0].hwnd, IDC_CHECKSUM_VALUE, buf);
			SetDlgItemText(sysbiosTabList[0].hwnd, IDC_CHECKSUM_RESULT, "Pass");

			// set modified flag
			awdbeSetModified(myID);
		}

		sprintf(buf, "%05X", from);
		SetDlgItemText(sysbiosTabList[0].hwnd, IDC_CHECKSUM_RANGE_START, buf);

		sprintf(buf, "%05X", to);
		SetDlgItemText(sysbiosTabList[0].hwnd, IDC_CHECKSUM_RANGE_END, buf);
	}
	else
	{
		if (showErr == TRUE)
			MessageBox(sysbiosTabList[0].hwnd, "Version 6.00PG of Award BIOSes do not have an internal checksum.", "Error", MB_OK);
	}
}

BOOL CALLBACK sysbiosTabCtrlFunc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	LPNMHDR	lpNM = (LPNMHDR)lParam;
	int curtab;
	RECT rc;
//--- START BETA CODE ---
	static bool setupMsgShown = FALSE;
//--- END BETA CODE ---

	switch (message)
	{
		case WM_NOTIFY:
			switch (lpNM->code)
			{
				case TCN_SELCHANGE:
					// refresh our contents
					awdbeRefreshSelf(myID);

					// find out which tab got selected
					curtab = SendMessage(GetDlgItem(hdlg, IDC_SYSBIOS_TABCTRL), TCM_GETCURSEL, 0, 0);

					// get *our* rectangle and take off some space for borders
					GetClientRect(hdlg, &rc);
					rc.left   += 16;
					rc.top    += 34;
					rc.right  -= 34;
					rc.bottom -= 48;

					// move the appropriate window into position and show it
					SetWindowPos(sysbiosTabList[curtab].hwnd, HWND_TOP, rc.left, rc.top, rc.right, rc.bottom, SWP_SHOWWINDOW);

//--- START BETA CODE ---
#if 0
					// show message if pane is setup pane
					if (curtab == 5)
					{
						if (setupMsgShown == FALSE)
						{
							MessageBox(sysbiosTabList[curtab].hwnd, "This tab is still under development.  The items in the BIOS Setup Menu are shown properly\n"
																"in the tree, but any changes made in any of the controls will not be applied to the BIOS image.\n"
																"You can also test-run the setup menu in a console window by clicking a menu header, then\n"
																"clicking the \"Run >\" button.\n\nThis message will only appear once per execution of the Editor.", "Warning", MB_OK);

							setupMsgShown = TRUE;
						}
					}
#endif
//--- END BETA CODE ---
					break;

				case TCN_SELCHANGING:
					// update ourselves, if not ignoring
					if (ignoreUpdate == FALSE)
						awdbeUpdateSelf(myID);

					// find out which tab we're about to destroy
					curtab = SendMessage(GetDlgItem(hdlg, IDC_SYSBIOS_TABCTRL), TCM_GETCURSEL, 0, 0);

					// hide this window
					SetWindowPos(sysbiosTabList[curtab].hwnd, 0, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE);
					break;
			}
			break;
	}

	return FALSE;
}

HWND sysbiosCreateDialog(HWND parentWnd)
{
	HWND hwnd, tabwnd;
	TCITEM tc;
	int t;
	SIZE sz;

	// create our dialog only containing the tab control
	hwnd   = CreateDialog(hinst, MAKEINTRESOURCE(IDD_SYSBIOS), parentWnd, sysbiosTabCtrlFunc);
	tabwnd = GetDlgItem(hwnd, IDC_SYSBIOS_TABCTRL);

	for (t = 0; t < 7; t++)
	{
		// add the items to it
		tc.mask    = TCIF_TEXT;
		tc.pszText = sysbiosTabList[t].name;
		SendMessage(tabwnd, TCM_INSERTITEM, sysbiosTabList[t].id, (LPARAM)&tc);

		// create this dialog box, too
		sysbiosTabList[t].hwnd = CreateDialog(hinst, MAKEINTRESOURCE(sysbiosTabList[t].dialogID), hwnd, sysbiosTabList[t].dialogProc);
	}

	// adjust size
	awdbeGetDialogSize(myID, &sz);

	if (sz.cx < 588)
		sz.cx = 588;

	if (sz.cy < 439)
		sz.cy = 439;

	awdbeResizeDialog(myID, sz);

	// select the first item in the tab
	ignoreUpdate = TRUE;
	SendMessage(tabwnd, TCM_SETCURFOCUS, 1, 0);
	SendMessage(tabwnd, TCM_SETCURFOCUS, 0, 0);
	ignoreUpdate = FALSE;

	return hwnd;
}

void sysbiosOnResizeDialog(HWND dialogWnd, RECT *rc)
{
	HWND tabwnd;
	RECT rc2;
	int curtab;

	// resize the tab dialog within this window
	tabwnd = GetDlgItem(dialogWnd, IDC_SYSBIOS_TABCTRL);

	rc2.top = 10;
	rc2.left = 10;
	rc2.right = (rc->right - rc->left) - 20;
	rc2.bottom = (rc->bottom - rc->top) - 20;
	SetWindowPos(tabwnd, NULL, rc2.left, rc2.top, rc2.right, rc2.bottom, SWP_NOZORDER);

	// now resize the sub-dialog within our tab
	curtab = SendMessage(GetDlgItem(dialogWnd, IDC_SYSBIOS_TABCTRL), TCM_GETCURSEL, 0, 0);

	// get *our* rectangle and take off some space for borders
	rc2.top    = 34;
	rc2.left   = 16;
	rc2.right  = (rc->right - rc->left) - 34;
	rc2.bottom = (rc->bottom - rc->top) - 48;

	// move the appropriate window into position and show it
	SetWindowPos(sysbiosTabList[curtab].hwnd, HWND_TOP, rc2.left, rc2.top, rc2.right, rc2.bottom, SWP_SHOWWINDOW);
}

void sysbiosOnDestroyDialog(HWND dialogWnd)
{
	int t;

	// notify the menu of destruction
	sysbiosDestroyMenu();

	// destroy all of our tabbed dialogs
	for (t = 0; t < 7; t++)
	{
		if (sysbiosTabList[t].hwnd != NULL)
		{
			DestroyWindow(sysbiosTabList[t].hwnd);
			sysbiosTabList[t].hwnd = NULL;
		}
	}

	// destroy the root tab window
	if (dialogWnd != NULL)
		DestroyWindow(dialogWnd);
}


int sysbiosFindLimit(uchar *base)
{
	uchar *ptr = base;
	int len;

	// first, skip all bytes that are not NULLs
	while (*ptr != 0)
		ptr++;

	// now advance past all NULLs until we hit a byte that is not NULL
	while (*ptr == 0)
		ptr++;

	// backup 1 byte (allowing for a NULL terminator)
	ptr--;

	// this is the maximum length allowed
	len = (ptr - base);
	if (len > 255)
		len = 255;

	return len;
}

void sysbiosUpdateLimit(HWND hdlg, int id, int curlen, int maxlen)
{
	char buf[32];

	sprintf(buf, "(%d/%d)", curlen, maxlen);

	switch (id)
	{
		case IDC_SYSBIOS_VERSION:   SetDlgItemText(hdlg, IDC_TEXT_SYSBIOS_VERSION, buf); break;
		case IDC_SYSBIOS_INFO:		SetDlgItemText(hdlg, IDC_TEXT_SYSBIOS_INFO,    buf); break;
		case IDC_SYSBIOS_MESSAGE:	SetDlgItemText(hdlg, IDC_TEXT_SYSBIOS_MESSAGE, buf); break;
		case IDC_SYSBIOS_ID:		SetDlgItemText(hdlg, IDC_TEXT_SYSBIOS_ID,      buf); break;
	}
}

void sysbiosOnLoad(fileEntry *fe)
{
	// zap some pointers/vars, free memory
	sysbiosBasePtr = NULL;
	sysbiosVersion = awdbeBIOSVerUnknown;

	sysbiosReleaseMenuItems();
}

void sysbiosRefreshDialog(HWND hwnd, fileEntry *fe)
{
	uchar *ptr = (uchar *)fe->data;
	char buf[256];

	// get the bios's version
	sysbiosVersion = awdbeGetBIOSVersion(myID);

	if (sysbiosVersion == awdbeBIOSVerUnknown)
	{
		MessageBox(NULL, "Unable to determine the System BIOS's version!", "Error", MB_OK);
		return;
	}

	// store base pointer
	sysbiosBasePtr = (uchar *)fe->data;

	// check to make sure we have a valid window handle (only the first is sufficient)
	if (sysbiosTabList[0].hwnd == NULL)
		return;

	// refresh file info
	SetDlgItemText(sysbiosTabList[0].hwnd, IDC_FILENAME, fe->name);

	sprintf(buf, "%04X", fe->type);
	SetDlgItemText(sysbiosTabList[0].hwnd, IDC_FILE_ID, buf);

	sprintf(buf, "%d", fe->size);
	SetDlgItemText(sysbiosTabList[0].hwnd, IDC_FILE_SIZE, buf);

	sprintf(buf, "%08X", fe->offset);
	SetDlgItemText(sysbiosTabList[0].hwnd, IDC_FILE_OFFSET, buf);

	// refresh main panel
	sysbiosRefreshMain(ptr);

	// refresh drive table
	sysbiosRefreshDriveTable(ptr);

	// refresh chipset regs
	sysbiosRefreshChipsetRegs(ptr);

	// refresh bios timing
	sysbiosRefreshBIOSTiming(ptr);

	// refresh bios options
	sysbiosRefreshBIOSOptions(ptr);

	// refresh menu
	sysbiosRefreshMenu(ptr);

	// refresh pci/irq routing
	sysbiosRefreshIRQRouting(ptr);
}

bool sysbiosUpdateDialog(HWND hwnd, fileEntry *fe)
{
	uchar *ptr = (uchar *)fe->data;
	bool modified = FALSE;
	awdbeBIOSVersion vers;

	// check to make sure we have a valid window handle (only the first is sufficient)
	if (sysbiosTabList[0].hwnd == NULL)
		return FALSE;

	// store the current version
	vers = sysbiosVersion;

	// update main panel
	if (sysbiosUpdateMain(ptr, &modified, &vers) == FALSE)
		return FALSE;

	// update bios timing
	sysbiosUpdateBIOSTiming(ptr, &modified, vers);

	// update bios options
	sysbiosUpdateBIOSOptions(ptr, &modified, vers);

	// done!  checksum, please...
	sysbiosRecalcChecksum(FALSE);

	return modified;
}

awdbeBIOSVersion sysbiosGetVersion(uchar *sptr, int len)
{
	awdbeBIOSVersion vers = awdbeBIOSVerUnknown;

	while (len--)
	{
		if (!memicmp(sptr, "v4.51PG", 7))
		{
			vers = awdbeBIOSVer451PG;
			len  = 0;
		}
		else if (!memicmp(sptr, "v6.00PG", 7))
		{
			vers = awdbeBIOSVer600PG;
			len  = 0;
		}
		else if (!memicmp(sptr, "v6.0", 4))
		{
			vers = awdbeBIOSVer60;
			len  = 0;
		}
		else
		{
			sptr++;
		}
	}

	return vers;
}
