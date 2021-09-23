//
// Award BIOS Editor - console.cpp
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: console.cpp,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <cassert>
#include "types.h"
#include "console.h"

HANDLE hCon = (HANDLE)-1;
bool madeConsole = FALSE;
WORD   attrib = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
SHORT  cursor_x, cursor_y, size_x, size_y;
SHORT *screen = nullptr, *scrptr;

HANDLE getConsoleHandle(void)
{
	return hCon;
}

void init_console(bool useExistConsole, int mode, PHANDLER_ROUTINE ctrlHandler)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	madeConsole = FALSE;

	if (useExistConsole == FALSE)
		hCon = (HANDLE)-1;
	else
		hCon = GetStdHandle(STD_OUTPUT_HANDLE);

	if (hCon == reinterpret_cast<HANDLE>(-1))
	{
		// no console, so allocate one...
		AllocConsole();
		hCon = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, nullptr, CONSOLE_TEXTMODE_BUFFER, nullptr);
		SetConsoleActiveScreenBuffer(hCon);
		FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));

		madeConsole = TRUE;
	}

	SetConsoleTextAttribute(hCon, attrib);

	switch (mode)
	{
		case MODE_80x25: go_25(); break;
		case MODE_80x50: go_50(); break;
	}

	GetConsoleScreenBufferInfo(hCon, &csbi);
	gotoxy(csbi.dwCursorPosition.X + 1, csbi.dwCursorPosition.Y + 1);

	if (GetConsoleOutputCP() != 437)
	{
		if (SetConsoleOutputCP(437) == 0)
		{
			// can't change codepage...
		}
	}

	// set control handler
	if (ctrlHandler != nullptr)
		SetConsoleCtrlHandler(ctrlHandler, TRUE);
}

void free_console(void)
{
	flip_to_console();
	update_console_position();

	if (screen) delete []screen;
	screen = nullptr;

	if (madeConsole)
	{
		CloseHandle(hCon);
		FreeConsole();
	}

	hCon = (HANDLE)-1;
}

void change_size(SHORT xs, SHORT ys)
{
	COORD size, pos;
	SMALL_RECT r;

	size.X = xs;
	size.Y = ys;

	// Try changing buffer size first.
	if (SetConsoleScreenBufferSize(hCon, size) == FALSE)
	{
		r.Top    = 0;
		r.Left   = 0;
		r.Right  = xs - 1;
		r.Bottom = ys - 1;

		// Buffer size set failed... size must be smaller than previous, so shrink window first.
		if (SetConsoleWindowInfo(hCon, TRUE, &r) == FALSE)
		{
			// Something bad... should do error checking here.
			return;
		}

		// Now set the size of the buffer.
		if (SetConsoleScreenBufferSize(hCon, size) == FALSE)
		{
			// Something bad too...
			return;
		}
	}

	size_x = xs;
	size_y = ys;

	if (screen) delete []screen;
	screen = new SHORT[static_cast<unsigned int>(size_x * size_y) * 2u];
	scrptr = screen;

	size.X   = size_x;
	size.Y   = size_y;
	pos.X    = 0;
	pos.Y    = 0;
	r.Top    = 0;
	r.Left   = 0;
	r.Right  = size_x - 1;
	r.Bottom = size_y - 1;

	ReadConsoleOutput(hCon, (CHAR_INFO *)screen, size, pos, &r);
}

void go_25(void)
{
	change_size(80, 25);
}

void go_50(void)
{
	change_size(80, 50);
}

void flip_to_console(void)
{
	COORD size = { size_x, size_y }, pos = { 0, 0 };
	SMALL_RECT r = { 0, 0, static_cast<SHORT>(size_x - 1), static_cast<SHORT>(size_y - 1) };

	WriteConsoleOutput(hCon, (CHAR_INFO *)screen, size, pos, &r);
}

void update_console_position(void)
{
	COORD pos;

	pos.X = cursor_x - 1;
	pos.Y = cursor_y - 1;
	SetConsoleCursorPosition(hCon, pos);
}

SHORT *get_screen_ptr(void)
{
	return screen;
}

void clrscr(void)
{
	int t;
	SHORT *ptr;

	ptr = screen;
	t = size_x * size_y;
	while (t--)
	{
		*ptr++ = 0x0020;
		*ptr++ = attrib;
	}

	gotoxy(1, 1);
}

void textattr(WORD c)
{
	attrib = c;
}

void textcolor(int c)
{
	attrib = (attrib & 0xF0) | (c & 0x0F);
}

void textbackground(WORD c)
{
	attrib = (attrib & 0x0F) | (c << 4);
}

void gotoxy(SHORT x, SHORT y)
{
	if (x > size_x) x = size_x;
	if (y > size_y) y = size_y;

	cursor_x = x;
	cursor_y = y;

//	scrptr = screen + ((size_x * (y - 1)) << 1) + ((x - 1) << 1);		// zero based coordinates
	scrptr = screen + ((size_x * y) << 1) + (x << 1);
}

SHORT wherex(void)
{
	return cursor_x;
}

SHORT wherey(void)
{
	return cursor_y;
}

void gettext(int x1, int y1, int x2, int y2, char *buf)
{
	COORD size = { static_cast<SHORT>((x2 - x1) + 1), static_cast<SHORT>((y2 - y1) + 1) }, pos = { 0, 0 };
	SMALL_RECT r = { static_cast<SHORT>(x1), static_cast<SHORT>(y1), static_cast<SHORT>(x2), static_cast<SHORT>(y2) };

//    ReadConsoleOutput(hCon, (CHAR_INFO *)buf, size, pos, &r);
}

void puttext(int x1, int y1, int x2, int y2, char *buf)
{
	COORD size = { static_cast<SHORT>((x2 - x1) + 1), static_cast<SHORT>((y2 - y1) + 1) }, pos = { 0, 0 };
	SMALL_RECT r = { static_cast<SHORT>(x1), static_cast<SHORT>(y1), static_cast<SHORT>(x2), static_cast<SHORT>(y2) };

//    WriteConsoleOutput(hCon, (CHAR_INFO *)buf, size, pos, &r);
}

/*
void get_char(int x, int y, int *ch, int *attrib)
{
	unsigned char *ScrPtr;

	x--; y--;

	ScrPtr = ((unsigned char *)Console_Row[y] + (x << 1));

	*ch     = *ScrPtr++;
	*attrib = *ScrPtr;
}

void put_char(int x, int y, int ch, int attrib)
{
	unsigned char *ScrPtr;

	x--; y--;

	ScrPtr = ((unsigned char *)Console_Row[y] + (x << 1));

	*ScrPtr++ = ch;
	*ScrPtr   = attrib;
}

void window(int l, int t, int r, int b)
{
	left_win = l;
	top_win = t;
	right_win = r;
	bottom_win = b;
	gotoxy(top_win, left_win);
}
*/

void scroll(void)
{
	int t;
	USHORT xsize;
	SHORT *sptr, *dptr;

	xsize = static_cast<USHORT>(size_x << 1u);
	sptr = screen + xsize;
	dptr = screen;

	t = (size_y - 1);
	while (t--)
	{
		memcpy(dptr, sptr, static_cast<size_t>(xsize << 1));
		dptr += xsize;
		sptr += xsize;
	}

	t = size_x;
	while (t--)
	{
		*dptr++ = 0x0020;
		*dptr++ = 0x0007;
	}
}

void c_linefeed(void)
{
	if (cursor_y == size_y)
		scroll();
	else
		gotoxy(cursor_x, cursor_y + 1);

	flip_to_console();
}

void c_carriage(void)
{
	gotoxy(1, cursor_y);
	flip_to_console();
}

void c_retreat(void)
{
	if (cursor_x == 1) return;

	gotoxy(cursor_x - 1, cursor_y);
//    c_putch(' ');
//    gotoxy(cursor_x - 1, cursor_y);
}

void c_rawputch(char ch)
{
	if (cursor_x > size_x)
	{
		cursor_x = 1;

		if (cursor_y != size_y)
			gotoxy(cursor_x, cursor_y + 1);
		else
		{
			scroll();
			gotoxy(cursor_x, cursor_y);
		}
	}
	else
	{
		cursor_x++;
	}

	*scrptr++ = ch;
	*scrptr++ = attrib;
}

void c_rawputs(char *s)
{
	while (*s)
		c_rawputch(*s++);
}

void c_rawputblk(char *s, int len)
{
	while (len--)
		c_rawputch(*s++);
}

void c_rawprintf(char *format, ...)
{
	char tempstr[256], *p;
	va_list stuff;

	p = tempstr;

	va_start(stuff, format);
	vsprintf(p, format, stuff);
	va_end(stuff);

	c_rawputs(tempstr);
}

void c_putch(char ch)
{
	if (ch == '\n')
		c_linefeed();
	else if (ch == '\r')
		c_carriage();
	else if (ch == '\b')
		c_retreat();
	else
		c_rawputch(ch);
}

void c_puts(char *s)
{
	while (*s)
		c_putch(*s++);
}

void c_printf(char *format, ...)
{
	char tempstr[256], *p;
	va_list stuff;

	p = tempstr;

	va_start(stuff, format);
	vsprintf(p, format, stuff);
	va_end(stuff);

	c_puts(tempstr);
}

bool c_kbhit(void)
{
	INPUT_RECORD ir;
	bool done = FALSE;
	DWORD cnt;
	
	while (!done)
	{
		// peek at the current record
		if (PeekConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &cnt) == 0)
			done = TRUE;
		else
		{
			// check for no records in buffer
			if (cnt == 0)
				done = TRUE;
			else
			{
				// was this record a key event?
				if (ir.EventType == KEY_EVENT)
				{
					// is this a key down event?
					if (ir.Event.KeyEvent.bKeyDown == TRUE)
						return TRUE;
				}

				// not a key event or not a key down, so discard the record by reading it
				ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &cnt);
			}
		}
	}

	return FALSE;
}

WORD c_getkeycode(void)
{
	INPUT_RECORD ir;
	DWORD cnt;

	ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &cnt);
	if (ir.EventType != KEY_EVENT)
		return (WORD)-1;

	return ir.Event.KeyEvent.wVirtualKeyCode;
}

// ----------------------------------------------------------------------------

void zap_line(SHORT y, char ch)
{
	int t;

	gotoxy(1, y);
	for (t = 0; t < 80; t++)
		c_rawputch(ch);
}

void center_text(char *str, SHORT y)
{
	int x;

	x = 40 - (strlen(str) >> 1);
	assert(x >= 0 && x < 0xFFFF);
	gotoxy(static_cast<SHORT>(x), y);
	c_puts(str);
}

void draw_window(SHORT x1, SHORT y1, SHORT x2, SHORT y2, int fg, int bg)
{
	SHORT t;
	char hbuf[80], *ptr;

	textcolor(fg);
	textbackground(bg);

	gotoxy(x1, y1); c_putch('+');
	gotoxy(x2, y1); c_putch('+');
	gotoxy(x1, y2); c_putch('+');
	gotoxy(x2, y2); c_putch('+');

	for (t = (x1 + 1); t < x2; t++)
	{
		gotoxy(t, y1); c_putch('-');
		gotoxy(t, y2); c_putch('-');
	}

	ptr = hbuf;
	*ptr++ = '|';
	for (t = (x1 + 1); t < x2; t++)
		*ptr++ = ' ';
	*ptr++ = '|';
	*ptr++ = 0;

	for (t = (y1 + 1); t < y2; t++)
	{
		gotoxy(x1, t); 
		c_puts(hbuf);
	}

	gotoxy(x1 + 1, y1 + 1);
}

void cursor_on(DWORD pct)
{
	CONSOLE_CURSOR_INFO ci;

	ci.dwSize   = pct;
	ci.bVisible = TRUE;
	SetConsoleCursorInfo(hCon, &ci);
}

void cursor_off(void)
{
	CONSOLE_CURSOR_INFO ci;

	ci.dwSize   = 1;
	ci.bVisible = FALSE;
	SetConsoleCursorInfo(hCon, &ci);
}
