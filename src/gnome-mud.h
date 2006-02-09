/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1998-2005 Robin Ericsson <lobbin@localhost.nu>
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

#ifndef __GNOME_MUD_H__
#define __GNOME_MUD_H__

#include <stdio.h>
#include <vte/vte.h>
#include <libgnetwork/gnetwork.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

/*
 * Different type of message, so I'll know what color to use.
 */
#define MESSAGE_ERR     0
#define MESSAGE_NORMAL  1
#define MESSAGE_SENT    2
#define MESSAGE_SYSTEM	3

/*
 * Maximum number of colors
 */
#define C_MAX 16


#endif /* __GNOME_MUD_H__ */
