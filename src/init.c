/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1998-2002 Robin Ericsson <lobbin@localhost.nu>
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
#include <gdk/gdkkeysyms.h>
#include <gnome.h>
#include <libintl.h>
#include <stdio.h>

#ifdef USE_PYTHON
#include <Python.h>
#endif

#include "gnome-mud.h"

static char const rcsid[] = 
	"$Id$";

/* Local functions */
static void	about_window   (GtkWidget *, gpointer);
static void	connect_window (GtkWidget *, gpointer);
static void	do_close       (GtkWidget *, gpointer);
static void do_disconnect  (GtkWidget *, gpointer);

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
GdkFont         *font_normal;
GdkFont         *font_fixed;
GList           *EntryHistory = NULL;
GList           *EntryCurr    = NULL;
bool             Keyflag      = TRUE;
gboolean         KeyPress     = FALSE;
  
extern GList *ProfilesList;
extern GList *ProfilesData;
extern GList *Profiles;

void close_window (GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy (GTK_WIDGET (data));
}

void destroy (GtkWidget *widget)
{
  GtkWidget *dialog;
  GtkWidget *label;
  gint retval;

  dialog = gnome_dialog_new(N_("Quit?"), 
			    GNOME_STOCK_BUTTON_YES,
			    GNOME_STOCK_BUTTON_NO,
			    NULL);
  gnome_dialog_set_parent(GNOME_DIALOG(dialog), GTK_WINDOW(window));

  label = gtk_label_new(_("Do you really want to quit?"));
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox), label, FALSE, FALSE, 0);  

  retval = gnome_dialog_run(GNOME_DIALOG(dialog));

  switch (retval)
  {
  	case 1:
    	gnome_dialog_close(GNOME_DIALOG(dialog));
    	return;
  }  

  if (EntryHistory != NULL)
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
    
    gnome_config_set_vector("/gnome-mud/Data/CommandHistory", i, tt);
  }
 
  if (ProfilesList != NULL)
  {
	  GList *t;
	  gint   i;
	  gchar const *prof[g_list_length(ProfilesList)];

	  for (t = g_list_first(ProfilesList), i = 0; t != NULL; t = t->next, i++)
	  {
		  prof[i] = (gchar *) t->data;
	  }

	  gnome_config_set_vector("/gnome-mud/Data/Profiles", i, prof);
  }

  if (ProfilesData != NULL)
  {
	  GList *t;

	  for (t = g_list_first(ProfilesData); t != NULL; t = t->next)
	  {
		  PROFILE_DATA *pd = (PROFILE_DATA *) t->data;
		  
		  profiledata_save(pd->name, pd->alias, "Alias");
		  profiledata_save(pd->name, pd->variables, "Variables");
		  profiledata_save(pd->name, pd->triggers, "Triggers");
		  profiledata_savekeys(pd->name, pd->kd);
	  }
  }

  if (Profiles != NULL)
  {
	  GList *t;
	  gint 	 i;
	  gchar name[50];

	  for (t = g_list_first(Profiles), i = 0; t != NULL; t = t->next, i++)
	  {
		  g_snprintf(name, 50, "/gnome-mud/Connection%d/Name", i);
		  gnome_config_set_string(name, ((WIZARD_DATA2 *) t->data)->name);
		  g_snprintf(name, 50, "/gnome-mud/Connection%d/Host", i);
		  gnome_config_set_string(name, ((WIZARD_DATA2 *) t->data)->hostname);
		  g_snprintf(name, 50, "/gnome-mud/Connection%d/Port", i);
		  gnome_config_set_string(name, ((WIZARD_DATA2 *) t->data)->port);
		  g_snprintf(name, 50, "/gnome-mud/Connection%d/Char", i);
		  gnome_config_set_string(name, ((WIZARD_DATA2 *) t->data)->playername);
		  g_snprintf(name, 50, "/gnome-mud/Connection%d/Pass", i);
		  gnome_config_private_set_string(name, ((WIZARD_DATA2 *) t->data)->password);
		  g_snprintf(name, 50, "/gnome-mud/Connection%d/Profile", i);
		  gnome_config_set_string(name, ((WIZARD_DATA2 *) t->data)->profile);
	  }

	  while(1)
	  {
		  g_snprintf(name, 50, "/gnome-mud/Connection%d", i);
		  if (gnome_config_has_section(name))
		  {
			  gnome_config_clean_section(name);
		  }
		  else
		  {
			  break;
		  }

		  i++;
	  }
  }
  
  gtk_main_quit ();
}

static void connect_window (GtkWidget *widget, gpointer data)
{
  GtkWidget *dialog;

  GtkWidget *label2, *label3;
  GtkWidget *table1;
  GtkWidget *entry_host, *entry_port;

  gint retval;

  dialog = gnome_dialog_new(_("Connect..."), 
			    GNOME_STOCK_BUTTON_OK,
			    GNOME_STOCK_BUTTON_CANCEL,
			    NULL);
  gnome_dialog_set_parent(GNOME_DIALOG(dialog), GTK_WINDOW(window));

  table1 = gtk_table_new (2, 2, FALSE);
  gtk_widget_show (table1);
  gtk_box_pack_start (GTK_BOX (GNOME_DIALOG(dialog)->vbox), table1, TRUE, TRUE, 0);
  gtk_table_set_row_spacings (GTK_TABLE (table1), 5);

  label2 = gtk_label_new (_("Host:"));
  gtk_widget_show (label2);
  gtk_table_attach (GTK_TABLE (table1), label2, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 5, 0);
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);

  label3 = gtk_label_new (_("Port:"));
  gtk_widget_show (label3);
  gtk_table_attach (GTK_TABLE (table1), label3, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 5, 0);
  gtk_misc_set_alignment (GTK_MISC (label3), 0, 0.5);

  entry_host = gtk_entry_new ();
  gtk_entry_set_text(GTK_ENTRY(entry_host), host);
  gtk_widget_show (entry_host);
  gtk_table_attach (GTK_TABLE (table1), entry_host, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  

  entry_port = gtk_entry_new ();
  gtk_entry_set_text(GTK_ENTRY(entry_port), port);
  gtk_widget_show (entry_port);
  gtk_table_attach (GTK_TABLE (table1), entry_port, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  retval = gnome_dialog_run(GNOME_DIALOG(dialog));

  switch (retval)
  {
  	case 0:
    	if (strlen(host) > 0)
      		g_free(host); 

    	if (strlen(port) > 0)
     		g_free(port);

    	host = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry_host)));
    	port = g_strdup(gtk_entry_get_text((GtkEntry *) entry_port));
    
    	make_connection(host, port, "Default");

    	gnome_dialog_close(GNOME_DIALOG(dialog));
    	break;

  	case 1:
    	gnome_dialog_close(GNOME_DIALOG(dialog));
    	break;
  }
}

static void about_window (GtkWidget *widget, gpointer data)
{
  static GtkWidget *about;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *hrefbox;

  const gchar *authors[] = {
	_("Robin Ericsson, main developer"),
	_("David Zanetti, bugfixes"),
	_("Ben Gertzfield, creation of rcfiles for AMCL"),
	_("Will Schenk, bugfixes"),
	_("desaster, bugfixes"),
	_("Paul Cameron, compile fixes, automapper"),
	_("Robert Brady, color selection"),
	_("Bret Robideaux, action/trigger module"),
	_("Maxim Kiselev, keybindings, colorsaving, command-divider, bugfixes"),
	_("Benjamin Curtis, recognition of TELNET codes and replies"),
	/* if your charset doesn't support o with dieresis (ö) just use o */
	_("Jörgen Kosche, focus-bugs patches, de.po maintainer"),
	_("Jeroen Ruigrok, various code cleanups and fixes"),
	/* if your charset doesn't support i with acute (í) just use i */
	_("Jorge García, various code cleanups and fixes"),
	_("Jordi Mallach, official Debian package, i18n support, "
		  "ca.po & es.po maintainer"),
	_("Martin Quinson, fr.po maintainer"),
	/* if your charset doesn't support e with circumflex (è), just use e */
	_("Staffan Thomèn and the creators of gEdit - module API"),
	_("Petter E. Stokke, Python scripting support"),
	NULL};

 
  if (about != NULL) {    
    gdk_window_raise (about->window);
    gdk_window_show (about->window);
    return;
  }
  
  about = gnome_about_new (_("GNOME-Mud"), VERSION,
			   "(C) 1998-2002 Robin Ericsson",
			   (const char **)authors,
			   _("A Multi-User Dungeon (MUD) client using GTK+/GNOME libraries."),
/*			   _("Send bug reports to: amcl-devel@lists.sourceforge.net"), */
			   NULL);
  gtk_signal_connect (GTK_OBJECT (about), "destroy", GTK_SIGNAL_FUNC
		      (gtk_widget_destroyed), &about);
  gnome_dialog_set_parent (GNOME_DIALOG (about), GTK_WINDOW (window));
  
  vbox = GNOME_DIALOG(about)->vbox;

  hbox = gtk_hbox_new(TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show(hbox);

  hrefbox = gnome_href_new("http://amcl.sourceforge.net/", _("GNOME-Mud homepage"));
  gtk_box_pack_start(GTK_BOX(hbox), hrefbox, FALSE, FALSE, 0);
  GTK_WIDGET_UNSET_FLAGS(hrefbox, GTK_CAN_FOCUS);
  gtk_widget_show(hrefbox);

  gtk_widget_show (about);
}

static void text_entry_select_child_cb(GtkList *list, GtkWidget *widget, gpointer data)
{
  EntryCurr = g_list_nth(EntryHistory, gtk_list_child_position(list, widget));
}

static void text_entry_send_command (CONNECTION_DATA *cn, gchar *cmd, GtkEntry *txt)
{
       gchar buf[256] ;

       g_snprintf(buf, 255, "%s\n", cmd) ;
       connection_send(cn, buf) ;
       gtk_signal_emit_stop_by_name (GTK_OBJECT(txt), "key_press_event");
}

static int text_entry_key_press_cb (GtkEntry *text_entry, GdkEventKey *event, gpointer data)
{
	KEYBIND_DATA	*scroll;
	CONNECTION_DATA *cd;
	gint   number;
	GList *li;
  
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
		if (EntryCurr)
		{
			if (!prefs.DisableKeys)
			{
				switch ( event->keyval )
				{
					case GDK_KP_1:
						text_entry_send_command(cd, "sw", text_entry);
						return TRUE;
						break;

					case GDK_KP_2:
						text_entry_send_command(cd, "s", text_entry);
						return TRUE;
						break;

					case GDK_KP_3:
						text_entry_send_command(cd, "se", text_entry);
						return TRUE;
						break;

					case GDK_KP_4:
						text_entry_send_command(cd, "w", text_entry);
						return TRUE;
						break;

					case GDK_KP_5:
						text_entry_send_command(cd, "look", text_entry);
						return TRUE;
						break;

					case GDK_KP_6:
						text_entry_send_command(cd, "e", text_entry);
						return TRUE;
						break;

					case GDK_KP_7:
						text_entry_send_command(cd, "nw", text_entry);
						return TRUE;
						break;

					case GDK_KP_8:
						text_entry_send_command(cd, "n", text_entry);
						return TRUE;
						break;

					case GDK_KP_9:
						text_entry_send_command(cd, "ne", text_entry);
						return TRUE;
						break;

					case GDK_KP_Add:
						text_entry_send_command(cd, "d", text_entry);
						return TRUE;
						break;

					case GDK_KP_Subtract:
						text_entry_send_command(cd, "u", text_entry);
						return TRUE;
						break;
				}
			}
		
			switch ( event->keyval )
			{
				case GDK_Page_Up:
					{
						GtkAdjustment *a;

						a = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(cd->vscrollbar));
					
						gtk_adjustment_set_value(a, a->value - (a->page_increment/2));
					}
					
					break;
				
				case GDK_Page_Down:
					{
						GtkAdjustment *a;

						a = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(cd->vscrollbar));

						gtk_adjustment_set_value(a, a->value + (a->page_increment/2));
					}
					break;
					
				case GDK_Up:
					if (EntryCurr->prev)
					{
						li = EntryCurr->prev;
						EntryCurr = li;
						
						gtk_entry_set_text (GTK_ENTRY(text_entry), (gchar *) li->data);
						gtk_entry_set_position (GTK_ENTRY(text_entry), GTK_ENTRY (text_entry)->text_length);
						gtk_entry_select_region (GTK_ENTRY(text_entry), 0, GTK_ENTRY (text_entry)->text_length);
						Keyflag = FALSE;
					}
	
					gtk_signal_emit_stop_by_name (GTK_OBJECT(text_entry), "key_press_event");
					return TRUE;
					break;
	
				case GDK_Down:
					if (EntryCurr->next)
					{
						li = EntryCurr->next;
						EntryCurr = li;
						gtk_entry_set_text (GTK_ENTRY (text_entry), (gchar *) li->data);
						gtk_entry_set_position (GTK_ENTRY (text_entry), GTK_ENTRY (text_entry)->text_length);
						gtk_entry_select_region (GTK_ENTRY (text_entry), 0, GTK_ENTRY (text_entry)->text_length);
					}
					else
					{
						EntryCurr = g_list_last (EntryHistory);
						gtk_entry_set_text (GTK_ENTRY (text_entry), "");
					}            
		
					gtk_signal_emit_stop_by_name (GTK_OBJECT(text_entry), "key_press_event");
					return TRUE;
					break;
			}
		}
	}

	return FALSE;
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

static void do_close (GtkWidget *widget, gpointer data)
{
	CONNECTION_DATA *cd;
	gint number, i;

	number = gtk_notebook_get_current_page (GTK_NOTEBOOK (main_notebook));

	cd = connections[number];

	if (cd != main_connection)
	{
		if (cd->connected)
		{
			disconnect (NULL, cd);
		}
    
		gtk_notebook_remove_page (GTK_NOTEBOOK (main_notebook), number);

		free_connection_data (cd);

		for (i = number; i < (MAX_CONNECTIONS - 1);)
		{
			int old = i++;
			connections[old] = connections[i];
		}
	}
}

static void do_disconnect (GtkWidget *widget, gpointer data)
{
  CONNECTION_DATA *cd;
  gint number;

  number = gtk_notebook_get_current_page (GTK_NOTEBOOK (main_notebook));

  cd = connections[number];

  if (cd->connected) {
    disconnect (NULL, cd);
  }
}

static void save_log_file_ok_cb(GtkWidget *widget, GtkFileSelection *file_selector)
{
	FILE *fp;
	CONNECTION_DATA *cd;
	gchar *textdata;
	
	gint   number	= gtk_notebook_get_current_page(GTK_NOTEBOOK(main_notebook));
	gchar *filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_selector));

	cd = connections[number];

	if ((fp = fopen(filename, "w")) == NULL)
	{
		popup_window(_("Could not open file for writing..."));
		return;
	}

	textdata = gtk_editable_get_chars(GTK_EDITABLE(cd->window), 0, -1);
	fputs(textdata, fp);	
	g_free(textdata);
	
	fclose(fp);
}

static void save_log_cb (GtkWidget *widget, gpointer data)
{
	gint  size = strlen(g_get_home_dir()) + 10;
	gchar *homedir;
	
	GtkWidget *file_selector = gtk_file_selection_new(_("Please select a log file..."));

	homedir = g_malloc0(sizeof(gchar *) * size);
	g_snprintf(homedir, size, "%s/", g_get_home_dir());
	
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selector), homedir);
	g_free(homedir);

	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(file_selector)->ok_button), "clicked", GTK_SIGNAL_FUNC(save_log_file_ok_cb), file_selector);
	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(file_selector)->ok_button), "clicked",
							  GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer) file_selector);
	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(file_selector)->cancel_button), "clicked",
							  GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer) file_selector);
	gtk_widget_show(file_selector);
}

static void window_menu_help_manual_activate_cb(GtkMenuItem *menuitem)
{
	gnome_help_goto(NULL, "ghelp:gnome-mud");
}

static GnomeUIInfo toolbar_menu[] = {
  GNOMEUIINFO_ITEM_STOCK(N_("Wizard..."), NULL, window_profiles,   GNOME_STOCK_PIXMAP_NEW),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK(N_("Profiles..."), NULL, window_profile_edit, GNOME_STOCK_PIXMAP_PREFERENCES),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK(N_("Connect..."), NULL, connect_window, GNOME_STOCK_PIXMAP_OPEN),
  GNOMEUIINFO_ITEM_STOCK(N_("Disconnect"), NULL, do_disconnect,  GNOME_STOCK_PIXMAP_CLOSE),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK(N_("Exit"), NULL, destroy, GNOME_STOCK_PIXMAP_EXIT),
  GNOMEUIINFO_END
};

static GnomeUIInfo file_menu[] = {
  GNOMEUIINFO_MENU_NEW_ITEM(N_("Connection Wizard..."), NULL, window_profiles, NULL),
  GNOMEUIINFO_MENU_NEW_ITEM(N_("MudList Listing..."), NULL, window_mudlist, NULL),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK(N_("Connect..."), NULL, connect_window, GNOME_STOCK_MENU_OPEN),
  GNOMEUIINFO_ITEM_STOCK(N_("Disconnect"), NULL, do_disconnect, GNOME_STOCK_MENU_CLOSE),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK(N_("Save log..."), NULL, save_log_cb, GNOME_STOCK_MENU_SAVE),
  GNOMEUIINFO_ITEM_STOCK(N_("Close Window"), NULL, do_close, GNOME_STOCK_MENU_BLANK),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_MENU_EXIT_ITEM(destroy, NULL),
  GNOMEUIINFO_END
};

static GnomeUIInfo plugin_menu[] = {
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK(N_("Plugin Information..."), NULL, do_plugin_information, GNOME_STOCK_MENU_PROP),
  GNOMEUIINFO_END
};

static GnomeUIInfo settings_menu[] = {
  GNOMEUIINFO_ITEM_STOCK(N_("Auto Mapper..."), NULL, window_automap, GNOME_STOCK_MENU_BLANK),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_SUBTREE(N_("Plugins"), plugin_menu),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_MENU_PREFERENCES_ITEM(window_prefs, NULL),
  GNOMEUIINFO_END
};

static GnomeUIInfo help_menu[] = {
	{ GNOME_APP_UI_ITEM, N_("GNOME-Mud manual"), N_("Open the GNOME-Mud manual"), 
		window_menu_help_manual_activate_cb, NULL, NULL,
		GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BOOK_BLUE, 0, 0, NULL 
	},
	GNOMEUIINFO_MENU_ABOUT_ITEM(about_window, NULL),
	GNOMEUIINFO_END
};

GnomeUIInfo main_menu[] = {
  GNOMEUIINFO_MENU_FILE_TREE(file_menu),
  GNOMEUIINFO_MENU_SETTINGS_TREE(settings_menu),
  GNOMEUIINFO_MENU_HELP_TREE(help_menu),
  GNOMEUIINFO_END
};


void init_window ()
{
  GtkWidget *combo;
  GtkWidget *label;
  GtkWidget *box_main;
  GtkWidget *box_main2;
  GtkWidget *box_h_low;
  char  buf[1024];
  
  font_fixed = gdk_font_load("fixed");

  window = gnome_app_new("gnome-mud", "GNOME Mud");
  gtk_widget_realize(window);
  gtk_signal_connect(GTK_OBJECT(window), "delete_event", GTK_SIGNAL_FUNC(destroy), NULL);
  
  box_main = gtk_vbox_new (FALSE, 0);
  gnome_app_set_contents(GNOME_APP(window), box_main);
  gnome_app_create_menus(GNOME_APP(window), main_menu);
  gnome_app_create_toolbar(GNOME_APP(window), toolbar_menu);
  
#ifdef USE_PYGTK
  box_user = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box_main), box_user, FALSE, FALSE, 0);
#endif
  
  box_main2 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box_main), box_main2, TRUE, TRUE, 5);

  main_notebook = gtk_notebook_new();
  GTK_WIDGET_UNSET_FLAGS(GTK_WIDGET(main_notebook), GTK_CAN_FOCUS);
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK (main_notebook), GTK_POS_BOTTOM);
  gtk_signal_connect (GTK_OBJECT (main_notebook), "switch-page",
		      GTK_SIGNAL_FUNC (switch_page_cb), NULL);
  gtk_box_pack_start (GTK_BOX (box_main2), main_notebook, TRUE, TRUE, 5);
  
  box_h_low = gtk_hbox_new (FALSE, 0);
  label = gtk_label_new (_("Main"));
  gtk_notebook_append_page(GTK_NOTEBOOK(main_notebook), box_h_low, label);
  
  main_connection = g_malloc0( sizeof (CONNECTION_DATA));
  main_connection->mccp = mudcompress_new();
  main_connection->notebook = 0;
  main_connection->profile = profiledata_find("Default");
  main_connection->window = gtk_text_new (NULL, NULL);
  main_connection->foreground = &prefs.Foreground;
  main_connection->background = &prefs.Background;
  GTK_WIDGET_UNSET_FLAGS(GTK_WIDGET(main_connection->window), GTK_CAN_FOCUS);
  gtk_widget_set_usize (main_connection->window, 500, 300);
  gtk_signal_connect (GTK_OBJECT (main_connection->window), "focus-in-event", GTK_SIGNAL_FUNC (grab_focus_cb), NULL);
  connections[0] = main_connection;
 
  main_connection->vscrollbar = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(main_connection->vscrollbar), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(box_h_low), main_connection->vscrollbar, TRUE, TRUE, 0);
  gtk_container_add(GTK_CONTAINER(main_connection->vscrollbar), main_connection->window);
  
  combo = gtk_combo_new();
  text_entry = GTK_COMBO(combo)->entry;
  gtk_combo_set_use_arrows(GTK_COMBO(combo), FALSE);
  gtk_combo_disable_activate(GTK_COMBO(combo));
  if (EntryHistory != NULL)
  {
	gtk_combo_set_popdown_strings(GTK_COMBO(combo), EntryHistory);
  }
  gtk_box_pack_start(GTK_BOX(box_main), combo, FALSE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT(text_entry), "key_press_event", GTK_SIGNAL_FUNC (text_entry_key_press_cb), NULL);
  gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->list), "select-child", GTK_SIGNAL_FUNC (text_entry_select_child_cb), NULL);
  gtk_signal_connect(GTK_OBJECT(text_entry), "activate", GTK_SIGNAL_FUNC (send_to_connection), (gpointer) combo); 
  gtk_widget_grab_focus (text_entry);
  
  gtk_widget_show_all (window);
  
  gtk_widget_realize (main_connection->window);
  gdk_window_set_background (GTK_TEXT (main_connection->window)->text_area, &prefs.Background);
 
  g_snprintf(buf, 1023, _("GNOME-Mud version %s (compiled %s, %s)\n"), VERSION, __TIME__, __DATE__);
  gtk_text_insert (GTK_TEXT (main_connection->window), font_normal, &prefs.Colors[7], NULL, buf, -1);
  gtk_text_insert (GTK_TEXT (main_connection->window), font_normal, &prefs.Colors[7], NULL, 
		   _("Distributed under the terms of the GNU General Public License.\n"), -1);
#ifdef USE_PYTHON
  g_snprintf(buf, 1023, _("\nPython version %s\n\n"), Py_GetVersion());
  gtk_text_insert (GTK_TEXT (main_connection->window), font_normal, &prefs.Colors[7], NULL, buf, -1);
#endif
}


