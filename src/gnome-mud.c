/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1998-2006 Robin Ericsson <lobbin@localhost.nu>
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

#if HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gconf/gconf-client.h>
#include <glib/gi18n.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkaboutdialog.h>
#include <stdio.h>
#include <sys/stat.h>
#include <gnet.h>
#include <gst/gst.h>

#ifdef USE_PYTHON
//#include <Python.h>
#endif

#include "gnome-mud.h"
#include "mud-connection-view.h"
#include "mud-window.h"
#include "mud-profile.h"
#include "modules.h"
#include "utils.h"

gboolean gconf_sanity_check_string (GConfClient *client, const gchar* key)
{
  gchar *string;
  GError *error = NULL;

  string = gconf_client_get_string (client, key, &error);

  if (error) {
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new (NULL,
                                     0,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_OK,
                                     _("There was an error accessing GConf: %s"),
                                     error->message);
    gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
    gtk_dialog_run(GTK_DIALOG(dialog));
    return FALSE;
  }
  if (!string) {
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new (NULL,
                                     0,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_OK,
                                     "<span weight=\"bold\" size=\"larger\">%s</span>\n\n%s",
                                     _("The default configuration values could not be retrieved correctly."),
                                     _("Please check your GConf configuration, specifically that the schemas have been installed correctly."));
    gtk_label_set_use_markup (GTK_LABEL (GTK_MESSAGE_DIALOG (dialog)->label), TRUE);
    gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy (dialog);
    return FALSE;
  }
  g_free (string);
  return TRUE;
}

int main (gint argc, char *argv[])
{
	GConfClient  *gconf_client;
	GError       *err = NULL;
	gchar         buf[500];

#ifdef ENABLE_NLS
	/* Initialize internationalization */
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif

	/* Initialize the GConf library */
	if (!gconf_init(argc, argv, &err))
	{
		g_error(_("Failed to init GConf: %s"), err->message);
		g_error_free(err);
		return 1;
	}

    /* Initialize the Gnet library */
    gnet_init();

	/* Initialize GStreamer */
	gst_init(&argc, &argv);

	gtk_init(&argc, &argv);

	/* Start a GConf client */
	gconf_client = gconf_client_get_default();
	if (!gconf_sanity_check_string (gconf_client, "/apps/gnome-mud/functionality/terminal_type")) {
		return 1;
	}
	gconf_client_add_dir(gconf_client, "/apps/gnome-mud", GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);


	mud_profile_load_profiles();

	gtk_window_set_icon_from_file(GTK_WINDOW(mud_window_get_window(mud_window_new(gconf_client))),
					PIXMAPSDIR "/gnome-mud.png", &err);

#ifdef USE_PYTHON
	//Py_SetProgramName(argv[0]);
	//Py_Initialize();
	//PySys_SetArgv(argc, argv);
	//python_init();
#endif

	g_snprintf(buf, 500, "%s/.gnome-mud/plugins/", g_get_home_dir());
	if(!g_file_test(buf, G_FILE_TEST_IS_DIR))
		mkdir(buf, 0777 );

	init_modules(buf);
	init_modules(PKGDATADIR);

	g_snprintf(buf, 500, "%s/.gnome-mud/logs/", g_get_home_dir());
	if(!g_file_test(buf, G_FILE_TEST_IS_DIR))
		mkdir(buf, 0777 );

	g_snprintf(buf, 500, "%s/.gnome-mud/audio/", g_get_home_dir());
	if(!g_file_test(buf, G_FILE_TEST_IS_DIR))
		mkdir(buf, 0777 );

	gtk_about_dialog_set_url_hook(utils_activate_url, NULL, NULL);
	gtk_main();
	gconf_client_suggest_sync(gconf_client, &err);

#ifdef USE_PYTHON
	//python_end();
	//Py_Finalize();
#endif
	//gdk_exit (0);

	return 0;
}
