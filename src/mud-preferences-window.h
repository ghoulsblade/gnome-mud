#ifndef MUD_PREFERENCES_WINDOW_H
#define MUD_PREFERENCES_WINDOW_H

G_BEGIN_DECLS

#include <gconf/gconf-client.h>

#define MUD_TYPE_PREFERENCES_WINDOW              (mud_preferences_window_get_type ())
#define MUD_PREFERENCES_WINDOW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_PREFERENCES_WINDOW, MudPreferencesWindow))
#define MUD_PREFERENCES_WINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_PREFERENCES_WINDOW, MudPreferencesWindowClass))
#define MUD_IS_PREFERENCES_WINDOW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_PREFERENCES_WINDOW))
#define MUD_IS_PREFERENCES_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_PREFERENCES_WINDOW))
#define MUD_PREFERENCES_WINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_PREFERENCES_WINDOW, MudPreferenesWindowClass))

typedef struct _MudPreferencesWindow            MudPreferencesWindow;
typedef struct _MudPreferencesWindowClass       MudPreferencesWindowClass;
typedef struct _MudPreferencesWindowPrivate     MudPreferencesWindowPrivate;

struct _MudPreferencesWindow
{
	GObject parent_instance;

	MudPreferencesWindowPrivate *priv;
};

struct _MudPreferencesWindowClass
{
	GObjectClass parent_class;
};

GType mud_preferences_window_get_type (void) G_GNUC_CONST;

MudPreferencesWindow* mud_preferences_window_new (GConfClient *client);

G_END_DECLS

#endif // MUD_PREFERENCES_WINDOW_H
