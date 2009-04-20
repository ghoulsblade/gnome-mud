/* GNOME-Mud - A simple Mud Client
 * mud-log.h
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

#ifndef MUD_LOG_H
#define MUD_LOG_H

G_BEGIN_DECLS

#include <gconf/gconf-client.h>

#define MUD_TYPE_LOG              (mud_log_get_type ())
#define MUD_LOG(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_LOG, MudLog))
#define MUD_LOG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_LOG, MudLogClass))
#define MUD_IS_LOG(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_LOG))
#define MUD_IS_LOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_LOG))
#define MUD_LOG_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_LOG, MudLogClass))
#define MUD_LOG_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MUD_TYPE_LOG, MudLogPrivate))

typedef struct _MudLog            MudLog;
typedef struct _MudLogClass       MudLogClass;
typedef struct _MudLogPrivate     MudLogPrivate;

struct _MudLogClass
{
    GObjectClass parent_class;
};

struct _MudLog
{
    GObject parent_instance;

    /*< Private >*/
    MudLogPrivate *priv;

    /*< Public >*/
    gchar *mud_name;
};

GType mud_log_get_type (void);

/*< Public Methods >*/
void mud_log_write_hook(MudLog *log, const gchar *data, gint length);
void mud_log_open(MudLog *log);
void mud_log_close(MudLog *log);
gboolean mud_log_islogging(MudLog *log);

G_END_DECLS

#endif // MUD_LOG_H
