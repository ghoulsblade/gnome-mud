#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib-object.h>

#include "mud-preferences.h"

static char const rcsid[] = "$Id: ";

struct _MudPreferencesPrivate
{
	GConfClient		*gconf_client;
};

static void mud_preferences_init        (MudPreferences *preferences);
static void mud_preferences_class_init  (MudPreferencesClass *preferences);
static void mud_preferences_finalize    (GObject *object);

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
}

static void
mud_preferences_class_init (MudPreferencesClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = mud_preferences_finalize;
}

static void
mud_preferences_finalize (GObject *object)
{
	MudPreferences *preferences;
	GObjectClass *parent_class;

	preferences = MUD_PREFERENCES(object);

	g_free(preferences->priv);

	g_message("finalizing preferences...\n");
	parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
	parent_class->finalize(object);
}

MudPreferences*
mud_preferences_new (GConfClient *client)
{
	static MudPreferences *prefs = NULL;

	if (prefs == NULL)
	{
		prefs = g_object_new(MUD_TYPE_PREFERENCES, NULL);
		prefs->priv->gconf_client = client;
	}

	return prefs;
}

