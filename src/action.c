/* AMCL - A simple Mud CLient
 * Copyright (C) 1998-1999 Robin Ericsson <lobbin@localhost.nu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* Action trigger code added by Bret Robideaux (fayd@alliances.org) */

#include "config.h"

#include <gtk/gtk.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>

#include "amcl.h"

static char const rcsid[] =
    "$Id$";


GList       *action_list2;
GtkWidget   *action_button_save;
GtkWidget   *action_button_delete;
GtkWidget   *texttrigger;
GtkWidget   *textaction;
GtkWidget   *action_window;
gint         action_selected_row    = -1;

/* internal function */
static void		 action_button_add (GtkWidget *button, gpointer data);
static void		 action_button_delete_cb (GtkWidget *button,
					          gpointer data);
static void		 action_close_window (GtkWidget *button);
static ACTION_DATA       *action_get_action_data (gchar *text, GList *action_list2);
static void		 action_selection_made (GtkWidget *clist, gint row,
						gint column,
						GdkEventButton *event,
						gpointer data);
static int		 match_line (gchar *trigger, gchar *incoming);

void add_action (gchar *trigger, gchar *action)
{
    ACTION_DATA *new_action;

    new_action = g_malloc0 (sizeof (ACTION_DATA));
    new_action->trigger  = g_strdup (trigger);
    new_action->action   = g_strdup (action);

    if (!action_list2) action_list2 = g_list_alloc ();
    action_list2 = g_list_append (action_list2, new_action);
}

void save_actions (GtkWidget *button, gpointer data)
{
    GList *tmp;
    ACTION_DATA *a;
    FILE *fp;

    if (!(fp = open_file ("actions", "w"))) return;
    for ( tmp = action_list2; tmp != NULL; tmp = tmp->next )
        if ( tmp->data )
        {
            a = (ACTION_DATA *) tmp->data;
	    if ( a->trigger[0] == '\0' ) continue;
            fprintf (fp, "%s - %s\n", a->trigger, a->action);
        }
    
    if ( fp )
        fclose (fp);

    gtk_widget_set_sensitive (button, FALSE);
    return;
}

void load_actions ()
{
    FILE *fp;
    gchar line[255] = "", *t, *a;
    
    if (!(fp = open_file ("actions", "r"))) return;
    while ( fgets (line, 80+80+5, fp) != NULL )
    {
	t = strtok(line, "-"); t[strlen(t)-1] = '\0';
	a = strtok(NULL, "-"); a[strlen(a)-1] = '\0';
	if (strlen(++a) || strlen(t))
	  add_action(t, a);
    }

    if (fp) fclose (fp);
}

static ACTION_DATA *action_get_action_data (gchar *text, GList *action_list2)
{
    GList       *tmp;
    ACTION_DATA *a;

    for (tmp = g_list_first (action_list2); tmp != NULL; tmp = tmp->next)
      if ( tmp->data != NULL )
      {
	  a = (ACTION_DATA *) tmp->data;
	  if ( !strcmp (a->trigger, text) )
	    return a;
       }
    return NULL;
}

void insert_actions (ACTION_DATA *a, GtkCList *clist)
{
    if ( a )
    {
        gchar *text[2];

        text[0] = a->trigger;
        text[1] = a->action;

	gtk_clist_append (GTK_CLIST (clist), text);        
    }
}

void action_selection_made (GtkWidget *button, gint row, gint column,
			    GdkEventButton *event, gpointer clist)
{
    gchar *text;

    action_selected_row = row;

    if ( GTK_CLIST (clist) )
    {
	gtk_clist_get_text (GTK_CLIST (clist), row, 0, &text);
	gtk_entry_set_text (GTK_ENTRY (texttrigger), text);
	gtk_clist_get_text (GTK_CLIST (clist), row, 1, &text);
	gtk_entry_set_text (GTK_ENTRY (textaction), text);
    }

    gtk_widget_set_sensitive (action_button_delete, TRUE);
    return;
}

void action_unselection_made (GtkWidget *clist, gint row, gint column,
			      GdkEventButton *event, gpointer data)
{
    action_selected_row = -1;
    gtk_widget_set_sensitive (action_button_delete, FALSE);
}

int check_actions (gchar *incoming, gchar *outgoing)
{
    GList	*tmp;
    ACTION_DATA *action;
    *outgoing = '\0';


    for ( tmp = action_list2; tmp != NULL; tmp = tmp->next )
	if (tmp->data )
	{
	    action = (ACTION_DATA *) tmp->data;
	    if (match_line(action->trigger, incoming))
	    {
		strcpy (outgoing, action->action);
		return 1;
	    }
	}

    return 0;
}

int match_line (gchar *trigger, gchar *incoming)
{
    int len = 0;
    gchar *iptr;


    if (!trigger || *trigger) return 0;

    iptr = incoming;
    len = strlen (trigger);

    while (*iptr)
	if (!strncmp(trigger, iptr++, len))
	    return 1;
    return 0;
}

void action_button_add_cb (GtkWidget *button, gpointer data)
{
    gchar *text[2];

    text[0] = gtk_entry_get_text (GTK_ENTRY (texttrigger));
    text[1] = gtk_entry_get_text (GTK_ENTRY (textaction));


    if ( text[0][0] == '\0' || text[1][0] == '\0' )
    {
        popup_window ("Please complete the action first.");
        return;
    }

    if ( strlen (text[0]) > 80)
    {
        popup_window ("Trigger too big.");
        return;
    }

    if ( strlen (text[1]) > 80)
    {
        popup_window ("Action too big.");
        return;
    }

    if ( action_get_action_data(text[0], action_list2) )
    {
	popup_window ("Can't have two of the same trigger ... yet.");
	return;
    }

    gtk_clist_append (GTK_CLIST (data), text);
	
    add_action (text[0], text[1]);
    gtk_widget_set_sensitive (action_button_delete, TRUE);
    gtk_widget_set_sensitive (action_button_save,   TRUE);

    if ( action_selected_row < 0 )
        gtk_clist_select_row (GTK_CLIST (data), 0, 0);
}

static void action_button_delete_cb (GtkWidget *button, gpointer data)
{
    ACTION_DATA *action;
    gchar *word;

    if ( action_selected_row == -1 )
    {
        popup_window ("No selection made.");
        return;
    }

    gtk_clist_get_text (GTK_CLIST (data), action_selected_row, 0, &word);

    action = action_get_action_data (word, action_list2);

    action_list2 = g_list_remove (action_list2, action);

    gtk_clist_remove (GTK_CLIST (data), action_selected_row);
    action_selected_row = -1;
    if ( action_list2 == NULL )
	gtk_widget_set_sensitive (action_button_delete, FALSE);
    gtk_widget_set_sensitive (action_button_save, TRUE);
}

static void action_close_window (GtkWidget *button)
{
    if ( prefs.AutoSave )
        save_actions (NULL, NULL);

    gtk_widget_set_sensitive (GTK_WIDGET(menu_option_action), TRUE);
    gtk_widget_destroy (action_window);
}

void window_action (GtkWidget *widget, gpointer data)
{
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *hbox2;
    GtkWidget *hbox3;
    GtkWidget *clist;
    GtkWidget *scrolled_win;
    GtkWidget *action_button_add;
    GtkWidget *action_button_quit;
    GtkWidget *label;
    GtkWidget *separator;
    gchar     *titles[2] = { "Trigger", "Action" };

    gtk_widget_set_sensitive (menu_option_action, FALSE);

    action_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (action_window), "Amcl Action Center");
    gtk_signal_connect_object (GTK_OBJECT (action_window), "destroy",
                               GTK_SIGNAL_FUNC(action_close_window),
                               NULL);
    gtk_widget_set_usize (action_window,450,320);

    vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (vbox), 5);
    gtk_container_add (GTK_CONTAINER (action_window), vbox);
    gtk_widget_show (vbox);

    clist = gtk_clist_new_with_titles (2, titles);
    gtk_signal_connect_object (GTK_OBJECT (clist), "select_row",
                               GTK_SIGNAL_FUNC (action_selection_made),
                               (gpointer) clist);
    gtk_signal_connect_object (GTK_OBJECT (clist), "unselect_row",
                               GTK_SIGNAL_FUNC (action_unselection_made),
                               NULL);
    gtk_clist_column_titles_passive (GTK_CLIST (clist));
    gtk_clist_set_shadow_type (GTK_CLIST (clist), GTK_SHADOW_IN);
    gtk_clist_set_column_width (GTK_CLIST (clist), 0, 150);
    gtk_clist_set_column_width (GTK_CLIST (clist), 1, 200);
    gtk_clist_set_column_justification (GTK_CLIST (clist), 0, GTK_JUSTIFY_LEFT);
    gtk_clist_set_column_justification (GTK_CLIST (clist), 1, GTK_JUSTIFY_LEFT);

    gtk_clist_column_titles_show (GTK_CLIST (clist));

    /* Create a scrolled window for the box ... */
    scrolled_win = gtk_scrolled_window_new(NULL, NULL);
 
    /* Add the clist to a scrolled window */
    gtk_container_add (GTK_CONTAINER (scrolled_win), clist);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);
    gtk_widget_show (scrolled_win);
    gtk_widget_show (clist);

    hbox3 = gtk_hbox_new (TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox3, FALSE, FALSE, 0);
    gtk_widget_show (hbox3);

    label = gtk_label_new ("Trigger");
    gtk_box_pack_start (GTK_BOX (hbox3), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    label = gtk_label_new ("Action");
    gtk_box_pack_start (GTK_BOX (hbox3), label, FALSE, TRUE, 0);
    gtk_widget_show (label);

    hbox2 = gtk_hbox_new (TRUE, 15);
    gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 0);
    gtk_widget_show (hbox2);

    texttrigger = gtk_entry_new ();
    textaction  = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (hbox2), texttrigger, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox2), textaction, FALSE, TRUE, 0);
    gtk_widget_show (texttrigger);
    gtk_widget_show (textaction);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, TRUE, 5);
    gtk_widget_show (separator);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    action_button_add    = gtk_button_new_with_label ("  add   ");
    action_button_quit   = gtk_button_new_with_label (" close  ");
    action_button_delete = gtk_button_new_with_label (" delete ");
    action_button_save   = gtk_button_new_with_label ("  save  ");
    gtk_signal_connect (GTK_OBJECT (action_button_add), "clicked",
                               GTK_SIGNAL_FUNC (action_button_add_cb),
                               (gpointer) clist);
    gtk_signal_connect (GTK_OBJECT (action_button_delete), "clicked",
                               GTK_SIGNAL_FUNC (action_button_delete_cb),
                               (gpointer) clist);
    gtk_signal_connect (GTK_OBJECT (action_button_save), "clicked",
                               GTK_SIGNAL_FUNC (save_actions),
                               NULL);
    gtk_signal_connect (GTK_OBJECT (action_button_quit), "clicked",
                               GTK_SIGNAL_FUNC (action_close_window),
                               NULL);

    gtk_box_pack_start (GTK_BOX (hbox), action_button_add,    TRUE, TRUE, 15);
    gtk_box_pack_start (GTK_BOX (hbox), action_button_delete, TRUE, TRUE, 15);
    gtk_box_pack_start (GTK_BOX (hbox), action_button_save,   TRUE, TRUE, 15);
    gtk_box_pack_start (GTK_BOX (hbox), action_button_quit,   TRUE, TRUE, 15);

    gtk_widget_show (action_button_add   );
    gtk_widget_show (action_button_quit  );
    gtk_widget_show (action_button_delete);
    gtk_widget_show (action_button_save  );

    gtk_widget_set_sensitive (action_button_delete, FALSE);
    gtk_widget_set_sensitive (action_button_save,   FALSE);
    
    g_list_foreach (action_list2, (GFunc) insert_actions, clist);
    gtk_clist_select_row (GTK_CLIST (clist), 0, 0);
    gtk_widget_show (action_window);

    return;
}
