/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1998-2001 Robin Ericsson <lobbin@localhost.nu>
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

#include "gnome-mud.h"

static char const rcsid[] = 
	"$Id$";

/* Local functions */
static void	about_window   (GtkWidget *, gpointer);
static void	connect_window (GtkWidget *, gpointer);
static void	do_close       (GtkWidget *, gpointer);
static void     do_disconnect  (GtkWidget *, gpointer);

extern gchar        *host;
extern gchar        *port;
extern SYSTEM_DATA   prefs;
extern GdkColor     *foreground;
extern GdkColor     *background;
extern GdkColor     color_lightgrey;
extern GdkColor     color_white;
extern GdkColor     color_black;

/* Global Variables */
CONNECTION_DATA *main_connection;
CONNECTION_DATA *connections[15];
GtkWidget       *main_notebook;
GtkWidget       *text_entry;

/* FIXME */
GtkWidget       *menu_option_colors;
/* END FIXME */

GtkWidget       *window;
GdkFont         *font_normal;
GdkFont         *font_fixed;
GtkCList        *lists[3];
GList           *EntryHistory = NULL;
GList           *EntryCurr    = NULL;
bool             Keyflag      = TRUE;
gboolean         KeyPress     = FALSE;

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

  switch (retval) {
  case 1:
    gnome_dialog_close(GNOME_DIALOG(dialog));
    return;
  }  

  if (EntryHistory != NULL) {
    GList *t;
    gint   i;
    gchar const *tt[g_list_length(EntryHistory)];
    
    for (t = g_list_first(EntryHistory), i = 0; t != NULL; t = t->next) {
      gchar *tmp = (gchar *) t->data;

      if (tmp[0] != '\0') {
	tt[i] = (gchar *) t->data;
	i++;
      }
    }
    
    gnome_config_set_vector("/gnome-mud/Data/CommandHistory", i, tt);
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

  switch (retval) {
  case 0:
    if (strlen(host) > 0)
      g_free(host); 

    if (strlen(port) > 0)
      g_free(port);

    host = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry_host)));
    port = g_strdup(gtk_entry_get_text((GtkEntry *) entry_port));
    
    make_connection(host, port);

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

  const gchar *authors[] = {"Robin Ericsson <lobbin@localhost.nu>", "and many others...", NULL};
  
  if (about != NULL) {    
    gdk_window_raise (about->window);
    gdk_window_show (about->window);
    return;
  }
  
  about = gnome_about_new (_("Gnome-Mud"), VERSION,
			   "(C) 1998-2000 Robin Ericsson",
			   (const char **)authors,
			   _("Send bug reports to: amcl-devel@lists.sourceforge.net"),
			   NULL);
  gtk_signal_connect (GTK_OBJECT (about), "destroy", GTK_SIGNAL_FUNC
		      (gtk_widget_destroyed), &about);
  gnome_dialog_set_parent (GNOME_DIALOG (about), GTK_WINDOW (window));
  
  vbox = GNOME_DIALOG(about)->vbox;

  hbox = gtk_hbox_new(TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show(hbox);

  hrefbox = gnome_href_new("http://amcl.sourceforge.net/", "Gnome-Mud homepage");
  gtk_box_pack_start(GTK_BOX(hbox), hrefbox, FALSE, FALSE, 0);
  GTK_WIDGET_UNSET_FLAGS(hrefbox, GTK_CAN_FOCUS);
  gtk_widget_show(hrefbox);

  gtk_widget_show (about);
}

static void text_entry_select_child_cb(GtkList *list, GtkWidget *widget, gpointer data)
{
  EntryCurr = g_list_nth(EntryHistory, gtk_list_child_position(list, widget));
}

static int text_entry_key_press_cb (GtkEntry *text_entry, GdkEventKey *event, gpointer data)
{
  CONNECTION_DATA *cd;
  gint   number;
  GList *li;
  
  number = gtk_notebook_get_current_page (GTK_NOTEBOOK (main_notebook));
  cd = connections[number];

  if ( event->state & GDK_CONTROL_MASK ) { }
  else   {
    if (EntryCurr)
      switch ( event->keyval ) {
      case GDK_Up:
	if (EntryCurr->prev) {
	  li = EntryCurr->prev;
	  EntryCurr = li;
	  gtk_entry_set_text (GTK_ENTRY (text_entry), (gchar *) li->data);
	  gtk_entry_set_position (GTK_ENTRY (text_entry) ,
				  GTK_ENTRY (text_entry)->text_length);
	  gtk_entry_select_region (GTK_ENTRY (text_entry), 0,
				   GTK_ENTRY (text_entry)->text_length);
	  Keyflag = FALSE;
	}
 	gtk_signal_emit_stop_by_name (GTK_OBJECT (text_entry),
 				      "key_press_event");
	return TRUE;
	break;
	
      case GDK_Down:
	if (EntryCurr->next) {
	  li = EntryCurr->next;
	  EntryCurr = li;
	  gtk_entry_set_text (GTK_ENTRY (text_entry), (gchar *) li->data);
	  gtk_entry_set_position (GTK_ENTRY (text_entry),
				  GTK_ENTRY (text_entry)->text_length);
	  gtk_entry_select_region (GTK_ENTRY (text_entry), 0,
				   GTK_ENTRY (text_entry)->text_length);
	} else {
	  EntryCurr = g_list_last (EntryHistory);
	  gtk_entry_set_text (GTK_ENTRY (text_entry), "");
	}            
 	gtk_signal_emit_stop_by_name (GTK_OBJECT (text_entry),  
 				      "key_press_event");
	return TRUE;
	break;
      }
  }

  return FALSE;
}

static void do_close (GtkWidget *widget, gpointer data)
{
  CONNECTION_DATA *cd;
  gint number;

  number = gtk_notebook_get_current_page (GTK_NOTEBOOK (main_notebook));

  cd = connections[number];

  if (cd != main_connection) {
    if (cd->connected)
      disconnect (NULL, cd);
    
    gtk_notebook_remove_page (GTK_NOTEBOOK (main_notebook), number);
    free_connection_data (cd);
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

static GnomeUIInfo toolbar_menu[] = {
  GNOMEUIINFO_ITEM_STOCK(N_("Wizard..."), NULL, window_wizard,   GNOME_STOCK_PIXMAP_NEW),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK(N_("Connect..."), NULL, connect_window, GNOME_STOCK_PIXMAP_OPEN),
  GNOMEUIINFO_ITEM_STOCK(N_("Disconnect"), NULL, do_disconnect,  GNOME_STOCK_PIXMAP_CLOSE),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK(N_("Exit"), NULL, destroy, GNOME_STOCK_PIXMAP_EXIT),
  GNOMEUIINFO_END
};

static GnomeUIInfo file_menu[] = {
  GNOMEUIINFO_MENU_NEW_ITEM(N_("Connection Wizard..."), NULL, window_wizard, NULL),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK(N_("Connect..."), NULL, connect_window, GNOME_STOCK_MENU_OPEN),
  GNOMEUIINFO_ITEM_STOCK(N_("Disconnect"), NULL, do_disconnect, GNOME_STOCK_MENU_CLOSE),
  GNOMEUIINFO_SEPARATOR,
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

/*
** FIXME, where is color menu?
*/
static GnomeUIInfo settings_menu[] = {
  GNOMEUIINFO_ITEM_DATA(N_("Alias..."), NULL, window_data, GINT_TO_POINTER(0), NULL),
  GNOMEUIINFO_ITEM_DATA(N_("Variables..."), NULL, window_data, GINT_TO_POINTER(2), NULL),
  GNOMEUIINFO_ITEM_DATA(N_("Actions/Triggers..."), NULL, window_data, GINT_TO_POINTER(1), NULL),
  GNOMEUIINFO_ITEM_STOCK(N_("Keybinds..."), NULL, window_keybind, GNOME_STOCK_MENU_BLANK),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_STOCK(N_("Auto Mapper"), NULL, window_automap, GNOME_STOCK_MENU_BLANK),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_SUBTREE(N_("Plugins"), plugin_menu),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_MENU_PREFERENCES_ITEM(window_prefs, NULL),
  GNOMEUIINFO_END
};

static GnomeUIInfo help_menu[] = {
  GNOMEUIINFO_ITEM_DATA(N_("README"), NULL, doc_dialog, GINT_TO_POINTER(1), NULL),
  GNOMEUIINFO_ITEM_DATA(N_("AUTHORS"), NULL, doc_dialog, GINT_TO_POINTER(2), NULL),
  GNOMEUIINFO_SEPARATOR,
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
  //accel_group = gtk_accel_group_new ();
  lists[0]   = (GtkCList *)gtk_clist_new(2);
  lists[1]   = (GtkCList *)gtk_clist_new(2);
  lists[2]   = (GtkCList *)gtk_clist_new(2);
  load_data (GTK_CLIST(lists[0]), "aliases");
  load_data (GTK_CLIST(lists[1]), "actions");
  load_data (GTK_CLIST(lists[2]), "vars");

  window = gnome_app_new("gnome-mud", "GNOME Mud");
  gtk_widget_realize(window);
  gtk_signal_connect(GTK_OBJECT(window), "delete_event", GTK_SIGNAL_FUNC(destroy), NULL);
  
  box_main = gtk_vbox_new (FALSE, 0);
  gnome_app_set_contents(GNOME_APP(window), box_main);
  gnome_app_create_menus(GNOME_APP(window), main_menu);
  gnome_app_create_toolbar(GNOME_APP(window), toolbar_menu);
  
  /* Accels menu - we have to redefine if they are not good enought */
/*   menu_main_accels    = gtk_menu_ensure_uline_accel_group(GTK_MENU (menu_main_menu)); */
/*   menu_options_accels = gtk_menu_ensure_uline_accel_group(GTK_MENU (menu_option_menu)); */
/*   menu_plugins_accels = gtk_menu_ensure_uline_accel_group(GTK_MENU (menu_plugin_menu)); */
  
/*   gtk_widget_add_accelerator (menu_main_wizard,     "activate", accel_group, */
/* 			      GDK_Z, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE); */
/*   gtk_widget_add_accelerator (menu_main_connect,    "activate", accel_group, */
/* 			      GDK_N, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE); */
/*   gtk_widget_add_accelerator (menu_main_disconnect, "activate", accel_group, */
/* 			      GDK_D, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE); */
/*   gtk_widget_add_accelerator (menu_main_close,      "activate", accel_group, */
/* 			      GDK_W, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE); */
/*   gtk_widget_add_accelerator (menu_main_quit,       "activate", accel_group, */
/* 			      GDK_Q, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE); */
/*   gtk_widget_add_accelerator (menu_option_alias,    "activate", accel_group, */
/* 			      GDK_A, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE); */
/*   gtk_widget_add_accelerator (menu_option_vars,    "activate", accel_group, */
/* 			      GDK_V, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE); */
/*   gtk_widget_add_accelerator (menu_option_prefs,    "activate", accel_group,  */
/* 			      GDK_P, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE); */
/*   gtk_widget_add_accelerator (menu_option_mapper,   "activate", accel_group,  */
/* 			      GDK_M, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE); */
/*   gtk_widget_add_accelerator (menu_option_colors,   "activate", accel_group,  */
/* 			      GDK_C, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE); */
/*   gtk_widget_add_accelerator (menu_option_action,   "activate", accel_group,  */
/* 			      GDK_T, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE); */
/*   gtk_widget_add_accelerator (menu_option_keys,     "activate", accel_group,  */
/* 			      GDK_K, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE); */
/*   gtk_widget_add_accelerator (menu_plugin_info,     "activate", accel_group,  */
/* 			      GDK_U, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE); */
/*   gtk_window_add_accel_group (GTK_WINDOW (window), accel_group); */
  /* end accels */
  
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
  main_connection->window = gtk_text_new (NULL, NULL);
  GTK_WIDGET_UNSET_FLAGS(GTK_WIDGET(main_connection->window), GTK_CAN_FOCUS);
  gtk_widget_set_usize (main_connection->window, 500, 300);
  gtk_box_pack_start(GTK_BOX(box_h_low), main_connection->window, TRUE, 
		     TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (main_connection->window), "focus-in-event",
		      GTK_SIGNAL_FUNC (grab_focus_cb), NULL);
  connections[0] = main_connection;
  
  foreground = &color_white;
  background = &color_black;
  
  main_connection->vscrollbar = gtk_vscrollbar_new 
    (GTK_TEXT(main_connection->window)->vadj);
  gtk_box_pack_start (GTK_BOX (box_h_low), main_connection->vscrollbar, 
		      FALSE, FALSE, 0);
  
  combo = gtk_combo_new();
  text_entry = GTK_COMBO(combo)->entry;
  gtk_combo_set_use_arrows(GTK_COMBO(combo), FALSE);
  gtk_combo_disable_activate(GTK_COMBO(combo));
  if (EntryHistory != NULL)
    gtk_combo_set_popdown_strings(GTK_COMBO(combo), EntryHistory);
  gtk_box_pack_start(GTK_BOX(box_main), combo, FALSE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT(text_entry), "key_press_event",
		      GTK_SIGNAL_FUNC (text_entry_key_press_cb), NULL);
  gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->list), "select-child",
		     GTK_SIGNAL_FUNC (text_entry_select_child_cb), NULL);
  gtk_signal_connect(GTK_OBJECT(text_entry), "activate", 
		     GTK_SIGNAL_FUNC (send_to_connection), (gpointer) combo); 
  gtk_widget_grab_focus (text_entry);
  
  gtk_widget_show_all (window);
  
  gtk_widget_realize (main_connection->window);
  gdk_window_set_background (GTK_TEXT (main_connection->window)->text_area, 
			     &color_black);
  
  get_version_info (buf);
  gtk_text_insert (GTK_TEXT (main_connection->window), font_normal, 
		   &color_lightgrey, NULL, 
		   _("Distributed under the terms of the GNU General Public License.\n"), -1);
  gtk_text_insert (GTK_TEXT (main_connection->window), font_normal, 
		   &color_lightgrey, NULL, buf, -1);
}
