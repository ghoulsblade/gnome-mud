#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gconf/gconf-client.h>
#include <glib/gi18n.h>
#include <gtk/gtkcolorsel.h>
#include <glib-object.h>
#include <string.h>

#include "gconf-helper.h"
#include "mud-preferences.h"

static char const rcsid[] = "$Id: ";

static guint signal_changed;

struct _MudPreferencesPrivate
{
	GConfClient		*gconf_client;

	GList *profile_list;

	guint prefs_signal;
	gint in_notification_count;
};

static void mud_preferences_init          (MudPreferences *preferences);
static void mud_preferences_class_init    (MudPreferencesClass *preferences);
static void mud_preferences_finalize      (GObject *object);
static void mud_preferences_gconf_changed (GConfClient *client, guint cnxn_id, GConfEntry *entry, gpointer data);

#define RETURN_IF_NOTIFYING(prefs) if ((prefs)->priv->in_notification_count) return

static char *color_to_string(const GdkColor *c)
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
mud_preferences_get_type (void)
{
	static GType object_type = 0;

	g_type_init();

	if (!object_type)
	{
		static const GTypeInfo object_info =
		{
			sizeof (MudPreferencesClass),
			NULL,
			NULL,
			(GClassInitFunc) mud_preferences_class_init,
			NULL,
			NULL,
			sizeof (MudPreferences),
			0,
			(GInstanceInitFunc) mud_preferences_init,
		};

		object_type = g_type_register_static(G_TYPE_OBJECT, "MudPreferences", &object_info, 0);
	}

	return object_type;
}

static void
mud_preferences_init (MudPreferences *preferences)
{
	preferences->priv = g_new0(MudPreferencesPrivate, 1);

	preferences->priv->in_notification_count = 0;
}

static void
mud_preferences_class_init (MudPreferencesClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = mud_preferences_finalize;

	signal_changed = 
	g_signal_new("changed",
				 G_OBJECT_CLASS_TYPE(object_class),
				 G_SIGNAL_RUN_LAST,
				 G_STRUCT_OFFSET(MudPreferencesClass, changed),
				 NULL, NULL,
				 g_cclosure_marshal_VOID__POINTER,
				 G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void
mud_preferences_finalize (GObject *object)
{
	MudPreferences *preferences;
	GObjectClass *parent_class;

	preferences = MUD_PREFERENCES(object);

	g_list_free(preferences->priv->profile_list);
	g_free(preferences->priv);

	g_free(preferences->preferences.FontName);	
	g_free(preferences->preferences.TabLocation);	
	g_free(preferences->preferences.CommDev);	
	g_free(preferences->preferences.TerminalType);	
	g_free(preferences->preferences.MudListFile);
	g_free(preferences->preferences.LastLogDir);
	
	g_message("finalizing preferences...\n");
	parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
	parent_class->finalize(object);
}

static gboolean
set_TerminalType(MudPreferences *prefs, const gchar *candidate)
{
	if (candidate && strcmp(prefs->preferences.TerminalType, candidate) == 0)
		return FALSE;

	if (candidate != NULL)
	{
		g_free(prefs->preferences.TerminalType);
		prefs->preferences.TerminalType = g_strdup(candidate);
		return TRUE;
	}

	return FALSE;
}

static gboolean
set_Foreground(MudPreferences *prefs, const gchar *candidate)
{
	GdkColor color;

	if (candidate && gdk_color_parse(candidate, &color))
	{
		if (!gdk_color_equal(&color, &prefs->preferences.Foreground))
		{
			prefs->preferences.Foreground.red = color.red;
			prefs->preferences.Foreground.green = color.green;
			prefs->preferences.Foreground.blue = color.blue;

			return TRUE;
		}
	}

	return FALSE;
}

static gboolean
set_Background(MudPreferences *prefs, const gchar *candidate)
{
	GdkColor color;

	if (candidate && gdk_color_parse(candidate, &color))
	{
		if (!gdk_color_equal(&color, &prefs->preferences.Background))
		{
			prefs->preferences.Background.red = color.red;
			prefs->preferences.Background.green = color.green;
			prefs->preferences.Background.blue = color.blue;

			return TRUE;
		}
	}

	return FALSE;
}

static gboolean
set_Colors(MudPreferences *prefs, const gchar *candidate)
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
		memcpy(prefs->preferences.Colors, colors, C_MAX * sizeof(GdkColor));
		g_free(colors);
		return TRUE;
	}

	return FALSE;
}

static gboolean
set_FontName(MudPreferences *prefs, const gchar *candidate)
{
	if (candidate && strcmp(prefs->preferences.FontName, candidate) == 0)
		return FALSE;

	if (candidate != NULL)
	{
		g_free(prefs->preferences.FontName);
		prefs->preferences.FontName = g_strdup(candidate);
		return TRUE;
	}

	return FALSE;
}

static gboolean
set_CommDev(MudPreferences *prefs, const gchar *candidate)
{
	if (candidate && strcmp(prefs->preferences.CommDev, candidate) == 0)
		return FALSE;
	
	if (candidate != NULL)
	{
		g_free(prefs->preferences.CommDev);
		prefs->preferences.CommDev = g_strdup(candidate);
		return TRUE;
	}

	return FALSE;
}

static gboolean
set_History(MudPreferences *prefs, const gint candidate)
{
	if (candidate >= 1 && candidate != prefs->preferences.History)
	{
		prefs->preferences.History = candidate;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

static gboolean
set_Scrollback(MudPreferences *prefs, const gint candidate)
{
	if (candidate >= 1 && candidate != prefs->preferences.Scrollback)
	{
		prefs->preferences.Scrollback = candidate;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

static void
mud_preferences_gconf_changed(GConfClient *client, guint cnxn_id, GConfEntry *entry, gpointer data)
{
	MudPreferences *prefs = MUD_PREFERENCES(data);
	MudPreferenceMask mask;
	GConfValue *val;
	gchar *key;

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
		if (setting != prefs->preferences.FName)    \
		{                                           \
			mask.FName = TRUE;                      \
			prefs->preferences.FName = setting;     \
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
		mask.FName = set_##FName(prefs, setting);
#define UPDATE_INTEGER(KName, FName, Preset)        \
	}                                               \
else if (strcmp(key, KName) == 0)                   \
	{                                               \
		gint setting = (Preset);                    \
                                                    \
		if (val && val->type == GCONF_VALUE_INT)    \
			setting = gconf_value_get_int(val);     \
				                                    \
		mask.FName = set_##FName(prefs, setting);
		

	
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
	}
	
#undef UPDATE_BOOLEAN
#undef UPDATE_STRING
#undef UPDATE_INTEGER
	g_message("Message from gconf...");

	g_signal_emit(G_OBJECT(prefs), signal_changed, 0, &mask);
}

void
mud_preferences_set_scrolloutput (MudPreferences *prefs, gboolean value)
{
	RETURN_IF_NOTIFYING(prefs);

	gconf_client_set_bool(prefs->priv->gconf_client,
						  "/apps/gnome-mud/functionality/scroll_on_output",
						  value, NULL);
}

void
mud_preferences_set_disablekeys (MudPreferences *prefs, gboolean value)
{
	RETURN_IF_NOTIFYING(prefs);

	gconf_client_set_bool(prefs->priv->gconf_client,
						  "/apps/gnome-mud/functionality/system_keys",
						  value, NULL);
}

void
mud_preferences_set_keeptext (MudPreferences *prefs, gboolean value)
{
	RETURN_IF_NOTIFYING(prefs);

	gconf_client_set_bool(prefs->priv->gconf_client,
						  "/apps/gnome-mud/functionality/keeptext",
						  value, NULL);
}

void
mud_preferences_set_echotext (MudPreferences *prefs, gboolean value)
{
	RETURN_IF_NOTIFYING(prefs);

	gconf_client_set_bool(prefs->priv->gconf_client, 
						  "/apps/gnome-mud/functionality/echo",
						  value, NULL);
}

void
mud_preferences_set_commdev (MudPreferences *prefs, const gchar *value)
{
	RETURN_IF_NOTIFYING(prefs);

	gconf_client_set_string(prefs->priv->gconf_client,
							"/apps/gnome-mud/functionality/commdev",
							value, NULL);
}

void
mud_preferences_set_terminal (MudPreferences *prefs, const gchar *value)
{
	RETURN_IF_NOTIFYING(prefs);

	gconf_client_set_string(prefs->priv->gconf_client,
							"/apps/gnome-mud/functionality/terminal_type",
							value, NULL);
}

void
mud_preferences_set_font (MudPreferences *prefs, const gchar *value)
{
	RETURN_IF_NOTIFYING(prefs);

	gconf_client_set_string(prefs->priv->gconf_client,
							"/apps/gnome-mud/ui/font",
							value, NULL);
}

void
mud_preferences_set_foreground (MudPreferences *prefs, guint r, guint g, guint b)
{
	GdkColor color;
	gchar *s;
	
	RETURN_IF_NOTIFYING(prefs);

	color.red = r;
	color.green = g;
	color.blue = b;
	
	s = color_to_string(&color);
	gconf_client_set_string(prefs->priv->gconf_client,
							"/apps/gnome-mud/ui/foreground_color", s, NULL);
	g_free(s);
}

void
mud_preferences_set_background (MudPreferences *prefs, guint r, guint g, guint b)
{
	GdkColor color;
	gchar *s;
	
	RETURN_IF_NOTIFYING(prefs);

	color.red = r;
	color.green = g;
	color.blue = b;
	
	s = color_to_string(&color);
	gconf_client_set_string(prefs->priv->gconf_client,
							"/apps/gnome-mud/ui/background_color", s, NULL);
	g_free(s);
}

void
mud_preferences_set_colors (MudPreferences *prefs, gint nr, guint r, guint g, guint b)
{
	GdkColor color[C_MAX];
	gchar *s;
	gint i;

	RETURN_IF_NOTIFYING(prefs);

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
			color[i].red = prefs->preferences.Colors[i].red;
			color[i].green = prefs->preferences.Colors[i].green;
			color[i].blue = prefs->preferences.Colors[i].blue;
		}
	}

	s = gtk_color_selection_palette_to_string(color, C_MAX);
	gconf_client_set_string(prefs->priv->gconf_client,
							"/apps/gnome-mud/ui/palette", s, NULL);
}

void
mud_preferences_set_history(MudPreferences *prefs, const gint value)
{
	RETURN_IF_NOTIFYING(prefs);

	gconf_client_set_int(prefs->priv->gconf_client,
						 "/apps/gnome-mud/functionality/history_count",
						 value, NULL);
}

void
mud_preferences_set_scrollback(MudPreferences *prefs, const gint value)
{
	RETURN_IF_NOTIFYING(prefs);

	gconf_client_set_int(prefs->priv->gconf_client,
						 "/apps/gnome-mud/ui/scrollback_lines",
						 value, NULL);
}

const GList*
mud_preferences_get_profiles (MudPreferences *prefs)
{
	return prefs->priv->profile_list;
}

void
mud_preferences_load_profiles (MudPreferences *prefs)
{
	GSList *profiles, *entry;

	profiles = gconf_client_get_list(prefs->priv->gconf_client, "/apps/gnome-mud/profiles/list", GCONF_VALUE_STRING, NULL);

	for (entry = profiles; entry != NULL; entry = g_slist_next(entry))
	{
		MudProfile *profile;
		gchar *pname, *epname;

		pname = g_strdup((gchar *) entry->data);
		epname = gconf_escape_key(pname, -1);

		profile = mud_profile_new(pname);

		prefs->priv->profile_list = g_list_append(prefs->priv->profile_list, (gpointer) profile);
		
		g_free(epname);
		g_free(pname);
	}
}

MudPreferences*
mud_preferences_new (GConfClient *client)
{
	static MudPreferences *prefs = NULL;

	if (prefs == NULL)
	{
		prefs = g_object_new(MUD_TYPE_PREFERENCES, NULL);
		prefs->priv->gconf_client = client;

		gm_gconf_load_preferences(client, &prefs->preferences);

		gconf_client_notify_add(client, "/apps/gnome-mud", 
								mud_preferences_gconf_changed, 
								prefs, NULL, NULL);
	
		mud_preferences_load_profiles(prefs);
	}
	
	return prefs;
}

