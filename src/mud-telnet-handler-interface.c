/* GNOME-Mud - A simple Mud Client
 * mud-telnet-handler-interface.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <glib-object.h>

#include "mud-telnet-handler-interface.h"
#include "mud-telnet.h"

/* Interface Prototypes */
static void mud_telnet_handler_base_init (gpointer klass);

/* Define the Dummy Type */
GType
mud_telnet_handler_get_type (void)
{
    static GType mud_telnet_handler_iface_type = 0;

    if(mud_telnet_handler_iface_type == 0)
    {
        static const GTypeInfo info =
        {
            sizeof (MudTelnetHandlerInterface),
            mud_telnet_handler_base_init, /* base_init */
            NULL                          /* base_finalize */
        };

        mud_telnet_handler_iface_type =
            g_type_register_static(G_TYPE_INTERFACE,
                                   "MudTelnetHandler",
                                   &info,
                                   0);
    }

    return mud_telnet_handler_iface_type;
}

/* Interface Functions */
static void
mud_telnet_handler_base_init(gpointer klass)
{
    static gboolean initialized = FALSE;

    if(!initialized)
    {
        g_object_interface_install_property(klass,
                                            g_param_spec_boolean(
                                                "enabled",
                                                "Enabled",
                                                "Is handler enabled",
                                                FALSE,
                                                G_PARAM_READABLE));

        g_object_interface_install_property(klass,
                                            g_param_spec_int(
                                                "handles-option",
                                                "Handles Option",
                                                "The TelOpt Number it handles",
                                                -1, 50000,-1,
                                                G_PARAM_READABLE));

        g_object_interface_install_property(klass,
                                            g_param_spec_object(
                                                "telnet",
                                                "Telnet",
                                                "The parent telnet object",
                                                MUD_TYPE_TELNET,
                                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        initialized = TRUE;
    }
}

/* Interface Methods */
void
mud_telnet_handler_enable(MudTelnetHandler *self)
{
    g_return_if_fail(MUD_IS_TELNET_HANDLER(self));

    MUD_TELNET_HANDLER_GET_INTERFACE (self)->Enable(self);
}

void
mud_telnet_handler_disable(MudTelnetHandler *self)
{
    g_return_if_fail(MUD_IS_TELNET_HANDLER(self));

    MUD_TELNET_HANDLER_GET_INTERFACE(self)->Disable(self);
}

void
mud_telnet_handler_handle_sub_neg(MudTelnetHandler *self,
                                  guchar *buf,
                                  guint len)
{
    g_return_if_fail(MUD_IS_TELNET_HANDLER(self));

    MUD_TELNET_HANDLER_GET_INTERFACE (self)->HandleSubNeg (self, buf, len);
}

