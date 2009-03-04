/* GNOME-Mud - A simple Mud Client
 * mud-parse-alias.c
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib-object.h>
#include <glib/gi18n.h>
#include <string.h>
#include <gconf/gconf-client.h>

#include "mud-parse-base.h"
#include "mud-parse-alias.h"
#include "utils.h"

GType mud_parse_alias_get_type (void);
static void mud_parse_alias_init (MudParseAlias *parse_alias);
static void mud_parse_alias_class_init (MudParseAliasClass *klass);
static void mud_parse_alias_finalize (GObject *object);

void mud_parse_alias_parse(const gchar *data, gchar *stripped_data, gint ovector[1020], MudConnectionView *view, MudRegex *regex);

// MudParseAlias class functions
GType
mud_parse_alias_get_type (void)
{
    static GType object_type = 0;

    g_type_init();

    if (!object_type)
    {
        static const GTypeInfo object_info =
        {
            sizeof (MudParseAliasClass),
            NULL,
            NULL,
            (GClassInitFunc) mud_parse_alias_class_init,
            NULL,
            NULL,
            sizeof (MudParseAlias),
            0,
            (GInstanceInitFunc) mud_parse_alias_init,
        };

        object_type = g_type_register_static(G_TYPE_OBJECT, "MudParseAlias", &object_info, 0);
    }

    return object_type;
}

static void
mud_parse_alias_init (MudParseAlias *pa)
{

}

static void
mud_parse_alias_class_init (MudParseAliasClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = mud_parse_alias_finalize;
}

static void
mud_parse_alias_finalize (GObject *object)
{
    MudParseAlias *parse_alias;
    GObjectClass *parent_class;

    parse_alias = MUD_PARSE_ALIAS(object);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

// MudParseAlias Methods
gboolean
mud_parse_alias_do(gchar *data, MudConnectionView *view, MudRegex *regex, MudParseAlias *alias)
{
    gchar *profile_name;
    gchar *actions;
    gchar *regexstr;
    GSList *aliases, *entry;
    GConfClient *client;
    GError *error = NULL;
    gchar keyname[2048];
    gint enabled;
    gint ovector[1020];
    gboolean send_line = TRUE;

    client = gconf_client_get_default();

    profile_name = mud_profile_get_name(mud_connection_view_get_current_profile(view));

    g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/aliases/list", profile_name);
    aliases = gconf_client_get_list(client, keyname, GCONF_VALUE_STRING, &error);

    for (entry = aliases; entry != NULL; entry = g_slist_next(entry))
    {
        g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/aliases/%s/enabled", profile_name, (gchar *)entry->data);
        enabled = gconf_client_get_int(client, keyname, &error);

        if(enabled)
        {
            g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/aliases/%s/regex", profile_name, (gchar *)entry->data);
            regexstr = gconf_client_get_string(client, keyname, &error);

            if(mud_regex_check((const gchar *)data, strlen(data), regexstr, ovector, regex))
            {
                g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/aliases/%s/actions", profile_name, (gchar *)entry->data);
                actions = gconf_client_get_string(client, keyname, &error);

                send_line = FALSE;
                mud_parse_base_parse((const gchar *)actions, data, ovector, view, regex);

                if(actions)
                    g_free(actions);
            }

            if(regexstr)
                g_free(regexstr);
        }
    }

    for(entry = aliases; entry != NULL; entry = g_slist_next(entry))
        if(entry->data)
            g_free(entry->data);

    if(aliases)
        g_slist_free(aliases);

    g_object_unref(client);

    return send_line;
}

// Instantiate MudParseAlias
MudParseAlias*
mud_parse_alias_new(void)
{
    return g_object_new(MUD_TYPE_PARSE_ALIAS, NULL);
}
