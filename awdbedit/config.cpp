//
// Award BIOS Editor - config.cpp
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: config.cpp,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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
#include "config.h"


cfgStruct config;
static char progName[256], progSubName[256];


void configInit(const char *name, const char *subname)
{
	strncpy(progName, name, 256);
	strncpy(progSubName, subname, 256);

	progName[255]	 = 0;
	progSubName[255] = 0;
}

void configCreate(void)
{
	ZeroMemory(&config, sizeof(cfgStruct));

	config.treeViewWidth = 174;
	config.rootXSize     = 640;
	config.rootYSize     = 480;

	configSave();
}

void configLoad(void)
{
	HKEY soft, awbedit, curr;
	DWORD foo, len;

	RegOpenKeyEx(HKEY_CURRENT_USER, "Software", 0, KEY_ALL_ACCESS, &soft);
	if (RegOpenKeyEx(soft, progName, 0, KEY_ALL_ACCESS, &awbedit) != ERROR_SUCCESS)
	{
		// Key not found, create default config.
		configCreate();
		RegCloseKey(soft);
		return;
	}

	if (RegOpenKeyEx(awbedit, progSubName, 0, KEY_ALL_ACCESS, &curr) != ERROR_SUCCESS)
	{
		// Key not found for this version, create default config.
		configCreate();
		RegCloseKey(awbedit);
		RegCloseKey(soft);
		return;
	}

	// Key found, so read configuration data.
	len = sizeof(cfgStruct);
	if (RegQueryValueEx(curr, "Config", nullptr, &foo, (uint8_t *)&config, &len) != ERROR_SUCCESS)
		configCreate();

	RegCloseKey(curr);
	RegCloseKey(awbedit);
	RegCloseKey(soft);
}

const char REGISTRY_KEY_NAME_Software[] = "Software";
const char REGISTRY_KEY_NAME_Config[] = "Config";
static const char EMPTY_STRING[] = "";

void configSave(void)
{
	HKEY soft, awbedit, curr;
	DWORD foo;

	// Create config block for HKEY_CURRENT_USER.
	RegOpenKeyEx(HKEY_CURRENT_USER, const_cast<char *>(REGISTRY_KEY_NAME_Software), 0, KEY_ALL_ACCESS, &soft);
	RegCreateKeyEx(soft, progName, 0, const_cast<char *>(EMPTY_STRING), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &awbedit, &foo);
	RegCreateKeyEx(awbedit, progSubName, 0, const_cast<char *>(EMPTY_STRING), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &curr, &foo);
	RegSetValueEx(curr, REGISTRY_KEY_NAME_Config, 0, REG_BINARY, (const uint8_t *)&config, sizeof(cfgStruct));
	RegCloseKey(curr);
	RegCloseKey(awbedit);
	RegCloseKey(soft);
}
