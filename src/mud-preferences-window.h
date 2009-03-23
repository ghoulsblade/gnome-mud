/* GNOME-Mud - A simple Mud Client
 * mud-preferences-window.h
 * Copyright (C) 1998-2005 Robin Ericsson <lobbin@localhost.nu>
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

#ifndef MUD_PREFERENCES_WINDOW_H
#define MUD_PREFERENCES_WINDOW_H

G_BEGIN_DECLS

#include <glib.h>
#include <gconf/gconf-client.h>

#define MUD_TYPE_PREFERENCES_WINDOW              (mud_preferences_window_get_type ())
#define MUD_PREFERENCES_WINDOW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_PREFERENCES_WINDOW, MudPreferencesWindow))
#define MUD_PREFERENCES_WINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_PREFERENCES_WINDOW, MudPreferencesWindowClass))
#define MUD_IS_PREFERENCES_WINDOW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_PREFERENCES_WINDOW))
#define MUD_IS_PREFERENCES_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_PREFERENCES_WINDOW))
#define MUD_PREFERENCES_WINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_PREFERENCES_WINDOW, MudPreferenesWindowClass))

typedef struct _MudPreferencesWindow            MudPreferencesWindow;
typedef struct _MudPreferencesWindowClass       MudPreferencesWindowClass;
typedef struct _MudPreferencesWindowPrivate     MudPreferencesWindowPrivate;

#include "mud-window.h"

struct _MudPreferencesWindow
{
    GObject parent_instance;

    MudPreferencesWindowPrivate *priv;
};

struct _MudPreferencesWindowClass
{
    GObjectClass parent_class;
};

GType mud_preferences_window_get_type (void) G_GNUC_CONST;

MudPreferencesWindow* mud_preferences_window_new (const gchar *profile, MudWindow *window);

G_END_DECLS

#endif // MUD_PREFERENCES_WINDOW_H
