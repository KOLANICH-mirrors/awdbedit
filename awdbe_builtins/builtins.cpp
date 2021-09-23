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

const char BUILTIN_ITEM_NAME_Boot_block[] = "Boot block";
const char BUILTIN_ITEM_NAME_Decompression_block[] = "Decompression block";
const char BUILTIN_ITEM_NAME_Fullscreen_bitmap[] = "Fullscreen bitmap";
const char BUILTIN_ITEM_NAME_CPU_ucode[] = "CPU micro code";
const char BUILTIN_ITEM_NAME_EPA_pattern[] = "EPA pattern";
const char BUILTIN_ITEM_NAME_ACPI_table[] = "ACPI table";
const char BUILTIN_ITEM_NAME_VSA_driver[] = "VSA driver";
const char BUILTIN_ITEM_NAME_HPM_ROM[] = "HPM ROM";
const char BUILTIN_ITEM_NAME_HPC_ROM[] = "HPC ROM";
const char BUILTIN_ITEM_NAME_Antivirus_ROM[] = "Antivirus ROM";
const char BUILTIN_ITEM_NAME_Font_ROMs[] = "Font ROMs";

const char BUILTIN_ITEM_NAME_FONT0_ROM[] = "FONT0 ROM";
const char BUILTIN_ITEM_NAME_FONT1_ROM[] = "FONT1 ROM";
const char BUILTIN_ITEM_NAME_FONT2_ROM[] = "FONT2 ROM";
const char BUILTIN_ITEM_NAME_FONT3_ROM[] = "FONT3 ROM";
const char BUILTIN_ITEM_NAME_FONT4_ROM[] = "FONT4 ROM";
const char BUILTIN_ITEM_NAME_FONT5_ROM[] = "FONT5 ROM";

const char BUILTIN_ITEM_NAME_YGROUP_ROM[] = "YGROUP ROM";
const char BUILTIN_ITEM_NAME_MIB_ROM[] = "MIB ROM";

const char BUILTIN_ITEM_NAME_OEM_ROMs[] = "OEM ROMs";
const char BUILTIN_ITEM_NAME_OEM_ROM0[] = "OEM0 ROM";
const char BUILTIN_ITEM_NAME_OEM_ROM1[] = "OEM1 ROM";
const char BUILTIN_ITEM_NAME_OEM_ROM2[] = "OEM2 ROM";
const char BUILTIN_ITEM_NAME_OEM_ROM3[] = "OEM3 ROM";
const char BUILTIN_ITEM_NAME_OEM_ROM4[] = "OEM4 ROM";
const char BUILTIN_ITEM_NAME_OEM_ROM5[] = "OEM5 ROM";
const char BUILTIN_ITEM_NAME_OEM_ROM6[] = "OEM6 ROM";
const char BUILTIN_ITEM_NAME_OEM_ROM7[] = "OEM7 ROM";

const char BUILTIN_ITEM_NAME_EPA_ROMs[] = "EPA ROMs";
const char BUILTIN_ITEM_NAME_EPA_ROM1[] = "EPA1 ROM";
const char BUILTIN_ITEM_NAME_EPA_ROM2[] = "EPA2 ROM";
const char BUILTIN_ITEM_NAME_EPA_ROM3[] = "EPA3 ROM";
const char BUILTIN_ITEM_NAME_EPA_ROM4[] = "EPA4 ROM";
const char BUILTIN_ITEM_NAME_EPA_ROM5[] = "EPA5 ROM";
const char BUILTIN_ITEM_NAME_EPA_ROM6[] = "EPA6 ROM";
const char BUILTIN_ITEM_NAME_EPA_ROM7[] = "EPA7 ROM";

const char BUILTIN_ITEM_NAME_LOGO_ROMs[] = "LOGO ROMs";
const char BUILTIN_ITEM_NAME_LOGO_ROM1[] = "LOGO1 ROM";
const char BUILTIN_ITEM_NAME_LOGO_ROM2[] = "LOGO2 ROM";
const char BUILTIN_ITEM_NAME_LOGO_ROM3[] = "LOGO3 ROM";
const char BUILTIN_ITEM_NAME_LOGO_ROM4[] = "LOGO4 ROM";
const char BUILTIN_ITEM_NAME_LOGO_ROM5[] = "LOGO5 ROM";
const char BUILTIN_ITEM_NAME_LOGO_ROM6[] = "LOGO6 ROM";
const char BUILTIN_ITEM_NAME_LOGO_ROM7[] = "LOGO7 ROM";

const char BUILTIN_ITEM_NAME_Award_Flash_ROM[] = "Award Flash ROM";

const char BUILTIN_ITEM_NAME_VGA_ROMs[] = "VGA ROMs";
const char BUILTIN_ITEM_NAME_VGA_ROM1[] = "VGA ROM[1]";
const char BUILTIN_ITEM_NAME_VGA_ROM2[] = "VGA ROM[2]";
const char BUILTIN_ITEM_NAME_VGA_ROM3[] = "VGA ROM[3]";
const char BUILTIN_ITEM_NAME_VGA_ROM4[] = "VGA ROM[4]";
const char BUILTIN_ITEM_NAME_VGA_ROM5[] = "VGA ROM[5]";
const char BUILTIN_ITEM_NAME_VGA_ROM6[] = "VGA ROM[6]";

const char BUILTIN_ITEM_NAME_PCI_drivers[] = "PCI drivers";
const char BUILTIN_ITEM_NAME_PCI_driverA[] = "PCI driver[A]";
const char BUILTIN_ITEM_NAME_PCI_driverB[] = "PCI driver[B]";
const char BUILTIN_ITEM_NAME_PCI_driverC[] = "PCI driver[C]";
const char BUILTIN_ITEM_NAME_PCI_driverD[] = "PCI driver[D]";
const char BUILTIN_ITEM_NAME_PCI_driverE[] = "PCI driver[E]";
const char BUILTIN_ITEM_NAME_PCI_driverF[] = "PCI driver[F]";
const char BUILTIN_ITEM_NAME_PCI_driverG[] = "PCI driver[G]";
const char BUILTIN_ITEM_NAME_PCI_driverH[] = "PCI driver[H]";
const char BUILTIN_ITEM_NAME_PCI_driverI[] = "PCI driver[I]";
const char BUILTIN_ITEM_NAME_PCI_driverJ[] = "PCI driver[J]";
const char BUILTIN_ITEM_NAME_PCI_driverK[] = "PCI driver[K]";
const char BUILTIN_ITEM_NAME_PCI_driverL[] = "PCI driver[L]";
const char BUILTIN_ITEM_NAME_PCI_driverM[] = "PCI driver[M]";
const char BUILTIN_ITEM_NAME_PCI_driverN[] = "PCI driver[N]";
const char BUILTIN_ITEM_NAME_PCI_driverO[] = "PCI driver[O]";
const char BUILTIN_ITEM_NAME_PCI_driverP[] = "PCI driver[P]";
const char BUILTIN_ITEM_NAME_PCI_driverQ[] = "PCI driver[Q]";
const char BUILTIN_ITEM_NAME_PCI_driverR[] = "PCI driver[R]";
const char BUILTIN_ITEM_NAME_PCI_driverS[] = "PCI driver[S]";
const char BUILTIN_ITEM_NAME_PCI_driverT[] = "PCI driver[T]";
const char BUILTIN_ITEM_NAME_PCI_driverU[] = "PCI driver[U]";
const char BUILTIN_ITEM_NAME_PCI_driverV[] = "PCI driver[V]";
const char BUILTIN_ITEM_NAME_PCI_driverW[] = "PCI driver[W]";
const char BUILTIN_ITEM_NAME_PCI_driverX[] = "PCI driver[X]";
const char BUILTIN_ITEM_NAME_PCI_driverY[] = "PCI driver[Y]";
const char BUILTIN_ITEM_NAME_PCI_driverZ[] = "PCI driver[Z]";

const char BUILTIN_ITEM_NAME_ISA_ROMs_[] = "ISA ROMs";
const char BUILTIN_ITEM_NAME_ISA_ROM_A[] = "ISA ROM[A]";
const char BUILTIN_ITEM_NAME_ISA_ROM_B[] = "ISA ROM[B]";
const char BUILTIN_ITEM_NAME_ISA_ROM_C[] = "ISA ROM[C]";
const char BUILTIN_ITEM_NAME_ISA_ROM_D[] = "ISA ROM[D]";
const char BUILTIN_ITEM_NAME_ISA_ROM_1[] = "ISA ROM[1]";
const char BUILTIN_ITEM_NAME_ISA_ROM_2[] = "ISA ROM[2]";
const char BUILTIN_ITEM_NAME_ISA_ROM_3[] = "ISA ROM[3]";
const char BUILTIN_ITEM_NAME_ISA_ROM_4[] = "ISA ROM[4]";

const char BUILTIN_ITEM_NAME_System_BIOS[] = "System BIOS";
const char BUILTIN_ITEM_NAME_BIOS_Setup_Menu[] = "BIOS Setup Menu";


awdbeItem builtinsItemList[] = {
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	TYPEID_BOOTBLOCK,	BUILTIN_ITEM_NAME_Boot_block,			0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	TYPEID_DECOMPBLOCK, BUILTIN_ITEM_NAME_Decompression_block,	0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x4000,				BUILTIN_ITEM_NAME_Fullscreen_bitmap,	0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x4001,				BUILTIN_ITEM_NAME_CPU_ucode,			0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x4002,				BUILTIN_ITEM_NAME_EPA_pattern,			0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x4003,				BUILTIN_ITEM_NAME_ACPI_table,			0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x4004,				BUILTIN_ITEM_NAME_VSA_driver,			0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x4005,				BUILTIN_ITEM_NAME_HPM_ROM,				0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x4006,				BUILTIN_ITEM_NAME_HPC_ROM,				0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x4007,				BUILTIN_ITEM_NAME_Antivirus_ROM,		0 },

	{ 0, AWDBE_SUBMENU | AWDBE_INCLUDABLE,	0,					BUILTIN_ITEM_NAME_Font_ROMs,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4008,				BUILTIN_ITEM_NAME_FONT0_ROM,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4009,				BUILTIN_ITEM_NAME_FONT1_ROM,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x400A,				BUILTIN_ITEM_NAME_FONT2_ROM,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x400B,				BUILTIN_ITEM_NAME_FONT3_ROM,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x400C,				BUILTIN_ITEM_NAME_FONT4_ROM,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x400D,				BUILTIN_ITEM_NAME_FONT5_ROM,			0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x400E,				BUILTIN_ITEM_NAME_YGROUP_ROM,			0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x400F,				BUILTIN_ITEM_NAME_MIB_ROM,				0 },

// 0x4010 and 0x4011 are in the EPA/LOGO rom groups below
	{ 0, AWDBE_SUBMENU | AWDBE_INCLUDABLE,	0,					BUILTIN_ITEM_NAME_OEM_ROMs,				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4012,				BUILTIN_ITEM_NAME_OEM_ROM0,				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4013,				BUILTIN_ITEM_NAME_OEM_ROM1,				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4014,				BUILTIN_ITEM_NAME_OEM_ROM2,				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4015,				BUILTIN_ITEM_NAME_OEM_ROM3,				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4016,				BUILTIN_ITEM_NAME_OEM_ROM4,				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4017,				BUILTIN_ITEM_NAME_OEM_ROM5,				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4018,				BUILTIN_ITEM_NAME_OEM_ROM6,				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4019,				BUILTIN_ITEM_NAME_OEM_ROM7,				0 },

	{ 0, AWDBE_SUBMENU | AWDBE_INCLUDABLE,	0,					BUILTIN_ITEM_NAME_EPA_ROMs,				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4010,				BUILTIN_ITEM_NAME_EPA_ROM1,				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x401A,				BUILTIN_ITEM_NAME_EPA_ROM2,				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x401B,				BUILTIN_ITEM_NAME_EPA_ROM3,				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x401C,				BUILTIN_ITEM_NAME_EPA_ROM4,				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x401D,				BUILTIN_ITEM_NAME_EPA_ROM5,				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x401E,				BUILTIN_ITEM_NAME_EPA_ROM6,				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x401F,				BUILTIN_ITEM_NAME_EPA_ROM7,				0 },

	{ 0, AWDBE_SUBMENU | AWDBE_INCLUDABLE,	0,					BUILTIN_ITEM_NAME_LOGO_ROMs,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4011,				BUILTIN_ITEM_NAME_LOGO_ROM1,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4020,				BUILTIN_ITEM_NAME_LOGO_ROM2,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4021,				BUILTIN_ITEM_NAME_LOGO_ROM3,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4022,				BUILTIN_ITEM_NAME_LOGO_ROM4,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4023,				BUILTIN_ITEM_NAME_LOGO_ROM5,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4024,				BUILTIN_ITEM_NAME_LOGO_ROM6,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4025,				BUILTIN_ITEM_NAME_LOGO_ROM7,			0 },

	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x4026,				BUILTIN_ITEM_NAME_Award_Flash_ROM,		0 },

	{ 0, AWDBE_SUBMENU | AWDBE_INCLUDABLE,	0,					BUILTIN_ITEM_NAME_VGA_ROMs,				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4080,				BUILTIN_ITEM_NAME_VGA_ROM1,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4081,				BUILTIN_ITEM_NAME_VGA_ROM2,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4082,				BUILTIN_ITEM_NAME_VGA_ROM3,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4083,				BUILTIN_ITEM_NAME_VGA_ROM4,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4084,				BUILTIN_ITEM_NAME_VGA_ROM5,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4085,				BUILTIN_ITEM_NAME_VGA_ROM6,			0 },

	{ 0, AWDBE_SUBMENU | AWDBE_INCLUDABLE,	0,					BUILTIN_ITEM_NAME_PCI_drivers,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4086,				BUILTIN_ITEM_NAME_PCI_driverA,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4087,				BUILTIN_ITEM_NAME_PCI_driverB,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4088,				BUILTIN_ITEM_NAME_PCI_driverC,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4089,				BUILTIN_ITEM_NAME_PCI_driverD,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x408A,				BUILTIN_ITEM_NAME_PCI_driverE,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x408B,				BUILTIN_ITEM_NAME_PCI_driverF,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x408C,				BUILTIN_ITEM_NAME_PCI_driverG,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x408D,				BUILTIN_ITEM_NAME_PCI_driverH,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x408E,				BUILTIN_ITEM_NAME_PCI_driverI,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x408F,				BUILTIN_ITEM_NAME_PCI_driverJ,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4090,				BUILTIN_ITEM_NAME_PCI_driverK,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4091,				BUILTIN_ITEM_NAME_PCI_driverL,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4092,				BUILTIN_ITEM_NAME_PCI_driverM,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4093,				BUILTIN_ITEM_NAME_PCI_driverN,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4094,				BUILTIN_ITEM_NAME_PCI_driverO,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4095,				BUILTIN_ITEM_NAME_PCI_driverP,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4096,				BUILTIN_ITEM_NAME_PCI_driverQ,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4097,				BUILTIN_ITEM_NAME_PCI_driverR,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4098,				BUILTIN_ITEM_NAME_PCI_driverS,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x4099,				BUILTIN_ITEM_NAME_PCI_driverT,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x409A,				BUILTIN_ITEM_NAME_PCI_driverU,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x409B,				BUILTIN_ITEM_NAME_PCI_driverV,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x409C,				BUILTIN_ITEM_NAME_PCI_driverW,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x409D,				BUILTIN_ITEM_NAME_PCI_driverX,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x409E,				BUILTIN_ITEM_NAME_PCI_driverY,		0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x409F,				BUILTIN_ITEM_NAME_PCI_driverZ,		0 },

	{ 0, AWDBE_SUBMENU | AWDBE_INCLUDABLE,	0,					BUILTIN_ITEM_NAME_ISA_ROMs_,				0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x40A0,				BUILTIN_ITEM_NAME_ISA_ROM_A,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x40A1,				BUILTIN_ITEM_NAME_ISA_ROM_B,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x40A2,				BUILTIN_ITEM_NAME_ISA_ROM_C,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x40A3,				BUILTIN_ITEM_NAME_ISA_ROM_D,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x40A4,				BUILTIN_ITEM_NAME_ISA_ROM_1,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x40A5,				BUILTIN_ITEM_NAME_ISA_ROM_2,			0 },
	{ 0, AWDBE_SUBITEM | AWDBE_INCLUDABLE,	0x40A6,				BUILTIN_ITEM_NAME_ISA_ROM_3,			0 },

	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x5000,				BUILTIN_ITEM_NAME_System_BIOS,			0 },
	{ 0, AWDBE_ITEM    | AWDBE_INCLUDABLE,	0x6000,				BUILTIN_ITEM_NAME_BIOS_Setup_Menu,		0 }
};


BOOL WINAPI DllMain(HINSTANCE hModule, DWORD fdwReason, [[maybe_unused]] LPVOID lpvReserved)
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

const char DESCRIPTION_TEXT[] = "\xFA\xB9\xC0\xD2""Builtins";

char *builtinsDescription(void)
{
	return const_cast<char*>(DESCRIPTION_TEXT);
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

	if (bfe != nullptr)
	{
		sysbiosOnLoad(bfe);
	}
}

bool builtinsDetect(fileEntry *fe)
{
	return TRUE;
}

INT_PTR CALLBACK unknownFunc([[maybe_unused]] HWND hdlg, UINT message, [[maybe_unused]] WPARAM wParam, [[maybe_unused]] LPARAM lParam)
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
	HWND hwnd = nullptr;

	switch (item->biosTypeID)
	{
		case 0x4000:				// fullscreen bitmap
		case 0x4002:				// epa logo
			if (isEPALogo((uint8_t *)fe->data, fe->size))
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
	if (hwnd == nullptr)
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

INT_PTR CALLBACK aboutBoxProc(HWND hDlg, UINT message, [[maybe_unused]] WPARAM wParam, [[maybe_unused]] LPARAM lParam)
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
