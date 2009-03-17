/* GNOME-Mud - A simple Mud Client
 * zmp-package-interface.h
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

#ifndef ZMP_PACKAGE_INTERFACE_H
#define ZMP_PACKAGE_INTERFACE_H

G_BEGIN_DECLS

#define ZMP_PACKAGE_TYPE                (zmp_package_get_type ())
#define ZMP_PACKAGE(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), ZMP_PACKAGE_TYPE, ZmpPackage))
#define ZMP_IS_PACKAGE(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ZMP_PACKAGE_TYPE))
#define ZMP_PACKAGE_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), ZMP_PACKAGE_TYPE, ZmpPackageInterface))

typedef struct _ZmpPackage ZmpPackage; // Dummy object
typedef struct _ZmpPackageInterface ZmpPackageInterface;

#include <glib.h>
#include "mud-telnet.h"
#include "handlers/mud-telnet-handlers.h"

struct _ZmpPackageInterface
{
    GTypeInterface parent;

    /* Interface Methods */
    void (*register_commands)(MudTelnetZmp *zmp);
};

GType zmp_package_get_type (void);

void zmp_package_register_commands(ZmpPackage *self, MudTelnetZmp *zmp);

G_END_DECLS

#endif // ZMP_PACKAGE_INTERFACE_H

