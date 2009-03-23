/* GNOME-Mud - A simple Mud Client
 * mud-profile-manager.c
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
#include <glib/gi18n.h>

#include "gnome-mud.h"
#include "mud-window.h"
#include "mud-profile-manager.h"
#include "mud-connection-view.h"

struct _MudProfileManagerPrivate
{
    GSList *profiles;
    MudWindow *parent;
};

/* Property Identifiers */
enum
{
    PROP_MUD_PROFILE_MANAGER_0,
    PROP_PARENT
};

/* Create the Type */
G_DEFINE_TYPE(MudProfileManager, mud_profile_manager, G_TYPE_OBJECT);

/* Class Functions */
static void mud_profile_manager_init (MudProfileManager *profile_manager);
static void mud_profile_manager_class_init (MudProfileManagerClass *klass);
static void mud_profile_manager_finalize (GObject *object);
static GObject *mud_profile_manager_constructor (GType gtype,
                                                 guint n_properties,
                                                 GObjectConstructParam *properties);
static void mud_profile_manager_set_property(GObject *object,
                                             guint prop_id,
                                             const GValue *value,
                                             GParamSpec *pspec);
static void mud_profile_manager_get_property(GObject *object,
                                             guint prop_id,
                                             GValue *value,
                                             GParamSpec *pspec);

/* Private Methods */
static void mud_profile_manager_load_profiles (MudProfileManager *self);

/* MudProfileManager class functions */
static void
mud_profile_manager_class_init (MudProfileManagerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object constructor */
    object_class->constructor = mud_profile_manager_constructor;

    /* Override base object's finalize */
    object_class->finalize = mud_profile_manager_finalize;

    /* Override base object property methods */
    object_class->set_property = mud_profile_manager_set_property;
    object_class->get_property = mud_profile_manager_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(MudProfileManagerPrivate));

    /* Install Properties */
    g_object_class_install_property(object_class,
            PROP_PARENT,
            g_param_spec_object("parent-window",
                "parent MudWindow",
                "The parent MudWindow",
                MUD_TYPE_WINDOW,
                G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

}

static void
mud_profile_manager_init (MudProfileManager *self)
{
    /* Get our private data */
    self->priv = MUD_PROFILE_MANAGER_GET_PRIVATE(self);

    /* set private members to defaults */
    self->priv->profiles = NULL;
}

static GObject *
mud_profile_manager_constructor (GType gtype,
                                 guint n_properties,
                                 GObjectConstructParam *properties)
{
    MudProfileManager *self;
    GObject *obj;
    MudProfileManagerClass *klass;
    GObjectClass *parent_class;

    /* Chain up to parent constructor */
    klass = MUD_PROFILE_MANAGER_CLASS( g_type_class_peek(MUD_TYPE_PROFILE_MANAGER) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    self = MUD_PROFILE_MANAGER(obj);

    if(!self->priv->parent)
    {
        g_printf("ERROR: Tried to instantiate MudProfileManager without passing parent MudWindow\n");
        g_error("Tried to instantiate MudProfileManager without passing parent MudWindow");
    }

    mud_profile_manager_load_profiles(self);
    
    return obj;
}

static void
mud_profile_manager_finalize (GObject *object)
{
    MudProfileManager *profile_manager;
    GObjectClass *parent_class;
    GSList *entry;

    profile_manager = MUD_PROFILE_MANAGER(object);

    entry = profile_manager->priv->profiles;

    while(entry)
    {
        MudProfile *profile = MUD_PROFILE(entry->data);
        g_object_unref(profile);

        entry = g_slist_next(entry);
    }

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

static void
mud_profile_manager_set_property(GObject *object,
                                 guint prop_id,
                                 const GValue *value,
                                 GParamSpec *pspec)
{
    MudProfileManager *self;

    self = MUD_PROFILE_MANAGER(object);

    switch(prop_id)
    {
        case PROP_PARENT:
            self->priv->parent = MUD_WINDOW(g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
mud_profile_manager_get_property(GObject *object,
                                 guint prop_id,
                                 GValue *value,
                                 GParamSpec *pspec)
{
    MudProfileManager *self;

    self = MUD_PROFILE_MANAGER(object);

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

/* Private Methods */
static void
mud_profile_manager_load_profiles (MudProfileManager *self)
{
    GSList *entry;
    GSList *profiles;
    MudProfile *profile;
    GConfClient *client;

    g_return_if_fail(MUD_IS_PROFILE_MANAGER(self));
    g_return_if_fail(self->priv->profiles == NULL);

    client = gconf_client_get_default();

    profiles = gconf_client_all_dirs(client,
                                     "/apps/gnome-mud/profiles",
                                     NULL);

    profile = g_object_new(MUD_TYPE_PROFILE,
                           "name", "Default",
                           "parent", self,
                           NULL);

    self->priv->profiles = g_slist_append(self->priv->profiles,
                                          profile);

    entry = profiles;
    
    while(entry)
    {
        gchar *name = g_path_get_basename((gchar *)entry->data);

        if(!g_str_equal(name, "Default"))
        {
            profile = g_object_new(MUD_TYPE_PROFILE,
                    "name", g_strdup(name),
                    "parent", self,
                    NULL);

            self->priv->profiles = g_slist_append(self->priv->profiles,
                                                  profile);
        }

        g_free(name);
        g_free(entry->data);

        entry = g_slist_next(entry);
    }

    g_slist_free(profiles);

    g_object_unref(client);
}

/* Public Methods */
void
mud_profile_manager_add_profile(MudProfileManager *self,
                                const gchar *name)
{
    MudProfile *profile, *default_profile;
    gchar *escaped_name;
    GConfClient *client;

    g_return_if_fail(MUD_IS_PROFILE_MANAGER(self));

    client = gconf_client_get_default();
    default_profile = mud_profile_manager_get_profile_by_name(self,
                                                              "Default");

    escaped_name = gconf_escape_key(name, -1);

    profile = g_object_new(MUD_TYPE_PROFILE,
                           "name", escaped_name,
                           "parent", self,
                           NULL);

    g_free(escaped_name);

    mud_profile_copy_preferences(default_profile, profile);

    gconf_client_suggest_sync(client, NULL);

    self->priv->profiles = g_slist_append(self->priv->profiles,
                                          profile);

    mud_window_populate_profiles_menu(self->priv->parent);

    g_object_unref(client);
}

void
mud_profile_manager_delete_profile(MudProfileManager *self,
                                   const gchar *name)
{
    MudProfile *profile, *default_profile;
    GError *error = NULL;
    gchar *key, *pname;
    const GSList *views, *view_entry;
    GSList *connections, *entry;
    GConfClient *client;

    g_return_if_fail(MUD_IS_PROFILE_MANAGER(self));

    client = gconf_client_get_default();

    profile = mud_profile_manager_get_profile_by_name(self, name);
    default_profile = mud_profile_manager_get_profile_by_name(self, "Default");

    if (profile)
    {
        self->priv->profiles = g_slist_remove(self->priv->profiles,
                                              profile);

        key = g_strdup_printf("/apps/gnome-mud/profiles/%s", name);
        gconf_client_recursive_unset(client, key, 0, NULL);
        g_free(key);

       /* If the user deletes a profile we need to switch
        * any connections using that profile to Default to prevent
        * mass breakage */
       connections = gconf_client_all_dirs(client,
                                           "/apps/gnome-mud/muds",
                                           NULL);

       entry = connections;

       while(entry)
       {
           gchar *base = g_path_get_basename((gchar *)entry->data);
           key = g_strdup_printf("/apps/gnome-mud/muds/%s/profile",
                                 base);
           g_free(base);

           pname = gconf_client_get_string(client, key, NULL);

           if(g_str_equal(pname, name))
               gconf_client_set_string(client, key, "Default", NULL);

           g_free(key);
           g_free(entry->data);
           entry = g_slist_next(entry);
       }

       g_slist_free(connections);

       gconf_client_suggest_sync(client, NULL);

       /* Update the profiles menu */
       mud_window_populate_profiles_menu(self->priv->parent);
    }

    g_object_unref(client);
}

const GSList*
mud_profile_manager_get_profiles (MudProfileManager *self)
{
    if(!MUD_IS_PROFILE_MANAGER(self))
        return NULL;

    return self->priv->profiles;
}

MudProfile*
mud_profile_manager_get_profile_by_name(MudProfileManager *self,
                                        const gchar *name)
{
    GSList *entry = NULL;
    MudProfile *profile;

    g_return_if_fail(MUD_IS_PROFILE_MANAGER(self));

    entry = self->priv->profiles;

    while(entry)
    {
        profile = MUD_PROFILE(entry->data);

        if(g_str_equal(profile->name, name))
            return profile;

        entry = g_slist_next(entry);
    }

    return NULL;
}

