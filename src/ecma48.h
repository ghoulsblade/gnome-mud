/* GNOME-Mud - A simple Mud Client
 * ecma48.h
 * Copyright (C) 2005-2009 Les Harris <lharris@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef ECMA_48_H
#define ECMA_48_H

G_BEGIN_DECLS

#define ECMA_ESCAPE_BYTE         0x1B
#define ECMA_ESCAPE_BYTE_C       '\x1B'

#define ECMA_ATTRIBUTE_NORMAL    0
#define ECMA_ATTRIBUTE_BOLD      1
#define ECMA_ATTRIBUTE_ITALIC    3
#define ECMA_ATTRIBUTE_UNDERLINE 4
#define ECMA_ATTRIBUTE_BLINK     5
#define ECMA_ATTRIBUTE_REVERSE   7
#define ECMA_ATTRIBUTE_NODISPLAY 8

#define ECMA_FORECOLOR_BLACK     30
#define ECMA_FORECOLOR_RED       31
#define ECMA_FORECOLOR_GREEN     32
#define ECMA_FORECOLOR_YELLOW    33
#define ECMA_FORECOLOR_BLUE      34
#define ECMA_FORECOLOR_MAGENTA   35
#define ECMA_FORECOLOR_CYAN      36
#define ECMA_FORECOLOR_WHITE     37

#define ECMA_BACKCOLOR_BLACK     40
#define ECMA_BACKCOLOR_RED       41
#define ECMA_BACKCOLOR_GREEN     42
#define ECMA_BACKCOLOR_YELLOW    43
#define ECMA_BACKCOLOR_BLUE      44
#define ECMA_BACKCOLOR_MAGENTA   45
#define ECMA_BACKCOLOR_CYAN      46
#define ECMA_BACKCOLOR_WHITE     47

#define XTERM_FORECOLOR          38
#define XTERM_BACKCOLOR          48

G_END_DECLS

#endif // ECMA_48_H

