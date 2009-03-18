/* GNOME-Mud - A simple Mud Client
 * mud-subwindow.h
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

#ifndef MUD_SUBWINDOW_H
#define MUD_SUBWINDOW_H

G_BEGIN_DECLS

#define MUD_TYPE_SUBWINDOW              (mud_subwindow_get_type ())
#define MUD_SUBWINDOW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_SUBWINDOW, MudSubwindow))
#define MUD_SUBWINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_SUBWINDOW, MudSubwindowClass))
#define MUD_IS_SUBWINDOW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_SUBWINDOW))
#define MUD_IS_SUBWINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_SUBWINDOW))
#define MUD_SUBWINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_SUBWINDOW, MudSubwindowClass))
#define MUD_SUBWINDOW_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MUD_TYPE_SUBWINDOW, MudSubwindowPrivate))

typedef struct _MudSubwindow            MudSubwindow;
typedef struct _MudSubwindowClass       MudSubwindowClass;
typedef struct _MudSubwindowPrivate     MudSubwindowPrivate;

#include <glib.h>

struct _MudSubwindowClass
{
    GObjectClass parent_class;
};

struct _MudSubwindow
{
    GObject parent_instance;

    /*< private >*/
    MudSubwindowPrivate *priv;
};

GType mud_subwindow_get_type (void);

void mud_subwindow_show(MudSubwindow *self);
void mud_subwindow_hide(MudSubwindow *self);
void mud_subwindow_enable_input(MudSubwindow *self, gboolean enable);
void mud_subwindow_set_title(MudSubwindow *self, const gchar *title);
void mud_subwindow_set_size(MudSubwindow *self, guint width, guint height);
void mud_subwindow_reread_profile(MudSubwindow *self);
void mud_subwindow_feed(MudSubwindow *self,
                        const gchar *data,
                        guint length);

G_END_DECLS

#endif // MUD_SUBWINDOW_H

