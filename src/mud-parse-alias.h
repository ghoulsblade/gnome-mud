/* GNOME-Mud - A simple Mud Client
 * mud-parse-alias.h
 * Copyright (C) 2006-2009 Les Harris <lharris@gnome.org>
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

#ifndef MUD_PARSE_ALIAS_H
#define MUD_PARSE_ALIAS_H

G_BEGIN_DECLS

#define MUD_TYPE_PARSE_ALIAS              (mud_parse_alias_get_type ())
#define MUD_PARSE_ALIAS(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_PARSE_ALIAS, MudParseAlias))
#define MUD_PARSE_ALIAS_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_PARSE_ALIAS, MudParseAliasClass))
#define MUD_IS_PARSE_ALIAS(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_PARSE_ALIAS))
#define MUD_IS_PARSE_ALIAS_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_PARSE_ALIAS))
#define MUD_PARSE_ALIAS_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_PARSE_ALIAS, MudParseAliasClass))
#define MUD_PARSE_ALIAS_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MUD_TYPE_PARSE_ALIAS, MudParseAliasPrivate))

typedef struct _MudParseAlias            MudParseAlias;
typedef struct _MudParseAliasClass       MudParseAliasClass;
typedef struct _MudParseAliasPrivate     MudParseAliasPrivate;

struct _MudParseAliasClass
{
	GObjectClass parent_class;
};

struct _MudParseAlias
{
	GObject parent_instance;

        /*< Private >*/
	MudParseAliasPrivate *priv;
};

GType mud_parse_alias_get_type (void);

gboolean mud_parse_alias_do(MudParseAlias *self, gchar *data);

G_END_DECLS

#endif // MUD_PARSE_ALIAS_H

