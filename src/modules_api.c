/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1999-2001 Robin Ericsson <lobbin@localhost.nu>
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

#ifdef __MODULE__
#include "config.h"
#include <gnome.h>
#include <libintl.h>
#include <stdio.h>

#include "gnome-mud.h"
#include "modules.h"

/* FIXME */
extern GtkWidget  *window;

static char const rcsid[] = 
    "$Id$";

extern CONNECTION_DATA *main_connection;

void plugin_popup_message (gchar *message)
{
  popup_window (message);
}

void plugin_add_connection_text(CONNECTION_DATA *connection, gchar *message, gint color)
{
  if (connection == NULL || connection->window == NULL)
    textfield_add (main_connection->window, message, color);
  else
    textfield_add (connection->window, message, color);
}

gboolean plugin_register_menu (gint handle, gchar *name, gchar *function)
{
  GtkSignalFunc  sig_function;

  if ((sig_function = (GtkSignalFunc) dlsym ((void *) handle, function)) == NULL) {
    g_message (_("Error register menu: %s"), dlerror());
    return FALSE;
  }

  /* FIXME */
/*   menu_place = gtk_menu_item_new_with_label(name); */
/*   gtk_menu_prepend (GTK_MENU (settings_menu[7].widget), menu_place); */
/*   gtk_widget_show (menu_place); */
/*   gtk_signal_connect (GTK_OBJECT (menu_place), "activate", */
/* 		      sig_function, NULL); */
  
  return TRUE;
}

gboolean plugin_register_data (gint handle, gchar *function, PLUGIN_DATA_DIRECTION dir)
{
  PLUGIN_DATA    * data;
  plugin_datafunc  datafunc;

  if ((datafunc = (plugin_datafunc) dlsym ((void *) handle, function)) == NULL) {
    g_message (_("Error register for data %s: %s"), dir == PLUGIN_DATA_IN ? "incoming" : "outgoing",
	       dlerror());
    return FALSE;
  }

  data = g_new0(PLUGIN_DATA, 1);

  if ((data->plugin = plugin_get_plugin_object_by_handle(handle)) == NULL)
    g_message(_("Error getting plugin from handle."));

  data->datafunc = datafunc;
  data->dir      = dir;

  Plugin_data_list = g_list_append(Plugin_data_list, (gpointer) data);

  return TRUE;
}

gboolean plugin_register_data_incoming (gint handle, gchar *function)
{
  return plugin_register_data (handle, function, PLUGIN_DATA_IN);
}

gboolean plugin_register_data_outgoing (gint handle, gchar *function)
{
  return plugin_register_data (handle, function, PLUGIN_DATA_OUT);
}

#endif /* __MODULE__ */
