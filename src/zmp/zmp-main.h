/* GNOME-Mud - A simple Mud Client
 * zmp-main.h
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

#ifndef ZMP_MAIN_H
#define ZMP_MAIN_H

G_BEGIN_DECLS

#include <glib.h>

#define ZMP_TYPE_MAIN              (zmp_main_get_type ())
#define ZMP_MAIN(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), ZMP_TYPE_MAIN, ZmpMain))
#define ZMP_MAIN_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), ZMP_TYPE_MAIN, ZmpMainClass))
#define ZMP_IS_MAIN(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), ZMP_TYPE_MAIN))
#define ZMP_IS_MAIN_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), ZMP_TYPE_MAIN))
#define ZMP_MAIN_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), ZMP_TYPE_MAIN, ZmpMainClass))
#define ZMP_MAIN_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ZMP_TYPE_MAIN, ZmpMainPrivate))

typedef struct _ZmpMain            ZmpMain;
typedef struct _ZmpMainClass       ZmpMainClass;
typedef struct _ZmpMainPrivate     ZmpMainPrivate;

#include <glib.h>
#include "zmp-package-interface.h"

struct _ZmpMainClass
{
    GObjectClass parent_class;
};

struct _ZmpMain
{
    GObject parent_instance;

    /*< private >*/
    ZmpMainPrivate *priv;
};

GType zmp_main_get_type (void);

void zmp_main_register_commands(ZmpMain *self);
ZmpPackage *zmp_main_get_package_by_name(ZmpMain *self, const gchar *package);

G_END_DECLS

#endif // ZMP_MAIN_H

