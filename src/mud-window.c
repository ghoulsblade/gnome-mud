#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glade/glade.h>
#include <gtk/gtkaboutdialog.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkentry.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkpaned.h>
#include <gtk/gtktextview.h>
#include <gtk/gtktextbuffer.h>
#include <gtk/gtktextiter.h>
#include <gdk/gdkkeysyms.h>
#include <libgnome/gnome-i18n.h>
#include <stdlib.h>

#include "gnome-mud.h"
#include "mud-connection-view.h"
#include "mud-preferences-window.h"
#include "mud-window.h"
#include "mud-window-mudlist.h"
#include "mud-window-mconnect.h"
#include "modules.h"

static char const rcsid[] = "$Id: ";

struct _MudWindowPrivate
{
	GConfClient *gconf_client;

	GSList *mud_views_list;
	
	GtkWidget *window;
	GtkWidget *notebook;
	GtkWidget *textentry;
	GtkWidget *textview;
	GtkWidget *textviewscroll;
	GtkWidget *mainvpane;

	GtkWidget *blank_label;
	GtkWidget *current_view;

	gchar *host;
	gchar *port;

	gint nr_of_tabs;
	gint toggleState;
};

typedef struct MudViewEntry
{
	gint id;
	MudConnectionView *view;
} MudViewEntry;

GtkWidget *pluginMenu;

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
	MudViewEntry *entry;
	
	entry = g_new(MudViewEntry, 1);
	
	g_assert(window != NULL);
	g_assert(view != NULL);

	if (window->priv->nr_of_tabs++ == 0)
	{
		gtk_notebook_remove_page(GTK_NOTEBOOK(window->priv->notebook), 0);
	}
	
	nr = gtk_notebook_append_page(GTK_NOTEBOOK(window->priv->notebook), mud_connection_view_get_viewport(view), NULL);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(window->priv->notebook), nr);

	mud_connection_view_set_id(view, nr);
	mud_connection_view_set_parent(view, window);

	entry->id = nr;
	entry->view = view;
	
	window->priv->mud_views_list = g_slist_append(window->priv->mud_views_list, entry);
	
	if (window->priv->nr_of_tabs > 1)
	{
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(window->priv->notebook), TRUE);
	}
}

static void
mud_window_remove_connection_view(MudWindow *window, gint nr)
{
	GSList *entry, *rementry;
	
	rementry = NULL;
	rementry = g_slist_append(rementry, NULL);
	
	g_object_unref(window->priv->current_view);
	gtk_notebook_remove_page(GTK_NOTEBOOK(window->priv->notebook), nr);
	
	for(entry = window->priv->mud_views_list; entry != NULL; entry = g_slist_next(entry))
	{
		if(((MudViewEntry *)entry->data)->id == nr)
		{
			rementry->data = entry->data;
		}
	}

	window->priv->mud_views_list = g_slist_remove(window->priv->mud_views_list, rementry->data);

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

static gboolean
mud_window_textview_keypress(GtkWidget *widget, GdkEventKey *event, MudWindow *window)
{
	gchar *text;
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(window->priv->textview));
	GtkTextIter start, end;
	
	if(event->keyval == GDK_KP_Enter)
	{
		if(window->priv->current_view)
		{
			gtk_text_buffer_get_bounds(buffer, &start, &end);
	
			text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
			mud_connection_view_send(MUD_CONNECTION_VIEW(window->priv->current_view), text);
			
			gtk_text_buffer_select_range(buffer, &start, &end);

			return TRUE;
			
		}
	}

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
	mud_preferences_window_new("Default");
}

static void
mud_window_list_cb(GtkWidget *widget, MudWindow *window)
{
	mud_window_mudlist_new();
}

static void
mud_window_about_cb(GtkWidget *widget, MudWindow *window)
{
	GtkWidget *dialog;
	GladeXML *glade;
	
	glade = glade_xml_new(GLADEDIR "/main.glade", "about_window", "about_window");
	dialog = glade_xml_get_widget(glade, "about_window");

	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), "0.10.9");
	gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(dialog), "GNOME-Mud Homepage");
	gtk_dialog_run(GTK_DIALOG(dialog));
	
	g_object_unref(glade);	
}

static void
mud_window_mconnect_dialog(GtkWidget *widget, MudWindow *window)
{
	GtkWidget *mywig;
	
	mywig = window->priv->window;
	
	mud_window_mconnect_new(window, mywig);
}

static void
mud_window_inputtoggle_cb(GtkWidget *widget, MudWindow *window)
{
	gint w, h;
	
	if(window->priv->toggleState)
	{
		gtk_widget_hide(window->priv->textview);
		gtk_widget_hide(window->priv->textviewscroll);
		gtk_widget_show(window->priv->textentry);

		gtk_window_get_size(GTK_WINDOW(window->priv->window), &w, &h);
	
		gtk_paned_set_position(GTK_PANED(window->priv->mainvpane),h - 62);

		window->priv->toggleState = 0;	
	}
	else
	{
		gtk_widget_hide(window->priv->textentry);
		gtk_widget_show(window->priv->textview);
		gtk_widget_show(window->priv->textviewscroll);	

		gtk_window_get_size(GTK_WINDOW(window->priv->window), &w, &h);
	
		gtk_paned_set_position(GTK_PANED(window->priv->mainvpane),h - 124);
		
		window->priv->toggleState = 1;
	}
}

gboolean
mud_window_size_request(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data)
{	
	gint w, h;
	MudWindow *window = (MudWindow *)user_data;
	
	gtk_window_get_size(GTK_WINDOW(window->priv->window), &w, &h);
	
	if(!window->priv->toggleState)
		gtk_paned_set_position(GTK_PANED(window->priv->mainvpane),h - 62);

	return FALSE;
}

static void
mud_window_connect_dialog(GtkWidget *widget, MudWindow *window)
{
	GladeXML *glade;
	GtkWidget *dialog;
	GtkWidget *entry_host;
	GtkWidget *entry_port;
	gint result;

	glade = glade_xml_new(GLADEDIR "/connect.glade", "connect_window", "connect_window");
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
		
		view = mud_connection_view_new("Default", host, iport, window->priv->window);
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
	gint w, h;
	
	window->priv = g_new0(MudWindowPrivate, 1);

	/* set members */
	window->priv->host = g_strdup("");
	window->priv->port = g_strdup("");

	window->priv->mud_views_list = NULL;
	
	/* start glading */
	glade = glade_xml_new(GLADEDIR "/main.glade", "main_window", "main_window");
	window->priv->window = glade_xml_get_widget(glade, "main_window");
	gtk_widget_show_all(window->priv->window);

	/* connect quit buttons */
	g_signal_connect(window->priv->window, "destroy", G_CALLBACK(mud_window_close), window);
	g_signal_connect(glade_xml_get_widget(glade, "menu_quit"), "activate", G_CALLBACK(mud_window_close), window);
	//FIXME g_signal_connect(glade_xml_get_widget(glade, "toolbar_quit"), "clicked", G_CALLBACK(mud_window_close), window);

	/* connect connect buttons */
	g_signal_connect(glade_xml_get_widget(glade, "main_connect"), "activate", G_CALLBACK(mud_window_mconnect_dialog), window);
	g_signal_connect(glade_xml_get_widget(glade, "menu_connect"), "activate", G_CALLBACK(mud_window_connect_dialog), window);
	g_signal_connect(glade_xml_get_widget(glade, "menu_mudlist"), "activate",
G_CALLBACK(mud_window_list_cb), window);
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
	
	g_signal_connect(glade_xml_get_widget(glade, "menu_about"), "activate", G_CALLBACK(mud_window_about_cb), window);
	
	/* other objects */
	window->priv->notebook = glade_xml_get_widget(glade, "notebook");
	g_signal_connect(window->priv->notebook, "switch-page", G_CALLBACK(mud_window_notebook_page_change), window);

	window->priv->textviewscroll = glade_xml_get_widget(glade, "text_view_scroll");
	window->priv->textview = glade_xml_get_widget(glade, "text_view");

	g_signal_connect(window->priv->textview, "key_press_event", G_CALLBACK(mud_window_textview_keypress), window);
	
	gtk_widget_hide(window->priv->textviewscroll);
	gtk_widget_hide(window->priv->textview);
	
	window->priv->toggleState = 0;

	window->priv->textentry = glade_xml_get_widget(glade, "text_entry");
	g_signal_connect(window->priv->textentry, "key_press_event", G_CALLBACK(mud_window_textentry_keypress), window);
	g_signal_connect(window->priv->textentry, "activate", G_CALLBACK(mud_window_textentry_activate), window);

	window->priv->mainvpane = glade_xml_get_widget(glade, "main_vpane");
	gtk_window_get_size(GTK_WINDOW(window->priv->window), &w, &h);
	
	gtk_paned_set_position(GTK_PANED(window->priv->mainvpane),h - 62);
	
	g_signal_connect(glade_xml_get_widget(glade, "toggle_input"), "clicked", G_CALLBACK(mud_window_inputtoggle_cb), window);

	g_signal_connect(glade_xml_get_widget(glade, "plugin_list"), "activate", G_CALLBACK(do_plugin_information), NULL);
	
	pluginMenu = glade_xml_get_widget(glade, "plugin_menu_menu");

	window->priv->current_view = NULL;
	window->priv->nr_of_tabs = 0;
	window->priv->blank_label = glade_xml_get_widget(glade, "startup_label");
	g_object_ref(window->priv->blank_label);
	
	/* set nice default info to display */
	g_snprintf(buf, 1023, _("GNOME-Mud version %s (compiled %s, %s)\n"
							"Distributed under the terms of the GNU General Public License.\n"),
							VERSION, __TIME__, __DATE__);
	gtk_label_set_text(GTK_LABEL(window->priv->blank_label), buf);
	
	g_signal_connect(window->priv->window, "configure-event", G_CALLBACK(mud_window_size_request), window);

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

	GSList *entry;
	MudWindow    *window;
	GObjectClass *parent_class;

	window = MUD_WINDOW(object);

	for(entry = window->priv->mud_views_list; entry != NULL; entry = g_slist_next(entry))
	{
		g_object_unref(((MudViewEntry *)entry->data)->view);	
	}
	
	g_free(window->priv);

	parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
	parent_class->finalize(object);

	gtk_main_quit();
}

void
mud_window_handle_plugins(MudWindow *window, gint id, gchar *data, gint dir)
{
	GSList *entry, *viewlist;
	MudViewEntry *mudview;
	GList *plugin_list;
	PLUGIN_DATA *pd;

	viewlist = window->priv->mud_views_list;
	
	for(entry = viewlist; entry != NULL; entry = g_slist_next(entry))
	{
		mudview = (MudViewEntry *)entry->data;
		
		if(mudview->id == id)
		{	
			if(dir)
			{
				for(plugin_list = g_list_first(Plugin_data_list); plugin_list != NULL; plugin_list = plugin_list->next)
				{
					if(plugin_list->data != NULL)
					{
						pd = (PLUGIN_DATA *)plugin_list->data;
			
						if(pd->plugin && pd->plugin->enabeled && (pd->dir == PLUGIN_DATA_IN))
						{
							(*pd->datafunc)(pd->plugin, (gchar *)data, mudview->view);
						}
					}
				}
			}
			else
			{

				for(plugin_list = g_list_first(Plugin_data_list); plugin_list != NULL; plugin_list = plugin_list->next)
				{
					if(plugin_list->data != NULL)
					{
						pd = (PLUGIN_DATA *)plugin_list->data;
			
						if(pd->plugin && pd->plugin->enabeled && (pd->dir == PLUGIN_DATA_OUT))
						{
							(*pd->datafunc)(pd->plugin, (gchar *)data, mudview->view);
						}
					}
				}
			}
		}
	}
}

MudWindow*
mud_window_new (GConfClient *client)
{
	MudWindow *window;
	
	window = g_object_new(MUD_TYPE_WINDOW, NULL);
	window->priv->gconf_client = client;

	return window;
}

