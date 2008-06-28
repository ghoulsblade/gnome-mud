/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1998-2006 Robin Ericsson <lobbin@localhost.nu>
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
#include <gtk/gtkcolorsel.h>
#include <gtk/gtkcombobox.h>

#include "gconf-helper.h"
#include "mud-profile.h"
#include "utils.h"

gulong signal_changed;
GList *profile_list = NULL;

struct _MudProfilePrivate
{
	GConfClient *gconf_client;

	MudPrefs preferences;
	gint in_notification_count;

};

static void mud_profile_init          (MudProfile *profile);
static void mud_profile_class_init    (MudProfileClass *profile);
static void mud_profile_finalize      (GObject *object);
static void mud_profile_gconf_changed (GConfClient *client, guint cnxn_id, GConfEntry *entry, gpointer data);

#define RETURN_IF_NOTIFYING(profile)  if ((profile)->priv->in_notification_count) return

MudProfile*
get_profile(const gchar *name)
{
	GList *entry = NULL;
	MudProfile *profile;

	entry = profile_list;

	for (entry = profile_list; entry != NULL; entry = entry->next)
	{
		profile = MUD_PROFILE(entry->data);

		if (!strcmp(profile->name, name))
		{
			return profile;
		}
	}

	return NULL;
}

static char*
color_to_string(const GdkColor *c)
{
	char *s;
	char *ptr;

	s = g_strdup_printf("#%2X%2X%2X", c->red / 256, c->green / 256, c->blue / 256);

	for (ptr = s; *ptr; ptr++)
	{
		if (*ptr == ' ')
		{
			*ptr = '0';
		}
	}

	return s;
}

GType
mud_profile_get_type (void)
{
	static GType object_type = 0;

	g_type_init();

	if (!object_type)
	{
		static const GTypeInfo object_info =
		{
			sizeof (MudProfileClass),
			NULL,
			NULL,
			(GClassInitFunc) mud_profile_class_init,
			NULL,
			NULL,
			sizeof (MudProfile),
			0,
			(GInstanceInitFunc) mud_profile_init,
		};

		object_type = g_type_register_static(G_TYPE_OBJECT, "MudProfile", &object_info, 0);
	}

	return object_type;
}

static void
mud_profile_init (MudProfile *profile)
{
	profile->priv = g_new0(MudProfilePrivate, 1);
	profile->priv->in_notification_count = 0;

	profile->priv->gconf_client = gconf_client_get_default();
}

static void
mud_profile_class_init (MudProfileClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = mud_profile_finalize;
    signal_changed =
    g_signal_new("changed",
				 G_OBJECT_CLASS_TYPE(object_class),
				 G_SIGNAL_RUN_LAST,
				 G_STRUCT_OFFSET(MudProfileClass, changed),
				 NULL, NULL,
				 g_cclosure_marshal_VOID__POINTER,
				 G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void
mud_profile_finalize (GObject *object)
{
	MudProfile *profile;
	GObjectClass *parent_class;

	profile = MUD_PROFILE(object);

	g_free(profile->priv->preferences.FontName);
	g_free(profile->priv->preferences.TabLocation);
	g_free(profile->priv->preferences.CommDev);
	g_free(profile->priv->preferences.TerminalType);
	g_free(profile->priv->preferences.MudListFile);
	g_free(profile->priv->preferences.LastLogDir);

	g_free(profile->priv);
	g_free(profile->name);

	parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
	parent_class->finalize(object);
}

void
mud_profile_delete(const gchar *name)
{
	MudProfile *profile;
	GSList *profiles, *entry, *rementry;
	GError *error = NULL;
	gchar buf[512];
	GConfClient *client;


	client = gconf_client_get_default();

	rementry = NULL;
	rementry = g_slist_append(rementry, NULL);
	profile = get_profile(name);

	if (profile)
	{
		profile_list = g_list_remove(profile_list, profile);

		g_snprintf(buf, 512, "/apps/gnome-mud/profiles/list");
		profiles = gconf_client_get_list(client, buf, GCONF_VALUE_STRING, &error);
		for (entry = profiles; entry != NULL; entry = g_slist_next(entry))
		{
			if (strcmp((gchar *)entry->data, name) == 0)
			{
				rementry->data = entry->data;
			}
		}

		profiles = g_slist_remove(profiles, rementry->data);
		gconf_client_set_list(client, buf, GCONF_VALUE_STRING, profiles, &error);
	}
}

MudProfile*
mud_profile_new (const gchar *name)
{
	MudProfile *profile;
	GSList *profiles, *entry;
	GError *error = NULL;
	gint newflag;

	g_assert(name != NULL);

	profile = get_profile(name);
	if (profile == NULL)
	{
		profile = g_object_new(MUD_TYPE_PROFILE, NULL);
		profile->name = g_strdup(name);
		profile->preferences = &profile->priv->preferences;

		gm_gconf_load_preferences(profile);

		profile_list = g_list_append(profile_list, profile);

		if (!strcmp(name, "Default"))
		{
			gconf_client_notify_add(profile->priv->gconf_client,
						"/apps/gnome-mud",
						mud_profile_gconf_changed,
						profile, NULL, NULL);
		}
		else
		{
			gchar buf[512];

			newflag = 1;

			g_snprintf(buf, 512, "/apps/gnome-mud/profiles/list");
			profiles = gconf_client_get_list(gconf_client_get_default(), buf, GCONF_VALUE_STRING, &error);

			for (entry = profiles; entry != NULL; entry = g_slist_next(entry))
			{
				if(!strcmp((gchar *)entry->data,name))
				{
					newflag = 0;
				}
			}

			if (newflag)
			{
				profiles = g_slist_append(profiles, (void *)g_strdup(name));
				gconf_client_set_list(gconf_client_get_default(), buf, GCONF_VALUE_STRING, profiles, &error);
			}

			g_snprintf(buf, 512, "/apps/gnome-mud/profiles/%s", name);
			gconf_client_notify_add(profile->priv->gconf_client,
						buf,
						mud_profile_gconf_changed,
						profile, NULL, NULL);
		}
	}

	return profile;
}

static gboolean
set_TerminalType(MudProfile *profile, const gchar *candidate)
{
	if (candidate && strcmp(profile->priv->preferences.TerminalType, candidate) == 0)
		return FALSE;

	if (candidate != NULL)
	{
		g_free(profile->priv->preferences.TerminalType);
		profile->priv->preferences.TerminalType = g_strdup(candidate);
		return TRUE;
	}

	return FALSE;
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
set_History(MudProfile *profile, const gint candidate)
{
	if (candidate >= 1 && candidate != profile->priv->preferences.History)
	{
		profile->priv->preferences.History = candidate;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
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

static const gchar*
mud_profile_gconf_get_key(MudProfile *profile, const gchar *key)
{
	static gchar buf[2048];
	gchar extra_path[512] = "";

	if (strcmp(profile->name, "Default"))
	{
		g_snprintf(extra_path, 512, "profiles/%s/", profile->name);
	}

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

	path = g_strsplit_set(gconf_entry_get_key(entry), "/", 6);

	if (!strcmp(profile->name, "Default") && !strcmp(path[3], "profiles"))
	{
		g_strfreev(path);
		return;
	}

	g_strfreev(path);

	val = gconf_entry_get_value(entry);
	key = g_path_get_basename(gconf_entry_get_key(entry));

#define UPDATE_BOOLEAN(KName, FName, Preset)        \
	}                                               \
else if (strcmp(key, KName) == 0)                   \
	{                                               \
		gboolean setting = (Preset);                \
		                                            \
		if (val && val->type == GCONF_VALUE_BOOL)   \
			setting = gconf_value_get_bool(val);    \
		                                            \
		if (setting != profile->priv->preferences.FName)    \
		{                                           \
			mask.FName = TRUE;                      \
			profile->priv->preferences.FName = setting;     \
		}
#define UPDATE_STRING(KName, FName, Preset)         \
	}                                               \
else if (strcmp(key, KName) == 0)                   \
	{                                               \
		const gchar *setting = (Preset);            \
			                                        \
		if (val && val->type == GCONF_VALUE_STRING) \
			setting = gconf_value_get_string(val);  \
			                                        \
		mask.FName = set_##FName(profile, setting);
#define UPDATE_INTEGER(KName, FName, Preset)        \
	}                                               \
else if (strcmp(key, KName) == 0)                   \
	{                                               \
		gint setting = (Preset);                    \
                                                    \
		if (val && val->type == GCONF_VALUE_INT)    \
			setting = gconf_value_get_int(val);     \
				                                    \
		mask.FName = set_##FName(profile, setting);



	if (0)
	{
		;
		UPDATE_BOOLEAN("echo",				EchoText,		TRUE);
		UPDATE_BOOLEAN("keeptext",			KeepText,		FALSE);
		UPDATE_BOOLEAN("system_keys",		DisableKeys,	FALSE);
		UPDATE_BOOLEAN("scroll_on_output",	ScrollOnOutput,	FALSE);
		UPDATE_STRING("commdev",			CommDev,		";");
		UPDATE_STRING("terminal_type",		TerminalType,	"ansi");
		UPDATE_INTEGER("history_count",		History,		10);
		UPDATE_INTEGER("scrollback_lines",	Scrollback,		500);
		UPDATE_STRING("font",				FontName,		"monospace 12");
		UPDATE_STRING("foreground_color",	Foreground,		"#FFFFFF");
		UPDATE_STRING("background_color",	Background,		"#000000");
		UPDATE_STRING("palette",			Colors,			"#000000:#AA0000:#00AA00:#AA5500:#0000AA:#AA00AA:#00AAAA:#AAAAAA:#555555:#FF5555:#55FF55:#FFFF55:#5555FF:#FF55FF:#55FFFF:#FFFFFF");
	    UPDATE_STRING("proxy_version", ProxyVersion, "5");
	    UPDATE_STRING("proxy_hostname", ProxyHostname, "127.0.0.1");
	    UPDATE_STRING("encoding", Encoding, "ISO-8859-1");
	    UPDATE_BOOLEAN("use_proxy", UseProxy, FALSE);
	    UPDATE_BOOLEAN("remote_encoding", UseRemoteEncoding, FALSE);
	}

#undef UPDATE_BOOLEAN
#undef UPDATE_STRING
#undef UPDATE_INTEGER
	g_signal_emit(G_OBJECT(profile), signal_changed, 0, &mask);
}

void
mud_profile_set_scrolloutput (MudProfile *profile, gboolean value)
{
	const gchar *key = mud_profile_gconf_get_key(profile, "functionality/scroll_on_output");
	RETURN_IF_NOTIFYING(profile);

	gconf_client_set_bool(profile->priv->gconf_client, key, value, NULL);
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

void
mud_profile_load_profiles ()
{
	GSList *profiles, *entry;

	g_return_if_fail(profile_list == NULL);

	profiles = gconf_client_get_list(gconf_client_get_default(), "/apps/gnome-mud/profiles/list", GCONF_VALUE_STRING, NULL);

	for (entry = profiles; entry != NULL; entry = g_slist_next(entry))
	{
		MudProfile *profile;
		gchar *pname, *epname;

		pname = g_strdup((gchar *) entry->data);
		epname = gconf_escape_key(pname, -1);

		profile = mud_profile_new(pname);

		g_free(epname);
		g_free(pname);
	}
}

void
mud_profile_copy_preferences(MudProfile *from, MudProfile *to)
{
	gint i;

	mud_profile_set_echotext(to, from->preferences->EchoText);
	mud_profile_set_keeptext(to, from->preferences->KeepText);
	mud_profile_set_disablekeys(to, from->preferences->DisableKeys);
	mud_profile_set_scrolloutput(to, from->preferences->ScrollOnOutput);
	mud_profile_set_commdev(to, from->preferences->CommDev);
	mud_profile_set_terminal(to, from->preferences->TerminalType);
	mud_profile_set_history(to, from->preferences->History);
	mud_profile_set_scrollback(to, from->preferences->Scrollback);
	mud_profile_set_font(to, from->preferences->FontName);
	mud_profile_set_foreground(to, from->preferences->Foreground.red,
				       from->preferences->Foreground.green,
				       from->preferences->Foreground.blue);
	mud_profile_set_background(to, from->preferences->Foreground.red,
				       from->preferences->Foreground.green,
				       from->preferences->Foreground.blue);
	for (i = 0; i < C_MAX; i++)
	{
		mud_profile_set_colors(to, i, from->preferences->Colors[i].red,
					      from->preferences->Colors[i].green,
					      from->preferences->Colors[i].blue);
	}
	mud_profile_set_encoding_combo(to, from->preferences->Encoding);
	mud_profile_set_encoding_check(to, from->preferences->UseRemoteEncoding);
	mud_profile_set_proxy_check(to, from->preferences->UseProxy);
	mud_profile_set_proxy_combo_full(to, from->preferences->ProxyVersion);
	mud_profile_set_proxy_entry(to, from->preferences->ProxyHostname);
}

GList *
mud_profile_process_command(MudProfile *profile, const gchar *data, GList *commandlist)
{
	gint i;
	gchar **commands = g_strsplit(data, profile->preferences->CommDev, -1);

    if(commands[0])
    {
	    commandlist = g_list_append(commandlist, g_strdup(commands[0]));

	    for (i = 1; commands[i] != NULL; i++)
	    {
		    commandlist = mud_profile_process_command(profile, commands[i], commandlist);
	    }
	}

	g_strfreev(commands);

	return commandlist;
}

GList *
mud_profile_process_commands(MudProfile *profile, const gchar *data)
{
	return mud_profile_process_command(profile, data, NULL);
}

gchar *
mud_profile_from_number(gint num)
{
	GList *entry;
	gint counter = 0;

	for (entry = (GList *)profile_list; entry != NULL; entry = g_list_next(entry))
	{
		if (counter == num)
		{
			return (gchar *)MUD_PROFILE(entry->data)->name;
		}

		counter++;
	}

	return NULL;
}

gint
mud_profile_num_from_name(gchar *name)
{
	GList *entry;
	gint counter = 0;

	for (entry = (GList *)profile_list; entry != NULL; entry = g_list_next(entry))
	{
		if (!strcmp((gchar *)MUD_PROFILE(entry->data)->name,name))
		{
			return counter;
		}

		counter++;
	}

	return -1;
}

gchar *
mud_profile_get_name(MudProfile *profile)
{
	return profile->name;
}

const GList*
mud_profile_get_profiles ()
{
	return profile_list;
}
