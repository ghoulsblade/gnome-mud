/* AMCL - A simple Mud CLient
 * Copyright (C) 1998-2000 Robin Ericsson <lobbin@localhost.nu>
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

#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>

#include "amcl.h"

static char const rcsid[] =
    "$Id$";

extern SYSTEM_DATA  prefs;
extern GtkWidget   *menu_main_wizard;
extern GdkColor     color_yellow;
extern GdkColor     color_black;

gint          wizard_selected_row;
GtkWidget    *wizard_entry_name;
GtkWidget    *wizard_entry_host;
GtkWidget    *wizard_entry_port;
GtkWidget    *wizard_check_autologin;
GtkWidget    *wizard_entry_player;
GtkWidget    *wizard_entry_password;
GtkWidget    *wizard_window;
GtkWidget    *button_update;
GtkWidget    *button_delete;
GtkWidget    *button_connect;
static GList *wizard_connection_list2;

void free_connection_data (CONNECTION_DATA *c)
{
  mudcompress_delete(c->mccp);
  gtk_timeout_remove(c->mccp_timer);
  g_free (c->host);
  g_free (c->port);
  g_free (c);
}

void free_wizard_data ( WIZARD_DATA *w )
{
    g_free (w->name);
    g_free (w->hostname);
    g_free (w->port);
    g_free (w->playername);
    g_free (w->password);
    g_free (w);
}

void load_wizard ()
{
    WIZARD_DATA *w = NULL;
    FILE *fp;
    gchar line[1024];

    if (!(fp = open_file ("connections", "r"))) return;

    while ( fgets (line, 1024, fp) != NULL )
    {
        gchar *name;
        gchar value[1004] = "";

        name = (gchar *) g_malloc0 ( 20 * sizeof (gchar));

        sscanf (line, "%s %[^\n]", name, value);

	if ( !strcmp (name, "Version") )
	  continue;
      
	if ( !strcmp (name, "Connection") )
        {
            if ( w != NULL )
            {
                if ( wizard_connection_list2 == NULL)
                {
                    wizard_connection_list2 = g_list_alloc ();
                }
                wizard_connection_list2 = g_list_append (wizard_connection_list2, w);
            }
            w = (WIZARD_DATA *) g_malloc0 ( sizeof (WIZARD_DATA) );
            w->name = g_strdup (value);
            w->playername = g_strdup ("");
            w->password = g_strdup ("");
        }

        if ( !strcmp (name, "Hostname") )
            w->hostname = g_strdup (value);

        if ( !strcmp (name, "Port") )
            w->port = g_strdup (value);

        if ( !strcmp (name, "Player") )
            w->playername = g_strdup (value);

        if ( !strcmp (name, "Password") )
            w->password = g_strdup (value);

        if ( !strcmp (name, "AutoLogin") )
            w->autologin = TRUE;

        g_free (name);
    }

    /* added 19980915 by David Zanetti <dave@lynx.co.nz>
     * this should fix a segfault when there's a zero-byte connections
     * file
     */
    if (w)
    {
    /* end addition */
      if ( w->name )
      {
	  if ( wizard_connection_list2 == NULL )
            wizard_connection_list2 = g_list_alloc ();
	  wizard_connection_list2 = g_list_append (wizard_connection_list2, w);
      }
      else if ( w != NULL )
        free_wizard_data (w);

      /* DSZ 19980915 - moved here from bellow the fclose(fp); */
      wizard_connection_list2 = wizard_connection_list2->next;
      wizard_connection_list2->prev = NULL;
    }
    /* end */    

    if (fp) fclose (fp);
}

void save_wizard ()
{
    GList       *tmp;
    WIZARD_DATA *w;
    FILE *fp;

    if (!(fp = open_file ("connections", "w"))) return;

    fprintf(fp, "Version %d\n", WIZARD_SAVEFILE_VERSION);
    for ( tmp = wizard_connection_list2; tmp != NULL; tmp = tmp->next )
    {
        if ( tmp->data )
        {
            w = (WIZARD_DATA *) tmp->data;

            fprintf (fp, "Connection %s\n", w->name);

            if ( strlen (w->hostname) )
                fprintf (fp, "Hostname %s\n", w->hostname);
            if ( strlen (w->port) )
                fprintf (fp, "Port %s\n", w->port);
            if ( strlen (w->playername) )
                fprintf (fp, "Player %s\n", w->playername);
            if ( strlen (w->password) )
                fprintf (fp, "Password %s\n", w->password);
            if ( w->autologin == TRUE )
                fprintf (fp, "AutoLogin YES\n");
            fprintf (fp, "\n");
        }
        w = NULL;
    }

    if (fp) fclose (fp);
}

WIZARD_DATA *wizard_get_wizard_data ( gchar *text )
{
    GList       *tmp;
    WIZARD_DATA *w;

    for (tmp = g_list_first (wizard_connection_list2); tmp != NULL; tmp = tmp->next)
    {
        if ( tmp->data != NULL)
        {
            w = (WIZARD_DATA *) tmp->data;

            if ( !strcmp (w->name, text) )
                 return w;
        }
    }

    return NULL;
}

void wizard_close_window ()
{
  if (prefs.AutoSave)
    save_wizard();
  
  gtk_widget_set_sensitive (menu_main_wizard, TRUE);
  gtk_widget_destroy (wizard_window);
}

void wizard_clist_append (WIZARD_DATA *w, GtkCList *clist)
{
    if ( w )
    {
        gchar *text[1];

        text[0] = w->name;

        gtk_clist_append (GTK_CLIST (clist), text);
    }
}

void wizard_selection_made (GtkWidget *clist, gint row, gint column,
                            GdkEventButton *event, gpointer data)
{
    WIZARD_DATA *w;
    
    gchar *text;

    wizard_selected_row = row;

    gtk_clist_get_text ((GtkCList*) data, row, 0, &text);

    w = wizard_get_wizard_data ( text );

    if ( w != NULL)
    {
        if ( w->name)
            gtk_entry_set_text (GTK_ENTRY (wizard_entry_name), w->name);
        if ( w->hostname)
            gtk_entry_set_text (GTK_ENTRY (wizard_entry_host), w->hostname);
        if ( w->port)
            gtk_entry_set_text (GTK_ENTRY (wizard_entry_port), w->port);
        gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (wizard_check_autologin), w->autologin);

        if ( w->autologin )
        {
            if ( w->playername )
                gtk_entry_set_text (GTK_ENTRY (wizard_entry_player), w->playername);
            if ( w->password )
                gtk_entry_set_text (GTK_ENTRY (wizard_entry_password), w->password);
        }
        else
        {
            gtk_entry_set_text (GTK_ENTRY (wizard_entry_player), "");
            gtk_entry_set_text (GTK_ENTRY (wizard_entry_password), "");
            gtk_widget_set_sensitive (wizard_entry_player, FALSE);
            gtk_widget_set_sensitive (wizard_entry_password, FALSE);
        }
    }

    gtk_widget_set_sensitive (button_update, TRUE);
    gtk_widget_set_sensitive (button_delete, TRUE);
    gtk_widget_set_sensitive (button_connect, TRUE);
}

void wizard_unselection_made (GtkWidget *clist, gint row, gint column,
                              GdkEventButton *event, gpointer data)
{
    wizard_selected_row = -1;

    gtk_widget_set_sensitive (button_update, FALSE);
    gtk_widget_set_sensitive (button_delete, FALSE);
    gtk_widget_set_sensitive (button_connect, FALSE);
}

void wizard_button_connect (GtkWidget *button, gpointer data)
{
  CONNECTION_DATA *cd;
  WIZARD_DATA *w;
  gchar *word;
  
  if ( wizard_selected_row < 0 ) {
    popup_window ("No selection made");
    return;
  }
  
  gtk_clist_get_text ((GtkCList *) data, wizard_selected_row, 0, &word);
  
  w = wizard_get_wizard_data(word);
  
  cd = make_connection (w->hostname, w->port);
  
  if ( cd && cd->connected) 
  {
    if (  w->autologin && w->playername && w->password ) {
      connection_send (cd, w->playername);
      connection_send (cd, "\n");
      connection_send (cd, w->password);
      connection_send (cd, "\n");
    }
    
    wizard_close_window ();
  }
}

void wizard_button_delete (GtkWidget *button, gpointer data)
{
    WIZARD_DATA *w;
    gchar *word;
    
    if ( wizard_selected_row < 0 )
    {
        popup_window ("No selection made");
        return;
    }
    
    gtk_clist_get_text ((GtkCList *) data, wizard_selected_row, 0, &word);

    w = wizard_get_wizard_data (word);
    
    wizard_connection_list2 = g_list_remove (wizard_connection_list2, w);
    
    gtk_clist_remove ((GtkCList *) data, wizard_selected_row);
    wizard_selected_row = -1;

    if ( wizard_connection_list2 == NULL )
    {
        gtk_widget_set_sensitive (button_update, FALSE);
        gtk_widget_set_sensitive (button_delete, FALSE);
        gtk_widget_set_sensitive (button_connect, FALSE);
    }
}

void wizard_button_modify (GtkWidget *button, gpointer data)
{
    WIZARD_DATA *w;
    gchar *texta[1];

    texta[0] = gtk_entry_get_text (GTK_ENTRY (wizard_entry_name));

    if ( texta[0] == NULL || texta[0][0] == '\0' )
    {
        popup_window ( "Your connection doesn't have a name." );
        return;
    }

    if ( (  w = wizard_get_wizard_data (texta[0]) ) == NULL )
    {
        popup_window ( "As for the moment, everything but the name can be "
                       "changed.\n\nIf you need to change the name of the "
                       "connection, you have to use delete.");
        return;
    }

    g_free (w->hostname);   w->hostname   = g_strdup (gtk_entry_get_text (GTK_ENTRY (wizard_entry_host)));
    g_free (w->port);       w->port       = g_strdup (gtk_entry_get_text (GTK_ENTRY (wizard_entry_port)));
    g_free (w->playername); w->playername = g_strdup (gtk_entry_get_text (GTK_ENTRY (wizard_entry_player)));
    g_free (w->password);   w->password   = g_strdup (gtk_entry_get_text (GTK_ENTRY (wizard_entry_password)));

    if ( GTK_TOGGLE_BUTTON (wizard_check_autologin)->active )
        w->autologin = TRUE;
    else
        w->autologin = FALSE;
}

void wizard_button_add (GtkWidget *button, gpointer data)
{
    WIZARD_DATA *w;
    gchar *texta[1];

    texta[0] = gtk_entry_get_text (GTK_ENTRY (wizard_entry_name));

    if ( texta[0] == NULL || texta[0][0] == '\0' )
    {
        popup_window ( "Your connection doesn't have a name." );
        return;
    }

    if ( wizard_get_wizard_data (texta[0]) )
    {
        popup_window ("Can't add an existing connection.");
        return;
    }

    gtk_clist_append ((GtkCList *) data, texta);

    if ( !wizard_connection_list2 || !wizard_connection_list2->data )
        gtk_clist_select_row ((GtkCList *) data, 0, 0);

    w = (WIZARD_DATA *) g_malloc0 ( sizeof (WIZARD_DATA) );

    w->name       = g_strdup (gtk_entry_get_text (GTK_ENTRY (wizard_entry_name)));
    w->hostname   = g_strdup (gtk_entry_get_text (GTK_ENTRY (wizard_entry_host)));
    w->port       = g_strdup (gtk_entry_get_text (GTK_ENTRY (wizard_entry_port)));
    w->playername = g_strdup (gtk_entry_get_text (GTK_ENTRY (wizard_entry_player)));
    w->password   = g_strdup (gtk_entry_get_text (GTK_ENTRY (wizard_entry_password)));
    if ( GTK_TOGGLE_BUTTON (wizard_check_autologin)->active )
        w->autologin = TRUE;
    else
        w->autologin = FALSE;

    wizard_connection_list2 = g_list_append (wizard_connection_list2, w);

    gtk_widget_set_sensitive (button_update, TRUE);
    gtk_widget_set_sensitive (button_delete, TRUE);
    gtk_widget_set_sensitive (button_connect, TRUE);
}

void wizard_check_callback (GtkWidget *widget, GtkWidget *check_button)
{
    if ( GTK_TOGGLE_BUTTON (check_button)->active )
    {
        gtk_widget_set_sensitive (wizard_entry_player, TRUE);
        gtk_widget_set_sensitive (wizard_entry_password, TRUE);
    }
    else
    {
        gtk_widget_set_sensitive (wizard_entry_player, FALSE);
        gtk_widget_set_sensitive (wizard_entry_password, FALSE);
    }
}


void window_wizard (GtkWidget *widget, gpointer data)
{

    GtkWidget *hbox;
    GtkWidget *hbox2;
    GtkWidget *hbox3;
    GtkWidget *vbox_base;
    GtkWidget *vbox;
    GtkWidget *clist;
    GtkWidget *scrolled_win;
    GtkWidget *label;
    GtkWidget *button_add;
    GtkWidget *button_save;
    GtkWidget *button_close;
    GtkWidget *separator;
    GtkTooltips *tooltip;

    gchar *titles[1] = { "Connections" };
    
    tooltip = gtk_tooltips_new ();
    gtk_tooltips_set_colors (tooltip, &color_yellow, &color_black);

    gtk_widget_set_sensitive (menu_main_wizard, FALSE);
    
    wizard_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (wizard_window), "Amcl Connection Wizard");
    gtk_signal_connect_object (GTK_OBJECT (wizard_window), "destroy",
                               GTK_SIGNAL_FUNC(wizard_close_window), NULL );
    gtk_widget_set_usize (wizard_window,450,380);

    vbox_base = gtk_vbox_new (FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (vbox_base), 5);
    gtk_container_add (GTK_CONTAINER (wizard_window), vbox_base);
    gtk_widget_show (vbox_base);

    hbox = gtk_hbox_new (FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (hbox), 5);
    gtk_container_add (GTK_CONTAINER (vbox_base), hbox);
    gtk_widget_show (hbox);

    clist = gtk_clist_new_with_titles ( 1, titles);
    gtk_signal_connect_object (GTK_OBJECT (clist), "select_row",
                               GTK_SIGNAL_FUNC (wizard_selection_made),
                               (gpointer) clist);
    gtk_signal_connect_object (GTK_OBJECT (clist), "unselect_row",
                               GTK_SIGNAL_FUNC (wizard_unselection_made),
                               NULL);
    gtk_clist_set_shadow_type (GTK_CLIST (clist), GTK_SHADOW_IN);

    gtk_clist_set_column_width (GTK_CLIST (clist), 0, 50);
    gtk_clist_column_titles_passive (GTK_CLIST (clist));

    /* Create a scrolled window for the box ... */
    scrolled_win = gtk_scrolled_window_new(NULL, NULL);

    /* Add the clist to a scrolled window */
    gtk_container_add (GTK_CONTAINER (scrolled_win), clist);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0);
    gtk_widget_show (scrolled_win);
    gtk_widget_show (clist);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_border_width (GTK_CONTAINER (vbox), 5);
    gtk_container_add (GTK_CONTAINER (hbox), vbox);
    gtk_widget_show (vbox);

    label = gtk_label_new ("Connection Name:");
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    wizard_entry_name = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (vbox), wizard_entry_name, FALSE, FALSE, 0);
    gtk_tooltips_set_tip (tooltip, wizard_entry_name, "This is what you will "
                          "call the connection, and will also be used in when "
                          "you chose a connection in the list to the left",
                          NULL);
    gtk_widget_show (wizard_entry_name);

    label = gtk_label_new ("\nHost:");
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    wizard_entry_host = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (vbox), wizard_entry_host, FALSE, FALSE, 0);
    gtk_tooltips_set_tip (tooltip, wizard_entry_host, "This is the host of "
                          "where the mud you will connect to is located.",
                          NULL);
    gtk_widget_show (wizard_entry_host);

    label = gtk_label_new ("\nPort:");
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    wizard_entry_port = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (vbox), wizard_entry_port, FALSE, FALSE, 0);
    gtk_tooltips_set_tip (tooltip, wizard_entry_port, "This is the port of "
                          "the host that the mud is located on.\n"
                          "Default set to: 23",
                          NULL);
    gtk_widget_show (wizard_entry_port);

    wizard_check_autologin = gtk_check_button_new_with_label ("Auto Login?");
    gtk_signal_connect (GTK_OBJECT (wizard_check_autologin), "toggled",
                        GTK_SIGNAL_FUNC (wizard_check_callback),
                        wizard_check_autologin);
    gtk_box_pack_start (GTK_BOX (vbox), wizard_check_autologin, FALSE, FALSE, 0);
    gtk_tooltips_set_tip (tooltip, wizard_check_autologin,
                          "Should AMCL login to this mud automatically?\n"
                          "For this to work, Player Name and Password must "
                          "be set.",
                          NULL);
    GTK_WIDGET_UNSET_FLAGS (wizard_check_autologin, GTK_CAN_FOCUS);
    gtk_widget_show (wizard_check_autologin);
    
    label = gtk_label_new ("\nPlayer Name:");
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    wizard_entry_player = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (vbox), wizard_entry_player, FALSE, FALSE, 0);
    gtk_tooltips_set_tip (tooltip, wizard_entry_player,
                          "This is the player you login to the mud with, this "
                          "only works if AutoLogin is set.",
                          NULL);
    gtk_widget_show (wizard_entry_player);

    label = gtk_label_new ("\nPassword:");
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    wizard_entry_password = gtk_entry_new ();
    gtk_entry_set_visibility (GTK_ENTRY (wizard_entry_password), FALSE);
    gtk_box_pack_start (GTK_BOX (vbox), wizard_entry_password, FALSE, FALSE, 0);
    gtk_tooltips_set_tip (tooltip, wizard_entry_password,
                          "Use this together with PlayerName and AutoLogin.",
                          NULL);
    gtk_widget_show (wizard_entry_password);

    hbox2 = gtk_hbox_new (FALSE, 5);
    gtk_container_add (GTK_CONTAINER (vbox_base), hbox2);
    gtk_widget_show (hbox2);

    button_add     = gtk_button_new_with_label ("  add   ");
    button_update  = gtk_button_new_with_label ("  apply ");
    button_delete  = gtk_button_new_with_label (" delete ");
    gtk_signal_connect (GTK_OBJECT (button_add), "clicked",
                               GTK_SIGNAL_FUNC (wizard_button_add),
                               (gpointer) clist);
    gtk_signal_connect (GTK_OBJECT (button_update), "clicked",
                               GTK_SIGNAL_FUNC (wizard_button_modify),
                               (gpointer) clist);
    gtk_signal_connect (GTK_OBJECT (button_delete), "clicked",
                               GTK_SIGNAL_FUNC (wizard_button_delete),
                               (gpointer) clist);
    gtk_box_pack_start (GTK_BOX (hbox2), button_add,    TRUE, TRUE, 15);
    gtk_box_pack_start (GTK_BOX (hbox2), button_update, TRUE, TRUE, 15);
    gtk_box_pack_start (GTK_BOX (hbox2), button_delete, TRUE, TRUE, 15);
    gtk_widget_show (button_add);
    gtk_widget_show (button_update);
    gtk_widget_show (button_delete);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox_base), separator, FALSE, TRUE, 5);
    gtk_widget_show (separator);

    hbox3 = gtk_hbox_new (FALSE, 5);
    gtk_container_add (GTK_CONTAINER (vbox_base), hbox3);
    gtk_widget_show (hbox3);

    button_connect = gtk_button_new_with_label (" connect ");
    button_save    = gtk_button_new_with_label ("  save   ");
    button_close   = gtk_button_new_with_label ("  close  ");
    gtk_signal_connect (GTK_OBJECT (button_connect), "clicked",
                        GTK_SIGNAL_FUNC (wizard_button_connect),
                        (gpointer) clist);
    gtk_signal_connect_object (GTK_OBJECT (button_close), "clicked",
                               GTK_SIGNAL_FUNC (wizard_close_window),
                               NULL);
    gtk_signal_connect_object (GTK_OBJECT (button_save), "clicked",
                               GTK_SIGNAL_FUNC (save_wizard), NULL);
    gtk_box_pack_start (GTK_BOX (hbox3), button_connect, TRUE, TRUE, 15);
    gtk_box_pack_start (GTK_BOX (hbox3), button_save,    TRUE, TRUE, 15);
    gtk_box_pack_start (GTK_BOX (hbox3), button_close,   TRUE, TRUE, 15);
    gtk_widget_show (button_connect);
    gtk_widget_show (button_save);
    gtk_widget_show (button_close);

    gtk_widget_set_sensitive (button_update, FALSE);
    gtk_widget_set_sensitive (button_delete, FALSE);
    gtk_widget_set_sensitive (button_connect, FALSE);

    g_list_foreach (wizard_connection_list2, (GFunc) wizard_clist_append, clist);
    gtk_clist_select_row (GTK_CLIST (clist), 0, 0);
    
    gtk_widget_show (wizard_window);
}
