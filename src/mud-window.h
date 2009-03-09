/* GNOME-Mud - A simple Mud Client
 * mud-window.h
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

#ifndef MUD_WINDOW_H
#define MUD_WINDOW_H

G_BEGIN_DECLS

#include <gtk/gtk.h>
#include "mud-tray.h"

#define TYPE_MUD_WINDOW             (mud_window_get_type ())
#define MUD_WINDOW(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), TYPE_MUD_WINDOW, MudWindow))
#define MUD_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_MUD_WINDOW, MudWindowClass))
#define IS_MUD_WINDOW(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object), TYPE_MUD_WINDOW))
#define IS_MUD_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_MUD_WINDOW))
#define MUD_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_MUD_WINDOW, MudWindowClass))
#define MUD_WINDOW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_MUD_WINDOW, MudWindowPrivate))

typedef struct _MudWindow           MudWindow;
typedef struct _MudWindowClass      MudWindowClass;
typedef struct _MudWindowPrivate    MudWindowPrivate;

struct _MudWindowClass
{
    GObjectClass parent_class;
};

struct _MudWindow
{
    GObject parent_instance;

    /*< private >*/
    MudWindowPrivate *priv;

    /*< public >*/
    GtkWindow *window;
    MudTray *tray;
};

GType mud_window_get_type (void);

void mud_window_add_connection_view(MudWindow *window, GObject *view, gchar *tabLbl);
void mud_window_populate_profiles_menu(MudWindow *window);
void mud_window_profile_menu_set_active(MudWindow *window, gchar *name);
void mud_window_close_current_window(MudWindow *window);
void mud_window_disconnected(MudWindow *window);

G_END_DECLS

#endif /* MUD_WINDOW_H */
