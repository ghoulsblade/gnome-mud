/* GNOME-Mud - A simple Mud Client
 * mud-line-buffer.h
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

#ifndef MUD_LINE_BUFFER_H
#define MUD_LINE_BUFFER_H

G_BEGIN_DECLS

#include <glib-object.h>

#define MUD_TYPE_LINE_BUFFER              (mud_line_buffer_get_type ())
#define MUD_LINE_BUFFER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_LINE_BUFFER, MudLineBuffer))
#define MUD_LINE_BUFFER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_LINE_BUFFER, MudLineBufferClass))
#define MUD_IS_LINE_BUFFER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_LINE_BUFFER))
#define MUD_IS_LINE_BUFFER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_LINE_BUFFER))
#define MUD_LINE_BUFFER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_LINE_BUFFER, MudLineBufferClass))
#define MUD_LINE_BUFFER_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MUD_TYPE_LINE_BUFFER, MudLineBufferPrivate))

typedef struct _MudLineBuffer            MudLineBuffer;
typedef struct _MudLineBufferClass       MudLineBufferClass;
typedef struct _MudLineBufferPrivate     MudLineBufferPrivate;

struct _MudLineBufferClass
{
    GObjectClass parent_class;
};

struct _MudLineBuffer
{
    GObject parent_instance;

    /*< private >*/
    MudLineBufferPrivate *priv;
};

GType         mud_line_buffer_get_type (void);

void          mud_line_buffer_add_data(MudLineBuffer *self,
                                       const gchar *data,
                                       guint length);

void          mud_line_buffer_flush(MudLineBuffer *self);

gchar *       mud_line_buffer_get_lines(MudLineBuffer *self);

const gchar * mud_line_buffer_get_line(MudLineBuffer *self,
                                       guint line);

gchar *       mud_line_buffer_get_range(MudLineBuffer *self,
                                        guint start,
                                        guint end);

G_END_DECLS

#endif // MUD_LINE_BUFFER_H

