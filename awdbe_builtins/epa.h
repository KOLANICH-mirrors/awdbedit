#pragma once
//
// Award BIOS Editor - epa.h
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: epa.h,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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

#ifndef EPA_H
#define EPA_H

bool isEPALogo(uint8_t *data, uint32_t size);

HWND epaCreateDialog(HWND parentWnd);
void epaRefreshDialog(HWND hwnd, fileEntry *fe);
void epaOnDestroyDialog(HWND hwnd);

void epaMakeV1Bitmap(uint8_t *data, uint8_t *outmap);
void epaMakeV2Bitmap(uint8_t *data, uint32_t size, uint8_t *outmap);
void epaMakeV2VGABitmap(uint8_t *data, uint32_t size, uint8_t *outmap);
void epaFlipBitmap(uint8_t *outmap, int width, int height);
void epaBlitBitmap(HDC blitToDC);

#endif
