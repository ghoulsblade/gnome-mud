/* GNOME-Mud - A simple Mud Client
 * zmp-subwindow.h
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

#ifndef ZMP_SUBWINDOW_H
#define ZMP_SUBWINDOW_H

G_BEGIN_DECLS

#include <glib.h>

#define ZMP_TYPE_SUBWINDOW              (zmp_subwindow_get_type ())
#define ZMP_SUBWINDOW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), ZMP_TYPE_SUBWINDOW, ZmpSubwindow))
#define ZMP_SUBWINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), ZMP_TYPE_SUBWINDOW, ZmpSubwindowClass))
#define ZMP_IS_SUBWINDOW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), ZMP_TYPE_SUBWINDOW))
#define ZMP_IS_SUBWINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), ZMP_TYPE_SUBWINDOW))
#define ZMP_SUBWINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), ZMP_TYPE_SUBWINDOW, ZmpSubwindowClass))
#define ZMP_SUBWINDOW_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), ZMP_TYPE_SUBWINDOW, ZmpSubwindowPrivate))

typedef struct _ZmpSubwindow            ZmpSubwindow;
typedef struct _ZmpSubwindowClass       ZmpSubwindowClass;
typedef struct _ZmpSubwindowPrivate     ZmpSubwindowPrivate;

#include "handlers/mud-telnet-handlers.h"

struct _ZmpSubwindowClass
{
    GObjectClass parent_class;
};

struct _ZmpSubwindow
{
    GObject parent_instance;

    /*< private >*/
    ZmpSubwindowPrivate *priv;
};

GType zmp_subwindow_get_type (void);

G_END_DECLS

#endif // ZMP_SUBWINDOW_H

