#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glade/glade-xml.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkimage.h>
#include <glib/gi18n.h>
#include <gtk/gtkaboutdialog.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkimage.h>
#include <gdk/gdkpixbuf.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkpaned.h>
#include <gtk/gtktextview.h>
#include <gtk/gtktextbuffer.h>
#include <gtk/gtktextiter.h>
#include <gtk/gtkimagemenuitem.h>
#include <vte/vte.h>
#include <string.h>

#include "gnome-mud.h"
#include "mud-connection-view.h"
#include "mud-preferences-window.h"
#include "mud-window.h"
#include "mud-window-mudlist.h"
#include "mud-tray.h"
#include "mud-window-mconnect.h"
#include "modules.h"
#include "mud-profile.h"
#include "mud-window-profile.h"

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

	GtkWidget *startlog;
	GtkWidget *stoplog;
	GtkWidget *bufferdump;

	GtkWidget *blank_label;
	GtkWidget *current_view;

	GtkWidget *mi_profiles;
	
	GtkWidget *image;

	GSList *profileMenuList;

	gchar *host;
	gchar *port;

	gint nr_of_tabs;
	gint toggleState;
	
	MudTray *tray;
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
mud_window_add_connection_view(MudWindow *window, MudConnectionView *view, gchar *tabLbl)
{
	gint nr;
	MudViewEntry *entry;

	entry = g_new(MudViewEntry, 1);
	
	g_assert(window != NULL);
	g_assert(view != NULL);

	if (window->priv->nr_of_tabs++ == 0)
	{
		gtk_notebook_remove_page(GTK_NOTEBOOK(window->priv->notebook), 0);
		window->priv->image = NULL;
	}
	
	nr = gtk_notebook_append_page(GTK_NOTEBOOK(window->priv->notebook), mud_connection_view_get_viewport(view), gtk_label_new(tabLbl));
	gtk_notebook_set_current_page(GTK_NOTEBOOK(window->priv->notebook), nr);

	gtk_widget_set_sensitive(window->priv->startlog, TRUE);
	gtk_widget_set_sensitive(window->priv->bufferdump, TRUE);

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
		window->priv->image = gtk_image_new();
		gtk_widget_show(window->priv->image);
		gtk_notebook_append_page(GTK_NOTEBOOK(window->priv->notebook), window->priv->image, NULL);
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
		
		if(window->priv->nr_of_tabs == 0)
			mud_tray_update_icon(window->priv->tray, offline_connecting);
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
	gchar *name;

	window->priv->current_view = g_object_get_data(G_OBJECT(gtk_notebook_get_nth_page(notebook, arg)), "connection-view");

	if (window->priv->nr_of_tabs != 0)
	{
		name = mud_profile_get_name(mud_connection_view_get_current_profile(MUD_CONNECTION_VIEW(window->priv->current_view)));
	
		mud_window_profile_menu_set_active(name, window);

		if(mud_connection_view_islogging(MUD_CONNECTION_VIEW(window->priv->current_view)))
		{
			gtk_widget_set_sensitive(window->priv->startlog, FALSE);
			gtk_widget_set_sensitive(window->priv->stoplog, TRUE);
		}
		else
		{
			gtk_widget_set_sensitive(window->priv->startlog, TRUE);
			gtk_widget_set_sensitive(window->priv->stoplog, FALSE);
		}
	}
	else
	{
		gtk_widget_set_sensitive(window->priv->startlog, FALSE);
		gtk_widget_set_sensitive(window->priv->stoplog, FALSE);
		gtk_widget_set_sensitive(window->priv->bufferdump, FALSE);
	}

}

static void
mud_window_textentry_activate(GtkWidget *widget, MudWindow *window)
{
	gchar *tmp;

	tmp = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
		if (g_str_equal(tmp, ""))
			tmp = g_strdup(" ");
	if (window->priv->current_view)
		mud_connection_view_send(MUD_CONNECTION_VIEW(window->priv->current_view), tmp);
		if (gconf_client_get_bool(window->priv->gconf_client,
		   "/apps/gnome-mud/functionality/keeptext", NULL) == FALSE)
			gtk_entry_set_text(GTK_ENTRY(widget), g_strdup(""));
		free (tmp);
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
mud_window_profiles_cb(GtkWidget *widget, MudWindow *window)
{
	mud_window_profile_new(window);
}

static void
mud_window_about_cb(GtkWidget *widget, MudWindow *window)
{
	GtkWidget *dialog;
	GladeXML *glade;
	
	glade = glade_xml_new(GLADEDIR "/main.glade", "about_window", NULL);
	dialog = glade_xml_get_widget(glade, "about_window");

	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), VERSION);
	gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(dialog), "GNOME-Mud Homepage");
	gtk_dialog_run(GTK_DIALOG(dialog));
	
	g_object_unref(glade);	
}

static void
mud_window_mconnect_dialog(GtkWidget *widget, MudWindow *window)
{
	GtkWidget *mywig;
	
	mywig = window->priv->window;
	
	mud_window_mconnect_new(window, mywig, window->priv->tray);
}

static void
mud_window_inputtoggle_cb(GtkWidget *widget, MudWindow *window)
{
	
	if(window->priv->toggleState)
	{
		gtk_widget_hide(window->priv->textview);
		gtk_widget_hide(window->priv->textviewscroll);
		gtk_widget_show(window->priv->textentry);
	}
	else
	{
		gtk_widget_hide(window->priv->textentry);
		gtk_widget_show(window->priv->textview);
		gtk_widget_show(window->priv->textviewscroll);	
	}
	window->priv->toggleState = !window->priv->toggleState;
}

gboolean
mud_window_size_request(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data)
{	
	gint w, h;
	GdkPixbuf *buf;
	GError *gerr = NULL;
	MudWindow *window = (MudWindow *)user_data;
	
	gtk_window_get_size(GTK_WINDOW(window->priv->window), &w, &h);
	
	if (window->priv->nr_of_tabs == 0)
	{
		buf = gdk_pixbuf_new_from_file_at_size(GMPIXMAPSDIR "/gnome-mud.svg", w >> 1, h >> 1, &gerr);
	
		gtk_image_set_from_pixbuf(GTK_IMAGE(window->priv->image), buf);
	}

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

	glade = glade_xml_new(GLADEDIR "/connect.glade", "connect_window", NULL);
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
		
		mud_tray_update_icon(window->priv->tray, offline);
		
		view = mud_connection_view_new("Default", host, iport, window->priv->window, (GtkWidget *)window->priv->tray, g_strdup(host));
		mud_window_add_connection_view(window, view, g_strdup(host));

		
	}

	gtk_widget_destroy(dialog);
	g_object_unref(glade);
}

gboolean 
save_dialog_vte_cb (VteTerminal *terminal,glong column,glong row,gpointer data)
{
	return TRUE;
}

void
mud_window_buffer_cb(GtkWidget *widget, MudWindow *window)
{
	GladeXML *glade;
	GtkWidget *dialog;
	gint result;
	
	glade = glade_xml_new(GLADEDIR "/main.glade", "save_dialog", NULL);
	dialog = glade_xml_get_widget(glade, "save_dialog");
	
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), "buffer.txt");
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	if(result == GTK_RESPONSE_OK)
	{
		gchar *filename;
		FILE *file;
		
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		file = fopen(filename, "w");
		
		if(!file)
		{
			GtkWidget *errDialog;
	
			errDialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Could not save the file in specified location!"));

			gtk_dialog_run(GTK_DIALOG(errDialog));
			gtk_widget_destroy(errDialog);
		}	
		else
		{
			gchar *bufferText;
			GtkWidget *term;
			
			term = mud_connection_view_get_terminal(MUD_CONNECTION_VIEW(window->priv->current_view));
			
			bufferText = vte_terminal_get_text_range(VTE_TERMINAL(term),0,0,
											vte_terminal_get_row_count(VTE_TERMINAL(term)),
											vte_terminal_get_column_count(VTE_TERMINAL(term)),
											save_dialog_vte_cb,
											NULL,
											NULL);

			fwrite(bufferText, 1, strlen(bufferText), file);
			fclose(file);	
		}
		
		g_free(filename);
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
mud_window_select_profile(GtkWidget *widget, MudWindow *window)
{
	MudProfile *profile;
	GtkWidget *profileLabel;
	
	profileLabel = gtk_bin_get_child(GTK_BIN(widget));

	if (window->priv->current_view)
	{
		profile = get_profile(gtk_label_get_text(GTK_LABEL(profileLabel)));

		if (profile)
			mud_connection_view_set_profile(MUD_CONNECTION_VIEW(window->priv->current_view), profile);
	}
}

void 
mud_window_profile_menu_set_cb(GtkWidget *widget, gpointer data)
{
	gchar *name = (gchar *)data;
	GtkWidget *label;
	
	label = gtk_bin_get_child(GTK_BIN(widget));

	
	if (GTK_IS_LABEL(label) && !strcmp(name,gtk_label_get_text(GTK_LABEL(label))))
	{
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), TRUE);
	}
}

void
mud_window_startlog_cb(GtkWidget *widget, MudWindow *window)
{
	mud_connection_view_start_logging(MUD_CONNECTION_VIEW(window->priv->current_view));
	gtk_widget_set_sensitive(window->priv->startlog, FALSE);
	gtk_widget_set_sensitive(window->priv->stoplog, TRUE);
	
}

void
mud_window_stoplog_cb(GtkWidget *widget, MudWindow *window)
{
	mud_connection_view_stop_logging(MUD_CONNECTION_VIEW(window->priv->current_view));
	gtk_widget_set_sensitive(window->priv->stoplog, FALSE);
	gtk_widget_set_sensitive(window->priv->startlog, TRUE);
}

void 
mud_window_profile_menu_set_active(gchar *name, MudWindow *window)
{
	gtk_container_foreach(GTK_CONTAINER(window->priv->mi_profiles),mud_window_profile_menu_set_cb,(gpointer)name);
}

void mud_window_clear_profiles_menu(GtkWidget *widget, gpointer data)
{	
	gtk_widget_destroy(widget);
}

void
mud_window_populate_profiles_menu(MudWindow *window)
{
	const GList *profiles;
	GList *entry;
	GtkWidget *profile;
	GtkWidget *sep;
	GtkWidget *manage;
	GtkWidget *icon;
	
	window->priv->profileMenuList = NULL;
	
	profiles = mud_profile_get_profiles();
	
	gtk_container_foreach(GTK_CONTAINER(window->priv->mi_profiles), mud_window_clear_profiles_menu, NULL);

	for (entry = (GList *)profiles; entry != NULL; entry = g_list_next(entry))
	{
		profile = gtk_radio_menu_item_new_with_label(window->priv->profileMenuList, (gchar *)MUD_PROFILE(entry->data)->name);
		gtk_widget_show(profile);
		window->priv->profileMenuList = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(profile));

		gtk_menu_shell_append(GTK_MENU_SHELL(window->priv->mi_profiles), profile);

		g_signal_connect(G_OBJECT(profile), "activate", G_CALLBACK(mud_window_select_profile), window);

	}

	sep = gtk_separator_menu_item_new();
	gtk_widget_show(sep);
	gtk_menu_shell_append(GTK_MENU_SHELL(window->priv->mi_profiles), sep);

	icon = gtk_image_new_from_stock("gtk-edit", GTK_ICON_SIZE_MENU);

	manage = gtk_image_menu_item_new_with_mnemonic(_("_Manage Profiles..."));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(manage), icon);
	gtk_widget_show(manage);
	gtk_menu_shell_append(GTK_MENU_SHELL(window->priv->mi_profiles), manage);
	g_signal_connect(manage, "activate", G_CALLBACK(mud_window_profiles_cb), window);
}

static void
mud_window_init (MudWindow *window)
{
	GladeXML *glade;

	window->priv = g_new0(MudWindowPrivate, 1);

	/* set members */
	window->priv->host = g_strdup("");
	window->priv->port = g_strdup("");

	window->priv->mud_views_list = NULL;
	
	/* start glading */
	glade = glade_xml_new(GLADEDIR "/main.glade", "main_window", NULL);
	window->priv->window = glade_xml_get_widget(glade, "main_window");
	gtk_widget_show_all(window->priv->window);

	/* connect quit buttons */
	g_signal_connect(window->priv->window, "destroy", G_CALLBACK(mud_window_close), window);
	g_signal_connect(glade_xml_get_widget(glade, "menu_quit"), "activate", G_CALLBACK(mud_window_close), window);

	/* connect connect buttons */
	g_signal_connect(glade_xml_get_widget(glade, "main_connect"), "activate", G_CALLBACK(mud_window_mconnect_dialog), window);
	g_signal_connect(glade_xml_get_widget(glade, "menu_connect"), "activate", G_CALLBACK(mud_window_connect_dialog), window);
	g_signal_connect(glade_xml_get_widget(glade, "menu_mudlist"), "activate",
G_CALLBACK(mud_window_list_cb), window);
	g_signal_connect(glade_xml_get_widget(glade, "toolbar_connect"), "clicked", G_CALLBACK(mud_window_connect_dialog), window);

	/* connect disconnect buttons */
	g_signal_connect(glade_xml_get_widget(glade, "menu_disconnect"), "activate", G_CALLBACK(mud_window_disconnect_cb), window);
	g_signal_connect(glade_xml_get_widget(glade, "toolbar_disconnect"), "clicked", G_CALLBACK(mud_window_disconnect_cb), window);

	/* connect reconnect buttons */
	g_signal_connect(glade_xml_get_widget(glade, "menu_reconnect"), "activate", G_CALLBACK(mud_window_reconnect_cb), window);
	g_signal_connect(glade_xml_get_widget(glade, "toolbar_reconnect"), "clicked", G_CALLBACK(mud_window_reconnect_cb), window);

	/* connect close window button */
	g_signal_connect(glade_xml_get_widget(glade, "menu_closewindow"), "activate", G_CALLBACK(mud_window_closewindow_cb), window);

	/* logging */
	window->priv->startlog = glade_xml_get_widget(glade, "menu_start_logging");
	g_signal_connect(window->priv->startlog, "activate", G_CALLBACK(mud_window_startlog_cb), window);

	window->priv->stoplog = glade_xml_get_widget(glade, "menu_stop_logging");
	g_signal_connect(window->priv->stoplog, "activate", G_CALLBACK(mud_window_stoplog_cb), window);

	window->priv->bufferdump = glade_xml_get_widget(glade, "menu_dump_buffer");
	g_signal_connect(window->priv->bufferdump, "activate", G_CALLBACK(mud_window_buffer_cb), window);

	/* preferences window button */

	window->priv->mi_profiles = glade_xml_get_widget(glade, "mi_profiles_menu");
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
	
	window->priv->image = glade_xml_get_widget(glade, "image");

	g_signal_connect(glade_xml_get_widget(glade, "toggle_input"), "clicked", G_CALLBACK(mud_window_inputtoggle_cb), window);

	g_signal_connect(glade_xml_get_widget(glade, "plugin_list"), "activate", G_CALLBACK(do_plugin_information), NULL);
	
	pluginMenu = glade_xml_get_widget(glade, "plugin_menu_menu");

	window->priv->current_view = NULL;
	window->priv->nr_of_tabs = 0;

	
	g_signal_connect(window->priv->window, "configure-event", G_CALLBACK(mud_window_size_request), window);

	window->priv->profileMenuList = NULL;
	mud_window_populate_profiles_menu(window);

	window->priv->tray = mud_tray_new(window, window->priv->window);
	
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

