/* AMCL - A simple Mud CLient
 * Copyright (C) 1998-2000 Robin Ericsson <lobbin@localhost.nu>
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
#include <pwd.h>

#include "amcl.h"

static char const rcsid[] =
    "$Id$";


int main (gint argc, char *argv[])
{
  char buf[500];
  struct sigaction act;
  
  memset(&act, 0, sizeof(act));
  
  gtk_init (&argc, &argv);
  gdk_init (&argc, &argv);
  
  act.sa_handler = SIG_DFL;
  sigaction(SIGSEGV, &act, NULL);
  
  init_uid();
  
  load_aliases(); /* load aliases */
  load_actions(); /* load "on" actions */
  load_prefs  (); /* load preferences */
  load_wizard (); /* load connections */
  load_colors (); /* load colors */
  load_keys   (); /* load key bindings */
  init_window ();
  
  g_snprintf(buf, 500, "%s/.amcl/plugins/", uid_info->pw_dir);
  init_modules(buf);
  //init_modules(PKGDATADIR);
  
  gtk_main ( );

  save_plugins();
  
  gdk_exit (0);
  
  return 0;
}
