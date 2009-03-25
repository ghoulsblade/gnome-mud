/* GNOME-Mud - A simple Mud Client
 * mud-window-prefs.h
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

#ifndef MUD_WINDOW_PREFS_H
#define MUD_WINDOW_PREFS_H

G_BEGIN_DECLS

#include <glib.h>

#define MUD_TYPE_WINDOW_PREFS              (mud_window_prefs_get_type ())
#define MUD_WINDOW_PREFS(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_WINDOW_PREFS, MudWindowPrefs))
#define MUD_WINDOW_PREFS_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_WINDOW_PREFS, MudWindowPrefsClass))
#define MUD_IS_WINDOW_PREFS(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_WINDOW_PREFS))
#define MUD_IS_WINDOW_PREFS_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_WINDOW_PREFS))
#define MUD_WINDOW_PREFS_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_WINDOW_PREFS, MudWindowPrefsClass))
#define MUD_WINDOW_PREFS_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MUD_TYPE_WINDOW_PREFS, MudWindowPrefsPrivate))

typedef struct _MudWindowPrefs            MudWindowPrefs;
typedef struct _MudWindowPrefsClass       MudWindowPrefsClass;
typedef struct _MudWindowPrefsPrivate     MudWindowPrefsPrivate;

struct _MudWindowPrefsClass
{
    GObjectClass parent_class;
};

struct _MudWindowPrefs
{
    GObject parent_instance;

    /*< private >*/
    MudWindowPrefsPrivate *priv;
};

GType mud_window_prefs_get_type (void);

G_END_DECLS

#endif // MUD_WINDOW_PREFS_H

