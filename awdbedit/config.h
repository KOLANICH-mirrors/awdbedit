#pragma once
//
// Award BIOS Editor - config.h
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: config.h,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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

#ifndef CONFIG_H
#define CONFIG_H

typedef struct
{
	char lastSkin[256];
	char lastPath[256];

	char hexEditor[256];

	char recentFile[4][256];

	bool removeNoAsk;
	bool removeBDNoAsk;

	int	 treeViewWidth;
	int	 rootXSize, rootYSize;
} cfgStruct;

extern cfgStruct config;


void configInit(char *name, char *subname);

void configCreate(void);
void configLoad(void);
void configSave(void);

#endif
