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

#include "mud-window.h"
#include "mud-connection-view.h"

static char const rcsid[] = "$Id: ";

struct _MudWindowPrivate
{
	GtkWidget *window;
	GtkWidget *notebook;

	GtkWidget *blank_label;

	gchar *host;
	gchar *port;
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
	GtkWidget *child;
	
	g_assert(window != NULL);
	g_assert(view != NULL);
	
	gtk_notebook_append_page(GTK_NOTEBOOK(window->priv->notebook), mud_connection_view_get_viewport(view), NULL);
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
		
		view = mud_connection_view_new_with_params (host, iport);
		mud_window_add_connection_view(window, view);
		
		gtk_widget_destroy(dialog);
	}

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
	g_signal_connect(glade_xml_get_widget(glade, "toolbar_quit"), "clicked", G_CALLBACK(mud_window_close), window);

	/* connect connect buttons */
	g_signal_connect(glade_xml_get_widget(glade, "menu_connect"), "activate", G_CALLBACK(mud_window_connect_dialog), window);
	g_signal_connect(glade_xml_get_widget(glade, "toolbar_connect"), "clicked", G_CALLBACK(mud_window_connect_dialog), window);

	/* other objects */
	window->priv->notebook = glade_xml_get_widget(glade, "notebook");
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

	g_free(window->priv);

	parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
	parent_class->finalize(object);

	gtk_main_quit();
}

MudWindow*
mud_window_new ()
{
	MudWindow *window;

	window = g_object_new(MUD_TYPE_WINDOW, NULL);

	return window;
}

