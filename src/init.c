/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1998-2005 Robin Ericsson <lobbin@localhost.nu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"
#include <gconf/gconf-client.h>
#include <gdk/gdkkeysyms.h>
#include <gnome.h>
#include <stdio.h>
#include <vte/vte.h>

#ifdef USE_PYTHON
#include <Python.h>
#endif

#include "gnome-mud.h"
#include "directions.h"

static char const rcsid[] = 
	"$Id$";

extern gchar        *host;
extern gchar        *port;
extern SYSTEM_DATA   prefs;

/* Global Variables */
CONNECTION_DATA *main_connection;
CONNECTION_DATA *connections[MAX_CONNECTIONS];
GtkWidget       *main_notebook;
GtkWidget       *text_entry;

#ifdef USE_PYGTK
GtkWidget *box_user;
#endif

GtkWidget       *window;
GList           *EntryHistory = NULL;
GList           *EntryCurr    = NULL;
gboolean         Keyflag      = TRUE;
  
extern GList *AutoMapList;
extern GList *ProfilesList;
extern GList *ProfilesData;
extern GList *Profiles;

/* Local prototypes */
static void window_menu_file_close (GtkWidget *widget, gpointer data) ;

void close_window (GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy (GTK_WIDGET (data));
}

static gboolean dialog_close_cb(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	return TRUE;
}

gboolean destroy (GtkWidget *widget)
{
	GtkWidget *dialog;
	gint i, retval;
	gboolean connected = FALSE;

	for (i = (MAX_CONNECTIONS - 1) ; i > -1; i--)
	{
		if (connections[i] && connections[i]->connected)
		{
			connected = TRUE;
			break;
		}
	}
	
	if (connected)
	{
		dialog = gtk_message_dialog_new(GTK_WINDOW(window),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_YES_NO,
					_("Do you really want to quit?"));

		g_signal_connect(G_OBJECT(dialog), "delete-event", G_CALLBACK(dialog_close_cb), NULL);

		retval = gtk_dialog_run(GTK_DIALOG(dialog));

		if (retval == GTK_RESPONSE_NO || retval == GTK_RESPONSE_DELETE_EVENT)
		{
			gtk_widget_destroy(dialog);
			return TRUE;
		}
	}

	/* Close all windows except main */
	for (i = MAX_CONNECTIONS ; i != 0 ; i--)
	{
		if (connections[i])
		{
			gtk_notebook_set_page(GTK_NOTEBOOK(main_notebook), connections[i]->notebook) ;
			window_menu_file_close (NULL, NULL);
		}
	}

/*  if (EntryHistory != NULL)
  {
    GList *t;
    gint   i;
    gchar const *tt[g_list_length(EntryHistory)];
    
    for (t = g_list_first(EntryHistory), i = 0; t != NULL; t = t->next)
	{
      gchar *tmp = (gchar *) t->data;

      if (tmp[0] != '\0')
	  {
		tt[i] = (gchar *) t->data;
		i++;
      }
    }
  */  
	gtk_main_quit ();
	
	return FALSE;
}

static void window_menu_file_connect (GtkWidget *widget, gpointer data)
{
	GtkWidget *dialog, *table, *tmp, *entry_host, *entry_port;
	gint retval;

	dialog = gtk_dialog_new_with_buttons(_("Connect..."),
  				GTK_WINDOW(window),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
  				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OK, GTK_RESPONSE_OK,
				NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	table = gtk_table_new(2, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), 5);

	tmp = gtk_label_new(_("Host:"));
	gtk_table_attach(GTK_TABLE(table), tmp, 0, 1, 0, 1,
					(GtkAttachOptions) (GTK_FILL),
					(GtkAttachOptions) (0), 5, 0);
	gtk_misc_set_alignment(GTK_MISC(tmp), 0, 0.5);

	tmp = gtk_label_new(_("Port:"));
	gtk_widget_show(tmp);
	gtk_table_attach(GTK_TABLE(table), tmp, 0, 1, 1, 2,
					(GtkAttachOptions) (GTK_FILL),
					(GtkAttachOptions) (0), 5, 0);
	gtk_misc_set_alignment(GTK_MISC(tmp), 0, 0.5);

	entry_host = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry_host), host);
	gtk_entry_set_activates_default(GTK_ENTRY(entry_host), TRUE);
	gtk_table_attach(GTK_TABLE(table), entry_host, 1, 2, 0, 1,
					(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
					(GtkAttachOptions) (0), 0, 0);

	entry_port = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry_port), port);
	gtk_entry_set_activates_default(GTK_ENTRY(entry_port), TRUE);
	gtk_table_attach(GTK_TABLE(table), entry_port, 1, 2, 1, 2,
					(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
					(GtkAttachOptions) (0), 0, 0);

	gtk_widget_show_all(dialog);
	retval = gtk_dialog_run(GTK_DIALOG(dialog));

	switch (retval)
	{
		case GTK_RESPONSE_OK:
			if (strlen(host) > 0) g_free(host);
			if (strlen(port) > 0) g_free(port);

			host = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry_host)));
			port = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry_port)));

			make_connection(host, port, "Default");

		case GTK_RESPONSE_CANCEL:
			gtk_widget_destroy(dialog);
	}
}

static void window_menu_file_reconnect (GtkWidget *widget, gpointer data)
{
	CONNECTION_DATA *cd;
	gint i;
        GtkWidget		*label;
        GtkWidget		*image = NULL;
        GtkWidget		*box_label;


	i = gtk_notebook_get_current_page (GTK_NOTEBOOK(main_notebook));
	cd = connections[i];

	if (!cd)
	{
		textfield_add(main_connection, _("*** Internal error: no such connection.\n"), MESSAGE_ERR);
		return;
	}

	if (cd->connected)
	{
		disconnect(NULL, cd);
	}

	open_connection(cd);

        if (cd->connected == TRUE) {
                image = gtk_image_new_from_file(GMPIXMAPSDIR "/connection-online.png");
                label = gtk_label_new(cd->host);

                box_label = gtk_hbox_new (FALSE, 0);
                gtk_box_pack_start (GTK_BOX (box_label), image, TRUE, TRUE, 0);
                gtk_box_pack_start (GTK_BOX (box_label), label, FALSE, FALSE, 0);
                gtk_widget_show_all(box_label);

                gtk_notebook_set_tab_label(GTK_NOTEBOOK(main_notebook), gtk_notebook_get_nth_page(GTK_NOTEBOOK(main_notebook), i), box_label);
        }
} /* window_menu_file_reconnect */

static void window_menu_help_about (GtkWidget *widget, gpointer data)
{
  static GtkWidget *about = NULL;
  GtkWidget *hbox;
  GdkPixbuf *logo;

  if (!GTK_IS_WINDOW (about))
  {
  
	const char *authors[] = {
		"Robin Ericsson <lobbin@localhost.nu>",
		"Jordi Mallach <jordi@sindominio.net>",
		"Petter E. Stokke <gibreel@project23.no>",
		"Rachael Munns <vashti@dream.org.uk>",
		"Adam Luchjenbroers <adam@luchjenbroers.com>",
		"Nuno Sousa <nunofgs@punkass.com>",
		"",
		_("Artwork by:"),
		"Daniel Taylor <dantaylor@web.de>",
		NULL};
	const char *documenters[] = {
		"Jordi Mallach <jordi@sindominio.net>",
		"Petter E. Stokke <gibreel@project23.no>",
		NULL};

	  /* Translators: translate as your names & emails
	   * Paul Translator <paul@translator.org>         */
	const char *translator_credits = _("translator_credits");

	logo = gdk_pixbuf_new_from_file (PIXMAPSDIR "/gnome-mud.png", NULL);
  
	about = gnome_about_new (_("GNOME-Mud"), VERSION,
				 "\xc2\xa9 1998-2004 Robin Ericsson",
				 _("A Multi-User Dungeon (MUD) client for GNOME.\n"),
				 authors,
				 documenters,
				 strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
				 logo);

	if (logo != NULL)
		g_object_unref (logo);
  
	hbox = gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox),
			    gnome_href_new ("http://amcl.sourceforge.net/",
			    _("GNOME-Mud home page")),
			    FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX (GTK_DIALOG (about)->vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show_all (hbox);

	g_signal_connect(G_OBJECT(about), "destroy",
			 G_CALLBACK(gtk_widget_destroyed), &about);

	gtk_widget_show (about);
  }
  else
  {
	gtk_window_present (GTK_WINDOW (about));
  }
}

static void text_entry_send_command (CONNECTION_DATA *cn, gchar *cmd, GtkEntry *txt)
{
       gchar buf[256] ;

#ifndef WITHOUT_MAPPER
	GList* puck;
	for (puck = AutoMapList; puck != NULL; puck = puck->next)
		user_command(puck->data, cmd);
#endif
	
	g_snprintf(buf, 255, "%s\r\n", cmd) ;
       connection_send(cn, buf) ;
       gtk_signal_emit_stop_by_name (GTK_OBJECT(txt), "key_press_event");
}

static void text_entry_activate (GtkWidget *text_entry, gpointer data)
{
	CONNECTION_DATA *cd;
	const gchar *entry_text = gtk_entry_get_text(GTK_ENTRY(text_entry));

	Keyflag = TRUE;
	cd = connections[gtk_notebook_get_current_page(GTK_NOTEBOOK (main_notebook))];

	EntryCurr = g_list_last (EntryHistory);

	if (entry_text[0] == '\0') 
	{
		connection_send(cd, "\n");
		EntryCurr = NULL;
		return;
	}
  
	if (!EntryCurr || strcmp (EntryCurr->data, entry_text))
	{
		EntryHistory = g_list_append (EntryHistory, g_strdup (entry_text));

    	if ( g_list_length (EntryHistory) > prefs.History )
		{
      		EntryHistory = g_list_remove (EntryHistory, EntryHistory->data);
		}
		EntryCurr = NULL;
	}

	action_send_to_connection(g_strdup (entry_text), cd);

	if ( prefs.KeepText )
	{
    	gtk_entry_select_region (GTK_ENTRY (text_entry), 0, GTK_ENTRY (text_entry)->text_length);
	}
	else
	{
    	gtk_entry_set_text (GTK_ENTRY (text_entry), "");
  	}
}

static int text_entry_key_press_cb (GtkEntry *text_entry, GdkEventKey *event, gpointer data)
{
	KEYBIND_DATA	*scroll;
	CONNECTION_DATA *cd;
	gint   number;
	GList *li = NULL;
 
	number = gtk_notebook_get_current_page (GTK_NOTEBOOK (main_notebook));
	cd = connections[number];

	for (scroll = cd->profile->kd; scroll != NULL; scroll = scroll->next)
	{
		if ((scroll->state) == ((event->state) & 12) && (scroll->keyv) == (gdk_keyval_to_upper(event->keyval)))
		{
			text_entry_send_command(cd, scroll->data, text_entry);
			return TRUE;
		}		
	}

	if ( event->state & GDK_CONTROL_MASK ) { }
	else
	{
		if (!prefs.DisableKeys && cd->profile->directions)
		{
			switch ( event->keyval )
			{
				case GDK_KP_1:
					text_entry_send_command(cd, (gchar*) g_slist_nth_data(cd->profile->directions, DIRECTION_SOUTHWEST), text_entry);
					return TRUE;
					break;

				case GDK_KP_2:
					text_entry_send_command(cd, (gchar*) g_slist_nth_data(cd->profile->directions, DIRECTION_SOUTH), text_entry);
					return TRUE;
					break;

				case GDK_KP_3:
					text_entry_send_command(cd, (gchar*) g_slist_nth_data(cd->profile->directions, DIRECTION_SOUTHEAST), text_entry);
					return TRUE;
					break;

				case GDK_KP_4:
					text_entry_send_command(cd, (gchar*) g_slist_nth_data(cd->profile->directions, DIRECTION_WEST), text_entry);
					return TRUE;
					break;

				case GDK_KP_5:
					text_entry_send_command(cd, (gchar*) g_slist_nth_data(cd->profile->directions, DIRECTION_LOOK), text_entry);
					return TRUE;
					break;

				case GDK_KP_6:
					text_entry_send_command(cd, (gchar*) g_slist_nth_data(cd->profile->directions, DIRECTION_EAST), text_entry);
					return TRUE;
					break;

				case GDK_KP_7:
					text_entry_send_command(cd, (gchar*) g_slist_nth_data(cd->profile->directions, DIRECTION_NORTHWEST), text_entry);
					return TRUE;
					break;

				case GDK_KP_8:
					text_entry_send_command(cd, (gchar*) g_slist_nth_data(cd->profile->directions, DIRECTION_NORTH), text_entry);
					return TRUE;
					break;

				case GDK_KP_9:
					text_entry_send_command(cd, (gchar*) g_slist_nth_data(cd->profile->directions, DIRECTION_NORTHEAST), text_entry);
					return TRUE;
					break;

				case GDK_KP_Add:
					text_entry_send_command(cd, (gchar*) g_slist_nth_data(cd->profile->directions, DIRECTION_DOWN), text_entry);
					return TRUE;
					break;

				case GDK_KP_Subtract:
					text_entry_send_command(cd, (gchar*) g_slist_nth_data(cd->profile->directions, DIRECTION_UP), text_entry);
					return TRUE;
					break;
			}
		}

		switch ( event->keyval )
		{
			case GDK_Page_Up:
				{
					GtkAdjustment *a;
				
					a = vte_terminal_get_adjustment(VTE_TERMINAL(cd->window));
					gtk_adjustment_set_value(a, a->value - (a->page_increment/2));
				}
				break;
				
			case GDK_Page_Down:
				{
					GtkAdjustment *a;

					a = vte_terminal_get_adjustment(VTE_TERMINAL(cd->window));
					gtk_adjustment_set_value(a, a->value + (a->page_increment/2));
				}
				break;
					
			case GDK_Up:
				if (EntryCurr)
				{
					if (EntryCurr != g_list_first(EntryHistory))
					{
						li = EntryCurr->prev;
						EntryCurr = li;
					}
				}
				else
				{
					EntryCurr = li = g_list_last(EntryHistory);
				}
				
				if (li != NULL)
				{	
					gtk_entry_set_text (GTK_ENTRY(text_entry), (gchar *) li->data);
					gtk_entry_set_position (GTK_ENTRY(text_entry), GTK_ENTRY (text_entry)->text_length);
					gtk_entry_select_region (GTK_ENTRY(text_entry), 0, GTK_ENTRY (text_entry)->text_length);
					Keyflag = FALSE;
				}
				
				gtk_signal_emit_stop_by_name (GTK_OBJECT(text_entry), "key_press_event");
				return TRUE;
				break;

			case GDK_Down:
				if (EntryCurr)
				{
					li = EntryCurr->next;
					EntryCurr = li;

					if (li != NULL)
					{
						gtk_entry_set_text (GTK_ENTRY (text_entry), (gchar *) li->data);
						gtk_entry_set_position (GTK_ENTRY (text_entry), GTK_ENTRY (text_entry)->text_length);
						gtk_entry_select_region (GTK_ENTRY (text_entry), 0, GTK_ENTRY (text_entry)->text_length);
					}
				}
					
				if (!EntryCurr)
				{
					gtk_entry_set_text(GTK_ENTRY(text_entry), "");
				}
		
				gtk_signal_emit_stop_by_name (GTK_OBJECT(text_entry), "key_press_event");
				return TRUE;
				break;
		}
	}

	return FALSE;
}

CONNECTION_DATA *create_connection_data(gint notebook)
{
	CONNECTION_DATA *c;
	struct timeval tv;

	c = g_malloc0(sizeof(CONNECTION_DATA));
#ifdef ENABLE_MCCP
	c->mccp = mudcompress_new();
#endif /* ENABLE_MCCP */
	c->telnet_state = 0;
	c->telnet_subneg = 0;
	c->notebook = notebook;
	c->profile = profiledata_find("Default");
	c->window = vte_terminal_new();

	gettimeofday(&tv, NULL);
	c->last_log_flush = tv.tv_sec;

	vte_terminal_set_colors(VTE_TERMINAL(c->window), &prefs.Foreground, &prefs.Background, prefs.Colors, C_MAX);
	vte_terminal_set_scrollback_lines(VTE_TERMINAL(c->window), prefs.Scrollback);
	vte_terminal_set_scroll_on_output(VTE_TERMINAL(c->window), prefs.ScrollOnOutput);
	
	GTK_WIDGET_UNSET_FLAGS(GTK_WIDGET(c->window), GTK_CAN_FOCUS);

	gtk_signal_connect(GTK_OBJECT(c->window), "focus-in-event", GTK_SIGNAL_FUNC(grab_focus_cb), NULL);
	connections[notebook] = c;

	c->vscrollbar = gtk_vscrollbar_new(NULL);
	gtk_range_set_adjustment(GTK_RANGE(c->vscrollbar), VTE_TERMINAL(c->window)->adjustment);

	return c;
}

void free_connection_data (CONNECTION_DATA *c)
{
#ifdef ENABLE_MCCP
	mudcompress_delete(c->mccp);
	gtk_timeout_remove(c->mccp_timer);
#endif
	g_free (c->host);
	g_free (c->port);
	g_free (c);
}

static void window_menu_file_close (GtkWidget *widget, gpointer data)
{
	CONNECTION_DATA *cd;
	gint number, i, tabs;

	number = gtk_notebook_get_current_page (GTK_NOTEBOOK (main_notebook));
        tabs = gtk_notebook_get_n_pages(GTK_NOTEBOOK(main_notebook));

	cd = connections[number];

	if (cd->logging)
	{
		stop_logging_connection (cd);
	}

	if (cd->connected)
	{
		disconnect (NULL, cd);
	}

        if (tabs > 1) {
		gtk_notebook_remove_page (GTK_NOTEBOOK (main_notebook), number);
                tabs--;
                if (tabs == 1)
                        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(main_notebook), FALSE);


		free_connection_data (cd);

		for (i = number; i < (MAX_CONNECTIONS - 1);)
		{
			int old = i++;
			connections[old] = connections[i];
		}

                if (cd == main_connection)
                       main_connection = connections[0];
	}
}

static void window_menu_file_disconnect (GtkWidget *widget, gpointer data)
{
  CONNECTION_DATA *cd;
  gint number;

  number = gtk_notebook_get_current_page (GTK_NOTEBOOK (main_notebook));

  cd = connections[number];

  if (cd->connected) {
    disconnect (NULL, cd);
  }
}

static void window_menu_help_manual_activate_cb(GtkMenuItem *menuitem)
{
       GError *err;

       err = NULL;

       gnome_help_display("gnome-mud-manual", NULL, &err);
       
       if (err != NULL) {
	       GtkWidget *dialog;

	       dialog = gtk_message_dialog_new (GTK_WINDOW (window),
					        GTK_DIALOG_DESTROY_WITH_PARENT,
					        GTK_MESSAGE_ERROR,
					        GTK_BUTTONS_CLOSE,
					        _("There was an error displaying help: %s"),
					        err->message);

	       g_signal_connect (G_OBJECT (dialog), "response",
			         G_CALLBACK (gtk_widget_destroy),
			         NULL);

	       gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

	       gtk_widget_show (dialog);

	       g_error_free (err);
       }
}

static void window_menu_plugin_api_manual_activate_cb(GtkMenuItem *menuitem)
{
       GError *err;

       err = NULL;

       gnome_help_display("gnome-mud-plugin-api", NULL, &err);

       if (err != NULL) {
	       GtkWidget *dialog;

	       dialog = gtk_message_dialog_new (GTK_WINDOW (window),
					        GTK_DIALOG_DESTROY_WITH_PARENT,
					        GTK_MESSAGE_ERROR,
					        GTK_BUTTONS_CLOSE,
					        _("There was an error displaying help: %s"),
					        err->message);

	       g_signal_connect (G_OBJECT (dialog), "response",
			         G_CALLBACK (gtk_widget_destroy),
			         NULL);

	       gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

	       gtk_widget_show (dialog);

	       g_error_free (err);
       }
}

static GnomeUIInfo toolbar_menu[] = {
  GNOMEUIINFO_ITEM_STOCK(N_("Wizard..."), N_("Open the Connection Wizard"), window_profiles,   GNOME_STOCK_PIXMAP_NEW),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK(N_("Profiles..."), N_("Manage user profiles"), window_profile_edit, GNOME_STOCK_PIXMAP_PREFERENCES),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK(N_("Connect..."), N_("Connect to a mud"), window_menu_file_connect, GNOME_STOCK_PIXMAP_OPEN),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK(N_("Disconnect"), N_("Disconnect from the mud"), window_menu_file_disconnect,  GNOME_STOCK_PIXMAP_CLOSE),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK(N_("Reconnect"), N_("Reconnect to the mud"), window_menu_file_reconnect, GNOME_STOCK_PIXMAP_REFRESH),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK(N_("Exit"), N_("Quit GNOME-Mud"), destroy, GNOME_STOCK_PIXMAP_EXIT),
  GNOMEUIINFO_END
};

static GnomeUIInfo file_menu[] = {
  GNOMEUIINFO_MENU_NEW_ITEM(N_("Connection _Wizard..."), NULL, window_profiles, NULL),
  GNOMEUIINFO_MENU_NEW_ITEM(N_("_MudList Listing..."), NULL, window_mudlist, NULL),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK(N_("C_onnect..."), NULL, window_menu_file_connect, GNOME_STOCK_MENU_OPEN),
  GNOMEUIINFO_ITEM_STOCK(N_("_Disconnect"), NULL, window_menu_file_disconnect, GNOME_STOCK_MENU_CLOSE),
  GNOMEUIINFO_ITEM_STOCK(N_("_Reconnect"), NULL, window_menu_file_reconnect, GNOME_STOCK_MENU_REFRESH),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK(N_("S_tart Logging..."), NULL, window_menu_file_start_logging_cb, GNOME_STOCK_MENU_NEW),
  GNOMEUIINFO_ITEM_STOCK(N_("Sto_p Logging"), NULL, window_menu_file_stop_logging_cb, GNOME_STOCK_MENU_CLOSE),
  GNOMEUIINFO_ITEM_STOCK(N_("_Save Buffer..."), NULL, window_menu_file_save_buffer_cb, GNOME_STOCK_MENU_SAVE),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK(N_("_Close Window"), NULL, window_menu_file_close, GNOME_STOCK_MENU_BLANK),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_MENU_EXIT_ITEM(destroy, NULL),
  GNOMEUIINFO_END
};

static GnomeUIInfo plugin_menu[] = {
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK(N_("_Plugin Information..."), NULL, do_plugin_information, GNOME_STOCK_MENU_PROP),
  GNOMEUIINFO_END
};

static GnomeUIInfo settings_menu[] = {
#ifndef WITHOUT_MAPPER
  GNOMEUIINFO_ITEM_STOCK(N_("Auto _Mapper..."), NULL, window_automap, GNOME_STOCK_MENU_BLANK),
#endif
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_SUBTREE(N_("P_lugins"), plugin_menu),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_MENU_PREFERENCES_ITEM(window_prefs, NULL),
  GNOMEUIINFO_END
};

static GnomeUIInfo help_menu[] = {
	GNOME_APP_UI_ITEM, N_("User Manual"),
	N_("Display the GNOME-Mud User Manual"),
	window_menu_help_manual_activate_cb, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GTK_STOCK_HELP,
	0, 0, NULL,
	GNOME_APP_UI_ITEM, N_("Plugin API Manual"),
	N_("Display the GNOME-Mud Plugin API Manual"),
	window_menu_plugin_api_manual_activate_cb, NULL, NULL,
	GNOME_APP_PIXMAP_STOCK, GTK_STOCK_HELP,
	0, 0, NULL,
	GNOMEUIINFO_MENU_ABOUT_ITEM(window_menu_help_about, NULL),
	GNOMEUIINFO_END
};

GnomeUIInfo main_menu[] = {
  GNOMEUIINFO_MENU_FILE_TREE(file_menu),
  GNOMEUIINFO_MENU_SETTINGS_TREE(settings_menu),
  GNOMEUIINFO_MENU_HELP_TREE(help_menu),
  GNOMEUIINFO_END
};


void main_window ()
{
	GtkWidget *label;
	GtkWidget *box_main;
	GtkWidget *box_main2;
	GtkWidget *box_h_low;
        GtkWidget *box_label;
        GtkWidget *image = NULL;
	char  buf[1024];
  
	window = gnome_app_new("gnome-mud", "GNOME Mud");
	gtk_window_set_default_size(GTK_WINDOW(window), 500, 300);
	gtk_widget_realize(window);
	gtk_signal_connect(GTK_OBJECT(window), "delete-event", GTK_SIGNAL_FUNC(destroy), NULL);
  
	box_main = gtk_vbox_new (FALSE, 0);
	gnome_app_set_contents(GNOME_APP(window), box_main);
	gnome_app_create_menus(GNOME_APP(window), main_menu);
	gnome_app_create_toolbar(GNOME_APP(window), toolbar_menu);

#ifdef USE_PYGTK
	box_user = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box_main), box_user, FALSE, FALSE, 0);
#endif
  
	box_main2 = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box_main), box_main2, TRUE, TRUE, 1);

	main_notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(main_notebook), tab_location_by_gtk(prefs.TabLocation));
	GTK_WIDGET_UNSET_FLAGS(GTK_WIDGET(main_notebook), GTK_CAN_FOCUS);
	gtk_signal_connect (GTK_OBJECT (main_notebook), "switch-page", GTK_SIGNAL_FUNC (switch_page_cb), NULL);
	gtk_box_pack_start (GTK_BOX (box_main2), main_notebook, TRUE, TRUE, 1);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(main_notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(main_notebook), FALSE);

	box_h_low = gtk_hbox_new (FALSE, 0);

	image = gtk_image_new_from_file(GMPIXMAPSDIR "/connection-offline.png");
	/* Do we really need this now? */
	label = gtk_label_new (_("Main"));

	box_label = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box_label), image, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (box_label), label, FALSE, FALSE, 0);
	gtk_widget_show_all(box_label);

	gtk_notebook_append_page(GTK_NOTEBOOK(main_notebook), box_h_low, box_label);
 
	main_connection = create_connection_data(0);

	gtk_box_pack_start(GTK_BOX(box_h_low), main_connection->window, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box_h_low), main_connection->vscrollbar, FALSE, FALSE, 0);

	text_entry = gtk_entry_new();
	/* Need to fix the text entry cursor/text colour 
	gtk_widget_modify_base(text_entry, GTK_STATE_NORMAL, &prefs.Background);
	gtk_widget_modify_text(text_entry, GTK_STATE_NORMAL, &prefs.Foreground);
	*/ 

	if (EntryHistory != NULL)
	{
		// FIXME
		//gtk_combo_set_popdown_strings(GTK_COMBO(combo), EntryHistory);
	}
	
	gtk_box_pack_start(GTK_BOX(box_main), text_entry, FALSE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT(text_entry), "key_press_event", GTK_SIGNAL_FUNC (text_entry_key_press_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(text_entry), "activate", GTK_SIGNAL_FUNC (text_entry_activate), NULL); 
	gtk_widget_grab_focus (text_entry);
  
	gtk_widget_show_all (window);
	tray_create();

	vte_terminal_set_font_from_string(VTE_TERMINAL(main_connection->window), prefs.FontName);
 
	g_snprintf(buf, 1023, _("GNOME-Mud version %s (compiled %s, %s)\n"), VERSION, __TIME__, __DATE__);
	terminal_feed(main_connection->window, buf);
	terminal_feed(main_connection->window, _("Distributed under the terms of the GNU General Public License.\n"));
}

