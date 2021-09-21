//
// Award BIOS Editor - awdbe_exports.h
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: awdbe_exports.h,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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

#ifndef AWDBE_EXPORTS
#define AWDBE_EXPORTS

typedef struct
{
	char	*name;					// name of this file (incl NULL terminator)
	ulong	 nameLen;				// length of filename (minus NULL terminator)

	ulong	 size;					// size of this file
	ulong	 compSize;				// the compressed size this file had
	ushort	 type;					// type ID/magic of this file
	ushort	 crc;					// the crc this file had
	bool	 crcOK;					// TRUE if the file passed CRC on decompression, FALSE if not

	void	*data;					// the decompressed data of this file
	ulong	 offset;				// special/fixed offset of this file in the image, or 0 if none.
	ulong	 flags;					// special flags for this file (used to identify boot & decompression blocks)
} fileEntry;

#define FEFLAGS_DECOMP_BLOCK	0x00000001
#define FEFLAGS_BOOT_BLOCK		0x00000002


typedef struct
{
	ulong		 hash;				// the unique 32-bit hash value of this item

	ulong		 flags;				// any of the AWDBE_* flags below
	ushort		 biosTypeID;		// the 16-bit type ID of the component to be recognized
	char		*itemName;			// a descriptive name of this item
	ulong		 userData;			// any 32-bit user storable data
} awdbeItem;

#define AWDBE_ITEM			0x00000001
#define AWDBE_SUBITEM		0x00000002
#define AWDBE_SUBMENU		0x00000003
#define AWDBE_MENUMASK		0x00000003			// for internal use only!
#define AWDBE_INCLUDABLE	0x00000004

#define IS_ITEM(item)			((item->flags & AWDBE_MENUMASK) == AWDBE_ITEM)
#define IS_SUBITEM(item)		((item->flags & AWDBE_MENUMASK) == AWDBE_SUBITEM)
#define IS_SUBMENU(item)		((item->flags & AWDBE_MENUMASK) == AWDBE_SUBMENU)
#define IS_INCLUDABLE(item)		(item->flags & AWDBE_INCLUDABLE)

#define TYPEID_DECOMPBLOCK		0x0001		// these type IDs are required so the plugin system can access
#define TYPEID_BOOTBLOCK		0x0002		//   the decompression and boot blocks too


typedef struct
{
	char *	(*descriptionFunc)(void);
	void	(*aboutBoxFunc)(HWND parentWnd);

	void	(*initFunc)(ulong pluginID);

	void	(*onLoadFunc)(fileEntry *fe, int count);
	bool	(*detectFunc)(fileEntry *fe);

	HWND	(*createDialogFunc)(awdbeItem *item, fileEntry *fe, HWND parentWnd, RECT *rc);		// called to create dialog
	bool	(*updateDialogFunc)(awdbeItem *item, fileEntry *fe, HWND dialogWnd);				// called to update components' data with current dialog
	void	(*refreshDialogFunc)(awdbeItem *item, fileEntry *fe, HWND dialogWnd);				// called to update current dialog with components' data
	void	(*onDestroyDialogFunc)(awdbeItem *item, HWND dialogWnd);							// called on destruction of dialog

	void	(*onResizeDialogFunc)(awdbeItem *item, HWND dialogWnd, RECT *rc);					// called when dialog is resized
} awdbeFuncTable;


typedef enum
{
	awdbeBIOSVerUnknown	= 0,
	awdbeBIOSVer451PG,
	awdbeBIOSVer600PG,
	awdbeBIOSVer60
} awdbeBIOSVersion;

#ifdef AWDBE_EXPORT_FUNCS
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __declspec(dllimport)
#endif

EXPORT void awdbeAddToItemList(ulong pluginID, awdbeItem *itemList, int itemCount);
EXPORT fileEntry *awdbeSearchForID(ulong pluginID, ushort ID);
EXPORT void awdbeUpdateSelf(ulong pluginID);
EXPORT void awdbeRefreshSelf(ulong pluginID);

EXPORT void awdbeGetDialogSize(ulong pluginID, SIZE *sz);
EXPORT void awdbeResizeDialog(ulong pluginID, SIZE sz);

EXPORT void awdbeSetModified(ulong pluginID);

EXPORT int  awdbeDoPopup(HINSTANCE hinst, LPSTR resid, int xp, int yp);
EXPORT awdbeBIOSVersion awdbeGetBIOSVersion(ulong pluginID);

#endif
