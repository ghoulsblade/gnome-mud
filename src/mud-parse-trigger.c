/* GNOME-Mud - A simple Mud Client
 * mud-parse-trigger.c
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
#  include "config.h"
#endif

#include <gconf/gconf-client.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <string.h>

#include "mud-window.h"
#include "mud-connection-view.h"
#include "mud-profile.h"
#include "mud-parse-base.h"
#include "mud-parse-trigger.h"
#include "utils.h"

struct _MudParseTriggerPrivate
{
    MudParseBase *parent;
};


/* Property Identifiers */
enum
{
    PROP_MUD_PARSE_TRIGGER_0,
    PROP_PARENT
};

/* Create the Type */
G_DEFINE_TYPE(MudParseTrigger, mud_parse_trigger, G_TYPE_OBJECT);

/* Class Functions */
static void mud_parse_trigger_init (MudParseTrigger *pt);
static void mud_parse_trigger_class_init (MudParseTriggerClass *klass);
static void mud_parse_trigger_finalize (GObject *object);
static GObject *mud_parse_trigger_constructor (GType gtype,
                                               guint n_properties,
                                               GObjectConstructParam *properties);
static void mud_parse_trigger_set_property(GObject *object,
                                           guint prop_id,
                                           const GValue *value,
                                           GParamSpec *pspec);
static void mud_parse_trigger_get_property(GObject *object,
                                           guint prop_id,
                                           GValue *value,
                                           GParamSpec *pspec);
// MudParseTrigger class functions
static void
mud_parse_trigger_class_init (MudParseTriggerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object constructor */
    object_class->constructor = mud_parse_trigger_constructor;

    /* Override base object's finalize */
    object_class->finalize = mud_parse_trigger_finalize;

    /* Override base object property methods */
    object_class->set_property = mud_parse_trigger_set_property;
    object_class->get_property = mud_parse_trigger_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(MudParseTriggerPrivate));

    /* Install Properties */
    g_object_class_install_property(object_class,
            PROP_PARENT,
            g_param_spec_object("parent-base",
                "parent base",
                "the parent MudParseBase",
                MUD_TYPE_PARSE_BASE,
                G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));
}

static void
mud_parse_trigger_init (MudParseTrigger *self)
{
    /* Get our private data */
    self->priv = MUD_PARSE_TRIGGER_GET_PRIVATE(self);

    /* set private members to defaults */
    self->priv->parent = NULL;
}

static GObject *
mud_parse_trigger_constructor (GType gtype,
                               guint n_properties,
                               GObjectConstructParam *properties)
{
    MudParseTrigger *self;
    GObject *obj;
    MudParseTriggerClass *klass;
    GObjectClass *parent_class;

    /* Chain up to parent constructor */
    klass = MUD_PARSE_TRIGGER_CLASS( g_type_class_peek(MUD_TYPE_PARSE_TRIGGER) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    self = MUD_PARSE_TRIGGER(obj);

    if(!self->priv->parent)
    {
        g_printf("ERROR: Tried to instantiate MudParseTrigger without passing parent parse base\n");
        g_error("Tried to instantiate MudParseTrigger without passing parent parse base");
    }

    return obj;
}

static void
mud_parse_trigger_finalize (GObject *object)
{
    MudParseTrigger *parse_trigger;
    GObjectClass *parent_class;

    parse_trigger = MUD_PARSE_TRIGGER(object);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

static void
mud_parse_trigger_set_property(GObject *object,
                               guint prop_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
    MudParseTrigger *self;

    self = MUD_PARSE_TRIGGER(object);

    switch(prop_id)
    {
        /* Parent is Construct Only */
        case PROP_PARENT:
            self->priv->parent = MUD_PARSE_BASE(g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
mud_parse_trigger_get_property(GObject *object,
                               guint prop_id,
                               GValue *value,
                               GParamSpec *pspec)
{
    MudParseTrigger *self;

    self = MUD_PARSE_TRIGGER(object);

    switch(prop_id)
    {
        case PROP_PARENT:
            g_value_take_object(value, self->priv->parent);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

/* Public Methods */
gboolean
mud_parse_trigger_do(MudParseTrigger *self, gchar *data)
{
    gchar *profile_name;
    gchar *actions;
    gchar *regexstr;
    gchar *stripped_data;
    GSList *triggers, *entry;
    GConfClient *client;
    GError *error = NULL;
    gchar keyname[2048];
    gint enabled;
    gint gag;
    gint ovector[1020];
    gboolean doGag = FALSE;
    MudRegex *regex;
    MudConnectionView *view;

    g_return_if_fail(MUD_IS_PARSE_TRIGGER(self));

    client = gconf_client_get_default();

    g_object_get(self->priv->parent,
                 "parent-view", &view,
                 "regex", &regex,
                 NULL);

    g_object_get(view,
                 "profile-name", &profile_name,
                 NULL);

    g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/triggers/list", profile_name);
    triggers = gconf_client_get_list(client, keyname, GCONF_VALUE_STRING, &error);

    for (entry = triggers; entry != NULL; entry = g_slist_next(entry))
    {
        g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/triggers/%s/enabled", profile_name, (gchar *)entry->data);
        enabled = gconf_client_get_int(client, keyname, &error);

        if(enabled)
        {
            g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/triggers/%s/regex", profile_name, (gchar *)entry->data);
            regexstr = gconf_client_get_string(client, keyname, &error);

            stripped_data = utils_strip_ansi((const gchar *) data);

            if(mud_regex_check(regex, (const gchar *)stripped_data, strlen(stripped_data), (const gchar *)regexstr, ovector))
            {
                g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/triggers/%s/gag", profile_name, (gchar *)entry->data);
                gag = gconf_client_get_int(client, keyname, &error);

                // FIXME: Kill this global and get a sane
                // way of doing this in here. - lh
                if(gag)
                    doGag = TRUE;

                g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/triggers/%s/actions", profile_name, (gchar *)entry->data);
                actions = gconf_client_get_string(client, keyname, &error);

                mud_parse_base_parse(self->priv->parent,
                                     (const gchar *)actions, 
                                     stripped_data, 
                                     ovector);

                if(actions)
                    g_free(actions);
            }

            if(stripped_data)
                g_free(stripped_data);
        }
    }

    for(entry = triggers; entry != NULL; entry = g_slist_next(entry))
        if(entry->data)
            g_free(entry->data);

    if(triggers)
        g_slist_free(triggers);

    g_object_unref(client);

    g_free(profile_name);

    return doGag;
}

