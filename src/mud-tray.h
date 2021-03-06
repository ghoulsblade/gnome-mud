/* GNOME-Mud - A simple Mud Client
 * mud-tray.h
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

#ifndef MUD_TRAY_H
#define MUD_TRAY_H

G_BEGIN_DECLS

#include <gtk/gtk.h>

#define MUD_TYPE_TRAY              (mud_tray_get_type ())
#define MUD_TRAY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_TRAY, MudTray))
#define MUD_TRAY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_TRAY, MudTrayClass))
#define MUD_IS_TRAY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_TRAY))
#define MUD_IS_TRAY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_TRAY))
#define MUD_TRAY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_TRAY, MudTrayClass))
#define MUD_TRAY_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MUD_TYPE_TRAY, MudTrayPrivate))

typedef struct _MudTray            MudTray;
typedef struct _MudTrayClass       MudTrayClass;
typedef struct _MudTrayPrivate     MudTrayPrivate;

struct _MudTrayClass
{
    GObjectClass parent_class;
};

struct _MudTray
{
    GObject parent_instance;

    /*< private >*/
    MudTrayPrivate *priv;

    /*< public >*/
    GtkWidget *parent_window;
};

enum mud_tray_status
{
    offline,
    offline_connecting,
    online,
    online_connecting
};

GType mud_tray_get_type (void);

void mud_tray_update_icon(MudTray *tray, enum mud_tray_status icon);

G_END_DECLS

#endif // MUD_TRAY_H

