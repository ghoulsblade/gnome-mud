#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glade/glade.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkentry.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkwidget.h>
#include <libgnome/gnome-i18n.h>
#include <stdlib.h>

#include "mud-connection-view.h"
#include "mud-preferences.h"
#include "mud-preferences-window.h"
#include "mud-window.h"

static char const rcsid[] = "$Id: ";

struct _MudWindowPrivate
{
	GConfClient *gconf_client;

	MudPreferences *prefs;
	
	GtkWidget *window;
	GtkWidget *notebook;
	GtkWidget *textentry;

	GtkWidget *blank_label;
	GtkWidget *current_view;

	gchar *host;
	gchar *port;

	gint nr_of_tabs;
};

static int
mud_window_close(GtkWidget *widget, MudWindow *window)
{
	g_object_unref(window);

	return TRUE;
}

void
mud_window_add_connection_view(MudWindow *window, MudConnectionView *view)
{
	gint nr;
	
	g_assert(window != NULL);
	g_assert(view != NULL);

	if (window->priv->nr_of_tabs++ == 0)
	{
		gtk_notebook_remove_page(GTK_NOTEBOOK(window->priv->notebook), 0);
	}
	
	nr = gtk_notebook_append_page(GTK_NOTEBOOK(window->priv->notebook), mud_connection_view_get_viewport(view), NULL);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(window->priv->notebook), nr);

	if (window->priv->nr_of_tabs > 1)
	{
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(window->priv->notebook), TRUE);
	}
}

static void
mud_window_remove_connection_view(MudWindow *window, gint nr)
{
	g_object_unref(window->priv->current_view);
	gtk_notebook_remove_page(GTK_NOTEBOOK(window->priv->notebook), nr);
	
	if (--window->priv->nr_of_tabs < 2)
	{
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(window->priv->notebook), FALSE);
	}

	if (window->priv->nr_of_tabs == 0)
	{
		gtk_notebook_append_page(GTK_NOTEBOOK(window->priv->notebook), window->priv->blank_label, NULL);
	}
}
static void
mud_window_disconnect_cb(GtkWidget *widget, MudWindow *window)
{
	if (window->priv->current_view != NULL)
		mud_connection_view_disconnect(MUD_CONNECTION_VIEW(window->priv->current_view));
}

static void
mud_window_reconnect_cb(GtkWidget *widget, MudWindow *window)
{
	if (window->priv->current_view != NULL)
		mud_connection_view_reconnect(MUD_CONNECTION_VIEW(window->priv->current_view));
}

static void
mud_window_closewindow_cb(GtkWidget *widget, MudWindow *window)
{
	if (window->priv->nr_of_tabs > 0)
	{
		gint nr = gtk_notebook_get_current_page(GTK_NOTEBOOK(window->priv->notebook));

		mud_window_remove_connection_view(window, nr);
	}
}

static gboolean
mud_window_textentry_keypress(GtkWidget *widget, GdkEventKey *event, MudWindow *window)
{

	return FALSE;
}

static void
mud_window_notebook_page_change(GtkNotebook *notebook, GtkNotebookPage *page, gint arg, MudWindow *window)
{
	window->priv->current_view = g_object_get_data(G_OBJECT(gtk_notebook_get_nth_page(notebook, arg)), "connection-view");
}

static void
mud_window_textentry_activate(GtkWidget *widget, MudWindow *window)
{
	if (window->priv->current_view)
		mud_connection_view_send(MUD_CONNECTION_VIEW(window->priv->current_view), gtk_entry_get_text(GTK_ENTRY(widget)));
}

static void
mud_window_preferences_cb(GtkWidget *widget, MudWindow *window)
{
	mud_preferences_window_new(window->priv->prefs);
}

static void
mud_window_connect_dialog(GtkWidget *widget, MudWindow *window)
{
	GladeXML *glade;
	GtkWidget *dialog;
	GtkWidget *entry_host;
	GtkWidget *entry_port;
	gint result;

	glade = glade_xml_new(GLADEDIR "main.glade", "connect_window", "connect_window");
	dialog = glade_xml_get_widget(glade, "connect_window");
	
	entry_host = glade_xml_get_widget(glade, "entry_host");
	entry_port = glade_xml_get_widget(glade, "entry_post");
	gtk_entry_set_text(GTK_ENTRY(entry_host), window->priv->host);
	gtk_entry_set_text(GTK_ENTRY(entry_port), window->priv->port);

	result = gtk_dialog_run(GTK_DIALOG(dialog));
	if (result == GTK_RESPONSE_OK)
	{
		MudConnectionView *view;
		const gchar *host, *port;
		gint iport;
		
		g_free(window->priv->host);
		host = gtk_entry_get_text(GTK_ENTRY(entry_host));
		window->priv->host = g_strdup(host);

		g_free(window->priv->port);
		port = gtk_entry_get_text(GTK_ENTRY(entry_port));
		window->priv->port = g_strdup(port);
		iport = atoi(port);
		
		if (iport < 1)
		{
			iport = 23;
		}
		
		view = mud_connection_view_new(host, iport);
		mud_window_add_connection_view(window, view);
	}

	gtk_widget_destroy(dialog);
	g_object_unref(glade);
}

static void mud_window_init       (MudWindow *window);
static void mud_window_class_init (MudWindowClass *klass);
static void mud_window_finalize   (GObject *object);

GType
mud_window_get_type (void)
{
	static GType object_type = 0;

	g_type_init();

	if (!object_type)
	{
		static const GTypeInfo object_info =
		{
			sizeof (MudWindowClass),
			NULL,
			NULL,
			(GClassInitFunc) mud_window_class_init,
			NULL,
			NULL,
			sizeof (MudWindow),
			0,
			(GInstanceInitFunc) mud_window_init,
		};

		object_type = g_type_register_static(G_TYPE_OBJECT, "MudWindow", &object_info, 0);
	}

	return object_type;
}

static void
mud_window_init (MudWindow *window)
{
	GladeXML *glade;
	gchar buf[1024];
	
	window->priv = g_new0(MudWindowPrivate, 1);

	/* set members */
	window->priv->host = g_strdup("");
	window->priv->port = g_strdup("");
	
	/* start glading */
	glade = glade_xml_new(GLADEDIR "main.glade", "main_window", "main_window");
	window->priv->window = glade_xml_get_widget(glade, "main_window");
	gtk_widget_show_all(window->priv->window);

	/* connect quit buttons */
	g_signal_connect(window->priv->window, "destroy", G_CALLBACK(mud_window_close), window);
	g_signal_connect(glade_xml_get_widget(glade, "menu_quit"), "activate", G_CALLBACK(mud_window_close), window);
	//FIXME g_signal_connect(glade_xml_get_widget(glade, "toolbar_quit"), "clicked", G_CALLBACK(mud_window_close), window);

	/* connect connect buttons */
	g_signal_connect(glade_xml_get_widget(glade, "menu_connect"), "activate", G_CALLBACK(mud_window_connect_dialog), window);
	//FIXME g_signal_connect(glade_xml_get_widget(glade, "toolbar_connect"), "clicked", G_CALLBACK(mud_window_connect_dialog), window);

	/* connect disconnect buttons */
	g_signal_connect(glade_xml_get_widget(glade, "menu_disconnect"), "activate", G_CALLBACK(mud_window_disconnect_cb), window);
	//FIXME g_signal_connect(glade_xml_get_widget(glade, "toolbar_disconnect"), "clicked", G_CALLBACK(mud_window_disconnect_cb), window);

	/* connect reconnect buttons */
	g_signal_connect(glade_xml_get_widget(glade, "menu_reconnect"), "activate", G_CALLBACK(mud_window_reconnect_cb), window);
	//FIXME g_signal_connect(glade_xml_get_widget(glade, "toolbar_reconnect"), "clicked", G_CALLBACK(mud_window_reconnect_cb), window);

	/* connect close window button */
	g_signal_connect(glade_xml_get_widget(glade, "menu_closewindow"), "activate", G_CALLBACK(mud_window_closewindow_cb), window);

	/* preferences window button */
	g_signal_connect(glade_xml_get_widget(glade, "menu_preferences"), "activate", G_CALLBACK(mud_window_preferences_cb), window);
	
	/* other objects */
	window->priv->notebook = glade_xml_get_widget(glade, "notebook");
	g_signal_connect(window->priv->notebook, "switch-page", G_CALLBACK(mud_window_notebook_page_change), window);

	window->priv->textentry = glade_xml_get_widget(glade, "text_entry");
	g_signal_connect(window->priv->textentry, "key_press_event", G_CALLBACK(mud_window_textentry_keypress), window);
	g_signal_connect(window->priv->textentry, "activate", G_CALLBACK(mud_window_textentry_activate), window);

	window->priv->current_view = NULL;
	window->priv->nr_of_tabs = 0;
	window->priv->blank_label = glade_xml_get_widget(glade, "startup_label");
	g_object_ref(window->priv->blank_label);
	
	/* set nice default info to display */
	g_snprintf(buf, 1023, _("GNOME-Mud version %s (compiled %s, %s)\n"
							"Distributed under the terms of the GNU General Public License.\n"),
							VERSION, __TIME__, __DATE__);
	gtk_label_set_text(GTK_LABEL(window->priv->blank_label), buf);
	
	g_object_unref(glade);
}

static void
mud_window_class_init (MudWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = mud_window_finalize;
}

static void
mud_window_finalize (GObject *object)
{
	MudWindow    *window;
	GObjectClass *parent_class;

	window = MUD_WINDOW(object);

	g_warning("I need to unreference all existing MudConnectionView's over here...");
	g_free(window->priv);

	parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
	parent_class->finalize(object);

	gtk_main_quit();
}

MudWindow*
mud_window_new (GConfClient *client)
{
	MudWindow *window;
	
	window = g_object_new(MUD_TYPE_WINDOW, NULL);
	window->priv->gconf_client = client;

	window->priv->prefs = mud_preferences_new(client);

	return window;
}
