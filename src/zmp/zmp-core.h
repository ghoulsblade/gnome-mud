/* GNOME-Mud - A simple Mud Client
 * zmp-core.h
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

#ifndef ZMP_CORE_H
#define ZMP_CORE_H

G_BEGIN_DECLS

#include <glib.h>

#define ZMP_TYPE_CORE              (zmp_core_get_type ())
#define ZMP_CORE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), ZMP_TYPE_CORE, ZmpCore))
#define ZMP_CORE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), ZMP_TYPE_CORE, ZmpCoreClass))
#define ZMP_IS_CORE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), ZMP_TYPE_CORE))
#define ZMP_IS_CORE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), ZMP_TYPE_CORE))
#define ZMP_CORE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), ZMP_TYPE_CORE, ZmpCoreClass))
#define ZMP_CORE_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ZMP_TYPE_CORE, ZmpCorePrivate))

typedef struct _ZmpCore            ZmpCore;
typedef struct _ZmpCoreClass       ZmpCoreClass;
typedef struct _ZmpCorePrivate     ZmpCorePrivate;

#include "handlers/mud-telnet-handlers.h"

struct _ZmpCoreClass
{
    GObjectClass parent_class;
};

struct _ZmpCore
{
    GObject parent_instance;

    /*< private >*/
    ZmpCorePrivate *priv;
};

GType zmp_core_get_type (void);

void zmp_core_send_ident(MudTelnetZmp *self);

G_END_DECLS

#endif // ZMP_CORE_H

