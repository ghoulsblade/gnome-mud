#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glade/glade.h>
#include <gtk/gtkdialog.h>
#include <glib-object.h>

#include "mud-preferences-window.h"

static char const rcsid[] = "$Id: ";

struct _MudPreferencesWindowPrivate
{
	GConfClient		*gconf_client;
};

static void mud_preferences_window_init        (MudPreferencesWindow *preferences);
static void mud_preferences_window_class_init  (MudPreferencesWindowClass *preferences);
static void mud_preferences_window_finalize    (GObject *object);

GType
mud_preferences_window_get_type (void)
{
	static GType object_type = 0;

	g_type_init();

	if (!object_type)
	{
		static const GTypeInfo object_info =
		{
			sizeof (MudPreferencesWindowClass),
			NULL,
			NULL,
			(GClassInitFunc) mud_preferences_window_class_init,
			NULL,
			NULL,
			sizeof (MudPreferencesWindow),
			0,
			(GInstanceInitFunc) mud_preferences_window_init,
		};

		object_type = g_type_register_static(G_TYPE_OBJECT, "MudPreferencesWindow", &object_info, 0);
	}

	return object_type;
}

static void
mud_preferences_window_init (MudPreferencesWindow *preferences)
{
	GladeXML *glade;
	GtkWidget *dialog;
	
	preferences->priv = g_new0(MudPreferencesWindowPrivate, 1);

	glade = glade_xml_new(GLADEDIR "main.glade", "preferences_window", "preferences_window");
	dialog = glade_xml_get_widget(glade, "preferences_window");
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

	//g_signal_connect(G_OBJECT(editor), "destroy", G_CALLBACK(mud_prefereces_destroyed), preferences);
	
	gtk_widget_show_all(dialog);
	
	gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
	gtk_window_present(GTK_WINDOW(dialog));

	g_object_unref(G_OBJECT(glade));
}

static void
mud_preferences_window_class_init (MudPreferencesWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = mud_preferences_window_finalize;
}

static void
mud_preferences_window_finalize (GObject *object)
{
	MudPreferencesWindow *preferences;
	GObjectClass *parent_class;

	preferences = MUD_PREFERENCES_WINDOW(object);

	g_free(preferences->priv);

	g_message("finalizing preferences window...\n");
	parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
	parent_class->finalize(object);
}

MudPreferencesWindow*
mud_preferences_window_new (GConfClient *client)
{
	MudPreferencesWindow *prefs;

	prefs = g_object_new(MUD_TYPE_PREFERENCES_WINDOW, NULL);
	prefs->priv->gconf_client = client;

	return prefs;
}

