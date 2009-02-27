/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1999-2006 Robin Ericsson <lobbin@localhost.nu>
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

//#ifdef __MODULE__

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <glib/gi18n.h>
#include <glade/glade-xml.h>

#include "gnome-mud.h"
#include "mud-connection-view.h"
#include "mud-window.h"
#include "modules.h"

/* Hack for the moment, will refactor with plugin rewrite -lh */
gboolean PluginGag;

void plugin_gag_flag(void)
{
    PluginGag = TRUE;
}

void plugin_popup_message (gchar *message)
{
    popup_message (message);
}

void plugin_connection_send(gchar *text, MudConnectionView *view)
{
    mud_connection_view_send (view, text);
}

void plugin_add_connection_text(gchar *message, gint color, MudConnectionView *view)
{
    mud_connection_view_add_text(view, message, color);
}

gboolean plugin_register_menu (GModule *handle, gchar *name, gchar *function)
{
    GtkSignalFunc  sig_function;
    GtkWidget *newMenuItem;
    void *temp;
    gpointer *sigptr;

    /* Stupid hack for gmodule */
    temp = (void *)&sig_function;
    sigptr = (gpointer *)temp;

    if (!g_module_symbol(handle, function, sigptr))
    {
        g_message (_("Error while registering the menu: %s"), g_module_error());
        return FALSE;
    }

    newMenuItem = gtk_menu_item_new_with_label(name);
    gtk_widget_show(newMenuItem);

    gtk_menu_shell_prepend(GTK_MENU_SHELL(pluginMenu), newMenuItem);

    gtk_signal_connect(GTK_OBJECT(newMenuItem), "activate", sig_function, NULL);

    return TRUE;
}

gboolean plugin_register_data (GModule *handle, gchar *function, PLUGIN_DATA_DIRECTION dir)
{
    PLUGIN_DATA    * data;
    plugin_datafunc  datafunc;
    void *temp;
    gpointer *dataptr;

    /* Stupid hack for Gmodule */
    temp = (void *)&datafunc;
    dataptr = (gpointer *)temp;

    if (!g_module_symbol(handle, function, dataptr))
    {
        g_message (_("Error while registering data %s: %s"), dir == PLUGIN_DATA_IN ? "incoming" : "outgoing",
                g_module_error());
        return FALSE;
    }

    data = g_new0(PLUGIN_DATA, 1);

    if ((data->plugin = plugin_get_plugin_object_by_handle(handle)) == NULL)
        g_message(_("Error while getting plugin from handle."));

    data->datafunc = datafunc;
    data->dir      = dir;

    Plugin_data_list = g_list_append(Plugin_data_list, (gpointer) data);

    PluginGag = FALSE;

    return TRUE;
}

gboolean plugin_register_data_incoming (GModule *handle, gchar *function)
{
    return plugin_register_data (handle, function, PLUGIN_DATA_IN);
}

gboolean plugin_register_data_outgoing (GModule *handle, gchar *function)
{
    return plugin_register_data (handle, function, PLUGIN_DATA_OUT);
}

//#endif /* __MODULE__ */
