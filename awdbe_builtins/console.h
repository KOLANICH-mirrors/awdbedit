#pragma once
//
// Award BIOS Editor - console.h
// Copyright (C) 2002-2004, Michael Tedder
// All rights reserved
//
// $Id: console.h,v 1.3 2004/04/11 07:17:15 bpoint Exp $
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

#ifndef CONSOLE_H
#define CONSOLE_H

#define MODE_80x25          1
#define MODE_80x50          2

#define COLOR_BLACK			0
#define COLOR_BLUE			1
#define COLOR_GREEN			2
#define COLOR_CYAN			3
#define COLOR_RED			4
#define COLOR_MAGENTA		5
#define COLOR_BROWN			6
#define COLOR_GREY			7
#define COLOR_DGREY			8
#define COLOR_LBLUE			9
#define COLOR_LGREEN		10
#define COLOR_LCYAN			11
#define COLOR_LRED			12
#define COLOR_LMAGENTA		13
#define COLOR_YELLOW		14
#define COLOR_WHITE			15

HANDLE getConsoleHandle(void);

void init_console(bool useExistConsole, int mode, PHANDLER_ROUTINE ctrlHandler);
void free_console(void);
void flip_to_console(void);
SHORT *get_screen_ptr(void);
void update_console_position(void);

void go_25(void);
void go_50(void);
void clrscr(void);
void textattr(WORD c);
void textcolor(int c);
void textbackground(WORD c);
void gotoxy(SHORT x, SHORT y);
SHORT wherex(void);
SHORT wherey(void);
void gettext(int x1, int y1, int x2, int y2, char *buf);
void puttext(int x1, int y1, int x2, int y2, char *buf);
/*
void get_char(int x, int y, int *ch, int *attrib);
void put_char(int x, int y, int ch, int attrib);
void window(int l, int t, int r, int b);
*/
void scroll(void);
void c_linefeed(void);
void c_carriage(void);
void c_retreat(void);
void c_putch(char ch);
void c_puts(char *s);
void c_printf(char *format, ...);

void c_rawputch(char ch);
void c_rawputs(char *s);
void c_rawputblk(char *s, int len);
void c_rawprintf(char *format, ...);

void zap_line(SHORT y, char ch);
void center_text(char *str, int y);
void draw_window(int x1, int y1, int x2, int y2, int fg, int bg);

void cursor_on(DWORD pct);
void cursor_off(void);

bool c_kbhit(void);
WORD c_getkeycode(void);

#endif
