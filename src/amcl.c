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

#include <signal.h>

#include "amcl.h"

static char const rcsid[] =
    "$Id$";


int main (gint argc, char *argv[])
{
    struct sigaction act;

    memset(&act, 0, sizeof(act));

    gtk_init (&argc, &argv);
    gdk_init (&argc, &argv);

    act.sa_handler = SIG_DFL;
    sigaction(SIGSEGV, &act, NULL);
    
    load_aliases();
    load_actions();
    load_prefs  ();
    load_wizard ();
    init_colors ();
    init_window ();

    init_modules("./plugins/");

    gtk_main ( );
    gdk_exit (0);

    return 0;
}
