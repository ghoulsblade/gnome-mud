/* GNOME-Mud - A simple Mud Client
 * mud-profile.c
 * Copyright (C) 1998-2005 Robin Ericsson <lobbin@localhost.nu>
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

#include <string.h>

#include <gconf/gconf-client.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <glib/gprintf.h>

#include "mud-profile-manager.h"
#include "mud-profile.h"
#include "utils.h"

struct _MudProfilePrivate
{
    GConfClient *gconf_client;

    MudPrefs preferences;
    gint in_notification_count;
    guint gconf_signal;

    MudProfileManager *parent;
};

/* Create the Type */
G_DEFINE_TYPE(MudProfile, mud_profile, G_TYPE_OBJECT);

/* Property Identifiers */
enum
{
    PROP_MUD_PROFILE_0,
    PROP_NAME,
    PROP_PARENT
};

/* Signal Indices */
enum
{
    CHANGED,
    LAST_SIGNAL
};

/* Signal Identifier Map */
static guint mud_profile_signal[LAST_SIGNAL] = { 0 };

#define RETURN_IF_NOTIFYING(profile)  if ((profile)->priv->in_notification_count) return

/* Class Functions */
static void mud_profile_init          (MudProfile *profile);
static void mud_profile_class_init    (MudProfileClass *profile);
static void mud_profile_finalize      (GObject *object);
static GObject *mud_profile_constructor (GType gtype,
                                         guint n_properties,
                                         GObjectConstructParam *properties);
static void mud_profile_set_property(GObject *object,
                                     guint prop_id,
                                     const GValue *value,
                                     GParamSpec *pspec);
static void mud_profile_get_property(GObject *object,
                                     guint prop_id,
                                     GValue *value,
                                     GParamSpec *pspec);

/* Callbacks */
static void mud_profile_gconf_changed (GConfClient *client, guint cnxn_id, GConfEntry *entry, gpointer data);

/* Profile Set Functions */
static gboolean set_FontName(MudProfile *profile, const gchar *candidate);
static gboolean set_CommDev(MudProfile *profile, const gchar *candidate);
static gboolean set_Scrollback(MudProfile *profile, const gint candidate);
static gboolean set_ProxyVersion(MudProfile *profile, const gchar *candidate);
static gboolean set_ProxyHostname(MudProfile *profile, const gchar *candidate);
static gboolean set_Encoding(MudProfile *profile, const gchar *candidate);
static gboolean set_Foreground(MudProfile *profile, const gchar *candidate);
static gboolean set_Background(MudProfile *profile, const gchar *candidate);
static gboolean set_Colors(MudProfile *profile, const gchar *candidate);

/* Private Methods */
static void mud_profile_set_proxy_combo_full(MudProfile *profile, gchar *version);
static void mud_profile_load_preferences(MudProfile *profile);
static const gchar *mud_profile_gconf_get_key(MudProfile *profile, const gchar *key);

/* MudProfile Class Functions */
static void
mud_profile_class_init (MudProfileClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object constructor */
    object_class->constructor = mud_profile_constructor;

    /* Override base object finalize */
    object_class->finalize = mud_profile_finalize;

    /* Override base object property methods */
    object_class->set_property = mud_profile_set_property;
    object_class->get_property = mud_profile_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(MudProfilePrivate));

    /* Create and Install Properties */
    g_object_class_install_property(object_class,
            PROP_NAME,
            g_param_spec_string("name",
                "Name",
                "The name of the profile.",
                NULL,
                G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property(object_class,
            PROP_PARENT,
            g_param_spec_object("parent",
                "Parent",
                "The parent MudProfileManager",
                MUD_TYPE_PROFILE_MANAGER,
                G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

    /* Register Signals */
    mud_profile_signal[CHANGED] =
        g_signal_new("changed",
                G_OBJECT_CLASS_TYPE(object_class),
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET(MudProfileClass, changed),
                NULL, NULL,
                g_cclosure_marshal_VOID__POINTER,
                G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void
mud_profile_init (MudProfile *profile)
{
    /* Set private data */
    profile->priv = MUD_PROFILE_GET_PRIVATE(profile);

    /* Set defaults */ 
    profile->name = NULL;

    profile->priv->parent = NULL;
    
    profile->priv->in_notification_count = 0;
    profile->priv->gconf_client = gconf_client_get_default();
    profile->preferences = &profile->priv->preferences;
}

static GObject *
mud_profile_constructor (GType gtype,
                         guint n_properties,
                         GObjectConstructParam *properties)
{
    MudProfile *self;
    GObject *obj;
    MudProfileClass *klass;
    GObjectClass *parent_class;

    /* Chain up to parent constructor */
    klass = MUD_PROFILE_CLASS( g_type_class_peek(MUD_TYPE_PROFILE) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    self = MUD_PROFILE(obj);

    if(!self->name)
    {
        g_printf("ERROR: Tried to instantiate MudProfile without passing name.\n");
        g_error("Tried to instantiate MudProfile without passing name.");
    }

    if(!self->priv->parent || !MUD_IS_PROFILE_MANAGER(self->priv->parent))
    {
        g_printf("ERROR: Tried to instantiate MudProfile without passing parent manager.\n");
        g_error("Tried to instantiate MudProfile without passing parent manager.");
    }

    mud_profile_load_preferences(self);

    if (g_str_equal(self->name, "Default"))
    {
        self->priv->gconf_signal = gconf_client_notify_add(self->priv->gconf_client,
                                            "/apps/gnome-mud",
                                            mud_profile_gconf_changed,
                                            self,
                                            NULL,
                                            NULL);
    }
    else
    {
        gchar buf[512];

        g_snprintf(buf, 512, "/apps/gnome-mud/profiles/%s", self->name);
        self->priv->gconf_signal = gconf_client_notify_add(self->priv->gconf_client,
                                    buf,
                                    mud_profile_gconf_changed,
                                    self,
                                    NULL,
                                    NULL);
    }

    return obj;
}

static void
mud_profile_finalize (GObject *object)
{
    MudProfile *profile;
    GObjectClass *parent_class;
    GConfClient *client = gconf_client_get_default();

    profile = MUD_PROFILE(object);

    gconf_client_notify_remove(client, profile->priv->gconf_signal);

    g_free(profile->priv->preferences.FontName);
    g_free(profile->priv->preferences.CommDev);
    g_free(profile->priv->preferences.LastLogDir);
    g_free(profile->priv->preferences.Encoding);
    g_free(profile->priv->preferences.ProxyVersion);
    g_free(profile->priv->preferences.ProxyHostname);

    g_object_unref(profile->priv->gconf_client);

    g_free(profile->name);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

static void
mud_profile_set_property(GObject *object,
                         guint prop_id,
                         const GValue *value,
                         GParamSpec *pspec)
{
    MudProfile *self;
    gchar *new_string;

    self = MUD_PROFILE(object);

    switch(prop_id)
    {
        case PROP_NAME:
            new_string = g_value_dup_string(value);

            if(!self->name)
                self->name = g_strdup(new_string);
            else if( !g_str_equal(self->name, new_string) )
            {
                g_free(self->name);
                self->name = g_strdup(new_string);
            }

            g_free(new_string);
            break;

        case PROP_PARENT:
            self->priv->parent = MUD_PROFILE_MANAGER(g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
mud_profile_get_property(GObject *object,
                         guint prop_id,
                         GValue *value,
                         GParamSpec *pspec)
{
    MudProfile *self;

    self = MUD_PROFILE(object);

    switch(prop_id)
    {
        case PROP_NAME:
            g_value_set_string(value, self->name);
            break;

        case PROP_PARENT:
            g_value_take_object(value, self->priv->parent);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

/* MudProfile Public Methods */
void
mud_profile_copy_preferences(MudProfile *from, MudProfile *to)
{
    gint i;
    gchar *s;
    const gchar *key;

    g_return_if_fail(IS_MUD_PROFILE(from));
    g_return_if_fail(IS_MUD_PROFILE(to));

    mud_profile_set_echotext(to, from->preferences->EchoText);
    mud_profile_set_keeptext(to, from->preferences->KeepText);
    mud_profile_set_disablekeys(to, from->preferences->DisableKeys);
    mud_profile_set_commdev(to, from->preferences->CommDev);
    mud_profile_set_scrollback(to, from->preferences->Scrollback);
    mud_profile_set_font(to, from->preferences->FontName);
    mud_profile_set_foreground(to, from->preferences->Foreground.red,
            from->preferences->Foreground.green,
            from->preferences->Foreground.blue);
    mud_profile_set_background(to, from->preferences->Background.red,
            from->preferences->Background.green,
            from->preferences->Background.blue);

    for (i = 0; i < C_MAX; i++)
    {
        to->preferences->Colors[i].red = from->preferences->Colors[i].red;
        to->preferences->Colors[i].blue = from->preferences->Colors[i].blue;
        to->preferences->Colors[i].green = from->preferences->Colors[i].green;
    }
    key = mud_profile_gconf_get_key(to, "ui/palette");
    s = gtk_color_selection_palette_to_string(to->preferences->Colors, C_MAX);
    gconf_client_set_string(from->priv->gconf_client, key, s, NULL);

    mud_profile_set_encoding_combo(to, from->preferences->Encoding);
    mud_profile_set_encoding_check(to, from->preferences->UseRemoteEncoding);
    mud_profile_set_proxy_check(to, from->preferences->UseProxy);
    mud_profile_set_msp_check(to, from->preferences->UseRemoteDownload);
    mud_profile_set_proxy_combo_full(to, from->preferences->ProxyVersion);
    mud_profile_set_proxy_entry(to, from->preferences->ProxyHostname);
}

/* MudProfile Private Methods */
static void
mud_profile_load_preferences(MudProfile *profile)
{
    GConfClient *gconf_client;
    MudPrefs *prefs;
    GdkColor  color;
    GdkColor *colors;
    gint      n_colors;
    gchar     extra_path[512] = "", keyname[2048];
    gchar *p;
    MudProfile *default_profile;

    g_return_if_fail(IS_MUD_PROFILE(profile));

    gconf_client = gconf_client_get_default();
    prefs = profile->preferences;

    if (!g_str_equal(profile->name, "Default"))
        g_snprintf(extra_path, 512, "profiles/%s/", profile->name);

    if(!g_str_equal(profile->name, "Default"))
    {
        gchar *key;
        gchar *test_string;

        key = g_strdup_printf("/apps/gnome-mud/profiles/%s/functionality/encoding",
                               profile->name);

        test_string = gconf_client_get_string(gconf_client, key, NULL);

        if(!test_string)
        {
            default_profile = mud_profile_manager_get_profile_by_name(profile->priv->parent,
                    "Default");
            mud_profile_copy_preferences(default_profile, profile);

        }
        else
            g_free(test_string);
    }

#define	GCONF_GET_STRING(entry, subdir, variable)                                          \
    if(prefs->variable) g_free(prefs->variable);                                    \
    g_snprintf(keyname, 2048, "/apps/gnome-mud/%s" #subdir "/" #entry, extra_path); \
    p = gconf_client_get_string(gconf_client, keyname, NULL);\
    prefs->variable = g_strdup(p);

#define GCONF_GET_BOOLEAN(entry, subdir, variable)                                         \
    g_snprintf(keyname, 2048, "/apps/gnome-mud/%s" #subdir "/" #entry, extra_path); \
    prefs->variable = gconf_client_get_bool(gconf_client, keyname, NULL);

#define GCONF_GET_INT(entry, subdir, variable)                                             \
    g_snprintf(keyname, 2048, "/apps/gnome-mud/%s" #subdir "/" #entry, extra_path); \
    prefs->variable = gconf_client_get_int(gconf_client, keyname, NULL);

#define GCONF_GET_COLOR(entry, subdir, variable)                                           \
    g_snprintf(keyname, 2048, "/apps/gnome-mud/%s" #subdir "/" #entry, extra_path); \
    p = gconf_client_get_string(gconf_client, keyname, NULL);\
    if (p && gdk_color_parse(p, &color))                                                   \
    {                                                                                      \
        prefs->variable = color;                                                            \
    }

    GCONF_GET_STRING(font, 				ui,				FontName);
    GCONF_GET_COLOR(foreground_color,	ui,				Foreground);
    GCONF_GET_COLOR(background_color,	ui,				Background);
    GCONF_GET_INT(scrollback_lines,		ui,				Scrollback);
    GCONF_GET_STRING(commdev, 			functionality,	CommDev);
    GCONF_GET_BOOLEAN(echo,     		functionality,	EchoText);
    GCONF_GET_BOOLEAN(keeptext,			functionality,	KeepText);
    GCONF_GET_BOOLEAN(system_keys,		functionality,	DisableKeys);
    GCONF_GET_INT(flush_interval,		functionality,	FlushInterval);
    GCONF_GET_STRING(encoding,          functionality,  Encoding);
    GCONF_GET_STRING(proxy_version,     functionality,  ProxyVersion);
    GCONF_GET_BOOLEAN(use_proxy,        functionality,  UseProxy);
    GCONF_GET_BOOLEAN(remote_encoding,  functionality,  UseRemoteEncoding);
    GCONF_GET_STRING(proxy_hostname,    functionality,  ProxyHostname);
    GCONF_GET_BOOLEAN(remote_download,  functionality,  UseRemoteDownload);

    /* palette */
    g_snprintf(keyname, 2048, "/apps/gnome-mud/%sui/palette", extra_path);
    p = gconf_client_get_string(gconf_client, keyname, NULL);

    if (p)
    {
        gtk_color_selection_palette_from_string(p, &colors, &n_colors);
        if (n_colors < C_MAX)
        {
            g_printerr(ngettext("Palette had %d entry instead of %d\n",
                        "Palette had %d entries instead of %d\n",
                        n_colors),
                    n_colors, C_MAX);
        }
        memcpy(prefs->Colors, colors, C_MAX * sizeof(GdkColor));
        g_free(colors);
    }

    /* last log dir */
    g_snprintf(keyname, 2048, "/apps/gnome-mud/%sfunctionality/last_log_dir", extra_path);
    p = gconf_client_get_string(gconf_client, keyname, NULL);

    if (p == NULL || !g_ascii_strncasecmp(p, "", sizeof("")))
    {
        prefs->LastLogDir = g_strdup(g_get_home_dir());
    }
    else
    {
        prefs->LastLogDir = g_strdup(p);
    }

    g_object_unref(gconf_client);

#undef GCONF_GET_BOOLEAN
#undef GCONF_GET_COLOR
#undef GCONF_GET_INT
#undef GCONF_GET_STRING
}

static gchar *
color_to_string(const GdkColor *c)
{
    gchar *s;
    gchar *ptr;

    s = g_strdup_printf("#%2X%2X%2X", c->red / 256, c->green / 256, c->blue / 256);

    for (ptr = s; *ptr; ptr++)
        if (*ptr == ' ')
            *ptr = '0';

    return s;
}

static const gchar *
mud_profile_gconf_get_key(MudProfile *profile, const gchar *key)
{
    static gchar buf[2048];
    gchar extra_path[512] = "";

    if (strcmp(profile->name, "Default"))
        g_snprintf(extra_path, 512, "profiles/%s/", profile->name);

    g_snprintf(buf, 2048, "/apps/gnome-mud/%s%s", extra_path, key);

    return buf;
}

static void
mud_profile_gconf_changed(GConfClient *client, guint cnxn_id, GConfEntry *entry, gpointer data)
{
    MudProfile *profile = MUD_PROFILE(data);
    MudProfileMask mask;
    GConfValue *val;
    gchar *key;
    gchar **path = NULL;

    gboolean bool_setting;
    gint int_setting;
    const gchar *string_setting;

    path = g_strsplit_set(gconf_entry_get_key(entry), "/", 6);

    if (!strcmp(profile->name, "Default") && !strcmp(path[3], "profiles"))
    {
        g_strfreev(path);
        return;
    }

    g_strfreev(path);

    val = gconf_entry_get_value(entry);
    key = g_path_get_basename(gconf_entry_get_key(entry));

    if(strcmp(key, "echo") == 0)
    {
        bool_setting = TRUE;

        if(val && val->type == GCONF_VALUE_BOOL)
            bool_setting = gconf_value_get_bool(val);

        if(bool_setting != profile->priv->preferences.EchoText)
        {
            mask.EchoText = TRUE;
            profile->priv->preferences.EchoText = bool_setting;
        }
    }
    else if(strcmp(key, "keeptext") == 0)
    {
        bool_setting = FALSE;

        if(val && val->type == GCONF_VALUE_BOOL)
            bool_setting = gconf_value_get_bool(val);

        if(bool_setting != profile->priv->preferences.KeepText)
        {
            mask.KeepText = TRUE;
            profile->priv->preferences.KeepText = bool_setting;
        }
    }
    else if(strcmp(key, "system_keys") == 0)
    {
        bool_setting = FALSE;

        if(val && val->type == GCONF_VALUE_BOOL)
            bool_setting = gconf_value_get_bool(val);

        if(bool_setting != profile->priv->preferences.DisableKeys)
        {
            mask.DisableKeys = TRUE;
            profile->priv->preferences.DisableKeys = bool_setting;
        }
    }
    else if(strcmp(key, "use_proxy") == 0)
    {
        bool_setting = FALSE;

        if(val && val->type == GCONF_VALUE_BOOL)
            bool_setting = gconf_value_get_bool(val);

        if(bool_setting != profile->priv->preferences.UseProxy)
        {
            mask.UseProxy = TRUE;
            profile->priv->preferences.UseProxy = bool_setting;
        }
    }
    else if(strcmp(key, "remote_encoding") == 0)
    {
        bool_setting = FALSE;

        if(val && val->type == GCONF_VALUE_BOOL)
            bool_setting = gconf_value_get_bool(val);

        if(bool_setting != profile->priv->preferences.UseRemoteEncoding)
        {
            mask.UseRemoteEncoding = TRUE;
            profile->priv->preferences.UseRemoteEncoding = bool_setting;
        }
    }
    else if(strcmp(key, "remote_download") == 0)
    {
        bool_setting = FALSE;

        if(val && val->type == GCONF_VALUE_BOOL)
            bool_setting = gconf_value_get_bool(val);

        if(bool_setting != profile->priv->preferences.UseRemoteDownload)
        {
            mask.UseRemoteDownload = TRUE;
            profile->priv->preferences.UseRemoteDownload = bool_setting;
        }
    }
    else if(strcmp(key, "scrollback_lines") == 0)
    {
        int_setting = 500;

        if(val && val->type == GCONF_VALUE_INT)
            int_setting = gconf_value_get_int(val);

        mask.Scrollback = set_Scrollback(profile, int_setting);
    }
    else if(strcmp(key, "commdev") == 0)
    {
        string_setting = ";";

        if(val && val->type == GCONF_VALUE_STRING)
            string_setting = gconf_value_get_string(val);

        mask.CommDev = set_CommDev(profile, string_setting);
    }
    else if(strcmp(key, "font") == 0)
    {
        string_setting = "monospace 12";

        if(val && val->type == GCONF_VALUE_STRING)
            string_setting = gconf_value_get_string(val);

        mask.FontName = set_FontName(profile, string_setting);
    }
    else if(strcmp(key, "foreground_color") == 0)
    {
        string_setting = "#FFFFFF";

        if(val && val->type == GCONF_VALUE_STRING)
            string_setting = gconf_value_get_string(val);

        mask.Foreground = set_Foreground(profile, string_setting);
    }
    else if(strcmp(key, "background_color") == 0)
    {
        string_setting = "#FFFFFF";

        if(val && val->type == GCONF_VALUE_STRING)
            string_setting = gconf_value_get_string(val);

        mask.Background = set_Background(profile, string_setting);
    }
    else if(strcmp(key, "palette") == 0)
    {
        string_setting =
            "#000000:#AA0000:#00AA00:#AA5500:#0000AA:#AA00AA:#00AAAA:#AAAAAA:#555555:#FF5555:#55FF55:#FFFF55:#5555FF:#FF55FF:#55FFFF:#FFFFFF";
        if(val && val->type == GCONF_VALUE_STRING)
            string_setting = gconf_value_get_string(val);

        mask.Colors = set_Colors(profile, string_setting);
    }
    else if(strcmp(key, "proxy_version") == 0)
    {
        string_setting = "5";

        if(val && val->type == GCONF_VALUE_STRING)
            string_setting = gconf_value_get_string(val);

        mask.ProxyVersion = set_ProxyVersion(profile, string_setting);
    }
    else if(strcmp(key, "proxy_hostname") == 0)
    {
        string_setting = "127.0.0.1";

        if(val && val->type == GCONF_VALUE_STRING)
            string_setting = gconf_value_get_string(val);

        mask.ProxyHostname = set_ProxyHostname(profile, string_setting);
    }
    else if(strcmp(key, "encoding") == 0)
    {
        string_setting = "127.0.0.1";

        if(val && val->type == GCONF_VALUE_STRING)
            string_setting = gconf_value_get_string(val);

        mask.Encoding = set_Encoding(profile, string_setting);
    }

    if(key)
        g_free(key);

    g_signal_emit(profile, mud_profile_signal[CHANGED], 0, &mask);
}


/* Profile Set Functions */
static gboolean
set_FontName(MudProfile *profile, const gchar *candidate)
{
    if (candidate && strcmp(profile->priv->preferences.FontName, candidate) == 0)
        return FALSE;

    if (candidate != NULL)
    {
        g_free(profile->priv->preferences.FontName);
        profile->priv->preferences.FontName = g_strdup(candidate);
        return TRUE;
    }

    return FALSE;
}

static gboolean
set_CommDev(MudProfile *profile, const gchar *candidate)
{
    if (candidate && strcmp(profile->priv->preferences.CommDev, candidate) == 0)
        return FALSE;

    if (candidate != NULL)
    {
        g_free(profile->priv->preferences.CommDev);
        profile->priv->preferences.CommDev = g_strdup(candidate);
        return TRUE;
    }

    return FALSE;
}

static gboolean
set_Scrollback(MudProfile *profile, const gint candidate)
{
    if (candidate >= 1 && candidate != profile->priv->preferences.Scrollback)
    {
        profile->priv->preferences.Scrollback = candidate;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static gboolean
set_ProxyVersion(MudProfile *profile, const gchar *candidate)
{

    if (candidate && strcmp(profile->priv->preferences.ProxyVersion, candidate) == 0)
        return FALSE;

    if (candidate != NULL)
    {
        g_free(profile->priv->preferences.ProxyVersion);
        profile->priv->preferences.ProxyVersion = g_strdup(candidate);
        return TRUE;
    }

    return FALSE;
}

static gboolean
set_ProxyHostname(MudProfile *profile, const gchar *candidate)
{
    if (candidate && strcmp(profile->priv->preferences.ProxyHostname, candidate) == 0)
        return FALSE;

    if (candidate != NULL)
    {
        g_free(profile->priv->preferences.ProxyHostname);
        profile->priv->preferences.ProxyHostname = g_strdup(candidate);
        return TRUE;
    }

    return FALSE;
}

static gboolean
set_Encoding(MudProfile *profile, const gchar *candidate)
{
    if (candidate && strcmp(profile->priv->preferences.Encoding, candidate) == 0)
        return FALSE;

    if (candidate != NULL)
    {
        g_free(profile->priv->preferences.Encoding);
        profile->priv->preferences.Encoding = g_strdup(candidate);
        return TRUE;
    }

    return FALSE;
}

void
mud_profile_set_disablekeys (MudProfile *profile, gboolean value)
{
    const gchar *key = mud_profile_gconf_get_key(profile, "functionality/system_keys");
    RETURN_IF_NOTIFYING(profile);

    gconf_client_set_bool(profile->priv->gconf_client, key, value, NULL);
}

void
mud_profile_set_keeptext (MudProfile *profile, gboolean value)
{
    const gchar *key = mud_profile_gconf_get_key(profile, "functionality/keeptext");
    RETURN_IF_NOTIFYING(profile);

    gconf_client_set_bool(profile->priv->gconf_client, key, value, NULL);
}

void
mud_profile_set_echotext (MudProfile *profile, gboolean value)
{
    const gchar *key = mud_profile_gconf_get_key(profile, "functionality/echo");
    RETURN_IF_NOTIFYING(profile);

    gconf_client_set_bool(profile->priv->gconf_client, key, value, NULL);
}

void
mud_profile_set_commdev (MudProfile *profile, const gchar *value)
{
    const gchar *key = mud_profile_gconf_get_key(profile, "functionality/commdev");
    RETURN_IF_NOTIFYING(profile);

    gconf_client_set_string(profile->priv->gconf_client, key, value, NULL);
}

void
mud_profile_set_terminal (MudProfile *profile, const gchar *value)
{
    const gchar *key = mud_profile_gconf_get_key(profile, "functionality/terminal_type");
    RETURN_IF_NOTIFYING(profile);

    gconf_client_set_string(profile->priv->gconf_client, key, value, NULL);
}

void
mud_profile_set_encoding_combo(MudProfile *profile, const gchar *e)
{
    GError *error = NULL;
    const gchar *key = mud_profile_gconf_get_key(profile, "functionality/encoding");
    RETURN_IF_NOTIFYING(profile);

    gconf_client_set_string(profile->priv->gconf_client, key, e, &error);
}

void
mud_profile_set_encoding_check (MudProfile *profile, const gint value)
{
    const gchar *key = mud_profile_gconf_get_key(profile, "functionality/remote_encoding");
    RETURN_IF_NOTIFYING(profile);

    gconf_client_set_bool(profile->priv->gconf_client, key, value, NULL);
}

void
mud_profile_set_proxy_check (MudProfile *profile, const gint value)
{
    const gchar *key = mud_profile_gconf_get_key(profile, "functionality/use_proxy");
    RETURN_IF_NOTIFYING(profile);

    gconf_client_set_bool(profile->priv->gconf_client, key, value, NULL);
}

void
mud_profile_set_msp_check (MudProfile *profile, const gint value)
{
    const gchar *key = mud_profile_gconf_get_key(profile, "functionality/remote_download");
    RETURN_IF_NOTIFYING(profile);

    gconf_client_set_bool(profile->priv->gconf_client, key, value, NULL);
}

static void
mud_profile_set_proxy_combo_full(MudProfile *profile, gchar *version)
{
    const gchar *key = mud_profile_gconf_get_key(profile, "functionality/proxy_version");
    RETURN_IF_NOTIFYING(profile);

    gconf_client_set_string(profile->priv->gconf_client, key, version, NULL);
}

void
mud_profile_set_proxy_combo(MudProfile *profile, GtkComboBox *combo)
{
    gchar *version = gtk_combo_box_get_active_text(combo);

    mud_profile_set_proxy_combo_full(profile, version);
}

void
mud_profile_set_proxy_entry (MudProfile *profile, const gchar *value)
{
    const gchar *key = mud_profile_gconf_get_key(profile, "functionality/proxy_hostname");
    RETURN_IF_NOTIFYING(profile);

    if(value)
        gconf_client_set_string(profile->priv->gconf_client, key, value, NULL);
    else
        gconf_client_set_string(profile->priv->gconf_client, key, "", NULL);
}

void
mud_profile_set_font (MudProfile *profile, const gchar *value)
{
    const gchar *key = mud_profile_gconf_get_key(profile, "ui/font");
    RETURN_IF_NOTIFYING(profile);

    gconf_client_set_string(profile->priv->gconf_client, key, value, NULL);
}

void
mud_profile_set_foreground (MudProfile *profile, guint r, guint g, guint b)
{
    GdkColor color;
    gchar *s;
    const gchar *key = mud_profile_gconf_get_key(profile, "ui/foreground_color");

    RETURN_IF_NOTIFYING(profile);

    color.red = r;
    color.green = g;
    color.blue = b;

    s = color_to_string(&color);

    gconf_client_set_string(profile->priv->gconf_client, key, s, NULL);
    g_free(s);
}

void
mud_profile_set_background (MudProfile *profile, guint r, guint g, guint b)
{
    GdkColor color;
    gchar *s;
    const gchar *key = mud_profile_gconf_get_key(profile, "ui/background_color");

    RETURN_IF_NOTIFYING(profile);

    color.red = r;
    color.green = g;
    color.blue = b;

    s = color_to_string(&color);
    gconf_client_set_string(profile->priv->gconf_client, key, s, NULL);
    g_free(s);
}

void
mud_profile_set_colors (MudProfile *profile, gint nr, guint r, guint g, guint b)
{
    GdkColor color[C_MAX];
    gchar *s;
    gint i;
    const gchar *key = mud_profile_gconf_get_key(profile, "ui/palette");

    RETURN_IF_NOTIFYING(profile);

    for (i = 0; i < C_MAX; i++)
    {
        if (i == nr)
        {
            color[i].red = r;
            color[i].green = g;
            color[i].blue = b;
        }
        else
        {
            color[i].red = profile->priv->preferences.Colors[i].red;
            color[i].green = profile->priv->preferences.Colors[i].green;
            color[i].blue = profile->priv->preferences.Colors[i].blue;
        }
    }

    s = gtk_color_selection_palette_to_string(color, C_MAX);
    gconf_client_set_string(profile->priv->gconf_client, key, s, NULL);
}

void
mud_profile_set_history(MudProfile *profile, const gint value)
{
    const gchar *key = mud_profile_gconf_get_key(profile, "functionality/history_count");
    RETURN_IF_NOTIFYING(profile);

    gconf_client_set_int(profile->priv->gconf_client, key, value, NULL);
}

void
mud_profile_set_scrollback(MudProfile *profile, const gint value)
{
    const gchar *key = mud_profile_gconf_get_key(profile, "ui/scrollback_lines");
    RETURN_IF_NOTIFYING(profile);

    gconf_client_set_int(profile->priv->gconf_client, key, value, NULL);
}

static gboolean
set_Foreground(MudProfile *profile, const gchar *candidate)
{
    GdkColor color;

    if (candidate && gdk_color_parse(candidate, &color))
    {
        if (!gdk_color_equal(&color, &profile->priv->preferences.Foreground))
        {
            profile->priv->preferences.Foreground.red = color.red;
            profile->priv->preferences.Foreground.green = color.green;
            profile->priv->preferences.Foreground.blue = color.blue;

            return TRUE;
        }
    }

    return FALSE;
}

static gboolean
set_Background(MudProfile *profile, const gchar *candidate)
{
    GdkColor color;

    if (candidate && gdk_color_parse(candidate, &color))
    {
        if (!gdk_color_equal(&color, &profile->priv->preferences.Background))
        {
            profile->priv->preferences.Background.red = color.red;
            profile->priv->preferences.Background.green = color.green;
            profile->priv->preferences.Background.blue = color.blue;

            return TRUE;
        }
    }

    return FALSE;
}

static gboolean
set_Colors(MudProfile *profile, const gchar *candidate)
{
    GdkColor *colors;
    gint n_colors;

    if (candidate)
    {
        gtk_color_selection_palette_from_string(candidate, &colors, &n_colors);
        if (n_colors < C_MAX)
        {
            g_printerr(ngettext("Palette had %d entry instead of %d\n",
                        "Palette had %d entries instead of %d\n",
                        n_colors),
                    n_colors, C_MAX);

            return FALSE;
        }
        memcpy(profile->priv->preferences.Colors, colors, C_MAX * sizeof(GdkColor));
        g_free(colors);
        return TRUE;
    }

    return FALSE;
}

