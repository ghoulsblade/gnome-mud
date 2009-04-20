/* GNOME-Mud - A simple Mud Client
 * gnome-mud.c
 * Copyright (C) 1998-2005 Robin Ericsson <lobbin@localhost.nu>
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
#include "utils.h"
#include "debug-logger.h"
#include "mud-trigger.h"

gint
main (gint argc, char *argv[])
{
    MudWindow *window;
    GConfClient *client;
    DebugLogger *logger;
    GError      *err = NULL;
    GString *dir;

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

    gtk_init(&argc, &argv);

    /* Initialize the Gnet library */
    gnet_init();

#ifdef ENABLE_GST
    /* Initialize GStreamer */
    gst_init(&argc, &argv);
#endif

    client = gconf_client_get_default();
    gconf_client_add_dir(client, "/apps/gnome-mud",
            GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);

    dir = g_string_new(NULL);
    g_string_printf(dir,
                    "%s%cgnome-mud%clogs",
                    g_get_user_data_dir(),
                    G_DIR_SEPARATOR,
                    G_DIR_SEPARATOR);
    g_mkdir_with_parents(dir->str, 0755);      
    g_string_free(dir, TRUE);

    dir = g_string_new(NULL);
    g_string_printf(dir,
                    "%s%cgnome-mud%caudio",
                    g_get_user_data_dir(),
                    G_DIR_SEPARATOR,
                    G_DIR_SEPARATOR);
    g_mkdir_with_parents(dir->str, 0755); 
    g_string_free(dir, TRUE);

    gtk_about_dialog_set_url_hook(utils_activate_url, NULL, NULL);

    gtk_window_set_default_icon_name(GMUD_STOCK_ICON);

     /*Setup debug logging */
     logger = g_object_new(TYPE_DEBUG_LOGGER, 
                          "use-color", TRUE,
                          "closeable", FALSE,
                          NULL);

    debug_logger_add_domain(logger, "Gnome-Mud", TRUE);
    debug_logger_add_domain(logger, "Telnet", FALSE);
    debug_logger_add_standard_domains(logger);

#ifdef ENABLE_DEBUG_LOGGER
    debug_logger_create_window(logger);
#endif

    //g_log_set_always_fatal(G_LOG_LEVEL_CRITICAL|G_LOG_LEVEL_WARNING);

    /* Let 'er rip */
    window = g_object_new(MUD_TYPE_WINDOW, NULL);

    {
        MudLineBufferLine *line = g_new0(MudLineBufferLine, 1);
        MudTrigger *trigger = g_object_new(MUD_TYPE_TRIGGER,
                                           "trigger-key", "test",
                                           "profile-key", "test",
                                           "lines", 1,
                                           "action-type", MUD_TRIGGER_ACTION_TEXT,
                                           "action", "=== %0 ===\n\t%1\n\t%2",
                                           NULL);

        line->line = g_strdup("Foo says, \"foobazbar\"");

        mud_trigger_execute(trigger, line, strlen(line->line));

        g_free(line->line);
        g_free(line);
        g_object_unref(trigger);
    }

    {
        g_printf("%c%c%d%c%d%c%d%c", '\x1B', '[', 1 , ';', 36, ';', 40, 'm');
        g_printf("%s", "test");
        g_printf("%c%c%d%c\n", '\x1B', '[', 0, 'm');
    }

    gtk_main();

    gconf_client_suggest_sync(client, &err);
    
    g_object_unref(logger);
    g_object_unref(client);

    return 0;
}

