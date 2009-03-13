/* GNOME-Mud - A simple Mud Client
 * mud-parse-trigger.h
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

#ifndef MUD_PARSE_TRIGGER_H
#define MUD_PARSE_TRIGGER_H

G_BEGIN_DECLS

#define MUD_TYPE_PARSE_TRIGGER              (mud_parse_trigger_get_type ())
#define MUD_PARSE_TRIGGER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_PARSE_TRIGGER, MudParseTrigger))
#define MUD_PARSE_TRIGGER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_PARSE_TRIGGER, MudParseTriggerClass))
#define MUD_IS_PARSE_TRIGGER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_PARSE_TRIGGER))
#define MUD_IS_PARSE_TRIGGER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_PARSE_TRIGGER))
#define MUD_PARSE_TRIGGER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_PARSE_TRIGGER, MudParseTriggerClass))
#define MUD_PARSE_TRIGGER_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MUD_TYPE_PARSE_TRIGGER, MudParseTriggerPrivate))

typedef struct _MudParseTrigger            MudParseTrigger;
typedef struct _MudParseTriggerClass       MudParseTriggerClass;
typedef struct _MudParseTriggerPrivate     MudParseTriggerPrivate;

struct _MudParseTriggerClass
{
	GObjectClass parent_class;
};

struct _MudParseTrigger
{
	GObject parent_instance;

        /*< Private >*/
	MudParseTriggerPrivate *priv;
};

GType mud_parse_trigger_get_type (void);

gboolean mud_parse_trigger_do(MudParseTrigger *self, gchar *data);

G_END_DECLS

#endif // MUD_PARSE_TRIGGER_H

