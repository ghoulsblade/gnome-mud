/* GNOME-Mud - A simple Mud Client
 * mud-line-buffer.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <string.h>
#include <glib-object.h>

#include "mud-line-buffer.h"

struct _MudLineBufferPrivate
{
    gulong length;
    gulong maximum_line_count;

    GList *line_buffer;
    GString *incoming_buffer;
};

/* Property Identifiers */
enum
{
    PROP_MUD_LINE_BUFFER_0,
    PROP_LENGTH,
    PROP_LINE_COUNT
};

/* Signal Indices */
enum
{
    LINE_ADDED,
    LINE_REMOVED,
    PARTIAL_LINE_RECEIVED,
    LAST_SIGNAL
};

/* Signal Identifer Map */
static guint mud_line_buffer_signal[LAST_SIGNAL] = {0, 0, 0};

/* Create the Type */
G_DEFINE_TYPE(MudLineBuffer, mud_line_buffer, G_TYPE_OBJECT);

/* Class Functions */
static void mud_line_buffer_init (MudLineBuffer *self);
static void mud_line_buffer_class_init (MudLineBufferClass *klass);
static void mud_line_buffer_finalize (GObject *object);
static GObject *mud_line_buffer_constructor (GType gtype,
                                             guint n_properties,
                                             GObjectConstructParam *properties);
static void mud_line_buffer_set_property(GObject *object,
                                         guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec);
static void mud_line_buffer_get_property(GObject *object,
                                         guint prop_id,
                                         GValue *value,
                                         GParamSpec *pspec);

/* Private Methods */
static void mud_line_buffer_free_line(gpointer value, gpointer user_data);

/* MudLineBuffer class functions */
static void
mud_line_buffer_class_init (MudLineBufferClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object constructor */
    object_class->constructor = mud_line_buffer_constructor;

    /* Override base object's finalize */
    object_class->finalize = mud_line_buffer_finalize;

    /* Override base object property methods */
    object_class->set_property = mud_line_buffer_set_property;
    object_class->get_property = mud_line_buffer_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(MudLineBufferPrivate));

    /* Install Properties */
    g_object_class_install_property(object_class,
                        PROP_LENGTH,
                        g_param_spec_ulong("length",
                                           "Length",
                                           "Number of lines in buffer.",
                                           0,
                                           G_MAXULONG,
                                           0,
                                           G_PARAM_READABLE));

    g_object_class_install_property(object_class,
                        PROP_LINE_COUNT,
                        g_param_spec_ulong("maximum-line-count",
                                           "Maximum Line Count",
                                           "Total possible number of lines in buffer.",
                                           0,
                                           G_MAXULONG,
                                           20,
                                           G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

    /* Register Signals */
    mud_line_buffer_signal[LINE_ADDED] =
        g_signal_newv("line-added",
                      G_TYPE_FROM_CLASS(object_class),
                      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_HOOKS,
                      NULL,
                      NULL,
                      NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE,
                      0,
                      NULL);

    mud_line_buffer_signal[LINE_REMOVED] =
        g_signal_newv("line-removed",
                      G_TYPE_FROM_CLASS(object_class),
                      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_HOOKS,
                      NULL,
                      NULL,
                      NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE,
                      0,
                      NULL);

    mud_line_buffer_signal[PARTIAL_LINE_RECEIVED] =
        g_signal_new("partial-line-received",
                      G_TYPE_FROM_CLASS(object_class),
                      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_HOOKS,
                      0,
                      NULL,
                      NULL,
                      g_cclosure_marshal_VOID__STRING,
                      G_TYPE_NONE,
                      1,
                      G_TYPE_STRING);
}

static void
mud_line_buffer_init (MudLineBuffer *self)
{
    /* Get our private data */
    self->priv = MUD_LINE_BUFFER_GET_PRIVATE(self);

    /* Some Defaults */
    self->priv->length = 0;
}

static GObject *
mud_line_buffer_constructor (GType gtype,
                             guint n_properties,
                             GObjectConstructParam *properties)
{
    MudLineBuffer *self;
    GObject *obj;
    MudLineBufferClass *klass;
    GObjectClass *parent_class;

    /* Chain up to parent constructor */
    klass = MUD_LINE_BUFFER_CLASS( g_type_class_peek(MUD_TYPE_LINE_BUFFER) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    self = MUD_LINE_BUFFER(obj);

    self->priv->line_buffer = NULL;
    self->priv->incoming_buffer = g_string_new(NULL);

    return obj;
}

static void
mud_line_buffer_finalize (GObject *object)
{
    MudLineBuffer *self;
    GObjectClass *parent_class;

    self = MUD_LINE_BUFFER(object);

    g_list_foreach(self->priv->line_buffer, mud_line_buffer_free_line, NULL);
    g_list_free(self->priv->line_buffer);

    g_string_free(self->priv->incoming_buffer, TRUE);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

static void
mud_line_buffer_set_property(GObject *object,
                             guint prop_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
    MudLineBuffer *self;
    gulong new_ulong;

    self = MUD_LINE_BUFFER(object);

    switch(prop_id)
    {
        case PROP_LINE_COUNT:
            new_ulong = g_value_get_ulong(value);

            if(new_ulong != self->priv->maximum_line_count)
                self->priv->maximum_line_count = new_ulong;
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
mud_line_buffer_get_property(GObject *object,
                             guint prop_id,
                             GValue *value,
                             GParamSpec *pspec)
{
    MudLineBuffer *self;

    self = MUD_LINE_BUFFER(object);

    switch(prop_id)
    {
        case PROP_LENGTH:
            g_value_set_ulong(value, self->priv->length);
            break;

        case PROP_LINE_COUNT:
            g_value_set_ulong(value, self->priv->maximum_line_count);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

/* Public Methods */
void
mud_line_buffer_add_data(MudLineBuffer *self,
                         const gchar *data,
                         guint length)
{
    guint i;
    GString *data_buffer, *line;

    g_return_if_fail(MUD_IS_LINE_BUFFER(self));

    data_buffer = g_string_new_len(data, length);
    data_buffer = g_string_prepend(data_buffer,
                                   self->priv->incoming_buffer->str);

    g_string_free(self->priv->incoming_buffer, TRUE);
    self->priv->incoming_buffer = g_string_new(NULL);

    line = g_string_new(NULL);

    for(i = 0; i < data_buffer->len; ++i)
    {
        line = g_string_append_c(line, data_buffer->str[i]);

        if(data_buffer->str[i] == '\n')
        {
            ++self->priv->length;

            self->priv->line_buffer =
                g_list_append(self->priv->line_buffer,
                              g_string_free(line, FALSE));

            if(self->priv->length == self->priv->maximum_line_count + 1)
            {
                gchar *kill_line =
                    (g_list_first(self->priv->line_buffer))->data;

                self->priv->line_buffer = g_list_remove(self->priv->line_buffer,
                                                        kill_line);

                g_free(kill_line);

                --self->priv->length;

                g_signal_emit(self,
                              mud_line_buffer_signal[LINE_REMOVED],
                              0);
            }

            g_signal_emit(self,
                          mud_line_buffer_signal[LINE_ADDED],
                          0);

            line = g_string_new(NULL);
        }
    }

    if(line->len != 0)
    {
        self->priv->incoming_buffer =
            g_string_append(self->priv->incoming_buffer,
                    line->str);

        g_signal_emit(self,
                      mud_line_buffer_signal[PARTIAL_LINE_RECEIVED],
                      0,
                      line->str);
    }

    g_string_free(line, TRUE);
    g_string_free(data_buffer, TRUE);
}

void
mud_line_buffer_flush(MudLineBuffer *self)
{
    g_return_if_fail(MUD_IS_LINE_BUFFER(self));

    g_list_foreach(self->priv->line_buffer, mud_line_buffer_free_line, NULL);
    g_list_free(self->priv->line_buffer);
    g_string_free(self->priv->incoming_buffer, TRUE);

    self->priv->length = 0;
    self->priv->line_buffer = NULL;
    self->priv->incoming_buffer = g_string_new(NULL);
}

gchar *
mud_line_buffer_get_lines(MudLineBuffer *self)
{
    GList *entry;
    GString *lines;

    if(!MUD_IS_LINE_BUFFER(self))
    {
        g_critical("Invalid MudLineBuffer passed to mud_line_buffer_get_lines");
        return NULL;
    }

    entry = g_list_first(self->priv->line_buffer);
    lines = g_string_new(NULL);

    while(entry)
    {
        const gchar *line = (gchar *)entry->data;

        lines = g_string_append(lines, line);

        entry = g_list_next(entry);
    }

    return g_string_free(lines, (lines->len == 0) );
}

const gchar *
mud_line_buffer_get_line(MudLineBuffer *self,
                         guint line)
{
    if(!MUD_IS_LINE_BUFFER(self))
    {
        g_critical("Invalid MudLineBuffer passed to mud_line_buffer_get_line");
        return NULL;
    }

    if(line >= self->priv->length)
        return NULL;

    return g_list_nth_data(self->priv->line_buffer, line);
}

gchar *
mud_line_buffer_get_range(MudLineBuffer *self,
                          guint start,
                          guint end)
{
    guint i;
    GList *entry;
    GString *range;

    if(!MUD_IS_LINE_BUFFER(self))
    {
        g_critical("Invalid MudLineBuffer passed to mud_line_buffer_get_range");
        return NULL;
    }

    if(start >= self->priv->length ||
       end > self->priv->length)
        return NULL;

    if(start > end)
        return NULL;

    if(start == end)
        return g_strdup(mud_line_buffer_get_line(self, start));

    entry = g_list_nth(self->priv->line_buffer, start);
    range = g_string_new(NULL);

    for(i = start; i < end; ++i)
    {
        range = g_string_append(range, entry->data);

        entry = g_list_next(entry);
    }

    return g_string_free(range, (range->len == 0) );
}

/* Private Methods */
static void
mud_line_buffer_free_line(gpointer value,
                          gpointer user_data)
{
    gchar *line = (gchar *)value;

    if(line)
        g_free(line);
}

