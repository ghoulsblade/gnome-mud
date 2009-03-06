/* GNOME-Mud - A simple Mud Client
 * mud-connections.h
 * Copyright (C) 2008-2009 Les Harris <lharris@gnome.org>
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

#ifndef MUD_CONNECTIONS_H
#define MUD_CONNECTIONS_H

G_BEGIN_DECLS

#include "mud-window.h"

#define MUD_TYPE_CONNECTIONS              (mud_connections_get_type ())
#define MUD_CONNECTIONS(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_CONNECTIONS, MudConnections))
#define MUD_CONNECTIONS_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_CONNECTIONS, MudConnectionsClass))
#define MUD_IS_CONNECTIONS(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_CONNECTIONS))
#define MUD_IS_CONNECTIONS_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_CONNECTIONS))
#define MUD_CONNECTIONS_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_CONNECTIONS, MudConnectionsClass))
#define MUD_CONNECTIONS_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MUD_TYPE_CONNECTIONS, MudConnectionsPrivate))

typedef struct _MudConnections            MudConnections;
typedef struct _MudConnectionsClass       MudConnectionsClass;
typedef struct _MudConnectionsPrivate     MudConnectionsPrivate;

struct _MudConnectionsClass
{
    GObjectClass parent_class;
};

struct _MudConnections
{
    GObject parent_instance;

    /*< private >*/
    MudConnectionsPrivate *priv;

    /*< public >*/
    MudWindow *parent_window;
};

GType mud_connections_get_type (void);

G_END_DECLS

#endif // MUD_CONNECTIONS_H

