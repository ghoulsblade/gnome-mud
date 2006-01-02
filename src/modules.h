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

#include <dlfcn.h>
#include "mud-connection-view.h"
#include "mud-window.h"

#include "modules-structures.h"

/*
 * Functions
 */
PLUGIN_OBJECT *plugin_get_plugin_object_by_handle (gint handle   );
PLUGIN_OBJECT *plugin_query    (gchar *plugin_name, gchar *pp    );
void           plugin_register (PLUGIN_OBJECT *plugin            );
void	       popup_message(const gchar *data);
void 	       init_modules_win(MudWindow *win);
void do_plugin_information(GtkWidget *widget, gpointer data);
/*
 * Variables
 */
extern MudWindow *gGMudWindow;
extern GList *Plugin_list;
extern GList *Plugin_data_list;
