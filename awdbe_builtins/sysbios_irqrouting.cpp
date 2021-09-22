//
// Award BIOS Editor - sysbios_irqrouting.cpp
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: sysbios_irqrouting.cpp,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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
#include "sysbios_irqrouting.h"
#include "resource.h"

static uint8_t pciSlotID;
static uint8_t *modifyPCIIRQPtr;
static sysbiosPCIRoutingEntry *modifyPCIRoutePtr;

INT_PTR CALLBACK ModifyRoutingFunc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char buf[256];
	int val;

	switch (message)
	{
		case WM_INITDIALOG:
			sprintf(buf, "%d", pciSlotID);
			SetDlgItemText(hdlg, IDC_MODIFY_SLOT, buf);

			SendMessage(GetDlgItem(hdlg, IDC_MODIFY_IRQ), EM_SETLIMITTEXT, 2, 0);
			sprintf(buf, "%02X", *modifyPCIIRQPtr);
			SetDlgItemText(hdlg, IDC_MODIFY_IRQ, buf);

			SendMessage(GetDlgItem(hdlg, IDC_MODIFY_INTA), EM_SETLIMITTEXT, 2, 0);
			sprintf(buf, "%d", modifyPCIRoutePtr->inta);
			SetDlgItemText(hdlg, IDC_MODIFY_INTA, buf);

			SendMessage(GetDlgItem(hdlg, IDC_MODIFY_INTB), EM_SETLIMITTEXT, 2, 0);
			sprintf(buf, "%d", modifyPCIRoutePtr->intb);
			SetDlgItemText(hdlg, IDC_MODIFY_INTB, buf);

			SendMessage(GetDlgItem(hdlg, IDC_MODIFY_INTC), EM_SETLIMITTEXT, 2, 0);
			sprintf(buf, "%d", modifyPCIRoutePtr->intc);
			SetDlgItemText(hdlg, IDC_MODIFY_INTC, buf);

			SendMessage(GetDlgItem(hdlg, IDC_MODIFY_INTD), EM_SETLIMITTEXT, 2, 0);
			sprintf(buf, "%d", modifyPCIRoutePtr->intd);
			SetDlgItemText(hdlg, IDC_MODIFY_INTD, buf);
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					val = 0;
					GetDlgItemText(hdlg, IDC_MODIFY_IRQ, buf, 256);
					sscanf(buf, "%x", &val);
					*modifyPCIIRQPtr = (uint8_t)val;

					val = 0;
					GetDlgItemText(hdlg, IDC_MODIFY_INTA, buf, 256);
					sscanf(buf, "%d", &val);
					modifyPCIRoutePtr->inta = (uint8_t)val;

					val = 0;
					GetDlgItemText(hdlg, IDC_MODIFY_INTB, buf, 256);
					sscanf(buf, "%d", &val);
					modifyPCIRoutePtr->intb = (uint8_t)val;

					val = 0;
					GetDlgItemText(hdlg, IDC_MODIFY_INTC, buf, 256);
					sscanf(buf, "%d", &val);
					modifyPCIRoutePtr->intc = (uint8_t)val;

					val = 0;
					GetDlgItemText(hdlg, IDC_MODIFY_INTD, buf, 256);
					sscanf(buf, "%d", &val);
					modifyPCIRoutePtr->intd = (uint8_t)val;

				case IDCANCEL:
					EndDialog(hdlg, TRUE);
					break;
			}
			break;
	}

	return FALSE;
}

INT_PTR CALLBACK sysbiosIRQRoutingFunc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	LPNMHDR	lpNM = (LPNMHDR)lParam;
	HWND hlist;
	int t, val;
	uint8_t tempirq, *pciIRQPtr;
	sysbiosPCIRoutingEntry temproute, *pciRoutePtr;
	LPNMLISTVIEW lpnmlv;
	uint16_t *ptr16;

	LVCOLUMN colList[] = {
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 64, "PCI slot",	0, 0, 0, 0 },
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 50, "IRQ#",		0, 1, 0, 0 },
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 50, "INTA",		0, 2, 0, 0 },
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 50, "INTB",		0, 3, 0, 0 },
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 50, "INTC",		0, 4, 0, 0 },
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 50, "INTD",		0, 5, 0, 0 }
	};

	switch (message)
	{
		case WM_INITDIALOG:
			// get a handle to our list control
			hlist = GetDlgItem(hdlg, IDC_IRQ_ROUTING);

			// set full-row select and grid lines
			SendMessage(hlist, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

			// add the columns to our irq table
			for (t = 0; t < 6; t++)
				SendMessage(hlist, LVM_INSERTCOLUMN, t, (LPARAM)&colList[t]);

			// set the number of irqs in the table
			SendMessage(hlist, LVM_SETITEMCOUNT, 32, 0);
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
						val = awdbeDoPopup(hinst, MAKEINTRESOURCE(IDR_ROUTING_TABLE), lpnmlv->ptAction.x, lpnmlv->ptAction.y);
					else
					{
						awdbeDoPopup(hinst, MAKEINTRESOURCE(IDR_NOROUTING_TABLE), lpnmlv->ptAction.x, lpnmlv->ptAction.y);
						val = 0;
					}

					if (val != 0)
					{
						// make pointer to irq routing list
						ptr16     = (uint16_t *)(sysbiosBasePtr + 0x1FE91);
						pciIRQPtr = (uint8_t *)((sysbiosBasePtr + 0x10000) + *ptr16);

						// make pointer to INT mapping table
						ptr16       = (uint16_t *)(sysbiosBasePtr + 0x1FE99);
						pciRoutePtr = (sysbiosPCIRoutingEntry *)((sysbiosBasePtr + 0x10000) + *ptr16);

						switch (val)
						{
							case ID_ROUTING_MODIFY:
								// advance to the selected item
								for (t = 0; t < lpnmlv->iItem; t++)
								{
									pciIRQPtr++;
									pciRoutePtr++;
								}

								pciSlotID = lpnmlv->iItem;

								// stupid global variables
								modifyPCIIRQPtr   = &tempirq;
								modifyPCIRoutePtr = &temproute;
								memcpy(modifyPCIIRQPtr,   pciIRQPtr,   sizeof(uint8_t));
								memcpy(modifyPCIRoutePtr, pciRoutePtr, sizeof(sysbiosPCIRoutingEntry));

								DialogBox(hinst, MAKEINTRESOURCE(IDD_MODIFY_ROUTING), hdlg, ModifyRoutingFunc);

								if (memcmp(modifyPCIIRQPtr, pciIRQPtr, sizeof(uint8_t)) || memcmp(modifyPCIRoutePtr, pciRoutePtr, sizeof(sysbiosPCIRoutingEntry)))
								{
									memcpy(pciIRQPtr,   modifyPCIIRQPtr,   sizeof(uint8_t));
									memcpy(pciRoutePtr, modifyPCIRoutePtr, sizeof(sysbiosPCIRoutingEntry));

									awdbeSetModified(myID);
								}
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

void sysbiosRefreshIRQRouting(uint8_t *ptr)
{
	HWND hlist;
	uint16_t *ptr16;
	uint8_t *sptr, *mptr;
	LVITEM lvi;
	char buf[256];
	
	// get handle to the chipset regs window
	hlist = GetDlgItem(sysbiosTabList[6].hwnd, IDC_IRQ_ROUTING);

	// zap all entries currently in the chipset regs table
	SendMessage(hlist, LVM_DELETEALLITEMS, 0, 0);

	// haven't figured out Award/Phoenix 6.0 BIOS's yet
	if (sysbiosVersion != awdbeBIOSVer60)
	{
		// make irq pointer
		ptr16 = (uint16_t *)(ptr + 0x1FE91);
		sptr  = (uint8_t *)((ptr + 0x10000) + *ptr16);

		// make mapping pointer
		ptr16 = (uint16_t *)(ptr + 0x1FE99);
		mptr  = (uint8_t *)((ptr + 0x10000) + *ptr16);

		// add the irqs and pci slots
		lvi.mask  = LVIF_TEXT;
		lvi.iItem = 0;

		while (*sptr != 0xFF)
		{
			// slot
			sprintf(buf, "%d", lvi.iItem);
			lvi.iSubItem = 0;
			lvi.pszText  = buf;
			SendMessage(hlist, LVM_INSERTITEM, 0, (LPARAM)&lvi);

			// irq
			sprintf(buf, "%02X", *sptr);
			lvi.iSubItem = 1;
			lvi.pszText  = buf;
			SendMessage(hlist, LVM_SETITEM, 0, (LPARAM)&lvi);

			// inta
			sprintf(buf, "%d", *mptr++);
			lvi.iSubItem = 2;
			lvi.pszText  = buf;
			SendMessage(hlist, LVM_SETITEM, 0, (LPARAM)&lvi);

			// intb
			sprintf(buf, "%d", *mptr++);
			lvi.iSubItem = 3;
			lvi.pszText  = buf;
			SendMessage(hlist, LVM_SETITEM, 0, (LPARAM)&lvi);

			// intc
			sprintf(buf, "%d", *mptr++);
			lvi.iSubItem = 4;
			lvi.pszText  = buf;
			SendMessage(hlist, LVM_SETITEM, 0, (LPARAM)&lvi);

			// intd
			sprintf(buf, "%d", *mptr++);
			lvi.iSubItem = 5;
			lvi.pszText  = buf;
			SendMessage(hlist, LVM_SETITEM, 0, (LPARAM)&lvi);

			// advance to next slot
			lvi.iItem++;
			sptr++;
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
