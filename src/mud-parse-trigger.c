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

};

GType mud_parse_trigger_get_type (void);
static void mud_parse_trigger_init (MudParseTrigger *pt);
static void mud_parse_trigger_class_init (MudParseTriggerClass *klass);
static void mud_parse_trigger_finalize (GObject *object);

// MudParseTrigger class functions
GType
mud_parse_trigger_get_type (void)
{
    static GType object_type = 0;

    g_type_init();

    if (!object_type)
    {
        static const GTypeInfo object_info =
        {
            sizeof (MudParseTriggerClass),
            NULL,
            NULL,
            (GClassInitFunc) mud_parse_trigger_class_init,
            NULL,
            NULL,
            sizeof (MudParseTrigger),
            0,
            (GInstanceInitFunc) mud_parse_trigger_init,
        };

        object_type = g_type_register_static(G_TYPE_OBJECT, "MudParseTrigger", &object_info, 0);
    }

    return object_type;
}

static void
mud_parse_trigger_init (MudParseTrigger *pt)
{
    pt->priv = g_new0(MudParseTriggerPrivate, 1);
}

static void
mud_parse_trigger_class_init (MudParseTriggerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = mud_parse_trigger_finalize;
}

static void
mud_parse_trigger_finalize (GObject *object)
{
    MudParseTrigger *parse_trigger;
    GObjectClass *parent_class;

    parse_trigger = MUD_PARSE_TRIGGER(object);

    g_free(parse_trigger->priv);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

// MudParseTrigger Methods
gboolean
mud_parse_trigger_do(gchar *data, MudConnectionView *view, MudRegex *regex, MudParseTrigger *trig)
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

    client = gconf_client_get_default();

    profile_name = mud_profile_get_name(mud_connection_view_get_current_profile(view));

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

            if(mud_regex_check((const gchar *)stripped_data, strlen(stripped_data), (const gchar *)regexstr, ovector, regex))
            {
                g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/triggers/%s/gag", profile_name, (gchar *)entry->data);
                gag = gconf_client_get_int(client, keyname, &error);

                // FIXME: Kill this global and get a sane
                // way of doing this in here. - lh
                if(gag)
                    doGag = TRUE;

                g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/triggers/%s/actions", profile_name, (gchar *)entry->data);
                actions = gconf_client_get_string(client, keyname, &error);

                mud_parse_base_parse((const gchar *)actions, stripped_data, ovector, view, regex);

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

    return doGag;
}

// Instantiate MudParseTrigger
MudParseTrigger*
mud_parse_trigger_new(void)
{
    return g_object_new(MUD_TYPE_PARSE_TRIGGER, NULL);
}
