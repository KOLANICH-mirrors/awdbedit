//
// Award BIOS Editor - plugin.cpp
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: plugin.cpp,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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
#include <io.h>
#include <direct.h>
#include <windows.h>
#include <commctrl.h>
#include "types.h"
#define AWDBE_EXPORT_FUNCS
#include "awdbe_exports.h"
#include "bios.h"
#include "plugin.h"
#include "popupMenu.h"
#include "popupDialog.h"
#include "main.h"

static pluginFuncEntry *pluginFuncList;
static awdbeItemEntry *itemMenuList;

static HWND textwnd, progwnd;


static inline bool isBuiltinsPlugin(char *desc)
{
	if (((uint8_t)desc[0] == 0xFA) && ((uint8_t)desc[1] == 0xB9) && ((uint8_t)desc[2] == 0xC0) && ((uint8_t)desc[3] == 0xD2))
		return TRUE;

	return FALSE;
}

void pluginInit(void)
{
	pluginFuncList = NULL;
	itemMenuList   = NULL;
}

void pluginShutdown(void)
{
	pluginFuncEntry *pe = pluginFuncList, *nextpe;
	awdbeItemEntry *item = itemMenuList, *nextitem;

	// free all loaded plugins, and destroy the linked list
	while (pe != NULL)
	{
		delete []pe->fname;
		FreeLibrary(pe->hInst);

		nextpe = pe->next;
		delete pe;

		pe = nextpe;
	}

	// release our item entry linked list
	while (item != NULL)
	{
		nextitem = item->next;
		delete item;

		item = nextitem;
	}
}

void pluginInitScan(HWND text, HWND prog)
{
	textwnd = text;
	progwnd = prog;
}

uint32_t pluginScan(char *dir, bool doLoad)
{
	typedef awdbeFuncTable* (*registerPluginFunc)(void);
	long hFile;
	char cwd[256], fname[256], *ptr;
	struct _finddata_t fd;
	HINSTANCE hInst;
	FARPROC regproc;
	awdbeFuncTable *ftbl;
	registerPluginFunc regfunc;
	uint32_t count = 0;

	// save off the current dir
	_getcwd(cwd, 256);

	// change into the specified dir
	_chdir(dir);

	// look for all DLL files in this directory
	hFile = _findfirst("*.*", &fd);
	if (hFile != -1)
	{
		do
		{
			// if this is a directory, make a new path and scan through it
			if (fd.attrib == _A_SUBDIR)
			{
				// if this directory has a '.' as the first character, don't scan.
				if (fd.name[0] != '.')
				{
					sprintf(fname, "%s%s\\", dir, fd.name);
					count += pluginScan(fname, doLoad);
				}
			}
			else
			{
				// does this file have a .dll extension?
				ptr = fd.name + (strlen(fd.name) - 4);
				if (!memicmp(ptr, ".dll", 4))
				{
					// if we're only checking for DLLs, don't bother load it.
					if (doLoad == TRUE)
					{
						// load this DLL
						sprintf(fname, "%s%s", dir, fd.name);
						if ( (hInst = LoadLibrary(fname)) != NULL )
						{
							// look for our "awdbeRegisterPlugin" function in the DLL
							if ( (regproc = GetProcAddress(hInst, "awdbeRegisterPlugin")) != NULL )
							{
								// get a pointer to the register function and call it to get the plugin's function table
								regfunc = (registerPluginFunc)regproc;
								ftbl	= regfunc();
								
								// add this table to the internal plugin list
								pluginAdd(fname, hInst, ftbl);
							}
							else
							{
								// this is not our DLL... release it
								FreeLibrary(hInst);
							}
						}
					}

					count++;
				}
			}
		} while (_findnext(hFile, &fd) == 0);

		_findclose(hFile);
	}

	// return back to the stored dir
	_chdir(cwd);

	return count;
}

void pluginAdd(char *fname, HINSTANCE hInst, awdbeFuncTable *ftbl)
{
	char *desc;
	pluginFuncEntry *fe;

	// get the description of this plugin
	desc = ftbl->descriptionFunc();

	// check for the builtins magic in the description
	if (isBuiltinsPlugin(desc))
		desc += 4;

	// update our controls
	SetWindowText(textwnd, desc);
	SendMessage(progwnd, PBM_STEPIT, 0, 0);

	// insert this entry into our list
	if (pluginFuncList == NULL)
	{
		pluginFuncList = new pluginFuncEntry;
		fe = pluginFuncList;
	}
	else
	{
		fe = pluginFuncList;
		while (fe->next != NULL)
			fe = fe->next;

		fe->next = new pluginFuncEntry;
		fe = fe->next;
	}

	fe->fname = new char[strlen(fname) + 1];
	strcpy(fe->fname, fname);
	fe->hInst	= hInst;
	fe->functbl	= ftbl;
	fe->next	= NULL;

	// and call this plugin's init function
	fe->functbl->initFunc((uint64_t)fe);
}

uint32_t pluginMakeBaseHash(uint16_t typeID, char *name)
{
	uint32_t hash = 0x12AB3C7F, *lptr;
	int len = strlen(name);
	char *ptr;

	lptr = (uint32_t *)name;
	while (len > 4)
	{
		hash += *lptr++;
		len  -= 4;
	}

	ptr = (char *)lptr;
	while (len > 0)
	{
		hash ^= (*ptr++) << (8 * len);
		len--;
	}

	hash ^= (((uint32_t)typeID << 16) | (uint32_t)typeID);
	return hash;
}

bool pluginHashExists(uint32_t hash)
{
	awdbeItemEntry *ie = itemMenuList;
	awdbeItem *item;
	int count;

	// check for reserved hash values
	if (hash <= HASH_RESERVED)
		return TRUE;

	// iterate through all items in all lists to see if this hash value has already been assigned
	while (ie != NULL)
	{
		item  = ie->list;
		count = ie->count;

		while (count > 0)
		{
			if (item->hash == hash)
				return TRUE;

			item++;
			count--;
		}

		ie = ie->next;
	}

	return FALSE;
}

uint32_t pluginMakeHash(uint16_t typeID, char *name)
{
	uint32_t hash;

	hash = pluginMakeBaseHash(typeID, name);

	while (pluginHashExists(hash))
		hash = (((hash & 0x00FFFFFF) << 8) | ((hash & 0xFF000000) >> 24)) + 1;

	return hash;
}

void pluginGenerateHash(awdbeItem *itemlist, int count)
{
	int t;

	for (t = 0; t < count; t++)
	{
		itemlist->hash = pluginMakeHash(itemlist->biosTypeID, itemlist->itemName);
		itemlist++;
	}
}

awdbeItemEntry *pluginGetItemList(void)
{
	return itemMenuList;
}

awdbeItem *pluginFindResponder(fileEntry *fe)
{
	awdbeItemEntry *ie = itemMenuList;
	awdbeItem *item;
	uint32_t count;

	// iterate through each item list, looking for a matching typeID
	while (ie != NULL)
	{
		item  = ie->list;
		count = ie->count;

		while (count--)
		{
			if (item->biosTypeID == fe->type)
			{
				// found one!  call the plugins' detect function on this entry and see if it wants to handle it...
				if (ie->plugin->functbl->detectFunc(fe) == TRUE)
					return item;
			}

			item++;
		}

		ie = ie->next;
	}

	return NULL;
}

awdbeItem *pluginFindSubMenu(awdbeItem *item)
{
	awdbeItemEntry *ie = itemMenuList;
	awdbeItem *itemmin, *itemmax;

	// iterate through each item list, checking to see if the "item" pointer is within the bounds of the list we point to...
	while (ie != NULL)
	{
		itemmin = ie->list;
		itemmax = ie->list + ie->count;

		if ((item >= itemmin) && (item <= itemmax))
		{
			// start at "item" and walk backwards, looking for a AWDBE_SUBMENU definition, until we hit itemmin.
			while (item >= itemmin)
			{
				if ((item->flags & AWDBE_MENUMASK) == AWDBE_SUBMENU)
					return item;

				item--;
			}
		}

		ie = ie->next;
	}

	return NULL;
}

awdbeItem *pluginFindHash(uint32_t hash)
{
	awdbeItemEntry *ie = itemMenuList;
	awdbeItem *item;
	uint32_t count;

	// iterate through each item list, looking for a matching hash
	while (ie != NULL)
	{
		item  = ie->list;
		count = ie->count;

		while (count--)
		{
			if (item->hash == hash)
			{
				// found it!
				return item;
			}

			item++;
		}

		ie = ie->next;
	}

	return NULL;
}

pluginFuncEntry *pluginFindItem(awdbeItem *item)
{
	awdbeItemEntry *ie = itemMenuList;
	awdbeItem *itemmin, *itemmax;

	// iterate through each item list, checking to see if the "item" pointer is within the bounds of the list we point to...
	while (ie != NULL)
	{
		itemmin = ie->list;
		itemmax = ie->list + ie->count;

		if ((item >= itemmin) && (item <= itemmax))
		{
			// found it... use this item entry to lookup the associated plugin.
			return ie->plugin;
		}

		ie = ie->next;
	}

	return NULL;
}

void pluginCallOnLoad(fileEntry *fe, int count)
{
	pluginFuncEntry *pe = pluginFuncList;

	// call each plugins' onLoad function
	while (pe != NULL)
	{
		pe->functbl->onLoadFunc(fe, count);
		pe = pe->next;
	}
}

HWND pluginCallCreateDialog(awdbeItem *item, fileEntry *curFileEntry, HWND parentWnd, RECT *dlgrc)
{
	pluginFuncEntry	*pe;

	// lookup this plugin
	pe = pluginFindItem(item);

	if (pe != NULL)
	{
		// now call the plugin's dialog create function
		return pe->functbl->createDialogFunc(item, curFileEntry, parentWnd, dlgrc);
	}

	return NULL;
}

bool pluginCallUpdateDialog(awdbeItem *item, fileEntry *curFileEntry, HWND dialogWnd)
{
	pluginFuncEntry	*pe;

	// lookup this plugin
	pe = pluginFindItem(item);

	if (pe != NULL)
	{
		// now call the plugin's dialog create function
		return pe->functbl->updateDialogFunc(item, curFileEntry, dialogWnd);
	}

	return FALSE;
}

bool pluginCallRefreshDialog(awdbeItem *item, fileEntry *fe, HWND hwnd)
{
	pluginFuncEntry	*pe;

	// lookup this plugin
	pe = pluginFindItem(item);

	if (pe != NULL)
	{
		// before we call refresh, we need to make sure the plugin still handles this data.
		if (pe->functbl->detectFunc(fe) == FALSE)
			return FALSE;

		// now call the plugin's refresh function
		pe->functbl->refreshDialogFunc(item, fe, hwnd);
	}

	return TRUE;
}

void pluginCallOnDestroyDialog(awdbeItem *item, HWND hwnd)
{
	pluginFuncEntry	*pe;

	// lookup this plugin
	pe = pluginFindItem(item);

	if (pe != NULL)
	{
		// now call the plugin's on destroy function
		pe->functbl->onDestroyDialogFunc(item, hwnd);
	}
}

void pluginCallOnResizeDialog(awdbeItem *item, HWND hwnd, RECT *rc)
{
	pluginFuncEntry	*pe;

	// lookup this plugin
	pe = pluginFindItem(item);

	if (pe != NULL)
	{
		// now call the plugin's on destroy function
		pe->functbl->onResizeDialogFunc(item, hwnd, rc);
	}
}

void pluginAddToMenu(HMENU menu, uint32_t fromID)
{
	pluginFuncEntry *pe = pluginFuncList;
	char *desc;

	while (pe != NULL)
	{
		desc = pe->functbl->descriptionFunc();

		// check for the builtins magic in the description
		if (isBuiltinsPlugin(desc))
			desc += 4;

		AppendMenu(menu, MF_ENABLED, fromID++, desc);
		pe = pe->next;
	}
}

void pluginShowAboutBox(uint32_t index, HWND parentWnd)
{
	pluginFuncEntry *pe = pluginFuncList;

	while (pe != NULL)
	{
		if (index == 0)
		{
			pe->functbl->aboutBoxFunc(parentWnd);
			return;
		}

		index--;
		pe = pe->next;
	}
}


//---------------------------------------------------------------------------------------------------------------------
// plugin accessor functions

void awdbeAddToItemList(uint64_t pluginID, awdbeItem *itemList, int itemCount)
{
	pluginFuncEntry *pe = (pluginFuncEntry *)pluginID;
	awdbeItemEntry *ie;
	char *desc;

	// check for the builtins magic in the description
	desc = pe->functbl->descriptionFunc();
	if (((uint8_t)desc[0] == 0xFA) && ((uint8_t)desc[1] == 0xB9) && ((uint8_t)desc[2] == 0xC0) && ((uint8_t)desc[3] == 0xD2))
	{
		// insert this entry at the end of our list
		ie = itemMenuList;
		if (ie != NULL)
		{
			while (ie->next != NULL)
				ie = ie->next;

			ie->next = new awdbeItemEntry;
			ie = ie->next;
		}
		else
		{
			ie = new awdbeItemEntry;
			itemMenuList = ie;
		}

		ie->list   = itemList;
		ie->count  = itemCount;
		ie->next   = NULL;
		ie->plugin = pe;		
	}
	else
	{
		// insert this entry into the front of our list
		ie = new awdbeItemEntry;
		ie->list   = itemList;
		ie->count  = itemCount;
		ie->next   = itemMenuList;
		ie->plugin = pe;

		itemMenuList = ie;
	}

	// iterate through this list, generating a hash for each item
	pluginGenerateHash(ie->list, ie->count);
}

void awdbeUpdateSelf(uint64_t pluginID)
{
	biosUpdateCurrentDialog();
}

void awdbeRefreshSelf(uint64_t pluginID)
{
	biosRefreshCurrentDialog();
}

void awdbeGetDialogSize(uint64_t pluginID, SIZE *sz)
{
	biosGetDialogSize(sz);
}

void awdbeResizeDialog(uint64_t pluginID, SIZE sz)
{
	biosResizeDialog(sz);
}

AWDBEDIT_API fileEntry *awdbeSearchForID(uint64_t pluginID, uint16_t ID)
{
	return biosScanForID(ID);
}

void awdbeSetModified(uint64_t pluginID)
{
	biosSetModified(TRUE);
}

int awdbeDoPopup(HINSTANCE hinst, LPSTR resid, int xp, int yp)
{
	popupDialog *pdlg;
	popupMenu *pmenu;
	RECT rc;
	bool done;
	MSG msg;
	HDC dc;
	PAINTSTRUCT ps;
	int retval = 0;
	char buf[256];

	pmenu = globals.pMenu;
	memcpy(&rc, &globals.dialogrc, sizeof(RECT));

	pdlg = new popupDialog(hinst, globals.MainhWnd);

	pdlg->create(xp + rc.left, yp + rc.top, LoadMenu(hinst, resid), pmenu->getOpenStyle(), pmenu->getBackgroundImage(), 
		pmenu->getBackgroundColor(), pmenu->getTextColor(), pmenu->getInactiveColor());

	done = FALSE;

	while (!done)
	{
		if (!GetMessage(&msg, NULL, 0, 0))
		{
			done = TRUE;
		}
		else
		{
			TranslateMessage(&msg);

			if (msg.hwnd == pdlg->gethWnd())
			{
				switch (msg.message)
				{
					case WM_PAINT:
						dc = BeginPaint(msg.hwnd, &ps);
						pdlg->onPaint(msg.hwnd, dc);
						EndPaint(msg.hwnd, &ps);
						break;

					case WM_MOUSEMOVE:
						pdlg->onMouseMove(msg.hwnd, LOWORD(msg.lParam), HIWORD(msg.lParam), &retval);

						LoadString(hinst, retval, buf, 256);
						SendMessage(globals.hStatusBar, SB_SETTEXT, 0, (LPARAM)buf);
						break;

					case WM_LBUTTONDOWN:
					case WM_LBUTTONUP:
						if (pdlg->onLButtonDown(msg.hwnd, LOWORD(msg.lParam), HIWORD(msg.lParam), &retval) == TRUE)
							done = TRUE;
						break;
				}
			}
			else
			{
				if ((msg.message == WM_LBUTTONDOWN) || (msg.message == WM_RBUTTONDOWN) || (msg.message == WM_MOVE) || (msg.message == WM_CLOSE))
				{
					retval = 0;
					done   = TRUE;
				}

				DispatchMessage(&msg);
			}
		}
	}

	pdlg->destroy();
	delete pdlg;

	SendMessage(globals.hStatusBar, SB_SETTEXT, 0, (LPARAM)"");

	return retval;
}

awdbeBIOSVersion awdbeGetBIOSVersion(uint64_t pluginID)
{
	return biosGetVersion();
}
