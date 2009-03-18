/* GNOME-Mud - A simple Mud Client
 * zmp-package-interface.c
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

#include "zmp-package-interface.h"
#include "handlers/mud-telnet-handlers.h"
#include "mud-telnet.h"

/* Interface Prototypes */
static void zmp_package_base_init (gpointer klass);

/* Define the Dummy Type */
GType
zmp_package_get_type (void)
{
    static GType zmp_package_iface_type = 0;

    if(zmp_package_iface_type == 0)
    {
        static const GTypeInfo info =
        {
            sizeof (ZmpPackageInterface),
            zmp_package_base_init,        /* base_init */
            NULL                          /* base_finalize */
        };

        zmp_package_iface_type =
            g_type_register_static(G_TYPE_INTERFACE,
                                   "ZmpPackage",
                                   &info,
                                   0);
    }

    return zmp_package_iface_type;
}

/* Interface Functions */
static void
zmp_package_base_init(gpointer klass)
{
    static gboolean initialized = FALSE;

    if(!initialized)
    {
        g_object_interface_install_property(klass,
                                            g_param_spec_string(
                                                "package",
                                                "Package",
                                                "The name of the ZMP Package",
                                                NULL,
                                                G_PARAM_READABLE));

        g_object_interface_install_property(klass,
                                            g_param_spec_object(
                                                "parent",
                                                "Parent",
                                                "The parent MudTelnetZMP",
                                                MUD_TYPE_TELNET_ZMP,
                                                G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

        initialized = TRUE;
    }
}

/* Interface Methods */
void
zmp_package_register_commands(ZmpPackage *self, MudTelnetZmp *zmp)
{
    g_return_if_fail(MUD_IS_TELNET_ZMP(zmp));

    ZMP_PACKAGE_GET_INTERFACE (self)->register_commands(zmp);
}

