/* GNOME-Mud - A simple Mud Client
 * mud-window-profile.h
 * Copyright (C) 2008-2009 Les Harris <lharris@gnome.org>
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

#ifndef MUD_PROFILE_WINDOW_H
#define MUD_PROFILE_WINDOW_H

G_BEGIN_DECLS

#include <gconf/gconf-client.h>
#include "mud-window.h"

#define MUD_TYPE_PROFILE_WINDOW              (mud_profile_window_get_type ())
#define MUD_PROFILE_WINDOW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_PROFILE_WINDOW, MudProfileWindow))
#define MUD_PROFILE_WINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_PROFILE_WINDOW, MudProfileWindowClass))
#define MUD_IS_PROFILE_WINDOW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_PROFILE_WINDOW))
#define MUD_IS_PROFILE_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_PROFILE_WINDOW))
#define MUD_PROFILE_WINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_PROFILE_WINDOW, MudProfileWindowClass))
#define MUD_PROFILE_WINDOW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MUD_TYPE_PROFILE_WINDOW, MudProfileWindowPrivate))

typedef struct _MudProfileWindow            MudProfileWindow;
typedef struct _MudProfileWindowClass       MudProfileWindowClass;
typedef struct _MudProfileWindowPrivate     MudProfileWindowPrivate;

struct _MudProfileWindowClass
{
	GObjectClass parent_class;
};

struct _MudProfileWindow
{
	GObject parent_instance;

        /*< Private >*/
	MudProfileWindowPrivate *priv;

        /*< Public >*/
        MudWindow *parent_window;
};

GType mud_profile_window_get_type (void);

MudProfileWindow *mud_window_profile_new(MudWindow *window);

G_END_DECLS

#endif // MUD_PROFILE_WINDOW_H
