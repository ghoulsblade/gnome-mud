/* GNOME-Mud - A simple Mud Client
 * mud-telnet-mccp.h
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

#ifndef MUD_TELNET_MCCP_H
#define MUD_TELNET_MCCP_H

#ifdef ENABLE_MCCP

G_BEGIN_DECLS

#include <glib.h>
#include <zlib.h>

#define MUD_TYPE_TELNET_MCCP              (mud_telnet_mccp_get_type ())
#define MUD_TELNET_MCCP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_TELNET_MCCP, MudTelnetMccp))
#define MUD_TELNET_MCCP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_TELNET_MCCP, MudTelnetMccpClass))
#define MUD_IS_TELNET_MCCP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_TELNET_MCCP))
#define MUD_IS_TELNET_MCCP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_TELNET_MCCP))
#define MUD_TELNET_MCCP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_TELNET_MCCP, MudTelnetMccpClass))
#define MUD_TELNET_MCCP_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MUD_TYPE_TELNET_MCCP, MudTelnetMccpPrivate))

typedef struct _MudTelnetMccp            MudTelnetMccp;
typedef struct _MudTelnetMccpClass       MudTelnetMccpClass;
typedef struct _MudTelnetMccpPrivate     MudTelnetMccpPrivate;

typedef struct z_stream_s z_stream;

struct _MudTelnetMccpClass
{
    GObjectClass parent_class;
};

struct _MudTelnetMccp
{
    GObject parent_instance;

    /*< private >*/
    MudTelnetMccpPrivate *priv;
};

GType mud_telnet_mccp_get_type (void);

GString *mud_mccp_decompress(MudTelnetMccp *self, guchar *buffer, guint32 length);

G_END_DECLS

#endif // ENABLE_MCCP

#endif // MUD_TELNET_MCCP_H

