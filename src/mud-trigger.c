/* GNOME-Mud - A simple Mud Client
 * mud-trigger.c
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

#include "mud-line-buffer.h"
#include "mud-trigger.h"

struct _MudTriggerPrivate
{
    MudTriggerType type;
    MudLineBuffer *line_buffer;

    gulong lines;

    gboolean enabled;

    gboolean gag_output;
    
    gboolean multiline;
    gboolean filter;

    /* Glob Triggers */
    GPatternSpec *glob;
    gchar *glob_string;

    /* Regex Triggers */
    GRegex *regex;
    gchar *regex_string;

    /* Condition Triggers */
    GList *condition_items;
};

/* Property Identifiers */
enum
{
    PROP_MUD_TRIGGER_0,
};

/* Create the Type */
G_DEFINE_TYPE(MudTrigger, mud_trigger, G_TYPE_OBJECT);

/* Class Functions */
static void mud_trigger_init (MudTrigger *self);
static void mud_trigger_class_init (MudTriggerClass *klass);
static void mud_trigger_finalize (GObject *object);
static GObject *mud_trigger_constructor (GType gtype,
                                         guint n_properties,
                                         GObjectConstructParam *properties);
static void mud_trigger_set_property(GObject *object,
                                     guint prop_id,
                                     const GValue *value,
                                     GParamSpec *pspec);
static void mud_trigger_get_property(GObject *object,
                                     guint prop_id,
                                     GValue *value,
                                     GParamSpec *pspec);

/* MudTrigger class functions */
static void
mud_trigger_class_init (MudTriggerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object constructor */
    object_class->constructor = mud_trigger_constructor;

    /* Override base object's finalize */
    object_class->finalize = mud_trigger_finalize;

    /* Override base object property methods */
    object_class->set_property = mud_trigger_set_property;
    object_class->get_property = mud_trigger_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(MudTriggerPrivate));

    /* Install Properties */

}

static void
mud_trigger_init (MudTrigger *self)
{
    /* Get our private data */
    self->priv = MUD_TRIGGER_GET_PRIVATE(self);

    /* Set Defaults */
    self->priv->type = MUD_TRIGGER_TYPE_SINGLE;

}

static GObject *
mud_trigger_constructor (GType gtype,
                         guint n_properties,
                         GObjectConstructParam *properties)
{
    MudTrigger *self;
    GObject *obj;
    MudTriggerClass *klass;
    GObjectClass *parent_class;

    /* Chain up to parent constructor */
    klass = MUD_TRIGGER_CLASS( g_type_class_peek(MUD_TYPE_TRIGGER) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    self = MUD_TRIGGER(obj);

    return obj;
}

static void
mud_trigger_finalize (GObject *object)
{
    MudTrigger *self;
    GObjectClass *parent_class;

    self = MUD_TRIGGER(object);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

static void
mud_trigger_set_property(GObject *object,
                         guint prop_id,
                         const GValue *value,
                         GParamSpec *pspec)
{
    MudTrigger *self;

    self = MUD_TRIGGER(object);

    switch(prop_id)
    {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
mud_trigger_get_property(GObject *object,
                         guint prop_id,
                         GValue *value,
                         GParamSpec *pspec)
{
    MudTrigger *self;

    self = MUD_TRIGGER(object);

    switch(prop_id)
    {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

