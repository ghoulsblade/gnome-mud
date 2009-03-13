/* GNOME-Mud - A simple Mud Client
 * mud-parse-base.h
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

#ifndef MUD_PARSE_BASE_H
#define MUD_PARSE_BASE_H

G_BEGIN_DECLS

#define MUD_TYPE_PARSE_BASE              (mud_parse_base_get_type ())
#define MUD_PARSE_BASE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_PARSE_BASE, MudParseBase))
#define MUD_PARSE_BASE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_PARSE_BASE, MudParseBaseClass))
#define MUD_IS_PARSE_BASE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_PARSE_BASE))
#define MUD_IS_PARSE_BASE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_PARSE_BASE))
#define MUD_PARSE_BASE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_PARSE_BASE, MudParseBaseClass))
#define MUD_PARSE_BASE_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MUD_TYPE_PARSE_BASE, MudParseBasePrivate))

typedef struct _MudParseBase            MudParseBase;
typedef struct _MudParseBaseClass       MudParseBaseClass;
typedef struct _MudParseBasePrivate     MudParseBasePrivate;

#include "mud-connection-view.h"
#include "mud-regex.h"

struct _MudParseBaseClass
{
    GObjectClass parent_class;
};

struct _MudParseBase
{
    GObject parent_instance;

    /*< Private >*/
    MudParseBasePrivate *priv;

    /*< Public >*/
    MudConnectionView *parent_view;
    MudRegex *regex;
};

GType mud_parse_base_get_type (void);

gboolean mud_parse_base_do_triggers(MudParseBase *self, gchar *data);
gboolean mud_parse_base_do_aliases(MudParseBase *self, gchar *data);
void mud_parse_base_parse(MudParseBase *self,
                          const gchar *data,
                          gchar *stripped_data,
                          gint ovector[1020]);

G_END_DECLS

#endif // MUD_PARSE_BASE_H
