//
// Award BIOS Editor - sysbios_drive.cpp
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: sysbios_drive.cpp,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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
#include <assert.h>
#include <numeric>
#include "types.h"
#include "../awdbedit/awdbe_exports.h"
#include "builtins.h"
#include "sysbios.h"
#include "sysbios_drive.h"
#include "resource.h"

static int modifyDriveType;
static sysbiosDrvTblEntry *modifyDrivePtr;

template <typename NumT>
NumT aton(char * buf){
	char *end;
	auto res = strtoul(buf, &end, 10);
	assert(res < std::numeric_limits<NumT>::max());
	return static_cast<NumT>(res);
};

INT_PTR CALLBACK ModifyDriveFunc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char buf[256];

	switch (message)
	{
		case WM_INITDIALOG:
			sprintf(buf, "%d", modifyDriveType);
			SetDlgItemText(hdlg, IDC_MODIFY_TYPE, buf);

			SendMessage(GetDlgItem(hdlg, IDC_MODIFY_CYL), EM_SETLIMITTEXT, 5, 0);
			sprintf(buf, "%d", modifyDrivePtr->cylinders);
			SetDlgItemText(hdlg, IDC_MODIFY_CYL, buf);

			SendMessage(GetDlgItem(hdlg, IDC_MODIFY_HEADS), EM_SETLIMITTEXT, 3, 0);
			sprintf(buf, "%d", modifyDrivePtr->heads);
			SetDlgItemText(hdlg, IDC_MODIFY_HEADS, buf);

			SendMessage(GetDlgItem(hdlg, IDC_MODIFY_PRECOMP), EM_SETLIMITTEXT, 5, 0);
			sprintf(buf, "%d", modifyDrivePtr->precomp);
			SetDlgItemText(hdlg, IDC_MODIFY_PRECOMP, buf);

			SendMessage(GetDlgItem(hdlg, IDC_MODIFY_CONTROL), EM_SETLIMITTEXT, 5, 0);
			sprintf(buf, "%d", modifyDrivePtr->control);
			SetDlgItemText(hdlg, IDC_MODIFY_CONTROL, buf);

			SendMessage(GetDlgItem(hdlg, IDC_MODIFY_LZONE), EM_SETLIMITTEXT, 5, 0);
			sprintf(buf, "%d", modifyDrivePtr->landzone);
			SetDlgItemText(hdlg, IDC_MODIFY_LZONE, buf);

			SendMessage(GetDlgItem(hdlg, IDC_MODIFY_SECTORS), EM_SETLIMITTEXT, 3, 0);
			sprintf(buf, "%d", modifyDrivePtr->sectors);
			SetDlgItemText(hdlg, IDC_MODIFY_SECTORS, buf);
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					GetDlgItemText(hdlg, IDC_MODIFY_CYL, buf, 256);
					modifyDrivePtr->cylinders = aton<decltype(modifyDrivePtr->cylinders)>(buf);

					GetDlgItemText(hdlg, IDC_MODIFY_HEADS, buf, 256);
					modifyDrivePtr->heads = aton<decltype(modifyDrivePtr->heads)>(buf);

					GetDlgItemText(hdlg, IDC_MODIFY_PRECOMP, buf, 256);
					modifyDrivePtr->precomp = aton<decltype(modifyDrivePtr->precomp)>(buf);

					GetDlgItemText(hdlg, IDC_MODIFY_CONTROL, buf, 256);
					modifyDrivePtr->control = aton<decltype(modifyDrivePtr->control)>(buf);

					GetDlgItemText(hdlg, IDC_MODIFY_LZONE, buf, 256);
					modifyDrivePtr->landzone = aton<decltype(modifyDrivePtr->landzone)>(buf);

					GetDlgItemText(hdlg, IDC_MODIFY_SECTORS, buf, 256);
					modifyDrivePtr->sectors = aton<decltype(modifyDrivePtr->sectors)>(buf);

				case IDCANCEL:
					EndDialog(hdlg, TRUE);
					break;
			}
			break;
	}

	return FALSE;
}



INT_PTR CALLBACK sysbiosDriveTableFunc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	LPNMHDR	lpNM = (LPNMHDR)lParam;
	LPNMLISTVIEW lpnmlv;
	HWND hlist;
	int t, val;
	sysbiosDrvTblEntry *drvTblPtr, tempdrive;
	uint16_t *ptr16;

	LVCOLUMN colList[] = {
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 48, "Type",		0, 0, 0, 0 },
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 68, "Cylinders",	0, 1, 0, 0 },
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 54, "Heads",		0, 2, 0, 0 },
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 65, "Precomp",		0, 3, 0, 0 },
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 60, "Control",		0, 4, 0, 0 },
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 68, "Landzone",	0, 5, 0, 0 },
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 57, "Sectors",		0, 6, 0, 0 }
	};

	switch (message)
	{
		case WM_INITDIALOG:
			// get a handle to our list control
			hlist = GetDlgItem(hdlg, IDC_DRIVE_LIST);

			// set full-row select and grid lines
			SendMessage(hlist, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

			// add the columns to our drive table
			for (t = 0; t < 7; t++)
				SendMessage(hlist, LVM_INSERTCOLUMN, t, (LPARAM)&colList[t]);

			// set the number of drives in the table
			SendMessage(hlist, LVM_SETITEMCOUNT, 47, 0);
			return TRUE;

		case WM_NOTIFY:
			switch (lpNM->code)
			{
				case NM_RCLICK:
					lpnmlv = (LPNMLISTVIEW)lParam;
					
					// adjust point clicked with the offset of the list control within the dialog window
					lpnmlv->ptAction.x += 28;
					lpnmlv->ptAction.y += 44;

					// version 6.00PG BIOSes do not have a drive table
					if (sysbiosVersion != awdbeBIOSVer600PG)
						val = awdbeDoPopup(hinst, MAKEINTRESOURCE(IDR_DRIVE_TABLE), lpnmlv->ptAction.x, lpnmlv->ptAction.y);
					else
					{
						awdbeDoPopup(hinst, MAKEINTRESOURCE(IDR_NODRIVE_TABLE), lpnmlv->ptAction.x, lpnmlv->ptAction.y);
						val = 0;
					}

					if (val != 0)
					{
						// get pointer to pointer to the drive table
						ptr16 = (uint16_t *)(sysbiosBasePtr + 0x1E6F0);

						// make drive table pointer to this pointer
						drvTblPtr = (sysbiosDrvTblEntry *)((sysbiosBasePtr + 0x10000) + *ptr16);

						switch (val)
						{
							case ID_DRVTBL_MODIFY:
								// advance to the selected item
								for (t = 0; t < lpnmlv->iItem; t++)
									drvTblPtr++;

								// stupid global variables
								modifyDriveType = lpnmlv->iItem + 1;
								modifyDrivePtr  = &tempdrive;
								memcpy(modifyDrivePtr, drvTblPtr, sizeof(sysbiosDrvTblEntry));

								DialogBox(hinst, MAKEINTRESOURCE(IDD_MODIFY_DRIVE), hdlg, ModifyDriveFunc);

								if (memcmp(modifyDrivePtr, drvTblPtr, sizeof(sysbiosDrvTblEntry)))
								{
									memcpy(drvTblPtr, modifyDrivePtr, sizeof(sysbiosDrvTblEntry));
									awdbeSetModified(myID);
								}
								break;

							case ID_DRVTBL_CLEAR:
								// advance to the selected item
								for (t = 0; t < lpnmlv->iItem; t++)
									drvTblPtr++;

								// zap this entry
								drvTblPtr->cylinders = 0;
								drvTblPtr->heads     = 0;
								drvTblPtr->precomp   = 0;
								drvTblPtr->control   = 0;
								drvTblPtr->landzone  = 0;
								drvTblPtr->sectors   = 0;

								awdbeSetModified(myID);
								break;

							case ID_DRVTBL_CLEARALL:
								// zap all entries
								for (t = 0; t < 47; t++)
								{
									drvTblPtr->cylinders = 0;
									drvTblPtr->heads     = 0;
									drvTblPtr->precomp   = 0;
									drvTblPtr->control   = 0;
									drvTblPtr->landzone  = 0;
									drvTblPtr->sectors   = 0;

									drvTblPtr++;
								}

								awdbeSetModified(myID);
								break;
						}

						// recalculate the checksum and refresh
						sysbiosRecalcChecksum(FALSE);
						awdbeRefreshSelf(myID);
					}
					break;
			}
			break;
	}

	return FALSE;
}

void sysbiosRefreshDriveTable(uint8_t *ptr)
{
	HWND hlist;
	uint16_t *ptr16;
	sysbiosDrvTblEntry *drvTblPtr;
	LVITEM lvi;
	int t;
	char buf[256];

	// get handle to the drive list window
	hlist = GetDlgItem(sysbiosTabList[1].hwnd, IDC_DRIVE_LIST);

	// zap all entries currently in the drive table
	SendMessage(hlist, LVM_DELETEALLITEMS, 0, 0);

	// newer Award 6.00PG BIOS's have done away with the drive table... (but after they merged with Phoenix, their 6.0 [not PG] BIOS readded it.  make up your mind, guys...)
	if (sysbiosVersion != awdbeBIOSVer600PG)
	{
		// get pointer to pointer to the drive table
		ptr16 = (uint16_t *)(ptr + 0x1E6F0);

		// make drive table pointer to this pointer
		drvTblPtr = (sysbiosDrvTblEntry *)((ptr + 0x10000) + *ptr16);

		// add the drives
		lvi.mask  = LVIF_TEXT;
		lvi.iItem = 0;

		for (t = 0; t < 47; t++)
		{
			// type
			sprintf(buf, "%d", t + 1);
			lvi.iSubItem = 0;
			lvi.pszText  = buf;
			SendMessage(hlist, LVM_INSERTITEM, 0, (LPARAM)&lvi);

			// cyls
			sprintf(buf, "%d", drvTblPtr->cylinders);
			lvi.iSubItem = 1;
			lvi.pszText  = buf;
			SendMessage(hlist, LVM_SETITEM, 0, (LPARAM)&lvi);

			// heads
			sprintf(buf, "%d", drvTblPtr->heads);
			lvi.iSubItem = 2;
			lvi.pszText  = buf;
			SendMessage(hlist, LVM_SETITEM, 0, (LPARAM)&lvi);

			// precomp
			sprintf(buf, "%d", drvTblPtr->precomp);
			lvi.iSubItem = 3;
			lvi.pszText  = buf;
			SendMessage(hlist, LVM_SETITEM, 0, (LPARAM)&lvi);

			// control
			sprintf(buf, "%d", drvTblPtr->control);
			lvi.iSubItem = 4;
			lvi.pszText  = buf;
			SendMessage(hlist, LVM_SETITEM, 0, (LPARAM)&lvi);

			// landzone
			sprintf(buf, "%d", drvTblPtr->landzone);
			lvi.iSubItem = 5;
			lvi.pszText  = buf;
			SendMessage(hlist, LVM_SETITEM, 0, (LPARAM)&lvi);

			// sectors
			sprintf(buf, "%d", drvTblPtr->sectors);
			lvi.iSubItem = 6;
			lvi.pszText  = buf;
			SendMessage(hlist, LVM_SETITEM, 0, (LPARAM)&lvi);

			// advance to next drive
			lvi.iItem++;
			drvTblPtr++;
		}
	}
	else
	{
		// add a "no entries" line
		lvi.mask	 = LVIF_TEXT;
		lvi.iItem	 = 0;
		lvi.iSubItem = 0;
		lvi.pszText  = "No entries";
		SendMessage(hlist, LVM_INSERTITEM, 0, (LPARAM)&lvi);
	}
}
