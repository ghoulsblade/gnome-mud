/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1998-2002 Robin Ericsson <lobbin@localhost.nu>
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

#include <locale.h>
#include <gconf/gconf-client.h>
#include <gnome.h>
#include <pwd.h>
#include <signal.h>

#ifdef USE_PYTHON
#include <Python.h>
#endif

#include "gnome-mud.h"

static char const rcsid[] =
    "$Id$";

GConfClient *gconf_client;

int main (gint argc, char *argv[])
{
	GnomeProgram *program;
	GError *err = NULL;
	gchar buf[500];
	
	bindtextdomain(PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
	textdomain(PACKAGE);
  
	/* Initialize the GConf library */
	if (!gconf_init(argc, argv, &err))
	{
		g_error(_("Failed to init GConf: %s"), err->message);
		g_error_free(err);
		return 1;
	}

	program = gnome_program_init (PACKAGE, VERSION,
				      LIBGNOMEUI_MODULE,
				      argc, argv,
				      GNOME_PROGRAM_STANDARD_PROPERTIES,
				      GNOME_PARAM_POPT_TABLE,
				      NULL,
				      NULL);
	
	/* Start a GConf client */
	gconf_client = gconf_client_get_default();
	gconf_client_add_dir(gconf_client, "/apps/gnome-mud", GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	
  
	load_prefs   (); /* load preferences */
	load_profiles(); /* load connections and profiles */
	main_window  ();
 
#ifdef USE_PYTHON
	Py_SetProgramName(argv[0]);
	Py_Initialize();
	PySys_SetArgv(argc, argv);
	python_init();
#endif

	g_snprintf(buf, 500, "%s/.gnome-mud/plugins/", g_get_home_dir());
	init_modules(buf);
	init_modules(PKGDATADIR);
  
	gtk_main();
	gnome_config_sync();

#ifdef USE_PYTHON
	python_end();
	Py_Finalize();
#endif
	gdk_exit (0);

	return 0;
}
