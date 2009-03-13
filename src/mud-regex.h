/* GNOME-Mud - A simple Mud Client
 * mud-regex.h
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

#ifndef MUD_REGEX_H
#define MUD_REGEX_H

G_BEGIN_DECLS

#define MUD_TYPE_REGEX              (mud_regex_get_type ())
#define MUD_REGEX(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_REGEX, MudRegex))
#define MUD_REGEX_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_REGEX, MudRegexClass))
#define MUD_IS_REGEX(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_REGEX))
#define MUD_IS_REGEX_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_REGEX))
#define MUD_REGEX_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_REGEX, MudRegexClass))
#define MUD_REGEX_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MUD_TYPE_REGEX, MudRegexPrivate))

typedef struct _MudRegex            MudRegex;
typedef struct _MudRegexClass       MudRegexClass;
typedef struct _MudRegexPrivate     MudRegexPrivate;

struct _MudRegexClass
{
    GObjectClass parent_class;
};

struct _MudRegex
{
    GObject parent_instance;

    /*< Private >*/
    MudRegexPrivate *priv;
};

GType mud_regex_get_type (void);

gboolean mud_regex_check(MudRegex *regex, const gchar *data, guint length, const gchar *rx, gint ovector[1020]);
const gchar **mud_regex_test(const gchar *data, guint length, const gchar *rx, gint *rc, const gchar **error, gint *errorcode, gint *erroroffset);
void mud_regex_substring_clear(const gchar **substring_list);
const gchar **mud_regex_get_substring_list(MudRegex *regex, gint *count);

G_END_DECLS

#endif // MUD_REGEX_H

