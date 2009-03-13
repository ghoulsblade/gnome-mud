/* GNOME-Mud - A simple Mud Client
 * mud-telnet-echo.h
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

#ifndef MUD_TELNET_ECHO_H
#define MUD_TELNET_ECHO_H

G_BEGIN_DECLS

#include <glib.h>

#define MUD_TYPE_TELNET_ECHO              (mud_telnet_echo_get_type ())
#define MUD_TELNET_ECHO(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_TELNET_ECHO, MudTelnetEcho))
#define MUD_TELNET_ECHO_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_TELNET_ECHO, MudTelnetEchoClass))
#define MUD_IS_TELNET_ECHO(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_TELNET_ECHO))
#define MUD_IS_TELNET_ECHO_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_TELNET_ECHO))
#define MUD_TELNET_ECHO_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_TELNET_ECHO, MudTelnetEchoClass))
#define MUD_TELNET_ECHO_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MUD_TYPE_TELNET_ECHO, MudTelnetEchoPrivate))

typedef struct _MudTelnetEcho            MudTelnetEcho;
typedef struct _MudTelnetEchoClass       MudTelnetEchoClass;
typedef struct _MudTelnetEchoPrivate     MudTelnetEchoPrivate;

struct _MudTelnetEchoClass
{
    GObjectClass parent_class;
};

struct _MudTelnetEcho
{
    GObject parent_instance;

    /*< private >*/
    MudTelnetEchoPrivate *priv;
};

GType mud_telnet_echo_get_type (void);

G_END_DECLS

#endif // MUD_TELNET_ECHO_H

