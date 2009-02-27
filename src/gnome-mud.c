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
#include <gtk/gtk.h>
#include <stdio.h>
#include <sys/stat.h>
#include <gnet.h>

#ifdef ENABLE_GST
#include <gst/gst.h>
#endif

#include "gnome-mud.h"
#include "gnome-mud-icons.h"
#include "mud-connection-view.h"
#include "mud-window.h"
#include "mud-profile.h"
#include "modules.h"
#include "utils.h"

int main (gint argc, char *argv[])
{
    GConfClient  *client;
    GError       *err = NULL;
    gchar         buf[2048];

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

#ifdef ENABLE_GST
    /* Initialize GStreamer */
    gst_init(&argc, &argv);
#endif

    gtk_init(&argc, &argv);

    client = gconf_client_get_default();
    gconf_client_add_dir(client, "/apps/gnome-mud",
            GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);

    g_snprintf(buf, 2048, "%s/.gnome-mud/", g_get_home_dir());
    if(!g_file_test(buf, G_FILE_TEST_IS_DIR))
        mkdir(buf, 0777);

    g_snprintf(buf, 2048, "%s/.gnome-mud/plugins/", g_get_home_dir());
    if(!g_file_test(buf, G_FILE_TEST_IS_DIR))
        mkdir(buf, 0777 );

    g_snprintf(buf, 2048, "%s/.gnome-mud/logs/", g_get_home_dir());
    if(!g_file_test(buf, G_FILE_TEST_IS_DIR))
        mkdir(buf, 0777 );

    g_snprintf(buf, 2048, "%s/.gnome-mud/audio/", g_get_home_dir());
    if(!g_file_test(buf, G_FILE_TEST_IS_DIR))
        mkdir(buf, 0777 );

    init_modules(buf);
    init_modules(PKGDATADIR);

    gtk_about_dialog_set_url_hook(utils_activate_url, NULL, NULL);

    mud_profile_load_profiles();

    gtk_window_set_default_icon_name(GMUD_STOCK_ICON);

    mud_window_new();

    gtk_main();

    gconf_client_suggest_sync(client, &err);
    g_object_unref(client);

    return 0;
}
