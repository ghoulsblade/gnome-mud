#ifndef MUD_PREFERENCES_H
#define MUD_PREFERENCES_H

G_BEGIN_DECLS

#include <gconf/gconf-client.h>

#define MUD_TYPE_PREFERENCES              (mud_preferences_get_type ())
#define MUD_PREFERENCES(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_PREFERENCES, MudPreferences))
#define MUD_PREFERENCES_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_PREFERENCES, MudPreferencesClass))
#define MUD_IS_PREFERENCES(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_PREFERENCES))
#define MUD_IS_PREFERENCES_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_PREFERENCES))
#define MUD_PREFERENCES_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_PREFERENCES, MudPreferenesClass))

typedef struct _MudPreferences            MudPreferences;
typedef struct _MudPreferencesClass       MudPreferencesClass;
typedef struct _MudPreferencesPrivate     MudPreferencesPrivate;

struct _MudPreferences
{
	GObject parent_instance;

	MudPreferencesPrivate *priv;
};

struct _MudPreferencesClass
{
	GObjectClass parent_class;
};

GType mud_preferences_get_type (void) G_GNUC_CONST;

MudPreferences* mud_preferences_new (GConfClient *client);
GList *mud_preferences_get_profiles (MudPreferences *prefs);

G_END_DECLS

#endif // MUD_PREFERENCES_H
