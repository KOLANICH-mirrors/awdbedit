//
// Award BIOS Editor - bios.cpp
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: bios.cpp,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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
#include <shlobj.h>
#include <process.h>
#include <io.h>
#include <direct.h>
#include "types.h"
#include "config.h"
#include "resource.h"
#include "lzh.h"
#define AWDBE_EXPORT_FUNCS
#include "awdbe_exports.h"
#include "bios.h"
#include "plugin.h"
#include "main.h"


typedef enum
{
	LAYOUT_UNKNOWN = 0,
	LAYOUT_1_1_1,
	LAYOUT_2_1_1,
	LAYOUT_2_2_2
} biosLayout;

typedef struct
{
	bool		 modified;				// has this image been modified?
	char		 fname[256];			// full path/name of image loaded
	biosLayout	 layout;				// type of layout of the file table

	uchar		*imageData;				// the loaded image
	ulong		 imageSize;				// size of the loaded image (in bytes)

	fileEntry	*fileTable;				// uncompressed data of all files
	int			 fileCount;				// number of files in the file table
	ulong		 tableOffset;			// offset of dynamic file table from start of image

	ulong		 maxTableSize;			// maximum compressed size allowed in the file table
} biosStruct;

typedef struct updateEntry
{
	char	*path;
	char	*fname;
	time_t	lastWrite;

	struct updateEntry	*next;
} updateEntry;

static updateEntry *updateList = NULL;
static UINT updateTimerID = 0;
static bool updateIgnoreTimer = FALSE;

void biosAddToUpdateList(char *fname);
void biosClearUpdateList(void);


static HINSTANCE hinst;
static HWND hwnd, statusWnd, treeView, hPropDlgListWnd, hModDlgWnd;
static HTREEITEM recgItem, inclItem, unkItem;
static RECT *dlgrc;
static biosStruct biosdata;
static ulong curHash;
static ushort insertID;
static fileEntry *curFileEntry;
static bool ignoreNotify;
static char *biosChangedText = "This BIOS image has been changed.  Do you want to save your changes before loading a new one?";
static ulong biosFreeSpace;

void biosRemoveEntry(fileEntry *toRemove);


BOOL CALLBACK LoadSaveProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            SetDlgItemText(hdlg, IDC_LOADING_TEXT, "");
            SendMessage(GetDlgItem(hdlg, IDC_LOADING_PROGRESS), PBM_SETPOS, 0, 0);
            return TRUE;
    }

    return FALSE;
}

void biosTitleUpdate(void)
{
	char buf[256];
	int t;
	ulong size;
	fileEntry *fe;

	sprintf(buf, "%s - [%s%s", APP_VERSION, biosdata.fname, (biosdata.modified == TRUE) ? " *]" : "]");
	SetWindowText(hwnd, buf);

	// go through all files in the table with an offset of 0 and calculate a total size
	size = 0;

	for (t = 0; t < biosdata.fileCount; t++)
	{
		fe = &biosdata.fileTable[t];

		if (fe->offset == 0)
			size += fe->compSize;
	}

	sprintf(buf, "Used: %dK/%dK", size / 1024, biosdata.maxTableSize / 1024);
	SendMessage(statusWnd, SB_SETTEXT, 1, (LPARAM)buf);

	// set the global free size while we're at it...
	biosFreeSpace = biosdata.maxTableSize - size;
}

void biosInit(HINSTANCE hi, HWND hw, HWND statwnd, HWND treewnd, HTREEITEM recgitem, HTREEITEM inclitem, HTREEITEM unkitem, RECT *dlgrect)
{
	ulong count;
	HWND loaddlg, hwnd_loadtext, hwnd_loadprog;

	hinst		= hi;
	hwnd		= hw;
	statusWnd	= statwnd;
	treeView	= treewnd;
	recgItem	= recgitem;
	inclItem	= inclitem;
	unkItem		= unkitem;

	hPropDlgListWnd = NULL;
	hModDlgWnd		= NULL;

	dlgrc = dlgrect;
	ignoreNotify = FALSE;

	ZeroMemory(&biosdata, sizeof(biosStruct));

	// scan for plugins to load -- only getting the count first
	count = pluginScan(exePath, FALSE);

	// now, open our working dialog
    loaddlg = CreateDialog(hinst, MAKEINTRESOURCE(IDD_WORKING), hwnd, (DLGPROC)LoadSaveProc);
    
	SetWindowText(loaddlg, "Scanning for plugins...");
    hwnd_loadtext = GetDlgItem(loaddlg, IDC_LOADING_TEXT);
    hwnd_loadprog = GetDlgItem(loaddlg, IDC_LOADING_PROGRESS);

	SetWindowText(hwnd_loadtext, "");
    SendMessage(hwnd_loadprog, PBM_SETRANGE, 0, MAKELPARAM(0, count));
    SendMessage(hwnd_loadprog, PBM_SETSTEP, 1, 0);

	// initialize the plugin system with the window to the controls
	pluginInitScan(hwnd_loadtext, hwnd_loadprog);

	// load all of the plugins
	count = pluginScan(exePath, TRUE);
	
	if (count == 0)
	{
		MessageBox(hwnd, "No Award BIOS Editor compatible plugins were found in the current directory or\n"
						 "any subdirectory from the location where the Award BIOS Editor is located.  All\n"
						 "components within a loaded BIOS will show up as \'Unknown Items\'.  Please\n"
						 "ensure that you have unzipped the Award BIOS Editor with directories, and any\n"
						 "old vesion of the Editor or its plugins has been cleanly removed before\n"
						 "installing the new version.", "Warning", MB_OK);
	}

	// destroy our window
	DestroyWindow(loaddlg);
}

void biosFreeMemory(void)
{
	int t;
	fileEntry *fe;
	awdbeItem *item;

	// if the current hash points to a plugin, call it's "onDestroy" function to tell it it's about to be killed
	if ((curFileEntry != NULL) && (curHash > HASH_UNKNOWN_ITEM_MAX))
	{
		switch (curHash)
		{
			case HASH_SUBMENU_ITEM:
			case HASH_RECOGNIZED_ROOT:
			case HASH_UNKNOWN_ROOT:
			case HASH_INCLUDABLE_ROOT:
				break;

			default:
				// find the item that responds to this hash
				item = pluginFindHash(curHash);
				if (item != NULL)
				{
					// call this plugin's create dialog function to show the window
					pluginCallOnDestroyDialog(item, hModDlgWnd);
				}
				break;
		}
	}

	if (hPropDlgListWnd != NULL)
	{
		DestroyWindow(hPropDlgListWnd);
		hPropDlgListWnd = NULL;
	}

	if (hModDlgWnd != NULL)
	{
		DestroyWindow(hModDlgWnd);
		hModDlgWnd = NULL;
	}

	if (biosdata.fileTable != NULL)
	{
		for (t = 0; t < biosdata.fileCount; t++)
		{
			fe = &biosdata.fileTable[t];
			delete []fe->name;
			delete []fe->data;
		}

		delete []biosdata.fileTable;
		biosdata.fileTable = NULL;
	}

	if (biosdata.imageData != NULL)
	{
		delete []biosdata.imageData;
		biosdata.imageData = NULL;
	}
}

char *biosGetFilename(void)
{
	return biosdata.fname;
}

HWND biosGetDialog(void)
{
	return hModDlgWnd;
}

fileEntry *biosExpandTable(void)
{
	fileEntry *tempTable;

	// first, store the current table
	tempTable = biosdata.fileTable;

	// increase the file count and alloc a new one
	biosdata.fileCount++;
	biosdata.fileTable = new fileEntry[biosdata.fileCount];

	// copy the existing file table into the new one
	memcpy(biosdata.fileTable, tempTable, (biosdata.fileCount - 1) * sizeof(fileEntry));

	// delete the old table
	delete []tempTable;

	// return a pointer to the new entry, but zap its memory first...
	tempTable = &biosdata.fileTable[biosdata.fileCount - 1];
	memset(tempTable, 0, sizeof(fileEntry));

	return tempTable;
}

void biosWriteEntry(fileEntry *fe, lzhHeader *lzhhdr, ulong offset)
{
	ushort crc;
	lzhHeaderAfterFilename *lzhhdra;

	lzhhdra = (lzhHeaderAfterFilename *) ((lzhhdr->filename) + lzhhdr->filenameLen);

	fe->nameLen = lzhhdr->filenameLen;
	fe->name = new char[fe->nameLen + 1];
	memcpy(fe->name, lzhhdr->filename, fe->nameLen);
	fe->name[fe->nameLen] = 0;

	fe->size	 = lzhhdr->originalSize;
	fe->compSize = lzhhdr->compressedSize;
	fe->type	 = lzhhdr->fileType;
	fe->crc		 = lzhhdra->crc;
	fe->data	 = (void *)new uchar[fe->size];
	fe->offset	 = offset;
	fe->flags	 = 0;

	// decompress file
	if (lzhExpand(lzhhdr, fe->data, fe->size, &crc) != LZHERR_OK)
	{
		// error extracting
		MessageBox(hwnd, "Error extracting component!\n\nThis BIOS Image may be corrupted or damaged.  The editor will still continue to load\n"
			"the image, but certain components may not be editable.", "Notice", MB_OK);
	}
	else
	{
		if (fe->crc != crc)
		{
			// CRC failed
			fe->crcOK = FALSE;

			MessageBox(hwnd, "CRC check failed!\n\nThis BIOS Image may be corrupted or damaged.  The editor will still continue to load\n"
				"the image, but certain components may not be editable.", "Notice", MB_OK);
		}
		else
		{
			fe->crcOK = TRUE;
		}
	}
}

fileEntry *biosScanForID(ushort id)
{
	fileEntry *fe = &biosdata.fileTable[0];
	ulong count = biosdata.fileCount;

	while (count--)
	{
		if (fe->type == id)
			return fe;

		fe++;
	}

	return NULL;
}

void biosUpdateComponents(void)
{
	HTREEITEM hti;
	int t;
	char buf[512];
	TVINSERTSTRUCT tvis;
	fileEntry *fe;
	awdbeItem *item;
	ulong count;
	awdbeItem *subMenuPtr, *lastSubMenuPtr;
	HTREEITEM subMenuTree;
	awdbeItemEntry *ilist;

	// delete all existing items under the root tree
	ignoreNotify = TRUE;

	while ( (hti = (HTREEITEM)SendMessage(treeView, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)recgItem)) != NULL)
		SendMessage(treeView, TVM_DELETEITEM, 0, (LPARAM)hti);

	while ( (hti = (HTREEITEM)SendMessage(treeView, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)inclItem)) != NULL)
		SendMessage(treeView, TVM_DELETEITEM, 0, (LPARAM)hti);

	while ( (hti = (HTREEITEM)SendMessage(treeView, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)unkItem)) != NULL)
		SendMessage(treeView, TVM_DELETEITEM, 0, (LPARAM)hti);

	ignoreNotify = FALSE;

	// add the components found in our image...
	lastSubMenuPtr = NULL;

	for (t = 0; t < biosdata.fileCount; t++)
	{
		fe = &biosdata.fileTable[t];
		
		// call the plugin interface and see if anyone responds to this type ID...
		item = pluginFindResponder(fe);
		if (item != NULL)
		{
			// check if this item is a subitem
			if (IS_SUBITEM(item))
			{
				// find the submenu for this item
				subMenuPtr = pluginFindSubMenu(item);

				// flog an error if we didn't find one (so the plugin creator can fix his bug)
				if (subMenuPtr == NULL)
				{
					sprintf(buf, "Unable to find a submenu definition for the [%s] type!\n\nThis error is being generated "
						"due to an error in the item definition structure of one of your plugins.  If you are\ncreating a "
						"plugin, please check to make sure you have properly inserted AWDBE_SUBMENU tags in your item "
						"list.\nIf you are an end-user, please check and see if a newer (and fixed) version of your "
						"plugins are available.", item->itemName);

					MessageBox(hwnd, buf, "Error", MB_OK);
				}
				else
				{
					// if this menu pointer is *not* the same as the last one, build it.
					if (subMenuPtr != lastSubMenuPtr)
					{
						tvis.hParent		= recgItem;
						tvis.hInsertAfter	= TVI_LAST;
						tvis.itemex.mask	= TVIF_TEXT | TVIF_PARAM;
						tvis.itemex.pszText	= subMenuPtr->itemName;
						tvis.itemex.lParam	= HASH_SUBMENU_ITEM;
						subMenuTree = (HTREEITEM)SendMessage(treeView, TVM_INSERTITEM, 0, (LPARAM)&tvis);

						lastSubMenuPtr = subMenuPtr;
					}

					// add this item to the submenu
					tvis.hParent		= subMenuTree;
					tvis.hInsertAfter	= TVI_LAST;
					tvis.itemex.mask	= TVIF_TEXT | TVIF_PARAM;
					tvis.itemex.pszText	= item->itemName;
					tvis.itemex.lParam	= item->hash;
					SendMessage(treeView, TVM_INSERTITEM, 0, (LPARAM)&tvis);
				}
			}
			else
			{
				// add this item to the root tree
				tvis.hParent		= recgItem;
				tvis.hInsertAfter	= TVI_LAST;
				tvis.itemex.mask	= TVIF_TEXT | TVIF_PARAM;
				tvis.itemex.pszText	= item->itemName;
				tvis.itemex.lParam	= item->hash;
				SendMessage(treeView, TVM_INSERTITEM, 0, (LPARAM)&tvis);
			}
		}
		else
		{
			if (t <= HASH_UNKNOWN_ITEM_MAX)
			{
				// no one responded, so this is an unknown component.  insert it as such...
				sprintf(buf, "Other (%04X:0000)", fe->type);

				tvis.hParent		= unkItem;
				tvis.hInsertAfter	= TVI_LAST;
				tvis.itemex.mask	= TVIF_TEXT | TVIF_PARAM;
				tvis.itemex.pszText	= buf;
				tvis.itemex.lParam	= t;									// the index in the bios image is the identifier
				SendMessage(treeView, TVM_INSERTITEM, 0, (LPARAM)&tvis);
			}
		}
	}

	// now, add all includable components
	ilist = pluginGetItemList();

	// iterate through *ALL* lists
	lastSubMenuPtr = NULL;

	while (ilist != NULL)
	{
		item  = ilist->list;
		count = ilist->count;

		while (count--)
		{
			// make sure that this item is allowed to be included
			if (IS_INCLUDABLE(item))
			{
				// see if this ID is in our BIOS image
				fe = biosScanForID(item->biosTypeID);
				if (fe == NULL)
				{
					// check if this item is a submenu
					if (IS_SUBMENU(item))
					{
						// it is, so do nothing...
					}
					else if (IS_SUBITEM(item))
					{
						// find the submenu for this item
						subMenuPtr = pluginFindSubMenu(item);

						// flog an error if we didn't find one (so the plugin creator can fix his bug)
						if (subMenuPtr == NULL)
						{
							sprintf(buf, "Unable to find a submenu definition for the [%s] type!\n\nThis error is being generated "
								"due to an error in the item definition structure of one of your plugins.  If you are\ncreating a "
								"plugin, please check to make sure you have properly inserted AWDBE_SUBMENU tags in your item "
								"list.\nIf you are an end-user, please check and see if a newer (and fixed) version of your "
								"plugins are available.", item->itemName);

							MessageBox(hwnd, buf, "Error", MB_OK);
						}
						else
						{
							// if this menu pointer is *not* the same as the last one, build it.
							if (subMenuPtr != lastSubMenuPtr)
							{
								tvis.hParent		= inclItem;
								tvis.hInsertAfter	= TVI_LAST;
								tvis.itemex.mask	= TVIF_TEXT | TVIF_PARAM;
								tvis.itemex.pszText	= subMenuPtr->itemName;
								tvis.itemex.lParam	= HASH_SUBMENU_ITEM;
								subMenuTree = (HTREEITEM)SendMessage(treeView, TVM_INSERTITEM, 0, (LPARAM)&tvis);

								lastSubMenuPtr = subMenuPtr;
							}

							// add this item to the submenu
							tvis.hParent		= subMenuTree;
							tvis.hInsertAfter	= TVI_LAST;
							tvis.itemex.mask	= TVIF_TEXT | TVIF_PARAM;
							tvis.itemex.pszText	= item->itemName;
							tvis.itemex.lParam	= item->hash;
							SendMessage(treeView, TVM_INSERTITEM, 0, (LPARAM)&tvis);
						}
					}
					else
					{
						// add this item to the root tree
						tvis.hParent		= inclItem;
						tvis.hInsertAfter	= TVI_LAST;
						tvis.itemex.mask	= TVIF_TEXT | TVIF_PARAM;
						tvis.itemex.pszText	= item->itemName;
						tvis.itemex.lParam	= item->hash;
						SendMessage(treeView, TVM_INSERTITEM, 0, (LPARAM)&tvis);
					}
				}
				else
				{
					// this ID *is* in our BIOS image, so don't do anything... it's already been added to the recognized list.
				}
			}

			item++;
		}

		ilist = ilist->next;
	}

	// show the tree window (also causes a repaint)
	ShowWindow(treeView, SW_SHOWNORMAL);

	// automatically open the recognized and unknown items
	SendMessage(treeView, TVM_EXPAND, TVE_EXPAND, (LPARAM)recgItem);
	SendMessage(treeView, TVM_EXPAND, TVE_EXPAND, (LPARAM)unkItem);

	// and select the root recognized tree
	SendMessage(treeView, TVM_SELECTITEM, TVGN_CARET, (LPARAM)NULL);
	SendMessage(treeView, TVM_SELECTITEM, TVGN_CARET, (LPARAM)recgItem);
}

bool biosHandleModified(char *text)
{
	int res;

	// update any leftover data
	biosUpdateCurrentDialog();

	// check the modified flag
	if (biosdata.modified == FALSE)
		return TRUE;

	res = MessageBox(hwnd, text, "Notice", MB_YESNOCANCEL);

	switch (res)
	{
		case IDYES:
			// save first
			biosSave();
			break;

		case IDNO:
			// do nothing
			break;

		case IDCANCEL:
			return FALSE;
	}

	return TRUE;
}

bool biosOpenFile(char *fname)
{
	FILE *fp;
	uchar *ptr;
	uchar _0xEA;
	ulong count, _MRB;
    HWND loaddlg, hwnd_loadtext, hwnd_loadprog;
	lzhHeader *lzhhdr;
	lzhHeaderAfterFilename *lzhhdra;
	bool done;
	int curFile;
	uchar *nextUpdate, *bootBlockData = NULL, *decompBlockData = NULL;
	ulong bootBlockSize = 0, decompBlockSize = 0;
	fileEntry *fe;

	// warn if the current bios has been modified
	if (biosHandleModified(biosChangedText) == FALSE)
		return FALSE;

	// stop update checking for this image
	biosClearUpdateList();

	// open the image
	fp = fopen(fname, "rb");
	if (fp == NULL)
	{
		MessageBox(hwnd, "Unable to open BIOS image!", "Error", MB_OK);
		return FALSE;
	}

	// check if this is a real bios image
	fseek(fp, -16, SEEK_END);		// 16th byte from the EOF is a jump instruction (EAh)
	_0xEA = fgetc(fp);

	fseek(fp, -11, SEEK_END);		// 11th byte from EOF contains "*MRB*", but we're only gonna check for the first 4 bytes
	fread(&_MRB, 1, 4, fp);

	fseek(fp, 0, SEEK_END);			// size is divisible by 1024 (but not greater than 1Mb)
	biosdata.imageSize = ftell(fp);

	if ((_0xEA != 0xEA) || (_MRB != 'BRM*') || ((biosdata.imageSize % 1024) != 0) || (biosdata.imageSize > (1024 * 1024)))
	{
		if (MessageBox(hwnd, "This image does not appear to be a valid Award BIOS image.\n\n"
			"Do you wish to attempt to continue to loading anyway?", "Notice", MB_YESNO) == IDNO)
		{
			fclose(fp);
			return FALSE;
		}
	}

	// looks okay from here...
	count = biosdata.imageSize / 1024;
	strcpy(biosdata.fname, fname);

	// free any already allocated memory
	biosFreeMemory();

	// put up our loading dialog and initialize it
    loaddlg = CreateDialog(hinst, MAKEINTRESOURCE(IDD_WORKING), hwnd, (DLGPROC)LoadSaveProc);
    
	SetWindowText(loaddlg, "Loading Image...");
    hwnd_loadtext = GetDlgItem(loaddlg, IDC_LOADING_TEXT);
    hwnd_loadprog = GetDlgItem(loaddlg, IDC_LOADING_PROGRESS);

	SetWindowText(hwnd_loadtext, "Loading image into memory...");
    SendMessage(hwnd_loadprog, PBM_SETRANGE, 0, MAKELPARAM(0, count));
    SendMessage(hwnd_loadprog, PBM_SETSTEP, 1, 0);

	// allocate space and load the image into memory
	biosdata.imageData = new uchar[biosdata.imageSize];
	ptr	= biosdata.imageData;

	fseek(fp, 0, SEEK_SET);

	while (count--)
	{
		SendMessage(hwnd_loadprog, PBM_STEPIT, 0, 0);

		fread(ptr, 1024, 1, fp);
		ptr += 1024;
	}

	// close the file
	fclose(fp);

	// scan for the boot and decompression blocks, and extract them
	SetWindowText(hwnd_loadtext, "Scanning for Boot Block...");

	ptr	  = biosdata.imageData;
	count = biosdata.imageSize;
	while (count--)
	{
		if (!memicmp(ptr, "Award BootBlock Bios", 20))
		{
			bootBlockSize = biosdata.imageSize - (ptr - biosdata.imageData);
			bootBlockData = new uchar[bootBlockSize];
			memcpy(bootBlockData, ptr, bootBlockSize);

			count = 0;
		}

		ptr++;
	}

	if (bootBlockData == NULL)
	{
		MessageBox(hwnd, "Unable to locate the Boot Block within the BIOS Image!\n\n"
			"The editor will still be able to modify this image, but this component will be\n"
			"unaccessable.  Re-flashing with a saved version of this BIOS is NOT RECOMMENDED!", "Notice", MB_OK);
	}

	// next, decompression block...
	SetWindowText(hwnd_loadtext, "Scanning for Decompression Block...");

	ptr	  = biosdata.imageData;
	count = biosdata.imageSize;
	while (count--)
	{
		if (!memicmp(ptr, "= Award Decompression Bios =", 28))
		{
			// copy the decompression block
			decompBlockSize = (biosdata.imageSize - (ptr - biosdata.imageData)) - bootBlockSize;
			decompBlockData = new uchar[decompBlockSize];
			memcpy(decompBlockData, ptr, decompBlockSize);

			count = 0;
		}

		ptr++;
	}

	if (decompBlockData == NULL)
	{
		MessageBox(hwnd, "Unable to locate the Decompression Block within the BIOS Image!\n\n"
			"The editor will still be able to modify this image, but this component will be\n"
			"unaccessable.  Re-flashing with a saved version of this BIOS is NOT RECOMMENDED!", "Notice", MB_OK);
	}

	// load the file table
	biosdata.layout		 = LAYOUT_UNKNOWN;
	biosdata.fileCount	 = 0;
	biosdata.tableOffset = 0xDEADBEEF;

	SetWindowText(hwnd_loadtext, "Parsing File Table...");
	SendMessage(hwnd_loadprog, PBM_SETRANGE32, 0, biosdata.imageSize);
	SendMessage(hwnd_loadprog, PBM_SETPOS, 0, 0);

	// first, determine the offset of the file table
	ptr  = biosdata.imageData;
	done = FALSE;

	nextUpdate = ptr + 1024;

	while (!done)
	{
		if (!memcmp(ptr + 2, "-lh", 3))
		{
			biosdata.tableOffset = (ptr - biosdata.imageData);
			done = TRUE;
		}
		else
		{
			if ((ulong)(ptr - biosdata.imageData) >= biosdata.imageSize)
				done = TRUE;
		}

		ptr++;

		if (ptr >= nextUpdate)
		{
			SendMessage(hwnd_loadprog, PBM_SETPOS, (WPARAM)(ptr - biosdata.imageData), 0);
			nextUpdate = ptr + 1024;
		}
	}

	if (biosdata.tableOffset == 0xDEADBEEF)
	{
		MessageBox(hwnd, "Unable to locate a file table within the BIOS image!\n"
			"It is possible that this version of the editor simply does not support this type.\n\n"
			"Please check the homepage listed under Help->About and see if a new version is\n"
			"available for download.", "Error", MB_OK);

		DestroyWindow(loaddlg);
		return TRUE;
	}

	SendMessage(hwnd_loadprog, PBM_SETPOS, 0, 0);

	// next, determine the total size of the file table and file count, and try to determine the layout
	ptr  = biosdata.imageData + biosdata.tableOffset;
	done = FALSE;
	while (!done)
	{
		lzhhdr  = (lzhHeader *)ptr;
		lzhhdra = (lzhHeaderAfterFilename *) ((lzhhdr->filename) + lzhhdr->filenameLen);

		if ((lzhhdr->headerSize == 0) || (lzhhdr->headerSize == 0xFF))
			done = TRUE;
		else
		{
			if (lzhCalcSum(ptr + 2, lzhhdr->headerSize) != lzhhdr->headerSum)
			{
				MessageBox(hwnd, "BIOS Image Checksum failed!\n\nThis BIOS Image may be corrupted or damaged.  The editor will still continue to load\n"
					"the image, but certain components may not be editable.", "Notice", MB_OK);
			}

			// advance to next file
			biosdata.fileCount++;
			ptr += (2 + lzhhdr->headerSize + lzhhdr->compressedSize);

			// see how many bytes are needed to get to the next file, and adjust the type if necessary...
			if (biosdata.fileCount == 1)
			{
				// first file... could be anything...
				if (!memcmp(ptr + 4, "-lh", 3))
				{
					biosdata.layout = LAYOUT_2_2_2;
					ptr += 2;
				}
				else if (!memcmp(ptr + 3, "-lh", 3))
				{
					biosdata.layout = LAYOUT_1_1_1;
					ptr++;
				}
			}
			else
			{
				// next file, so we have some constraints to work with.
				if (!memcmp(ptr + 4, "-lh", 3))
				{
					if (biosdata.layout == LAYOUT_2_2_2)
					{
						// continue with 2_2_2...
						ptr += 2;
					}
					else
					{
						// uh-oh, this is a new one!
						biosdata.layout = LAYOUT_UNKNOWN;
					}
				}
				else if (!memcmp(ptr + 3, "-lh", 3))
				{
					if (biosdata.layout == LAYOUT_2_2_2)
					{
						if (biosdata.fileCount == 2)
						{
							// ok, we can switch here.
							biosdata.layout = LAYOUT_2_1_1;
							ptr++;
						}
						else
						{
							// hmm... don't know this one either!
							biosdata.layout = LAYOUT_UNKNOWN;
						}
					}
					else if (biosdata.layout == LAYOUT_2_1_1)
					{
						// no problems...
						ptr++;
					}
					else if (biosdata.layout == LAYOUT_1_1_1)
					{
						// no problems here either...
						ptr++;
					}
				}
				else
				{
					switch (biosdata.layout)
					{
						case LAYOUT_2_2_2:
//							if ( (*((ulong *) (ptr + 2)) == 0xFFFFFFFF) || (*((ulong *) (ptr + 2)) == 0x00000000) )
							if ( (*(ptr + 2) == 0xFF) || (*(ptr + 2) == 0x00) )
							{
								// ok, end of file table.
								ptr += 2;
							}
							else
							{
								// not good!
								biosdata.layout = LAYOUT_UNKNOWN;
							}
							break;

						case LAYOUT_2_1_1:
						case LAYOUT_1_1_1:
//							if ( (*((ulong *) (ptr + 1)) == 0xFFFFFFFF) || (*((ulong *) (ptr + 1)) == 0x00000000) )
							if ( (*(ptr + 1) == 0xFF) || (*(ptr + 1) == 0x00) )
							{
								// ok, end of file table.
								ptr++;
							}
							else
							{
								// not good!
								biosdata.layout = LAYOUT_UNKNOWN;
							}
							break;
					}
				}
			}
		}

		SendMessage(hwnd_loadprog, PBM_SETPOS, (WPARAM)(ptr - biosdata.imageData), 0);
	}

	// check for a valid layout
	if (biosdata.layout == LAYOUT_UNKNOWN)
	{
		MessageBox(hwnd, "Unable to determine the layout of the file table within the BIOS Image!\n"
			"It is possible that this version of the editor simply does not support this type.\n\n"
			"Please check the homepage listed under Help->About and see if a new version is\n"
			"available for download.", "Error", MB_OK);

		DestroyWindow(loaddlg);
		return TRUE;
	}

	// allocate our file table space...
	SetWindowText(hwnd_loadtext, "Loading File Table...");
	SendMessage(hwnd_loadprog, PBM_SETRANGE32, 0, biosdata.imageSize);
	SendMessage(hwnd_loadprog, PBM_SETPOS, 0, 0);

	biosdata.fileTable = new fileEntry[biosdata.fileCount];

	// decompress and load the file table into memory...
	ptr		= biosdata.imageData + biosdata.tableOffset;
	curFile = 0;
	done	= FALSE;

	while (!done)
	{
		lzhhdr  = (lzhHeader *)ptr;
		lzhhdra = (lzhHeaderAfterFilename *) ((lzhhdr->filename) + lzhhdr->filenameLen);

		if ((lzhhdr->headerSize == 0) || (lzhhdr->headerSize == 0xFF))
		{
			done = TRUE;
		}
		else
		{
			// fill out fileentry for this file
			fe = &biosdata.fileTable[curFile];
			biosWriteEntry(fe, lzhhdr, 0);

			// advance to next file
			ptr += (2 + lzhhdr->headerSize + lzhhdr->compressedSize);

			// skip past extra data
			switch (biosdata.layout)
			{
				case LAYOUT_2_2_2:
					ptr += 2;
					break;

				case LAYOUT_2_1_1:
					if (curFile == 0)
					{
						ptr += 2;
					}
					else
					{
						ptr++;
					}
					break;

				case LAYOUT_1_1_1:
					ptr++;
					break;
			}

			curFile++;
		}

		SendMessage(hwnd_loadprog, PBM_SETPOS, (WPARAM)(ptr - biosdata.imageData), 0);
	}

	// calculate available table space
	biosdata.maxTableSize = (biosdata.imageSize - biosdata.tableOffset) - (decompBlockSize + bootBlockSize);

	// scan for fixed-offset components
	SetWindowText(hwnd_loadtext, "Scanning for fixed components...");

	// continue until we hit the end of the image
	nextUpdate = ptr + 1024;

	while (ptr < (biosdata.imageData + (biosdata.imageSize - 6)))
	{
		if (!memcmp(ptr + 2, "-lh", 3) && (*(ptr + 6) == '-'))
		{
			// found something... maybe...
			lzhhdr  = (lzhHeader *)ptr;
			lzhhdra = (lzhHeaderAfterFilename *) ((lzhhdr->filename) + lzhhdr->filenameLen);

			if ((lzhhdr->headerSize != 0) && (lzhhdr->headerSize != 0xFF))
			{
				// looks somewhat okay -- check the checksum
				if (lzhCalcSum(ptr + 2, lzhhdr->headerSize) == lzhhdr->headerSum)
				{
					// we found something!  add it to our table
					fe = biosExpandTable();
					biosWriteEntry(fe, lzhhdr, (ulong)(ptr - biosdata.imageData));

					// if this offset is less than our maximum table size, then adjust the size appropriately...
					// (note: the dynamic file table cannot exceed the space occupied by any fixed components)
					if (biosdata.maxTableSize > fe->offset)
						biosdata.maxTableSize = fe->offset;

					// advance pointer past this file
					ptr += (2 + lzhhdr->headerSize + lzhhdr->compressedSize);
				}
			}
		}

		ptr++;

		if (ptr >= nextUpdate)
		{
			SendMessage(hwnd_loadprog, PBM_SETPOS, (WPARAM)(ptr - biosdata.imageData), 0);
			nextUpdate = ptr + 1024;
		}
	}

	// insert the decompression and boot blocks
	if (decompBlockData != NULL)
	{
		fe = biosExpandTable();
		fe->nameLen = strlen("decomp_blk.bin");
		fe->name = new char[fe->nameLen + 1];
		strcpy(fe->name, "decomp_blk.bin");

		fe->size	 = decompBlockSize;
		fe->compSize = 0;
		fe->type	 = TYPEID_DECOMPBLOCK;
		fe->crc		 = 0;
		fe->crcOK	 = TRUE;
		fe->data	 = (void *)new uchar[fe->size];
		fe->offset	 = 0;
		fe->flags	 = FEFLAGS_DECOMP_BLOCK;

		memcpy(fe->data, decompBlockData, decompBlockSize);
		delete []decompBlockData;
	}

	if (bootBlockData != NULL)
	{
		fe = biosExpandTable();
		fe->nameLen = strlen("boot_blk.bin");
		fe->name = new char[fe->nameLen + 1];
		strcpy(fe->name, "boot_blk.bin");

		fe->size	 = bootBlockSize;
		fe->compSize = 0;
		fe->type	 = TYPEID_BOOTBLOCK;
		fe->crc		 = 0;
		fe->crcOK	 = TRUE;
		fe->data	 = (void *)new uchar[fe->size];
		fe->offset	 = 0;
		fe->flags	 = FEFLAGS_BOOT_BLOCK;

		memcpy(fe->data, bootBlockData, bootBlockSize);
		delete []bootBlockData;
	}

	// kill our window
    DestroyWindow(loaddlg);

	// enable all editing controls
	enableControls(TRUE, TRUE);

	// call all plugins' onLoad functions
	pluginCallOnLoad(biosdata.fileTable, biosdata.fileCount);

	// populate the component list with the files in our table
	biosUpdateComponents();

	// zap the modified flag and update our main window's title and status bar...
	biosSetModified(FALSE);

	// and cleanup our temp directory!
	cleanTempPath();

	return TRUE;
}

bool biosOpen(void)
{
	OPENFILENAME ofn;
	char fname[256], *sptr, *dptr;

	// warn if the current bios has been modified
	if (biosHandleModified(biosChangedText) == FALSE)
		return FALSE;

	// display the open dialog
	fname[0] = 0;
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize			= sizeof(OPENFILENAME);
    ofn.hwndOwner			= hwnd;
	ofn.hInstance			= hinst;
    ofn.lpstrFilter			= "Award BIOS Image (*.awd,*.bin)\0*.awd;*.bin\0All Files (*.*)\0*.*\0\0";
	ofn.lpstrCustomFilter	= NULL;
	ofn.nMaxCustFilter		= 0;
	ofn.nFilterIndex		= 1;
    ofn.lpstrFile			= fname;
    ofn.nMaxFile			= 256;
	ofn.lpstrFileTitle		= NULL;
	ofn.nMaxFileTitle		= 0;
	ofn.lpstrInitialDir		= (config.lastPath[0] == 0) ? NULL : config.lastPath;
	ofn.lpstrTitle			= NULL;
    ofn.Flags				= OFN_FILEMUSTEXIST | OFN_ENABLESIZING | OFN_EXPLORER;
	ofn.nFileOffset			= 0;
	ofn.nFileExtension		= 0;
	ofn.lpstrDefExt			= NULL;
	ofn.lCustData			= NULL;
	ofn.lpfnHook			= NULL;
	ofn.lpTemplateName		= NULL;

    if (GetOpenFileName(&ofn) == FALSE)
		return FALSE;

	// strip out path from returned filename
	sptr = strchr(fname, '\\');
	if (sptr != NULL)
	{
		sptr++;
		strcpy(config.lastPath, fname);

		sptr = config.lastPath;
		do
		{
			dptr = strchr(sptr, '\\');
			if (dptr) sptr = dptr + 1;
		} while (dptr != NULL);

		*sptr = 0;
	}

	// zap the modified flag
	biosdata.modified = FALSE;

	// and call the open file handler...
	return biosOpenFile(fname);
}

void biosRevert(void)
{
	biosdata.modified = FALSE;
	biosOpenFile(biosdata.fname);
}

void biosWriteComponent(fileEntry *fe, FILE *fp, int fileIdx)
{
	uchar *tempbuf;
	ulong tempbufsize, usedsize;
	lzhErr err;
	lzhHeader *lzhhdr;
	uchar csum, *cptr, ebcount;
	ulong clen;

	// alloc a temp buffer for compression (assume file can't be compressed at all)
	tempbufsize	= fe->size;
	tempbuf		= new uchar[tempbufsize + sizeof(lzhHeader) + sizeof(lzhHeaderAfterFilename) + 256];

	// compress this file
	err = lzhCompress(fe->name, fe->nameLen, fe->data, fe->size, tempbuf, tempbufsize, &usedsize);

	// update the type and then fix the header sum
	lzhhdr = (lzhHeader *)tempbuf;
	lzhhdr->fileType  = fe->type;
	lzhhdr->headerSum = lzhCalcSum(tempbuf + 2, lzhhdr->headerSize);

	// write it to the output
	fwrite(tempbuf, 1, usedsize, fp);

	// calculate checksum over LZH header and compressed data
	cptr = tempbuf;
	clen = usedsize;
	csum = 0x00;
	while (clen--)
		csum += *cptr++;

	// write extra bytes, depending on the layout
	if (fileIdx != -1)
	{
		switch (biosdata.layout)
		{
			case LAYOUT_2_2_2:
				ebcount = 2;
				break;

			case LAYOUT_2_1_1:
				if (fileIdx == 0)
					ebcount = 2;
				else
					ebcount = 1;
				break;

			case LAYOUT_1_1_1:
				ebcount = 1;
				break;
		}
	}
	else
	{
		// always write 2 extra bytes
		ebcount = 2;
	}

	if (ebcount > 0)
	{
		fputc(0x00, fp);

		if (ebcount > 1)
			fputc(csum, fp);
	}

	// free our buffer
	delete []tempbuf;
}

bool biosSaveFile(char *fname)
{
    HWND loaddlg, hwnd_loadtext, hwnd_loadprog;
	FILE *fp;
	int t, pos, count;
	fileEntry *fe;
	ulong decompSize, bootSize;
	uchar ch, csum1, csum2, rcs1, rcs2;
	char buf[256];

	// open the file
	fp = fopen(fname, "wb");
	if (fp == NULL)
	{
		MessageBox(hwnd, "Unable to write BIOS image!", "Error", MB_OK);
		return FALSE;
	}

	// put up our saving dialog and initialize it
    loaddlg = CreateDialog(hinst, MAKEINTRESOURCE(IDD_WORKING), hwnd, (DLGPROC)LoadSaveProc);
    
	SetWindowText(loaddlg, "Saving Image...");
    hwnd_loadtext = GetDlgItem(loaddlg, IDC_LOADING_TEXT);
    hwnd_loadprog = GetDlgItem(loaddlg, IDC_LOADING_PROGRESS);

	SetWindowText(hwnd_loadtext, "Writing components...");
	SendMessage(hwnd_loadprog, PBM_SETRANGE32, 0, biosdata.fileCount);

	// first, flat out write the loaded image to restore extra code/data segments we couldn't load
	fwrite(biosdata.imageData, 1, biosdata.imageSize, fp);
	rewind(fp);

	// fill in FFh until we reach the start of the file table
	t = biosdata.tableOffset;
	while (t--)
		fputc(0xFF, fp);

	// iterate through all files with no fixed offset and no special flags, compress them, and write them
	for (t = 0; t < biosdata.fileCount; t++)
	{
		SendMessage(hwnd_loadprog, PBM_SETPOS, t, 0);

		fe = &biosdata.fileTable[t];
		if ((fe->offset == 0) && (fe->flags == 0))
			biosWriteComponent(fe, fp, t);
	}

	// write the decompression and boot blocks...
	SetWindowText(hwnd_loadtext, "Writing boot/decomp blocks...");

	fe = biosScanForID(TYPEID_DECOMPBLOCK);
	decompSize = ((fe == NULL) ? (0) : (fe->size));

	fe = biosScanForID(TYPEID_BOOTBLOCK);
	bootSize = ((fe == NULL) ? (0) : (fe->size));

	fseek(fp, biosdata.imageSize - (decompSize + bootSize), 0);

	// write the blocks
	fe = biosScanForID(TYPEID_DECOMPBLOCK);
	if (fe != NULL)
		fwrite(fe->data, 1, fe->size, fp);

	fe = biosScanForID(TYPEID_BOOTBLOCK);
	if (fe != NULL)
		fwrite(fe->data, 1, fe->size, fp);

	// now write components which have a fixed offset
	SetWindowText(hwnd_loadtext, "Writing fixed components...");

	for (t = 0; t < biosdata.fileCount; t++)
	{
		SendMessage(hwnd_loadprog, PBM_SETPOS, t, 0);

		fe = &biosdata.fileTable[t];
		if (fe->offset != 0)
		{
			fseek(fp, fe->offset, SEEK_SET);
			biosWriteComponent(fe, fp, -1);
		}
	}

	// finally, if the BIOS is version 6.00PG, update the internal checksum in the decompression block...
	if (biosGetVersion() == awdbeBIOSVer600PG)
	{
		fe = biosScanForID(TYPEID_DECOMPBLOCK);
		if (fe != NULL)
		{
			// re-open the file in read-only mode
			fclose(fp);
			fp = fopen(fname, "rb");

			// calculate the position of the checksum bytes
			pos = ((biosdata.imageSize - (decompSize + bootSize)) & 0xFFFFF000) + 0xFFE;

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			// calculate the checksum
			csum1 = 0x00;
			csum2 = 0xF8; //0x6E;
			count = pos;

			while (count--)
			{
				ch = fgetc(fp);
				csum1 += ch;
				csum2 += ch;
			}

#if 0
			rcs1 = fgetc(fp);
			rcs2 = fgetc(fp);

			sprintf(buf, "Current checksum: %02X %02X\nCalculated checksum: %02X %02X", rcs1, rcs2, csum1, csum2);
			MessageBox(hwnd, buf, "Notice", MB_OK);
#else

			// re-open the file in read-write mode
			fclose(fp);
			fp = fopen(fname, "r+b");

			// seek to the checksum position
			fseek(fp, pos, 0);

			// write the checksum bytes
			fputc(csum1, fp);
			fputc(csum2, fp);
#endif
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		}
	}

	// close the file
	fclose(fp);

	// kill our window
    DestroyWindow(loaddlg);

	return TRUE;
}

bool biosSave(void)
{
	bool ret;

	// update any current data
	biosUpdateCurrentDialog();

	// zap the modified flag
	biosdata.modified = FALSE;

	// and call the save file handler...
	ret = biosSaveFile(biosdata.fname);

	// if successful, update the title bar (removes the * mark)
	if (ret == TRUE)
		biosTitleUpdate();

	// return result
	return ret;
}

bool biosSaveAs(void)
{
	OPENFILENAME ofn;
	char fname[256];
	bool ret;

	// update any current data
	biosUpdateCurrentDialog();

	// display the save dialog
	fname[0] = 0;
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize			= sizeof(OPENFILENAME);
    ofn.hwndOwner			= hwnd;
	ofn.hInstance			= hinst;
    ofn.lpstrFilter			= "Award BIOS Image (*.awd,*.bin)\0*.awd;*.bin\0All Files (*.*)\0*.*\0\0";
	ofn.lpstrCustomFilter	= NULL;
	ofn.nMaxCustFilter		= 0;
	ofn.nFilterIndex		= 1;
    ofn.lpstrFile			= fname;
    ofn.nMaxFile			= 256;
	ofn.lpstrFileTitle		= NULL;
	ofn.nMaxFileTitle		= 0;
	ofn.lpstrInitialDir		= (config.lastPath[0] == 0) ? NULL : config.lastPath;
	ofn.lpstrTitle			= NULL;
    ofn.Flags				= OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_ENABLESIZING | OFN_EXPLORER;
	ofn.nFileOffset			= 0;
	ofn.nFileExtension		= 0;
	ofn.lpstrDefExt			= "awd";
	ofn.lCustData			= NULL;
	ofn.lpfnHook			= NULL;
	ofn.lpTemplateName		= NULL;

    if (GetSaveFileName(&ofn) == FALSE)
		return FALSE;

	// zap the modified flag
	biosdata.modified = FALSE;

	// and call the save file handler...
	ret = biosSaveFile(fname);

	// if successful, update the filename with the new one
	if (ret == TRUE)
	{
		strcpy(biosdata.fname, fname);
		biosTitleUpdate();
	}

	// return result
	return ret;
}

BOOL APIENTRY LayoutProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	biosLayout oldLayout;

    switch (message)
    {
		case WM_INITDIALOG:
			switch (biosdata.layout)
			{
				case LAYOUT_2_2_2: CheckDlgButton(hdlg, IDC_LAYOUT_2_2_2, BST_CHECKED); break;
				case LAYOUT_2_1_1: CheckDlgButton(hdlg, IDC_LAYOUT_2_1_1, BST_CHECKED); break;
				case LAYOUT_1_1_1: CheckDlgButton(hdlg, IDC_LAYOUT_1_1_1, BST_CHECKED); break;
			}
			break;

        case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					// save the current layout
					oldLayout = biosdata.layout;

					// check for a new one
					if (IsDlgButtonChecked(hdlg, IDC_LAYOUT_2_2_2) == BST_CHECKED)
						biosdata.layout = LAYOUT_2_2_2;
					else if (IsDlgButtonChecked(hdlg, IDC_LAYOUT_2_1_1) == BST_CHECKED)
						biosdata.layout = LAYOUT_2_1_1;
					else if (IsDlgButtonChecked(hdlg, IDC_LAYOUT_1_1_1) == BST_CHECKED)
						biosdata.layout = LAYOUT_1_1_1;

					// if different, update the modified flag
					if (biosdata.layout != oldLayout)
						biosSetModified(TRUE);

					// fall through to end dialog...

				case IDCANCEL:
					EndDialog(hdlg, TRUE);
					break;
			}
            return TRUE;
	}

	return FALSE;
}

BOOL APIENTRY PropertiesProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT rc, rc2;
	char buf[256];
	LVITEM lvi;
	int t, imgid = 0, boxsize, bytesperpixel, curX, heightOffset;
	fileEntry *fe, *fe2;
	ulong bootSize, decompSize;
	PAINTSTRUCT ps;
	HDC hdc;
	HIMAGELIST himl;
	HICON hicon;
	POINT pt[5];
	HBRUSH backBrush;
	COLORREF cref;
	HFONT hfnt;
	TEXTMETRIC tm;

	int iconTable[] = {
		IDI_BLOCK_RED, IDI_BLOCK_GREEN, IDI_BLOCK_BLUE, IDI_BLOCK_YELLOW, IDI_BLOCK_MAGENTA, IDI_BLOCK_CYAN,
		IDI_BLOCK_DKRED, IDI_BLOCK_DKGREEN, IDI_BLOCK_DKBLUE, IDI_BLOCK_DKYELLOW, IDI_BLOCK_DKMAGENTA, IDI_BLOCK_DKCYAN,
		IDI_BLOCK_GREY, IDI_BLOCK_DKGREY
	};

	COLORREF colorTable[] = {
		RGB(255, 0, 0), RGB(0, 255, 0), RGB(0, 0, 255), RGB(255, 255, 0), RGB(255, 0, 255), RGB(0, 255, 255),
		RGB(128, 0, 0), RGB(0, 128, 0), RGB(0, 0, 128), RGB(128, 128, 0), RGB(128, 0, 128), RGB(0, 128, 128),
		RGB(192, 192, 192), RGB(128, 128, 128)
	};

	LVCOLUMN colList[] = {
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT, 110, "Filename",	0, 0, 0, 0 },
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT,  50, "Size",		0, 1, 0, 0 },
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT,  72, "Compressed",	0, 2, 0, 0 },
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT,  65, "ID",			0, 3, 0, 0 },
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT,  45, "CRC",		0, 4, 0, 0 },
		{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, LVCFMT_LEFT,  54, "Check",		0, 5, 0, 0 },
	};

    switch (message)
    {
		case WM_PAINT:
			hdc = BeginPaint(hdlg, &ps);

			// get rect coords to find where to put the file table list box
			GetWindowRect(hdlg, &rc);
			GetWindowRect(GetDlgItem(hdlg, IDC_FILE_GRAPH), &rc2);

			// adjust to get local space in the dialog
			rc2.left -= rc.left;
			rc2.top -= rc.top;
			rc2.right -= rc.left;
			rc2.bottom -= rc.top;

			// adjust window with magic
			rc2.left -= 4;
			rc2.top -= 30;
			rc2.right -= 2;
			rc2.bottom -= 30;

			// draw the border of the graph in black
			SelectObject(hdc, GetStockObject(BLACK_PEN));

			pt[0].x = rc2.left;		pt[0].y = rc2.top;
			pt[1].x = rc2.right;	pt[1].y = rc2.top;
			pt[2].x = rc2.right;	pt[2].y = rc2.bottom;
			pt[3].x = rc2.left;		pt[3].y = rc2.bottom;
			pt[4].x = rc2.left;		pt[4].y = rc2.top;
			Polyline(hdc, pt, 5);

			// fill the inside in white
			backBrush = CreateSolidBrush(RGB(255, 255, 255));

			rc2.left++;
			rc2.top++;
			FillRect(hdc, &rc2, backBrush);
			DeleteObject(backBrush);

			// now draw all of the components
			curX = rc2.left;
			boxsize = rc2.right - rc2.left;			
			bytesperpixel = biosdata.imageSize / boxsize;
			memcpy(&rc, &rc2, sizeof(RECT));

			// advance left pointer for offset of file table
			rc.left += biosdata.tableOffset / bytesperpixel;

			for (t = 0; t < biosdata.fileCount; t++)
			{
				fe = &biosdata.fileTable[t];

				// pick the correct color
				if (fe->flags & FEFLAGS_DECOMP_BLOCK)
					cref = colorTable[12];
				else if (fe->flags & FEFLAGS_BOOT_BLOCK)
					cref = colorTable[13];
				else
				{
					cref = colorTable[imgid++];
					if (imgid >= 12)
						imgid = 0;
				}

				// create a brush in this color
				backBrush = CreateSolidBrush(cref);

				// check for a fixed offset or flags
				if (fe->offset == 0)
				{
					if (fe->flags == 0)
					{
						// a normal block... calculate how far we have to draw
						rc.right = rc.left + (fe->compSize / bytesperpixel);
						curX	 = rc.right;
					}
					else
					{
						// this is a decomp or boot block
						if (fe->flags & FEFLAGS_DECOMP_BLOCK)
						{
							// we need the size of the boot block first
							fe2 = biosScanForID(TYPEID_BOOTBLOCK);
							bootSize = ((fe2 == NULL) ? (0) : (fe2->size));

							rc.right = rc2.right - (fe2->size / bytesperpixel);
							rc.left  = rc.right - (fe->size / bytesperpixel);
						}
						else if (fe->flags & FEFLAGS_BOOT_BLOCK)
						{
							rc.left  = rc2.right - (fe->size / bytesperpixel);
							rc.right = rc2.right - 1;
						}
					}
				}
				else
				{
					// this is a fixed block
					rc.left  = rc2.left + (fe->offset / bytesperpixel);
					rc.right = rc.left + (fe->compSize / bytesperpixel);
				}

				rc.right++;
				FillRect(hdc, &rc, backBrush);
				DeleteObject(backBrush);

				// update drawing position
				if (rc.left == curX)
					rc.left = curX + 1;
				else
					rc.left = curX;
			}

			// draw some line markers
			pt[0].y = rc2.bottom;
			pt[1].y = rc2.bottom + 8;
			SelectObject(hdc, GetStockObject(BLACK_PEN));

			pt[0].x = rc2.left - 1;
			pt[1].x = rc2.left - 1;
			Polyline(hdc, pt, 2);

			pt[0].x = rc2.right;
			pt[1].x = rc2.right;
			Polyline(hdc, pt, 2);

			pt[0].x = (rc2.left + rc2.right) / 2;
			pt[1].x = (rc2.left + rc2.right) / 2;
			Polyline(hdc, pt, 2);

			// draw informational text size markers at the lines
			hfnt = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
			SelectObject(hdc, hfnt);
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, RGB(0, 0, 0));

			GetTextMetrics(hdc, &tm);
			heightOffset = 15 - tm.tmHeight;

			rc.left = rc2.left - 12;
			rc.top  = (rc.bottom + 8) + heightOffset;
			DrawText(hdc, "0Mbit", -1, &rc, DT_NOCLIP | DT_TOP | DT_LEFT);

			rc.left = ((rc2.left + rc2.right) / 2) - 12;
			t = biosdata.imageSize / 131072;
			if (t == 1)
				DrawText(hdc, "0.5Mbit", -1, &rc, DT_NOCLIP | DT_TOP | DT_LEFT);
			else
			{
				sprintf(buf, "%dMbit", t / 2);
				DrawText(hdc, buf, -1, &rc, DT_NOCLIP | DT_TOP | DT_LEFT);
			}

			rc.left = rc2.right - 12;
			sprintf(buf, "%dMbit", t);
			DrawText(hdc, buf, -1, &rc, DT_NOCLIP | DT_TOP | DT_LEFT);

			// done!
			EndPaint(hdlg, &ps);
			return 0;

        case WM_INITDIALOG:
			SetWindowText(GetDlgItem(hdlg, IDC_TEXT_FILENAME), biosdata.fname);

			sprintf(buf, "%d bytes (%dMbit)", biosdata.imageSize, biosdata.imageSize / 131072);
			SetWindowText(GetDlgItem(hdlg, IDC_TEXT_SIZE), buf);

			switch (biosdata.layout)
			{
				case LAYOUT_2_2_2: sprintf(buf, "2-2-2"); break;
				case LAYOUT_2_1_1: sprintf(buf, "2-1-1"); break;
				case LAYOUT_1_1_1: sprintf(buf, "1-1-1"); break;
				default:		   sprintf(buf, "UNKNOWN"); break;
			}

			SetWindowText(GetDlgItem(hdlg, IDC_TEXT_LAYOUT), buf);
			
			sprintf(buf, "%08Xh (%d)\n", biosdata.tableOffset, biosdata.tableOffset);
			SetWindowText(GetDlgItem(hdlg, IDC_TEXT_OFFSET), buf);

			fe = biosScanForID(TYPEID_BOOTBLOCK);
			bootSize = ((fe == NULL) ? (0) : (fe->size));
			sprintf(buf, "%d bytes", bootSize);
			SetWindowText(GetDlgItem(hdlg, IDC_TEXT_BOOT_BLOCK), buf);
			
			fe = biosScanForID(TYPEID_DECOMPBLOCK);
			decompSize = ((fe == NULL) ? (0) : (fe->size));
			sprintf(buf, "%d bytes", decompSize);
			SetWindowText(GetDlgItem(hdlg, IDC_TEXT_DECOMPRESSION_BLOCK), buf);

		//===========================================
		// create our file table view

			himl = ImageList_Create(12, 12, TRUE, 1, 1);

			for (t = 0; t < 14; t++)
			{
				hicon = LoadIcon(hinst, MAKEINTRESOURCE(iconTable[t]));
				ImageList_AddIcon(himl, hicon);
				DeleteObject(hicon);
			}

			// get rect coords to find where to put the file table list box
			GetWindowRect(hdlg, &rc);
			GetWindowRect(GetDlgItem(hdlg, IDC_FILE_TABLE), &rc2);

			// adjust to get local space in the dialog
			rc2.left -= rc.left;
			rc2.top -= rc.top;
			rc2.right -= rc.left;
			rc2.bottom -= rc.top;

			// adjust window with magic
			rc2.left -= 4;
			rc2.top -= 20;
			rc2.right -= 2;
			rc2.bottom -= 30;

			// create our file list
			hPropDlgListWnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "", WS_VISIBLE | WS_CHILD | LVS_REPORT, rc2.left, rc2.top,
				rc2.right - rc2.left, rc2.bottom - rc2.top, hdlg, NULL, hinst, NULL);

			// set full-row select
			SendMessage(hPropDlgListWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

			// set the image list
			SendMessage(hPropDlgListWnd, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)himl);

			// add the columns
			for (t = 0; t < 6; t++)
				SendMessage(hPropDlgListWnd, LVM_INSERTCOLUMN, t, (LPARAM)&colList[t]);

			// set the number of files
			SendMessage(hPropDlgListWnd, LVM_SETITEMCOUNT, biosdata.fileCount, 0);

			// add the files
			lvi.mask  = LVIF_TEXT | LVIF_IMAGE;
			lvi.iItem = 0;

			for (t = 0; t < biosdata.fileCount; t++)
			{
				fe = &biosdata.fileTable[t];

				// filename...
				if (fe->flags & FEFLAGS_DECOMP_BLOCK)
					lvi.iImage = 12;
				else if (fe->flags & FEFLAGS_BOOT_BLOCK)
					lvi.iImage = 13;
				else
				{
					lvi.iImage = imgid++;
					if (imgid >= 12)
						imgid = 0;
				}

				lvi.iSubItem = 0;
				lvi.pszText  = fe->name;
				SendMessage(hPropDlgListWnd, LVM_INSERTITEM, 0, (LPARAM)&lvi);

				// size...
				sprintf(buf, "%d", fe->size);
				lvi.iSubItem = 1;
				lvi.pszText  = buf;
				SendMessage(hPropDlgListWnd, LVM_SETITEM, 0, (LPARAM)&lvi);

				// compressed size...
				sprintf(buf, "%d", fe->compSize);
				lvi.iSubItem = 2;
				lvi.pszText  = buf;
				SendMessage(hPropDlgListWnd, LVM_SETITEM, 0, (LPARAM)&lvi);

				// ID...
				sprintf(buf, "%04X", fe->type);
				lvi.iSubItem = 3;
				lvi.pszText  = buf;
				SendMessage(hPropDlgListWnd, LVM_SETITEM, 0, (LPARAM)&lvi);

				// CRC...
				sprintf(buf, "%04X", fe->crc);
				lvi.iSubItem = 4;
				lvi.pszText  = buf;
				SendMessage(hPropDlgListWnd, LVM_SETITEM, 0, (LPARAM)&lvi);

				// CRC pass/fail...
				if (fe->crcOK == TRUE)
					lvi.pszText = "Pass";
				else
					lvi.pszText = "Fail";

				lvi.iSubItem = 5;
				SendMessage(hPropDlgListWnd, LVM_SETITEM, 0, (LPARAM)&lvi);

				// advance to next file
				lvi.iItem++;
			}
			return TRUE;

		case WM_CLOSE:
			// delete the imagelist that the file table display is using
			himl = (HIMAGELIST)SendMessage(hPropDlgListWnd, LVM_GETIMAGELIST, LVSIL_SMALL, 0);
			ImageList_Destroy(himl);

			if (hPropDlgListWnd != NULL)
			{
				DestroyWindow(hPropDlgListWnd);
				hPropDlgListWnd = NULL;
			}

			EndDialog(hdlg, TRUE);
			return TRUE;

        case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_LAYOUT_CHANGE:
					DialogBox(hinst, MAKEINTRESOURCE(IDD_LAYOUT_CHANGE), hdlg, LayoutProc);

					// update layout...
					switch (biosdata.layout)
					{
						case LAYOUT_2_2_2: sprintf(buf, "2-2-2"); break;
						case LAYOUT_2_1_1: sprintf(buf, "2-1-1"); break;
						case LAYOUT_1_1_1: sprintf(buf, "1-1-1"); break;
						default:		   sprintf(buf, "UNKNOWN"); break;
					}

					SetWindowText(GetDlgItem(hdlg, IDC_TEXT_LAYOUT), buf);
					break;

				case IDOK:
					PostMessage(hdlg, WM_CLOSE, 0, 0);
					break;
			}
            return TRUE;
    }

    return FALSE;
}

void biosProperties(void)
{
	if (biosdata.fileTable == NULL)
	{
		MessageBox(hwnd, "There is no BIOS Image loaded, so no property information is available.", "Notice", MB_OK);
		return;
	}

    DialogBox(hinst, MAKEINTRESOURCE(IDD_PROPERTIES), hwnd, PropertiesProc);
}

static bool isUnderRoot(HTREEITEM rootItem, HTREEITEM hitem)
{
	while ( (hitem = (HTREEITEM)SendMessage(treeView, TVM_GETNEXTITEM, TVGN_PARENT, (LPARAM)hitem)) != NULL)
	{
		if (hitem == rootItem)
			return TRUE;
	}
	
	return FALSE;
}

BOOL CALLBACK BiosInternalDialogFunc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	awdbeItem *item;

    switch (message)
    {
        case WM_INITDIALOG:
            return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_INSERT_FILE:
					// find the item that responds to the current hash
					item = pluginFindHash(curHash);
					if (item == NULL)
						return FALSE;

					// and create an insert dialog with the ID preset
					biosInsert(item->biosTypeID);
					break;
			}
			break;
    }

    return FALSE;
}

void biosUpdateCurrentDialog(void)
{
	HWND hedit;
	char buf[256];
	ulong len, data;
	ushort data16;
	awdbeItem *item;

	// first, update the data from our shared controls
	if ((curFileEntry != NULL) && (hModDlgWnd != NULL))
	{
		hedit = GetDlgItem(hModDlgWnd, IDC_FILENAME);
		if (IsWindow(hedit))
		{
			GetWindowText(hedit, buf, 256);
			len = strlen(buf);

			// compare strings
			if ( (len != curFileEntry->nameLen) || memcmp(buf, curFileEntry->name, len) )
			{
				delete []curFileEntry->name;

				curFileEntry->nameLen	= len;
				curFileEntry->name		= new char[curFileEntry->nameLen + 1];
				strcpy(curFileEntry->name, buf);

				biosSetModified(TRUE);
			}
		}

		hedit = GetDlgItem(hModDlgWnd, IDC_FILE_ID);
		if (IsWindow(hedit))
		{
			GetWindowText(hedit, buf, 256);
			sscanf(buf, "%04X", &data16);

			// compare data
			if (data16 != curFileEntry->type)
			{
				curFileEntry->type = data16;
				biosSetModified(TRUE);

				// since we changed the type ID of a component, we need to rebuild our list
				biosUpdateComponents();
			}
		}

		hedit = GetDlgItem(hModDlgWnd, IDC_FILE_OFFSET);
		if (IsWindow(hedit))
		{
			GetWindowText(hedit, buf, 256);
			sscanf(buf, "%08X", &data);

			// compare data
			if (data != curFileEntry->offset)
			{
				curFileEntry->offset = data;
				biosSetModified(TRUE);
			}
		}
	}

	// next, call the plugin to update its own data
	if (curHash > HASH_UNKNOWN_ITEM_MAX)
	{
		switch (curHash)
		{
			case HASH_SUBMENU_ITEM:
			case HASH_RECOGNIZED_ROOT:
			case HASH_UNKNOWN_ROOT:
			case HASH_INCLUDABLE_ROOT:
				// do nothing
				break;

			default:
				// find the item that responds to this hash
				item = pluginFindHash(curHash);
				if (item == NULL)
				{
					sprintf(buf, "pluginFindHash() returned NULL for hash=%08Xh.  This hash value should be in the switch()...", curHash);
					MessageBox(hwnd, buf, "Internal Error", MB_OK);
					return;
				}

				// update only if the old item was *not* under the "includable" tree (curFileEntry will be NULL if under an includable item)
				if (curFileEntry != NULL)
				{
					// call this plugin's update function
					if (pluginCallUpdateDialog(item, curFileEntry, hModDlgWnd) == TRUE)
						biosSetModified(TRUE);
				}
				break;
		}
	}	
}

void biosItemChanged(LPNMTREEVIEW lpnmtv)
{
	HWND hKillWnd, hedit;
	char buf[256];
	awdbeItem *item;
	RECT rcx;

	// guard against unwanted notifies
	if (ignoreNotify)
		return;

	// ignore NULL selects
	if (lpnmtv->itemNew.hItem == NULL)
		return;

	// update data from the current dialog (call plugin if necessary)
	biosUpdateCurrentDialog();

	// to prevent flicker when changing window panes, we store the handle of the window we want to kill first...
	// then we create the new window on top of the current one and then destroy the old one.
	hKillWnd = hModDlgWnd;

	// if the current hash points to a plugin, call it's "onDestroy" function to tell it it's about to be killed
	if ((curFileEntry != NULL) && (curHash > HASH_UNKNOWN_ITEM_MAX))
	{
		switch (curHash)
		{
			case HASH_SUBMENU_ITEM:
			case HASH_RECOGNIZED_ROOT:
			case HASH_UNKNOWN_ROOT:
			case HASH_INCLUDABLE_ROOT:
				break;

			default:
				// find the item that responds to this hash
				item = pluginFindHash(curHash);
				if (item != NULL)
				{
					// call this plugin's destroy dialog function to kill the window
					pluginCallOnDestroyDialog(item, hModDlgWnd);
				}
				break;
		}
	}

	// get the hash from the newly clicked item
	curHash = lpnmtv->itemNew.lParam;

	// check first for special hash values
	if (curHash <= HASH_UNKNOWN_ITEM_MAX)
	{
		curFileEntry = &biosdata.fileTable[curHash];
		hModDlgWnd   = CreateDialog(hinst, MAKEINTRESOURCE(IDDB_UNKNOWN), hwnd, BiosInternalDialogFunc);

		enableControls(TRUE, TRUE);
	}
	else
	{
		curFileEntry = NULL;

		switch (curHash)
		{
			case HASH_SUBMENU_ITEM:
				hModDlgWnd = CreateDialog(hinst, MAKEINTRESOURCE(IDDB_SUBMENU_ITEM), hwnd, BiosInternalDialogFunc);
				enableControls(TRUE, FALSE);
				break;

			case HASH_RECOGNIZED_ROOT:
				hModDlgWnd = CreateDialog(hinst, MAKEINTRESOURCE(IDDB_RECOGNIZED_ROOT), hwnd, BiosInternalDialogFunc);
				enableControls(TRUE, FALSE);
				break;

			case HASH_UNKNOWN_ROOT:
				hModDlgWnd = CreateDialog(hinst, MAKEINTRESOURCE(IDDB_UNKNOWN_ROOT), hwnd, BiosInternalDialogFunc);
				enableControls(TRUE, FALSE);
				break;

			case HASH_INCLUDABLE_ROOT:
				hModDlgWnd = CreateDialog(hinst, MAKEINTRESOURCE(IDDB_INCLUDABLE_ROOT), hwnd, BiosInternalDialogFunc);
				enableControls(TRUE, FALSE);
				break;

			default:
				// find the item that responds to this hash
				item = pluginFindHash(curHash);
				if (item == NULL)
				{
					sprintf(buf, "pluginFindHash() returned NULL for hash=%08Xh.  This hash value should be in the switch()...", curHash);
					MessageBox(hwnd, buf, "Internal Error", MB_OK);
					return;
				}

				// walk back up the tree to the top level, and see if this item is under the "includable" tree
				if (isUnderRoot(inclItem, lpnmtv->itemNew.hItem))
				{
					// this is an includable item, so we're obviously not pointing at any file in our table...
					curFileEntry = NULL;

					// put up the includable component dialog
					hModDlgWnd = CreateDialog(hinst, MAKEINTRESOURCE(IDDB_INCLUDABLE), hwnd, BiosInternalDialogFunc);

					enableControls(TRUE, FALSE);
				}
				else
				{
					// lookup this item's ID in our file table
					curFileEntry = biosScanForID(item->biosTypeID);

					// call this plugin's create dialog function to show the window
					hModDlgWnd = pluginCallCreateDialog(item, curFileEntry, hwnd, dlgrc);

					enableControls(TRUE, TRUE);
				}
				break;
		}
	}

	// move the new window on top and show it.
	SetWindowPos(hModDlgWnd, HWND_TOP, dlgrc->left, dlgrc->top, dlgrc->right, dlgrc->bottom, SWP_SHOWWINDOW);

	GetClientRect(hModDlgWnd, &rcx);
	pluginCallOnResizeDialog(item, hModDlgWnd, &rcx);

	// delete the old window
	DestroyWindow(hKillWnd);

	// pass focus to the dialog
	SetActiveWindow(hModDlgWnd);
	SetFocus(hModDlgWnd);

	// if we're not pointing at any file in our table, exit out here (and don't try to update the child dialog)
	if (curFileEntry == NULL)
		return;

	// get handles to the filename, type ID, and other controls, and update them.
	hedit = GetDlgItem(hModDlgWnd, IDC_FILENAME);
	if (IsWindow(hedit))
	{
		SetWindowText(hedit, curFileEntry->name);

		// if this is the decompression or boot block, disable the window to prevent changes to the name
		if ((curFileEntry->flags & FEFLAGS_DECOMP_BLOCK) || (curFileEntry->flags & FEFLAGS_BOOT_BLOCK))
			SendMessage(hedit, EM_SETREADONLY, TRUE, 0);
		else
			SendMessage(hedit, EM_SETREADONLY, FALSE, 0);
	}

	hedit = GetDlgItem(hModDlgWnd, IDC_FILE_ID);
	if (IsWindow(hedit))
	{
		sprintf(buf, "%04X", curFileEntry->type);
		SetWindowText(hedit, buf);

		// if this is the decompression or boot block, disable the window to prevent changes to the ID
		if ((curFileEntry->flags & FEFLAGS_DECOMP_BLOCK) || (curFileEntry->flags & FEFLAGS_BOOT_BLOCK))
			SendMessage(hedit, EM_SETREADONLY, TRUE, 0);
		else
			SendMessage(hedit, EM_SETREADONLY, FALSE, 0);
	}

	hedit = GetDlgItem(hModDlgWnd, IDC_FILE_SIZE);
	if (IsWindow(hedit))
	{
		sprintf(buf, "%d", curFileEntry->size);
		SetWindowText(hedit, buf);

		// this item cannot be modified
		SendMessage(hedit, EM_SETREADONLY, TRUE, 0);
	}

	hedit = GetDlgItem(hModDlgWnd, IDC_FILE_OFFSET);
	if (IsWindow(hedit))
	{
		sprintf(buf, "%08X", curFileEntry->offset);
		SetWindowText(hedit, buf);

		// if this is the decompression or boot block, disable the window to prevent changes to the offset
		if ((curFileEntry->flags & FEFLAGS_DECOMP_BLOCK) || (curFileEntry->flags & FEFLAGS_BOOT_BLOCK))
			SendMessage(hedit, EM_SETREADONLY, TRUE, 0);
		else
			SendMessage(hedit, EM_SETREADONLY, FALSE, 0);
	}
}

void biosTempCompressData(void *fname, ulong fnameLen, void *data, ulong size, ulong *compSize, ushort *crc)
{
	uchar *tempbuf;
	ulong tempbufsize, usedsize;
	lzhHeader *lzhhdr;
	lzhHeaderAfterFilename *lzhhdra;

	// do a temporary compression on this file to determine its compressed size and other stuff
	tempbufsize	= size;
	tempbuf		= new uchar[tempbufsize + sizeof(lzhHeader) + sizeof(lzhHeaderAfterFilename) + 256];
	lzhhdr		= (lzhHeader *)tempbuf;
	lzhhdra		= (lzhHeaderAfterFilename *) ((lzhhdr->filename) + lzhhdr->filenameLen);

	lzhCompress(fname, fnameLen, data, size, tempbuf, tempbufsize, &usedsize);

	*compSize	= lzhhdr->compressedSize;
	*crc		= lzhhdra->crc;

	delete []tempbuf;
}

bool biosAddComponent(char *fname, ushort id, ulong offset)
{
	char name[256], ext[256];
	FILE *fp;
	ulong size;
	fileEntry *fe;
	bool failedFit;

	// open the file
	fp = fopen(fname, "rb");
	if (fp == NULL)
		return FALSE;

	// get just the name of this file
	_splitpath(fname, NULL, NULL, name, ext);
	sprintf(fname, "%s%s", name, ext);

	// get the size too
	fseek(fp, 0, 2);
	size = ftell(fp);
	rewind(fp);

	// warn if the size is possibly too big
	if (size > (biosdata.imageSize * 8))
	{
		if (MessageBox(hwnd, "The size of the file you are trying to insert is possibly too big for the "
			"image loaded.\n\nDo you want to continue anyway?", "Notice", MB_YESNO) == IDNO)
		{
			return FALSE;
		}			
	}

	// create a new file table entry and read the file
	fe = biosExpandTable();

	fe->nameLen = strlen(fname);
	fe->name = new char[fe->nameLen + 1];
	strcpy(fe->name, fname);

	fe->size	 = size;
	fe->compSize = 0;
	fe->type	 = id;
	fe->crc		 = 0;
	fe->crcOK	 = TRUE;
	fe->data	 = (void *)new uchar[fe->size];
	fe->offset	 = offset;
	fe->flags	 = 0;

	fread(fe->data, 1, fe->size, fp);
	fclose(fp);

	// compress it
	biosTempCompressData(fe->name, fe->nameLen, fe->data, fe->size, &fe->compSize, &fe->crc);

	// now see if it fits
	failedFit = FALSE;

	if (fe->offset == 0)
	{
		if (fe->compSize > biosFreeSpace)
			failedFit = TRUE;
	}
	else
	{
		if ((fe->offset + fe->compSize) > biosdata.imageSize)
			failedFit = TRUE;
	}

	if (failedFit)
	{
		MessageBox(hwnd, "There is not enough free space in the BIOS image to insert this file.",
			"Error", MB_OK);

		biosRemoveEntry(fe);
		return FALSE;
	}

	return TRUE;
}

BOOL APIENTRY InsertProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char buf[256];
	OPENFILENAME ofn;
	ulong offset;
	ushort id;
	fileEntry *fe;
	int t;

    switch (message)
    {
		case WM_INITDIALOG:
			SetDlgItemText(hdlg, IDC_INSERT_OFFSET, "00000000");

			if (insertID != 0)
			{
				sprintf(buf, "%04X", insertID);
				SetDlgItemText(hdlg, IDC_INSERT_ID, buf);

				SetFocus(GetDlgItem(hdlg, IDC_INSERT_FILENAME));
			}
			else
			{
				SetDlgItemText(hdlg, IDC_INSERT_ID, "00000000");
			}
			break;

        case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_INSERT_SEARCH:
					// display an open dialog
					buf[0] = 0;
					ZeroMemory(&ofn, sizeof(OPENFILENAME));
					ofn.lStructSize			= sizeof(OPENFILENAME);
					ofn.hwndOwner			= hdlg;
					ofn.hInstance			= hinst;
					ofn.lpstrFilter			= "All Files (*.*)\0*.*\0\0";
					ofn.lpstrCustomFilter	= NULL;
					ofn.nMaxCustFilter		= 0;
					ofn.nFilterIndex		= 1;
					ofn.lpstrFile			= buf;
					ofn.nMaxFile			= 256;
					ofn.lpstrFileTitle		= NULL;
					ofn.nMaxFileTitle		= 0;
					ofn.lpstrInitialDir		= NULL;
					ofn.lpstrTitle			= NULL;
					ofn.Flags				= OFN_FILEMUSTEXIST | OFN_ENABLESIZING | OFN_EXPLORER;
					ofn.nFileOffset			= 0;
					ofn.nFileExtension		= 0;
					ofn.lpstrDefExt			= NULL;
					ofn.lCustData			= NULL;
					ofn.lpfnHook			= NULL;
					ofn.lpTemplateName		= NULL;

					if (GetOpenFileName(&ofn) == FALSE)
						return TRUE;

					SetDlgItemText(hdlg, IDC_INSERT_FILENAME, buf);
					break;

				case IDOK:
					// first, get our ID and check it
					GetDlgItemText(hdlg, IDC_INSERT_ID, buf, 256);
					sscanf(buf, "%04X", &id);

					if ((buf[0] == 0) || (id == 0))
					{
						MessageBox(hdlg, "An invalid type ID has been entered.  Please enter a value other than zero "
							"and try again.", "Error", MB_OK);

						return TRUE;
					}

					// make sure this type ID isn't being used
					for (t = 0; t < biosdata.fileCount; t++)
					{
						fe = &biosdata.fileTable[t];

						if (fe->type == id)
						{
							MessageBox(hdlg, "This type ID is already being used by this BIOS image.  Please choose a "
								"different ID and try again.", "Error", MB_OK);

							return TRUE;
						}
					}

					// next, get the offset and check it
					GetDlgItemText(hdlg, IDC_INSERT_OFFSET, buf, 256);
					sscanf(buf, "%08X", &offset);

					if (offset > biosdata.imageSize)
					{
						MessageBox(hdlg, "An invalid offset has been entered.  Please check the value and try again.",
							"Error", MB_OK);

						return TRUE;
					}

					// make sure the file specified exists
					GetDlgItemText(hdlg, IDC_INSERT_FILENAME, buf, 256);
					
					if ((buf[0] == 0) || (_access(buf, 04) != 0))
					{
						MessageBox(hdlg, "Unable to open the specified file!\n\nPlease check the path and/or filename "
							"entered and try again.", "Error", MB_OK);

						return TRUE;
					}

					if (biosAddComponent(buf, id, offset) == FALSE)
						return TRUE;

					// update our title and components...
					biosdata.modified = TRUE;

					biosUpdateComponents();
					biosTitleUpdate();

					// done!
					EndDialog(hdlg, TRUE);
					break;

				case IDCANCEL:
					EndDialog(hdlg, FALSE);
					break;
			}
            return TRUE;
	}

	return FALSE;
}

void biosInsert(ushort typeID)
{
	insertID = typeID;	
	DialogBox(hinst, MAKEINTRESOURCE(IDD_INSERT_FILE), hwnd, InsertProc);
}

void biosReplaceFile(char *fname)
{
	char name[256], ext[256];
	FILE *fp;
	ulong size, compSize, adjFreeSpace;
	ushort crc;
	bool failedFit;
	uchar *tempbuf;
	HWND hedit;
	awdbeItem *item;

	// open the file
	fp = fopen(fname, "rb");
	if (fp == NULL)
		return;

	// get just the name of this file
	_splitpath(fname, NULL, NULL, name, ext);
	sprintf(fname, "%s%s", name, ext);

	// get the size too
	fseek(fp, 0, 2);
	size = ftell(fp);
	rewind(fp);

	// warn if the size is possibly too big
	if (size > (biosdata.imageSize * 8))
	{
		if (MessageBox(hwnd, "The size of the file you are trying to insert is possibly too big for the "
			"image loaded.\n\nDo you want to continue anyway?", "Notice", MB_YESNO) == IDNO)
		{
			return;
		}			
	}

	// allocate a temp buffer and read the file in
	tempbuf = new uchar[size];
	fread(tempbuf, 1, size, fp);
	fclose(fp);

	// compress it
	biosTempCompressData(fname, strlen(fname), tempbuf, size, &compSize, &crc);

	// calculate the adjusted free space without the component we're replacing
	adjFreeSpace = biosFreeSpace + curFileEntry->compSize;

	// now see if the new component fits in the adjusted space
	failedFit = FALSE;

	if (curFileEntry->offset == 0)
	{
		if (compSize > adjFreeSpace)
			failedFit = TRUE;
	}
	else
	{
		if ((curFileEntry->offset + compSize) > biosdata.imageSize)
			failedFit = TRUE;
	}

	if (failedFit)
	{
		MessageBox(hwnd, "There is not enough free space in the BIOS image to insert this file.",
			"Error", MB_OK);

		delete []tempbuf;
		return;
	}

	// all ok... delete our old component and drop in the new one
	delete []curFileEntry->name;
	delete []curFileEntry->data;

	curFileEntry->nameLen = strlen(fname);
	curFileEntry->name = new char[curFileEntry->nameLen + 1];
	strcpy(curFileEntry->name, fname);

	// cheat and just use tempbuf as the new pointer here (just make sure we don't delete it in this func!)
	curFileEntry->data = tempbuf;

	// update sizes and crc
	curFileEntry->size		= size;
	curFileEntry->compSize	= compSize;
	curFileEntry->crc		= crc;

	// set modified flag and refresh the display
	biosSetModified(TRUE);

	// since we're not going to rebuild our component list (no need to), we do need to update the filename and size
	// in the currently shown dialog.  if we don't... then when the user moves from this one, the old filename will be
	// overwritten on top of ours.
	hedit = GetDlgItem(hModDlgWnd, IDC_FILENAME);
	if (IsWindow(hedit))
	{
		SetWindowText(hedit, curFileEntry->name);
	}

	hedit = GetDlgItem(hModDlgWnd, IDC_FILE_SIZE);
	if (IsWindow(hedit))
	{
		sprintf(fname, "%d", curFileEntry->size);
		SetWindowText(hedit, fname);
	}

	// we also need to call the plugin and have it refresh it's dialog... since a component changed, there's obviously gonna
	// be new stuff to be displayed.
	if (curHash > HASH_UNKNOWN_ITEM_MAX)
	{
		switch (curHash)
		{
			case HASH_SUBMENU_ITEM:
			case HASH_RECOGNIZED_ROOT:
			case HASH_UNKNOWN_ROOT:
			case HASH_INCLUDABLE_ROOT:
				break;

			default:
				// find the item that responds to this hash
				item = pluginFindHash(curHash);
				if (item != NULL)
				{
					// call the plugins' refresh function
					if (pluginCallRefreshDialog(item, curFileEntry, hModDlgWnd) == FALSE)
					{
						// this plugin can't handle this type anymore.  rebuild our component list...
						biosUpdateComponents();
					}
				}
				break;
		}
	}
}

void biosReplace(void)
{
	char fname[256];
	OPENFILENAME ofn;

	// do nothing if we're not pointing at anything
	if (curFileEntry == NULL)
		return;

	// display the open dialog
	fname[0] = 0;
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize			= sizeof(OPENFILENAME);
    ofn.hwndOwner			= hwnd;
	ofn.hInstance			= hinst;
    ofn.lpstrFilter			= "All Files (*.*)\0*.*\0\0";
	ofn.lpstrCustomFilter	= NULL;
	ofn.nMaxCustFilter		= 0;
	ofn.nFilterIndex		= 1;
    ofn.lpstrFile			= fname;
    ofn.nMaxFile			= 256;
	ofn.lpstrFileTitle		= NULL;
	ofn.nMaxFileTitle		= 0;
	ofn.lpstrInitialDir		= (config.lastPath[0] == 0) ? NULL : config.lastPath;
	ofn.lpstrTitle			= NULL;
    ofn.Flags				= OFN_FILEMUSTEXIST | OFN_ENABLESIZING | OFN_EXPLORER;
	ofn.nFileOffset			= 0;
	ofn.nFileExtension		= 0;
	ofn.lpstrDefExt			= NULL;
	ofn.lCustData			= NULL;
	ofn.lpfnHook			= NULL;
	ofn.lpTemplateName		= NULL;

    if (GetOpenFileName(&ofn) == FALSE)
		return;

	biosReplaceFile(fname);
}

void biosExtract(void)
{
	char fname[256];
	OPENFILENAME ofn;
	FILE *fp;

	// do nothing if we're not pointing at anything
	if (curFileEntry == NULL)
		return;

	// make a filename for this component
	strcpy(fname, curFileEntry->name);

	// display the save dialog
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize			= sizeof(OPENFILENAME);
    ofn.hwndOwner			= hwnd;
	ofn.hInstance			= hinst;
    ofn.lpstrFilter			= "All Files (*.*)\0*.*\0\0";
	ofn.lpstrCustomFilter	= NULL;
	ofn.nMaxCustFilter		= 0;
	ofn.nFilterIndex		= 1;
    ofn.lpstrFile			= fname;
    ofn.nMaxFile			= 256;
	ofn.lpstrFileTitle		= NULL;
	ofn.nMaxFileTitle		= 0;
	ofn.lpstrInitialDir		= (config.lastPath[0] == 0) ? NULL : config.lastPath;
	ofn.lpstrTitle			= NULL;
    ofn.Flags				= OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_ENABLESIZING | OFN_EXPLORER;
	ofn.nFileOffset			= 0;
	ofn.nFileExtension		= 0;
	ofn.lpstrDefExt			= NULL;
	ofn.lCustData			= NULL;
	ofn.lpfnHook			= NULL;
	ofn.lpTemplateName		= NULL;

    if (GetSaveFileName(&ofn) == FALSE)
		return;

	// open the file
	fp = fopen(fname, "wb");

	// write out the file
	fwrite(curFileEntry->data, 1, curFileEntry->size, fp);

	// close file
	fclose(fp);
}

int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
{
	switch (uMsg)
	{
		case BFFM_INITIALIZED:
			if (config.lastPath[0] != 0)
				SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)config.lastPath);
			break;
	}

	return 0;
}

void biosExtractAll(void)
{
	char fname[256], fullpath[256];
	fileEntry *fe;
	FILE *fp;
	int t;
	BROWSEINFO bi;
	LPITEMIDLIST pidlBrowse;
	IMalloc *pMalloc;

	// do nothing if nothing is loaded...
	if (biosdata.fileTable == NULL)
		return;

	if (SHGetMalloc(&pMalloc) == NOERROR)
	{
		bi.hwndOwner		= hwnd;
		bi.pidlRoot			= NULL;
		bi.pszDisplayName	= NULL;
		bi.lpszTitle		= "Select Directory for Extraction";
		bi.ulFlags			= BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
		bi.lpfn				= BrowseCallbackProc;
		bi.lParam			= 0;
		bi.iImage			= 0;
		pidlBrowse = SHBrowseForFolder(&bi);

		if (pidlBrowse != NULL)
		{
			if (SHGetPathFromIDList(pidlBrowse, fullpath))
			{
				// write out all of the files in our table...
				for (t = 0; t < biosdata.fileCount; t++)
				{
					fe = &biosdata.fileTable[t];

					// create a filename
					sprintf(fname, "%s\\%s", fullpath, fe->name);

					// write the data
					fp = fopen(fname, "wb");
					if (fp != NULL)
					{
						fwrite(fe->data, 1, fe->size, fp);
						fclose(fp);
					}
					else
					{
						sprintf(fname, "Unable to extract [%s]!  Skipping...\n", fe->name);
						MessageBox(hwnd, fname, "Error", MB_OK);
					}
				}
			}
			else
			{
				MessageBox(hwnd, "Unable to extract to the selected path!", "Error", MB_OK);
			}
		}

		// clean up
		pMalloc->Free(pidlBrowse);
		pMalloc->Release();
	}
}

BOOL APIENTRY RemoveProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
		case WM_INITDIALOG:
			CheckDlgButton(hdlg, IDC_ASK_AGAIN, BST_UNCHECKED);
			break;

        case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					if (IsDlgButtonChecked(hdlg, IDC_ASK_AGAIN) == BST_CHECKED)
						config.removeNoAsk = TRUE;

					EndDialog(hdlg, TRUE);
					break;

				case IDCANCEL:
					EndDialog(hdlg, FALSE);
					break;
			}
            return TRUE;
	}

	return FALSE;
}

BOOL APIENTRY RemoveBDProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
		case WM_INITDIALOG:
			CheckDlgButton(hdlg, IDC_ASK_AGAIN, BST_UNCHECKED);
			break;

        case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					if (IsDlgButtonChecked(hdlg, IDC_ASK_AGAIN) == BST_CHECKED)
						config.removeBDNoAsk = TRUE;

					EndDialog(hdlg, TRUE);
					break;

				case IDCANCEL:
					EndDialog(hdlg, FALSE);
					break;
			}
            return TRUE;
	}

	return FALSE;
}

void biosRemoveEntry(fileEntry *toRemove)
{
	int t;
	fileEntry *fe, *newTable;

	// allocate a new file table, minus one file
	newTable = new fileEntry[biosdata.fileCount - 1];

	// copy all entries into the new table, except for the currently selected one
	fe = &newTable[0];

	for (t = 0; t < biosdata.fileCount; t++)
	{
		if (toRemove != &biosdata.fileTable[t])
		{			
			memcpy(fe, &biosdata.fileTable[t], sizeof(fileEntry));
			fe++;
		}
	}

	// free the file data pointed to by the current pointer
	delete []toRemove->name;
	delete []toRemove->data;

	// delete the old table and move the new table over to the current table
	delete []biosdata.fileTable;
	biosdata.fileTable = newTable;
	biosdata.fileCount--;

	// zap curFileEntry (because it doesn't point to a valid table anymore)
	curFileEntry = NULL;
}

void biosRemove(void)
{
	int res;

	// do nothing if we're not pointing at anything
	if (curFileEntry == NULL)
		return;

	// pop up the dialog if we're supposed to...
	if (config.removeNoAsk == FALSE)
	{
		res = DialogBox(hinst, MAKEINTRESOURCE(IDD_REMOVE_COMPONENT), hwnd, RemoveProc);

		// if the user cancelled, end here
		if (res == FALSE)
			return;
	}

	// if this component is a boot or decompression block, flog a warning
	if ((curFileEntry->flags & FEFLAGS_BOOT_BLOCK) || (curFileEntry->flags & FEFLAGS_DECOMP_BLOCK))
	{
		if (config.removeBDNoAsk == FALSE)
		{
			res = DialogBox(hinst, MAKEINTRESOURCE(IDD_REMOVE_BDBLOCK), hwnd, RemoveBDProc);

			// if the user cancelled, end here
			if (res == FALSE)
				return;
		}
	}

	biosRemoveEntry(curFileEntry);

	// set modified flag and refresh the display
	biosdata.modified = TRUE;

	biosUpdateComponents();
	biosTitleUpdate();
}

void biosHexEdit(void)
{
	char fname[256];
	FILE *fp;
	int res;

	// do nothing if we're not pointing at anything
	if (curFileEntry == NULL)
		return;

	// update the current dialog before we write any data
	biosUpdateCurrentDialog();

	// create a filename
	sprintf(fname, "%s\\%s", fullTempPath, curFileEntry->name);

	// write out the data to our temporary path
	fp = fopen(fname, "wb");
	fwrite(curFileEntry->data, 1, curFileEntry->size, fp);
	fclose(fp);

	// add this component to our update list
	biosAddToUpdateList(fname);

	// spawn the hexeditor
	res = (int)ShellExecute(hwnd, NULL, config.hexEditor, fname, NULL, SW_SHOWNORMAL);
	if (res <= 32)
	{
		MessageBox(hwnd, "Unable to launch the configured hex editor!\n\nPlease check the path and/or filename under the "
			"Options->Configuration menu.", "Error", MB_OK);
	}
}

time_t biosGetLastWriteTime(updateEntry *ue)
{
	char cwd[256];
	struct _finddata_t fd;
	long hfile;

	// save the current dir
	_getcwd(cwd, 256);

	// change to the path specified
	if (_chdir(ue->path) >= 0)
	{
		// lookup our file
		hfile = _findfirst(ue->fname, &fd);
		if (hfile >= 0)
			_findclose(hfile);
	}

	// return to saved dir
	_chdir(cwd);

	// return last time value
	return fd.time_write;
}

void CALLBACK UpdateProc(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	updateEntry *ue = updateList;
	time_t lastwrite;
	char buf[256];

	if (updateIgnoreTimer == TRUE)
		return;

	updateIgnoreTimer = TRUE;

	while (ue != NULL)
	{
		lastwrite = biosGetLastWriteTime(ue);
		if (ue->lastWrite != lastwrite)
		{
			sprintf(buf, "%s has been modified by an external application.  Do you wish to load the\n"
				"changed contents into the current BIOS image?", ue->fname);

			if (MessageBox(hwnd, buf, "Notice", MB_YESNO) == IDYES)
			{
				sprintf(buf, "%s%s", ue->path, ue->fname);
				biosReplaceFile(buf);
			}

			// update lastwrite time regardless of choice
			ue->lastWrite = lastwrite;
		}

		ue = ue->next;
	}

	updateIgnoreTimer = FALSE;
}

void biosAddToUpdateList(char *fname)
{
	updateEntry *ue, *ui;
	char path[256], name[256], ext[256], fullname[256];

	// split up our fname into path/name components
	_splitpath(fname, NULL, path, name, ext);
	sprintf(fullname, "%s%s", name, ext);

	// make sure this entry is not already in the update list
	ue = updateList;
	while (ue != NULL)
	{
		if (!stricmp(ue->path, path) && !stricmp(ue->fname, fullname))
			return;

		ue = ue->next;
	}

	// create a new entry
	ue		  = new updateEntry;
	ue->path  = new char[strlen(path) + 1];
	ue->fname = new char[strlen(fullname) + 1];

	strcpy(ue->path, path);
	strcpy(ue->fname, fullname);

	ue->lastWrite = biosGetLastWriteTime(ue);
	ue->next	  = NULL;

	// add this file to our update list
	if (updateList == NULL)
	{
		updateList = ue;

		// since this is the first entry, set our timer...
		updateTimerID = SetTimer(NULL, 0, 2000, UpdateProc);
	}
	else
	{
		ui = updateList;
		while (ui->next != NULL)
			ui = ui->next;

		ui->next = ue;
	}
}

void biosClearUpdateList(void)
{
	updateEntry *ue = updateList, *nextue;

	// kill our timer
	KillTimer(NULL, updateTimerID);

	// nuke all update items
	while (ue != NULL)
	{
		delete []ue->path;
		delete []ue->fname;

		nextue = ue->next;
		delete ue;

		ue = nextue;
	}

	updateList = NULL;
}

void biosGetDialogSize(SIZE *sz)
{
	sz->cx = dlgrc->right;
	sz->cy = dlgrc->bottom;
}

void biosResizeDialog(SIZE sz)
{
	resizeControlsFromDialog(sz);
}

void biosResizeCurrentDialog(HWND hwnd, RECT *rc)
{
	awdbeItem *item;

	if (curHash > HASH_UNKNOWN_ITEM_MAX)
	{
		switch (curHash)
		{
			case HASH_SUBMENU_ITEM:
			case HASH_RECOGNIZED_ROOT:
			case HASH_UNKNOWN_ROOT:
			case HASH_INCLUDABLE_ROOT:
				break;

			default:
				// find the item that responds to this hash
				item = pluginFindHash(curHash);
				if (item != NULL)
				{
					// call the plugins' refresh function
					pluginCallOnResizeDialog(item, hwnd, rc);
				}
				break;
		}
	}
}

void biosSetModified(bool val)
{
	biosdata.modified = val;
	biosTitleUpdate();
}

void biosRefreshCurrentDialog(void)
{
	awdbeItem *item;

	if (curHash > HASH_UNKNOWN_ITEM_MAX)
	{
		switch (curHash)
		{
			case HASH_SUBMENU_ITEM:
			case HASH_RECOGNIZED_ROOT:
			case HASH_UNKNOWN_ROOT:
			case HASH_INCLUDABLE_ROOT:
				break;

			default:
				// find the item that responds to this hash
				item = pluginFindHash(curHash);
				if (item != NULL)
				{
					// call the plugins' refresh function
					pluginCallRefreshDialog(item, curFileEntry, hModDlgWnd);
				}
				break;
		}
	}
}

awdbeBIOSVersion biosGetVersion(void)
{
	awdbeBIOSVersion vers = awdbeBIOSVerUnknown;
	fileEntry *fe;
	uchar *sptr;
	int len;

	fe = biosScanForID(0x5000);
	if (fe == NULL)
		return vers;

	// get the bios's version
	sptr = ((uchar *)fe->data) + 0x1E060;
	len  = (*sptr++) - 1;

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
