/* AMCL - A simple Mud CLient
 * Copyright (C) 1998-1999 Robin Ericsson <lobbin@localhost.nu>
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

#include "config.h"
#include <gtk/gtk.h>
#include <glib.h>
#include <libintl.h>
#include <stdio.h>

#include "amcl.h"
#include "version.h"

#define _(string) gettext(string)

static char const rcsid[] =
    "$Id$";


char * get_version_info (char *buf)
{
    sprintf (buf, _("Welcome to AMCL Version %s\nCompiled by %s on %s the %s at %s\nUsing %s and %s.\n\n"),
             PROG_VERSION,
             COMPILE_BY, COMPILE_HOST, COMPILE_DATE, COMPILE_TIME,
             CC_VERSION, GTK_VERSION);
    return buf;
}
