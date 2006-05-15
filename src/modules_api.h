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
#include "gnome-mud.h"
#include "modules-structures.h"

/*
 * Runnable functions from inside the module.
 */
extern void     plugin_popup_message          (gchar *message                       );
extern void plugin_add_connection_text(gchar *message, gint color, MudConnectionView *view);
extern gboolean plugin_register_menu          (GModule *h, gchar *name, gchar *function );
extern gboolean plugin_register_data_incoming (GModule *h, gchar *function              );
extern gboolean plugin_register_data_outgoing (GModule *h, gchar *function              );
extern void plugin_connection_send	      (gchar *text,  MudConnectionView *view);

/* Hack for the moment, will refactor with plugin rewrite - lh */
extern gboolean PluginGag;
extern void plugin_gag_flag(void);

#endif /* __MODULE__ */
