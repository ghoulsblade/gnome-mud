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

#include <gnome.h>

#include "gnome-mud.h"

static char const rcsid[] =
    "$Id$";
    
static void profiles_new_connection()
{
}

static GnomeUIInfo profiles_popup_menu [] = {
		GNOMEUIINFO_MENU_NEW_ITEM(N_("_New Connection"), N_("Creates a new connection"), profiles_new_connection, NULL),
		GNOMEUIINFO_SEPARATOR,
		GNOMEUIINFO_MENU_NEW_ITEM(N_("_New Connection"), N_("Creates a new connection"), profiles_new_connection, NULL),
		GNOMEUIINFO_END
};

static int profiles_button_press (GtkWidget *widget, GdkEventButton *event, GtkTree *tree)
{
	GtkWidget *menu;
	GnomeUIInfo *uiinfo;

	g_message("profiles_button_press()");
	
	if (event->button != 3 || !(event->state & GDK_CONTROL_MASK))
		return FALSE;

	g_message("still here...");
	
	gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "button_press_event");

	uiinfo = profiles_popup_menu;
	menu = gnome_popup_menu_new (profiles_popup_menu);

	g_message("popupping menu");
	
	gnome_popup_menu_do_popup_modal (menu, NULL, NULL, event, tree);
	gtk_widget_destroy (menu);

	return TRUE;
}

void window_profiles (void)
{
	static GtkWidget *main_dialog;
	GtkWidget *dialog_vbox1;
	GtkWidget *main_hbox;
	GtkWidget *scrolledwindow1;
	GtkWidget *main_list;
	GtkWidget *main_tree;
	GtkWidget *notebook;
	GtkWidget *table_mud;
	GtkWidget *label_host;
	GtkWidget *label_port;
	GtkWidget *label_title;
	GtkWidget *entry_host;
	GtkWidget *entry_port;
	GtkWidget *entry_title;
	GtkWidget *label_misc;
	GtkWidget *hbuttonbox1;
	GtkWidget *button_lookup;
	GtkWidget *button_ping;
	GtkWidget *label_mud;
	GtkWidget *table_character;
	GtkWidget *label_character2;
	GtkWidget *entry_character;
	GtkWidget *label_password;
	GtkWidget *entry_password;
	GtkWidget *checkbutton_login;
	GtkWidget *label_notes;
	GtkWidget *scrolledwindow2;
	GtkWidget *text_notes;
	GtkWidget *label_character;
	GtkWidget *dialog_action_area1;
	GtkWidget *button1;
	GtkWidget *button2;
	GtkWidget *button3;
	GtkWidget *menu;
	
	if (main_dialog != NULL) {
	  gdk_window_raise(main_dialog->window);
	  gdk_window_show(main_dialog->window);
	  return;
	}
	
	main_dialog = gnome_dialog_new (_("GNOME-Mud Connection Wizard"), NULL);
	gtk_object_set_data (GTK_OBJECT (main_dialog), "main_dialog", main_dialog);
	gtk_window_set_policy (GTK_WINDOW (main_dialog), FALSE, FALSE, FALSE);
  
	dialog_vbox1 = GNOME_DIALOG (main_dialog)->vbox;
	gtk_object_set_data (GTK_OBJECT (main_dialog), "dialog_vbox1", dialog_vbox1);
	gtk_widget_show (dialog_vbox1);
  
	main_hbox = gtk_hbox_new (FALSE, 3);
	gtk_widget_ref (main_hbox);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "main_hbox", main_hbox,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (main_hbox);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), main_hbox, TRUE, TRUE, 0);
  
	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_ref (scrolledwindow1);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "scrolledwindow1", scrolledwindow1,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (scrolledwindow1);
	gtk_box_pack_start (GTK_BOX (main_hbox), scrolledwindow1, FALSE, TRUE, 0);
	gtk_widget_set_usize (scrolledwindow1, 180, -2);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  
	main_list = gtk_viewport_new (NULL, NULL);
	gtk_widget_ref (main_list);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "main_list", main_list,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (main_list);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), main_list);
  
	main_tree = gtk_tree_new ();
	gtk_widget_ref (main_tree);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "main_tree", main_tree,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (main_tree);
	gtk_container_add (GTK_CONTAINER (main_list), main_tree);

	gtk_signal_connect (GTK_OBJECT (main_tree), "button_press_event",
						GTK_SIGNAL_FUNC (profiles_button_press), main_tree);

  
	notebook = gtk_notebook_new ();
	gtk_widget_ref (notebook);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "notebook", notebook,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (notebook);
	gtk_box_pack_start (GTK_BOX (main_hbox), notebook, TRUE, TRUE, 0);
  
	table_mud = gtk_table_new (5, 2, FALSE);
	gtk_widget_ref (table_mud);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "table_mud", table_mud,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (table_mud);
	gtk_container_add (GTK_CONTAINER (notebook), table_mud);
	gtk_container_set_border_width (GTK_CONTAINER (table_mud), 8);
  
	label_host = gtk_label_new (_("Host:"));
	gtk_widget_ref (label_host);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "label_host", label_host,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label_host);
	gtk_table_attach (GTK_TABLE (table_mud), label_host, 0, 1, 0, 1,
					  (GtkAttachOptions) (GTK_FILL),
					  (GtkAttachOptions) (0), 0, 0);
	gtk_label_set_justify (GTK_LABEL (label_host), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment (GTK_MISC (label_host), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (label_host), 5, 1);
  
	label_port = gtk_label_new (_("Port:"));
	gtk_widget_ref (label_port);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "label_port", label_port,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label_port);
	gtk_table_attach (GTK_TABLE (table_mud), label_port, 0, 1, 1, 2,
					  (GtkAttachOptions) (GTK_FILL),
					  (GtkAttachOptions) (0), 0, 0);
	gtk_label_set_justify (GTK_LABEL (label_port), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment (GTK_MISC (label_port), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (label_port), 5, 1);
  
	label_title = gtk_label_new (_("Title:"));
	gtk_widget_ref (label_title);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "label_title", label_title,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label_title);
	gtk_table_attach (GTK_TABLE (table_mud), label_title, 0, 1, 2, 3,
					  (GtkAttachOptions) (GTK_FILL),
					  (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_title), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (label_title), 5, 1);
  
	entry_host = gtk_entry_new ();
	gtk_widget_ref (entry_host);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "entry_host", entry_host,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (entry_host);
	gtk_table_attach (GTK_TABLE (table_mud), entry_host, 1, 2, 0, 1,
					  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
					  (GtkAttachOptions) (0), 0, 0);
  
	entry_port = gtk_entry_new ();
	gtk_widget_ref (entry_port);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "entry_port", entry_port,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (entry_port);
	gtk_table_attach (GTK_TABLE (table_mud), entry_port, 1, 2, 1, 2,
					  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
					  (GtkAttachOptions) (0), 0, 0);
  
	entry_title = gtk_entry_new ();
	gtk_widget_ref (entry_title);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "entry_title", entry_title,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (entry_title);
	gtk_table_attach (GTK_TABLE (table_mud), entry_title, 1, 2, 2, 3,
					  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
					  (GtkAttachOptions) (0), 0, 0);
  
	label_misc = gtk_label_new ("");
	gtk_widget_ref (label_misc);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "label_misc", label_misc,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label_misc);
	gtk_table_attach (GTK_TABLE (table_mud), label_misc, 1, 2, 3, 4,
					  (GtkAttachOptions) (GTK_FILL),
					  (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_misc), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (label_misc), 2, 2);
  
	hbuttonbox1 = gtk_hbutton_box_new ();
	gtk_widget_ref (hbuttonbox1);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "hbuttonbox1", hbuttonbox1,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (hbuttonbox1);
	gtk_table_attach (GTK_TABLE (table_mud), hbuttonbox1, 1, 2, 4, 5,
					  (GtkAttachOptions) (GTK_FILL),
					  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox1), GTK_BUTTONBOX_START);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbuttonbox1), 0);
  
	button_lookup = gtk_button_new_with_label (_("Lookup "));
	gtk_widget_ref (button_lookup);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "button_lookup", button_lookup,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (button_lookup);
	gtk_container_add (GTK_CONTAINER (hbuttonbox1), button_lookup);
	GTK_WIDGET_SET_FLAGS (button_lookup, GTK_CAN_DEFAULT);
  
	button_ping = gtk_button_new_with_label (_("Ping"));
	gtk_widget_ref (button_ping);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "button_ping", button_ping,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (button_ping);
	gtk_container_add (GTK_CONTAINER (hbuttonbox1), button_ping);
	GTK_WIDGET_SET_FLAGS (button_ping, GTK_CAN_DEFAULT);
  
	label_mud = gtk_label_new (_("Mud"));
	gtk_widget_ref (label_mud);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "label_mud", label_mud,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label_mud);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 0), label_mud);
  
	table_character = gtk_table_new (6, 2, FALSE);
	gtk_widget_ref (table_character);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "table_character", table_character,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (table_character);
	gtk_container_add (GTK_CONTAINER (notebook), table_character);
  
	label_character2 = gtk_label_new (_("Character: "));
	gtk_widget_ref (label_character2);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "label_character2", label_character2,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label_character2);
	gtk_table_attach (GTK_TABLE (table_character), label_character2, 0, 1, 0, 1,
					  (GtkAttachOptions) (GTK_FILL),
					  (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_character2), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (label_character2), 5, 1);
  
	entry_character = gtk_entry_new ();
	gtk_widget_ref (entry_character);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "entry_character", entry_character,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (entry_character);
	gtk_table_attach (GTK_TABLE (table_character), entry_character, 1, 2, 0, 1,
					  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
					  (GtkAttachOptions) (0), 0, 0);
  
	label_password = gtk_label_new (_("Password:"));
	gtk_widget_ref (label_password);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "label_password", label_password,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label_password);
	gtk_table_attach (GTK_TABLE (table_character), label_password, 0, 1, 1, 2,
					  (GtkAttachOptions) (GTK_FILL),
					  (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_password), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (label_password), 5, 1);
  
	entry_password = gtk_entry_new ();
	gtk_widget_ref (entry_password);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "entry_password", entry_password,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (entry_password);
	gtk_table_attach (GTK_TABLE (table_character), entry_password, 1, 2, 1, 2,
					  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
					  (GtkAttachOptions) (0), 0, 0);
	gtk_entry_set_visibility (GTK_ENTRY (entry_password), FALSE);
  
	checkbutton_login = gtk_check_button_new_with_label (_(" Auto Login?"));
	gtk_widget_ref (checkbutton_login);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "checkbutton_login", checkbutton_login,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (checkbutton_login);
	gtk_table_attach (GTK_TABLE (table_character), checkbutton_login, 0, 2, 2, 3,
					  (GtkAttachOptions) (GTK_FILL),
					  (GtkAttachOptions) (0), 0, 0);
  
	label_notes = gtk_label_new (_("Notes:"));
	gtk_widget_ref (label_notes);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "label_notes", label_notes,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label_notes);
	gtk_table_attach (GTK_TABLE (table_character), label_notes, 0, 1, 3, 4,
					  (GtkAttachOptions) (GTK_FILL),
					  (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_notes), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (label_notes), 5, 1);
  
	scrolledwindow2 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_ref (scrolledwindow2);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "scrolledwindow2", scrolledwindow2,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (scrolledwindow2);
	gtk_table_attach (GTK_TABLE (table_character), scrolledwindow2, 1, 2, 3, 6,
					  (GtkAttachOptions) (GTK_FILL),
					  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow2), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  
	text_notes = gtk_text_new (NULL, NULL);
	gtk_widget_ref (text_notes);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "text_notes", text_notes,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (text_notes);
	gtk_container_add (GTK_CONTAINER (scrolledwindow2), text_notes);
	gtk_widget_set_usize (text_notes, -2, 150);
	gtk_text_set_editable (GTK_TEXT (text_notes), TRUE);
	
	label_character = gtk_label_new (_("Character"));
	gtk_widget_ref (label_character);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "label_character", label_character,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label_character);
	gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 1), label_character);
  
	dialog_action_area1 = GNOME_DIALOG (main_dialog)->action_area;
	gtk_object_set_data (GTK_OBJECT (main_dialog), "dialog_action_area1", dialog_action_area1);
	gtk_widget_show (dialog_action_area1);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area1), 8);
  
	gnome_dialog_append_button_with_pixmap (GNOME_DIALOG (main_dialog),
											_("Connect"), GNOME_STOCK_PIXMAP_JUMP_TO);
	button1 = GTK_WIDGET (g_list_last (GNOME_DIALOG (main_dialog)->buttons)->data);
	gtk_widget_ref (button1);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "button1", button1,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (button1);
	GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);
  
	gnome_dialog_append_button (GNOME_DIALOG (main_dialog), GNOME_STOCK_BUTTON_OK);
	button2 = GTK_WIDGET (g_list_last (GNOME_DIALOG (main_dialog)->buttons)->data);
	gtk_widget_ref (button2);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "button2", button2,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (button2);
	GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);
  
	gnome_dialog_append_button (GNOME_DIALOG (main_dialog), GNOME_STOCK_BUTTON_CLOSE);
	button3 = GTK_WIDGET (g_list_last (GNOME_DIALOG (main_dialog)->buttons)->data);
	gtk_widget_ref (button3);
	gtk_object_set_data_full (GTK_OBJECT (main_dialog), "button3", button3,
							  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (button3);
	GTK_WIDGET_SET_FLAGS (button3, GTK_CAN_DEFAULT);

	menu = gnome_popup_menu_new(profiles_popup_menu);
	
	gtk_signal_connect_object(GTK_OBJECT(button3), "clicked",
							  gtk_widget_destroy, (gpointer) main_dialog);
							  
	gtk_signal_connect(GTK_OBJECT(main_dialog), "destroy",
		     		   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &main_dialog);
		     		   
	gtk_widget_show(main_dialog);
}