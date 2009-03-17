/* GNOME-Mud - A simple Mud Client
 * mud-telnet-handler-interface.h
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

#ifndef MUD_TELNET_HANDLER_INTERFACE_H
#define MUD_TELNET_HANDLER_INTERFACE_H

G_BEGIN_DECLS

#define MUD_TELNET_HANDLER_TYPE                (mud_telnet_handler_get_type ())
#define MUD_TELNET_HANDLER(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), MUD_TELNET_HANDLER_TYPE, MudTelnetHandler))
#define MUD_IS_TELNET_HANDLER(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MUD_TELNET_HANDLER_TYPE))
#define MUD_TELNET_HANDLER_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), MUD_TELNET_HANDLER_TYPE, MudTelnetHandlerInterface))

typedef struct _MudTelnetHandler MudTelnetHandler; // Dummy object
typedef struct _MudTelnetHandlerInterface MudTelnetHandlerInterface;

#include <glib.h>
#include "mud-telnet.h"

struct _MudTelnetHandlerInterface
{
    GTypeInterface parent;

    /* Interface Methods */
    void (*Enable)      (MudTelnetHandler *self);
    void (*Disable)     (MudTelnetHandler *self);
    void (*HandleSubNeg)(MudTelnetHandler *self,
                         guchar *buf,
                         guint len);

};

GType mud_telnet_handler_get_type (void);

/* Typedefs for Casting */
typedef void (*MudTelnetHandlerEnableFunc)(MudTelnetHandler *self);
typedef void (*MudTelnetHandlerDisableFunc)(MudTelnetHandler *self);
typedef void (*MudTelnetHandlerHandleSubNeg)(MudTelnetHandler *self,
                                            guchar *buf,
                                            guint len);

/* Interface Method Prototypes */

void mud_telnet_handler_enable(MudTelnetHandler *self);
void mud_telnet_handler_disable(MudTelnetHandler *self);
void mud_telnet_handler_handle_sub_neg(MudTelnetHandler *self,
                                       guchar *buf,
                                       guint len);

G_END_DECLS

#endif // MUD_TELNET_HANDLER_INTERFACE_H

