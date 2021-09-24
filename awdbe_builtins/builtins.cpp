//
// Award BIOS Editor - builtins.cpp
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: builtins.cpp,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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
#include "types.h"
#include "../awdbedit/awdbe_exports.h"
#include "builtins.h"
#include "resource.h"
#include "epa.h"
#include "sysbios.h"

uint64_t myID;
HINSTANCE hinst;


INT_PTR CALLBACK aboutBoxProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


awdbeFuncTable builtinsTable = {
	builtinsDescription,
	builtinsAboutBox,

	builtinsInit,
	builtinsOnLoad,
	builtinsDetect,
	
	builtinsCreateDialog,
	builtinsUpdateDialog,
	builtinsRefreshDialog,
	builtinsOnDestroyDialog,

	builtinsOnResizeDialog
};

awdbeItem builtinsItemList[] = {
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	TYPEID_BOOTBLOCK,	"Boot block",			0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	TYPEID_DECOMPBLOCK, "Decompression block",	0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x4000,				"Fullscreen bitmap",	0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x4001,				"CPU micro code",		0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x4002,				"EPA pattern",			0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x4003,				"ACPI table",			0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x4004,				"VSA driver",			0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x4005,				"HPM ROM",				0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x4006,				"HPC ROM",				0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x4007,				"Antivirus ROM",		0 },
	{ 0, AWDBE_SUBMENU | AWDBE_INCLUDABLE,	0,					"Font ROMs",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4008,				"FONT0 ROM",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4009,				"FONT1 ROM",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x400A,				"FONT2 ROM",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x400B,				"FONT3 ROM",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x400C,				"FONT4 ROM",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x400D,				"FONT5 ROM",			0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x400E,				"YGROUP ROM",			0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x400F,				"MIB ROM",				0 },
// 0x4010 and 0x4011 are in the EPA/LOGO rom groups below
	{ 0, AWDBE_SUBMENU | AWDBE_INCLUDABLE,	0,					"OEM ROMs",				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4012,				"OEM0 ROM",				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4013,				"OEM1 ROM",				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4014,				"OEM2 ROM",				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4015,				"OEM3 ROM",				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4016,				"OEM4 ROM",				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4017,				"OEM5 ROM",				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4018,				"OEM6 ROM",				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4019,				"OEM7 ROM",				0 },
	{ 0, AWDBE_SUBMENU | AWDBE_INCLUDABLE,	0,					"EPA ROMs",				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4010,				"EPA1 ROM",				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x401A,				"EPA2 ROM",				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x401B,				"EPA3 ROM",				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x401C,				"EPA4 ROM",				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x401D,				"EPA5 ROM",				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x401E,				"EPA6 ROM",				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x401F,				"EPA7 ROM",				0 },
	{ 0, AWDBE_SUBMENU | AWDBE_INCLUDABLE,	0,					"LOGO ROMs",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4011,				"LOGO1 ROM",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4020,				"LOGO2 ROM",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4021,				"LOGO3 ROM",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4022,				"LOGO4 ROM",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4023,				"LOGO5 ROM",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4024,				"LOGO6 ROM",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4025,				"LOGO7 ROM",			0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x4026,				"Award Flash ROM",		0 },
	{ 0, AWDBE_SUBMENU | AWDBE_INCLUDABLE,	0,					"VGA ROMs",				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4080,				"VGA ROM[1]",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4081,				"VGA ROM[2]",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4082,				"VGA ROM[3]",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4083,				"VGA ROM[4]",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4084,				"VGA ROM[5]",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4085,				"VGA ROM[6]",			0 },
	{ 0, AWDBE_SUBMENU | AWDBE_INCLUDABLE,	0,					"PCI drivers",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4086,				"PCI driver[A]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4087,				"PCI driver[B]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4088,				"PCI driver[C]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4089,				"PCI driver[D]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x408A,				"PCI driver[E]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x408B,				"PCI driver[F]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x408C,				"PCI driver[G]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x408D,				"PCI driver[H]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x408E,				"PCI driver[I]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x408F,				"PCI driver[J]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4090,				"PCI driver[K]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4091,				"PCI driver[L]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4092,				"PCI driver[M]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4093,				"PCI driver[N]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4094,				"PCI driver[O]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4095,				"PCI driver[P]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4096,				"PCI driver[Q]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4097,				"PCI driver[R]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4098,				"PCI driver[S]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4099,				"PCI driver[T]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x409A,				"PCI driver[U]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x409B,				"PCI driver[V]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x409C,				"PCI driver[W]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x409D,				"PCI driver[X]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x409E,				"PCI driver[Y]",		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x409F,				"PCI driver[Z]",		0 },
	{ 0, AWDBE_SUBMENU | AWDBE_INCLUDABLE,	0,					"ISA ROMs",				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x40A0,				"ISA ROM[A]",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x40A1,				"ISA ROM[B]",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x40A2,				"ISA ROM[C]",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x40A3,				"ISA ROM[D]",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x40A4,				"ISA ROM[1]",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x40A5,				"ISA ROM[2]",			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x40A6,				"ISA ROM[3]",			0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x5000,				"System BIOS",			0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x6000,				"BIOS Setup Menu",		0 }
};


BOOL WINAPI DllMain(HINSTANCE hModule, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
			hinst = hModule;
			break;

		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}

	return TRUE;
}


extern "C" __declspec(dllexport) awdbeFuncTable *awdbeRegisterPlugin(void)
{
	return &builtinsTable;
}

char *builtinsDescription(void)
{
	return "\xFA\xB9\xC0\xD2""Builtins";
}

void builtinsAboutBox(HWND parentWnd)
{
	DialogBox(hinst, MAKEINTRESOURCE(IDD_ABOUT), parentWnd, aboutBoxProc);
}

void builtinsInit(uint64_t pluginID)
{
	myID = pluginID;

	awdbeAddToItemList(myID, builtinsItemList, sizeof(builtinsItemList) / sizeof(awdbeItem));
}

void builtinsOnLoad(fileEntry *fe, int count)
{
	fileEntry *bfe;

	// cheat and just ask the editor for the sysbios pointer...
	bfe = awdbeSearchForID(myID, 0x5000);

	if (bfe != NULL)
	{
		sysbiosOnLoad(bfe);
	}
}

bool builtinsDetect(fileEntry *fe)
{
	return TRUE;
}

INT_PTR CALLBACK unknownFunc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			return TRUE;
	}

	return FALSE;
}

HWND builtinsCreateDialog(awdbeItem *item, fileEntry *fe, HWND parentWnd, RECT *rc)
{
	HWND hwnd = NULL;

	switch (item->biosTypeID)
	{
		case 0x4000:				// fullscreen bitmap
		case 0x4002:				// epa logo
			if (isEPALogo((uchar *)fe->data, fe->size))
			{
				hwnd = epaCreateDialog(parentWnd);
				epaRefreshDialog(hwnd, fe);
			}
			break;

		case 0x5000:				// system bios
			hwnd = sysbiosCreateDialog(parentWnd);
			sysbiosRefreshDialog(hwnd, fe);
			break;

		case TYPEID_BOOTBLOCK:		// boot block
			break;

		case TYPEID_DECOMPBLOCK:	// decompression block
			break;
	}

	// check if not handled
	if (hwnd == NULL)
	{
		// create a default dialog
		hwnd = CreateDialog(hinst, MAKEINTRESOURCE(IDD_UNKNOWN), parentWnd, unknownFunc);
	}

	return hwnd;
}

bool builtinsUpdateDialog(awdbeItem *item, fileEntry *fe, HWND dialogWnd)
{
	bool modified = FALSE;

	switch (item->biosTypeID)
	{
		case 0x4000:				// fullscreen bitmap
		case 0x4002:				// epa logo
			// epa code doesn't have any dialogs which can be updated...
			return FALSE;

		case 0x5000:				// system bios
			modified = sysbiosUpdateDialog(dialogWnd, fe);
			break;

		case TYPEID_BOOTBLOCK:		// boot block
			break;

		case TYPEID_DECOMPBLOCK:	// decompression block
			break;
	}
	
	return modified;
}

void builtinsRefreshDialog(awdbeItem *item, fileEntry *fe, HWND dialogWnd)
{
	switch (item->biosTypeID)
	{
		case 0x4000:				// fullscreen bitmap
		case 0x4002:				// epa logo
			epaRefreshDialog(dialogWnd, fe);
			break;

		case 0x5000:				// system bios
			sysbiosRefreshDialog(dialogWnd, fe);
			break;

		case TYPEID_BOOTBLOCK:		// boot block
			break;

		case TYPEID_DECOMPBLOCK:	// decompression block
			break;
	}
}

void builtinsOnDestroyDialog(awdbeItem *item, HWND dialogWnd)
{
	switch (item->biosTypeID)
	{
		case 0x4000:				// fullscreen bitmap
		case 0x4002:				// epa logo
			epaOnDestroyDialog(dialogWnd);
			break;

		case 0x5000:				// system bios
			sysbiosOnDestroyDialog(dialogWnd);
			break;

		case TYPEID_BOOTBLOCK:		// boot block
			break;

		case TYPEID_DECOMPBLOCK:	// decompression block
			break;
	}
}

void builtinsOnResizeDialog(awdbeItem *item, HWND dialogWnd, RECT *rc)
{
	switch (item->biosTypeID)
	{
		case 0x4000:				// fullscreen bitmap
		case 0x4002:				// epa logo
			break;

		case 0x5000:				// system bios
			sysbiosOnResizeDialog(dialogWnd, rc);
			break;

		case TYPEID_BOOTBLOCK:		// boot block
			break;

		case TYPEID_DECOMPBLOCK:	// decompression block
			break;
	}
}

INT_PTR CALLBACK aboutBoxProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char buf[256];

	switch (message)
	{
		case WM_INITDIALOG:
			sprintf(buf, "Compiled on %s at %s", __DATE__, __TIME__);
			SetDlgItemText(hDlg, IDC_TEXT_COMPILETIME, buf);
			return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK)
				EndDialog(hDlg, TRUE);
			return TRUE;
	}

	return FALSE;
}
