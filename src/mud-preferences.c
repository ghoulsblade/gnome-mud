#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gconf/gconf-client.h>
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
				

	
	if (0)
	{
		;
		UPDATE_BOOLEAN("echo",				EchoText,		TRUE);
		UPDATE_BOOLEAN("keeptext",			KeepText,		FALSE);
		UPDATE_BOOLEAN("system_keys",		DisableKeys,	FALSE);
		UPDATE_BOOLEAN("scroll_on_output",	ScrollOnOutput,	FALSE);
		UPDATE_STRING("commdev",			CommDev,		";");
		UPDATE_STRING("terminal_type",		TerminalType,	"ansi");
	}
	
	
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
	}

	mud_preferences_load_profiles(prefs);
	
	return prefs;
}

