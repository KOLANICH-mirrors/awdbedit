//
// Award BIOS Editor - sysbios_chipset.cpp
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: sysbios_chipset.cpp,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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
#include "sysbios_chipset.h"
#include "resource.h"

static sysbiosChipRegEntry *modifyChipsetPtr;

void sysbiosChipsetBuildMap(char *buf, uint8_t mask, uint8_t value)
{
	char *ptr = buf;
	int t;

	t = 8;
	while (t--)
	{
		if (mask & 0x80)
		{
			if (value & 0x80)
				*ptr++ = '1';
			else
				*ptr++ = '0';
		}
		else
		{
			*ptr++ = 'X';
		}

		mask  <<= 1;
		value <<= 1;

		if (t == 4)
			*ptr++ = ' ';
	}

	*ptr = 0;
}

INT_PTR CALLBACK ModifyChipsetFunc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char buf[256];
	HWND hwnd;
	uint32_t mask, val;

	switch (message)
	{
		case WM_INITDIALOG:
			SendMessage(GetDlgItem(hdlg, IDC_MODIFY_INDEX), EM_SETLIMITTEXT, 2, 0);
			sprintf(buf, "%02X", modifyChipsetPtr->index);
			SetDlgItemText(hdlg, IDC_MODIFY_INDEX, buf);

			SendMessage(GetDlgItem(hdlg, IDC_MODIFY_DEVICE), EM_SETLIMITTEXT, 2, 0);
			sprintf(buf, "%02X", modifyChipsetPtr->device);
			SetDlgItemText(hdlg, IDC_MODIFY_DEVICE, buf);

			SendMessage(GetDlgItem(hdlg, IDC_MODIFY_FUNC), EM_SETLIMITTEXT, 1, 0);
			sprintf(buf, "%01X", modifyChipsetPtr->function);
			SetDlgItemText(hdlg, IDC_MODIFY_FUNC, buf);

			SendMessage(GetDlgItem(hdlg, IDC_MODIFY_MASK), EM_SETLIMITTEXT, 2, 0);
			sprintf(buf, "%02X", modifyChipsetPtr->mask);
			SetDlgItemText(hdlg, IDC_MODIFY_MASK, buf);

			SendMessage(GetDlgItem(hdlg, IDC_MODIFY_VALUE), EM_SETLIMITTEXT, 2, 0);
			sprintf(buf, "%02X", modifyChipsetPtr->value);
			SetDlgItemText(hdlg, IDC_MODIFY_VALUE, buf);

			sysbiosChipsetBuildMap(buf, modifyChipsetPtr->mask, modifyChipsetPtr->value);
			SetDlgItemText(hdlg, IDC_MODIFY_MAP, buf);
			break;

		case WM_COMMAND:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				hwnd = (HWND)lParam;

				if ((hwnd == GetDlgItem(hdlg, IDC_MODIFY_MASK)) || (hwnd == GetDlgItem(hdlg, IDC_MODIFY_VALUE)))
				{
					mask = 0;
					GetDlgItemText(hdlg, IDC_MODIFY_MASK, buf, 256);
					sscanf(buf, "%x", &mask);

					val = 0;
					GetDlgItemText(hdlg, IDC_MODIFY_VALUE, buf, 256);
					sscanf(buf, "%x", &val);

					sysbiosChipsetBuildMap(buf, (uint8_t)mask, (uint8_t)val);
					SetDlgItemText(hdlg, IDC_MODIFY_MAP, buf);
				}
			}
			else
			{
				switch (LOWORD(wParam))
				{
					case IDOK:
						val = 0;
						GetDlgItemText(hdlg, IDC_MODIFY_INDEX, buf, 256);
						sscanf(buf, "%x", &val);
						modifyChipsetPtr->index = (uint8_t)val;

						val = 0;
						GetDlgItemText(hdlg, IDC_MODIFY_DEVICE, buf, 256);
						sscanf(buf, "%x", &val);
						modifyChipsetPtr->device = (uint8_t)val;

						val = 0;
						GetDlgItemText(hdlg, IDC_MODIFY_FUNC, buf, 256);
						sscanf(buf, "%x", &val);
						modifyChipsetPtr->function = (uint8_t)val;

						val = 0;
						GetDlgItemText(hdlg, IDC_MODIFY_MASK, buf, 256);
						sscanf(buf, "%x", &val);
						modifyChipsetPtr->mask = (uint8_t)val;

						val = 0;
						GetDlgItemText(hdlg, IDC_MODIFY_VALUE, buf, 256);
						sscanf(buf, "%x", &val);
						modifyChipsetPtr->value = (uint8_t)val;

					case IDCANCEL:
						EndDialog(hdlg, TRUE);
						break;
				}
			}
			break;
	}

	return FALSE;
}



INT_PTR CALLBACK sysbiosChipsetRegsFunc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	LPNMHDR	lpNM = (LPNMHDR)lParam;
	LPNMLISTVIEW lpnmlv;
	HWND hlist;
	int t, val;
	sysbiosChipRegEntry *chipRegPtr, tempchipset;
	uint16_t *ptr16;

	LVCOLUMN colList[] = {
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 70, "Register Index",	0, 0, 0, 0 },
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 65, "Device",			0, 1, 0, 0 },
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 65, "Function",		0, 2, 0, 0 },
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 72, "Mask",			0, 3, 0, 0 },
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 72, "Value",			0, 4, 0, 0 },
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 76, "Binary Map",		0, 5, 0, 0 }
	};

	switch (message)
	{
		case WM_INITDIALOG:
			// get a handle to our list control
			hlist = GetDlgItem(hdlg, IDC_CHIPSET_REGS);

			// set full-row select and grid lines
			SendMessage(hlist, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

			// add the columns to our regs table
			for (t = 0; t < 6; t++)
				SendMessage(hlist, LVM_INSERTCOLUMN, t, (LPARAM)&colList[t]);

			// set the possible number of regs in the table
			SendMessage(hlist, LVM_SETITEMCOUNT, 255, 0);
			return TRUE;

		case WM_NOTIFY:
			switch (lpNM->code)
			{
				case NM_RCLICK:
					lpnmlv = (LPNMLISTVIEW)lParam;
					
					// adjust point clicked with the offset of the list control within the dialog window
					lpnmlv->ptAction.x += 28;
					lpnmlv->ptAction.y += 44;

					// version 6.0 BIOSes haven't been figured out yet...
					if (sysbiosVersion != awdbeBIOSVer60)
						val = awdbeDoPopup(hinst, MAKEINTRESOURCE(IDR_CHIPSET_TABLE), lpnmlv->ptAction.x, lpnmlv->ptAction.y);
					else
					{
						awdbeDoPopup(hinst, MAKEINTRESOURCE(IDR_NOCHIPSET_TABLE), lpnmlv->ptAction.x, lpnmlv->ptAction.y);
						val = 0;
					}

					if (val != 0)
					{
						// get pointer to pointer to the reg list
						ptr16 = (uint16_t *)(sysbiosBasePtr + 0x1F85F);

						// make register entry pointer to this pointer
						chipRegPtr = (sysbiosChipRegEntry *)((sysbiosBasePtr + 0x10000) + *ptr16);

						switch (val)
						{
							case ID_CHIPSET_MODIFY:
								// advance to the selected item
								for (t = 0; t < lpnmlv->iItem; t++)
									chipRegPtr++;

								// stupid global variables
								modifyChipsetPtr = &tempchipset;
								memcpy(modifyChipsetPtr, chipRegPtr, sizeof(sysbiosChipRegEntry));

								DialogBox(hinst, MAKEINTRESOURCE(IDD_MODIFY_CHIPSET), hdlg, ModifyChipsetFunc);

								if (memcmp(modifyChipsetPtr, chipRegPtr, sizeof(sysbiosChipRegEntry)))
								{
									memcpy(chipRegPtr, modifyChipsetPtr, sizeof(sysbiosChipRegEntry));
									awdbeSetModified(myID);
								}
								break;

							case ID_CHIPSET_RELOC:
								// relocate table
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

void sysbiosRefreshChipsetRegs(uint8_t *ptr)
{
	HWND hlist;
	sysbiosChipRegEntry *chipRegPtr;
	uint16_t *ptr16;
	LVITEM lvi;
	char buf[256];

	// get handle to the chipset regs window
	hlist = GetDlgItem(sysbiosTabList[2].hwnd, IDC_CHIPSET_REGS);

	// zap all entries currently in the chipset regs table
	SendMessage(hlist, LVM_DELETEALLITEMS, 0, 0);

	// haven't figured out Award/Phoenix 6.0 BIOS's yet
	if (sysbiosVersion != awdbeBIOSVer60)
	{
		// get pointer to pointer to the reg list
		ptr16 = (uint16_t *)(ptr + 0x1F85F);

		// make register entry pointer to this pointer
		chipRegPtr = (sysbiosChipRegEntry *)((ptr + 0x10000) + *ptr16);

		// add the registers
		lvi.mask  = LVIF_TEXT;
		lvi.iItem = 0;

		while ((chipRegPtr->type == 0x00) || (chipRegPtr->type == 0x02))
		{
			// index
			sprintf(buf, "(%02X)%02X", (chipRegPtr->device << 3) | chipRegPtr->function, chipRegPtr->index);
			lvi.iSubItem = 0;
			lvi.pszText  = buf;
			SendMessage(hlist, LVM_INSERTITEM, 0, (LPARAM)&lvi);

			// device
			sprintf(buf, "%02X", chipRegPtr->device);
			lvi.iSubItem = 1;
			lvi.pszText  = buf;
			SendMessage(hlist, LVM_SETITEM, 0, (LPARAM)&lvi);

			// function
			sprintf(buf, "%01X", chipRegPtr->function);
			lvi.iSubItem = 2;
			lvi.pszText  = buf;
			SendMessage(hlist, LVM_SETITEM, 0, (LPARAM)&lvi);

			// mask
			sprintf(buf, "%02X", chipRegPtr->mask);
			lvi.iSubItem = 3;
			lvi.pszText  = buf;
			SendMessage(hlist, LVM_SETITEM, 0, (LPARAM)&lvi);

			// value
			sprintf(buf, "%02X", chipRegPtr->value);
			lvi.iSubItem = 4;
			lvi.pszText  = buf;
			SendMessage(hlist, LVM_SETITEM, 0, (LPARAM)&lvi);

			// binary rep
			sysbiosChipsetBuildMap(buf, chipRegPtr->mask, chipRegPtr->value);

			lvi.iSubItem = 5;
			lvi.pszText  = buf;
			SendMessage(hlist, LVM_SETITEM, 0, (LPARAM)&lvi);

			// advance to next register
			lvi.iItem++;
			chipRegPtr++;
		}
	}
	else
	{
		// add an "unknown" line
		lvi.mask	 = LVIF_TEXT;
		lvi.iItem	 = 0;
		lvi.iSubItem = 0;
		lvi.pszText  = "Unknown";
		SendMessage(hlist, LVM_INSERTITEM, 0, (LPARAM)&lvi);
	}
}
