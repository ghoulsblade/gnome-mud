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
GtkWidget   *action_window;
GtkWidget   *textaction;
GtkWidget   *texttrigger;
gint         action_selected_row    = -1;
gint         action_selected_column = -1;

/* internal function */
static void		 action_button_add (GtkWidget *button, gpointer data);
static void		 action_button_delete (GtkWidget *button,
					       gpointer data);
static void		 action_close_window (void);
static ACTION_DATA	*action_get_action_data (gchar *text);
static void		 action_selection_made (GtkWidget *clist, gint row,
						gint column,
						GdkEventButton *event,
						gpointer data);
static void		 next_token (gchar *token, gchar *line);
static int		 match_line (gchar *trigger, gchar *incoming);


void save_actions (GtkWidget *button, gpointer data)
{
    GList *tmp;
    ACTION_DATA *a;
    gchar filename[256] = "";
    FILE *fp;

    g_snprintf (filename, 255, "%s%s", uid_info->pw_dir, "/.amcl");
    
    if (check_amcl_dir (filename) != 0)
        return;

    g_snprintf (filename, 255, "%s%s", uid_info->pw_dir, "/.amcl/actions");

    fp = fopen (filename, "w");

    for ( tmp = action_list2; tmp != NULL; tmp = tmp->next )
    {
        if ( tmp->data )
        {
            a = (ACTION_DATA *) tmp->data;

            if ( a->trigger[0] == '\0' )
                continue;

            fprintf (fp, "%s - %s\n", a->trigger, a->action);
        }
    }
    
    if ( fp )
        fclose (fp);

    return;
}

void load_actions ( void )
{
    FILE *fp;
    gchar filename[255] = "";
    gchar line[80+80+5];
    
    g_snprintf (filename, 255, "%s%s", uid_info->pw_dir, "/.amcl");
    if (check_amcl_dir (filename) != 0)
        return;

    g_snprintf (filename, 254, "%s%s", uid_info->pw_dir, "/.amcl/actions");

    fp = fopen (filename, "r");

    if ( fp == NULL )
    {
        return;
    }

    while ( fgets (line, 80+80+5, fp) != NULL )
    {
        gchar tmp[25];
        gchar templine[80+80+5];
        gchar trigger[80];
        gchar action[80];

        strcpy (templine, line);
        next_token (tmp, templine);
        strcpy (trigger, tmp);
        next_token (tmp, templine);
        while (strcmp(tmp, "-"))
        {
            strcat (trigger, " ");
            strcat (trigger, tmp);
            next_token (tmp, templine); 
        }

        next_token (tmp, templine);
        strcpy (action, tmp);
        next_token (tmp, templine);
        while (*tmp)
        {
            strcat (action, " ");
            strcat (action, tmp);
            next_token (tmp, templine); 
        }
/*
        sscanf (line, "%s %[^\n]", trigger, action);
*/
        add_action (trigger, action);
    }

    fclose (fp);
}

static ACTION_DATA *action_get_action_data (gchar *text)
{
    GList       *tmp;
    ACTION_DATA *a;

    for ( tmp = g_list_first (action_list2); tmp != NULL; tmp = tmp->next )
    {
        if ( tmp->data != NULL )
        {
            a = (ACTION_DATA *) tmp->data;

            if ( !strcmp (a->trigger, text) )
                return a;
        }
    }
}

void  add_action (gchar *trigger, gchar *action)
{
    ACTION_DATA *new_action;

    new_action = g_malloc0 (sizeof (ACTION_DATA));
    
    new_action->trigger   = g_strdup (trigger);
    new_action->action    = g_strdup (action);

    if ( action_list2 == NULL )
        action_list2 = g_list_alloc ();

    action_list2 = g_list_append (action_list2, new_action);
}

void insert_actions  (ACTION_DATA *a, GtkCList *clist)
{
    if ( a )
    {
        gchar *text[2];

        text[0] = a->trigger;
        text[1] = a->action;
        
        gtk_clist_append ((GtkCList *)clist, text);
    }
}

int check_actions (gchar *incoming, gchar *outgoing)
{
    GList       *tmp;
    ACTION_DATA *action;
    int found = 0;
    *outgoing = '\0';


    for ( tmp = action_list2; tmp != NULL; tmp = tmp->next )
    {
        if ( tmp->data )
        {
            action = (ACTION_DATA *) tmp->data;

            if (match_line(action->trigger, incoming))
            {
                found = 1;
                strcpy (outgoing, action->action);
                break;
            }
        }
    }


    return found;
}

void next_token (gchar *token, gchar *line)
{
    gchar *next = line;
    int loc = 0;
 
    while (*next == ' ') ++next;
 
    while (   (*next != ' ') && (*next != '\n') 
           && (*next != '\r') && (*next != '\0'))
    {
        token[loc] = *next;
        ++loc;
        ++next;
    } 

    if (*next) ++next;

    token[loc] = '\0';
    strcpy (line, next);    
}

int match_line (gchar *trigger, gchar *incoming)
{
    int found = 0;
    int len = 0;
    gchar *iptr;

    if (!trigger || !*trigger) return found;

    iptr = incoming;
    len = strlen (trigger);

    while (*iptr)
    {
        if (!strncmp(trigger, iptr, len))
        {
            found = 1;
            break;
        }
        ++iptr;
    }
     
    return found;
}

static void action_selection_made (GtkWidget *clist, gint row, gint column,
				   GdkEventButton *event, gpointer data)
{
    gchar *text;

    action_selected_row    = row;
    action_selected_column = column;

    if ( (GtkCList*) data )
    {
        gtk_clist_get_text ((GtkCList*) data, row, 0, &text);
        gtk_entry_set_text (GTK_ENTRY (texttrigger), text);
        gtk_clist_get_text ((GtkCList*) data, row, 1, &text);
        gtk_entry_set_text (GTK_ENTRY (textaction), text);
    }

    return;
}

void action_button_add (GtkWidget *button, gpointer data)
{
    gchar *text[2];
    GList       *tmp;
    ACTION_DATA *action;

    text[0]   = gtk_entry_get_text (GTK_ENTRY (texttrigger  ));
    text[1]   = gtk_entry_get_text (GTK_ENTRY (textaction));

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

    for ( tmp = action_list2; tmp != NULL; tmp = tmp->next )
    {
        if ( tmp->data )
        {
            action = (ACTION_DATA *) tmp->data;

            if ( action->trigger && !strcmp (text[0], action->trigger) )
            {
                popup_window ("Can't have two of the same trigger ... yet.");
                return;
            }
        }
    }

    gtk_clist_append ((GtkCList *) data, text);

    add_action (text[0], text[1]);

    if ( action_selected_row < 0 )
        gtk_clist_select_row (GTK_CLIST (data), 0, 0);
        
    return;
}

static void action_button_delete (GtkWidget *button, gpointer data)
{
    ACTION_DATA *action;
    gchar *word;

    if ( action_selected_row == -1 )
    {
        popup_window ("No selection made.");
        return;
    }

    gtk_clist_get_text ((GtkCList*) data, action_selected_row, 0, &word);

    action = action_get_action_data (word);

    action_list2 = g_list_remove (action_list2, action);

    gtk_clist_remove ((GtkCList*) data, action_selected_row);
    action_selected_row = -1;

    return;
}

static void action_close_window (void)
{
    if ( prefs.AutoSave )
        save_actions (NULL, NULL);

    gtk_widget_set_sensitive (menu_option_action, TRUE);
    gtk_widget_destroy (action_window);
}


void window_action (GtkWidget *widget, gpointer data)
{
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *hbox2;
    GtkWidget *hbox3;
    GtkWidget *clist;
    GtkWidget *button_add;
    GtkWidget *button_quit;
    GtkWidget *button_delete;
    GtkWidget *button_save;
    GtkWidget *label;
    GtkWidget *separator;

    gchar     *titles[2] = { "Trigger", "Action" };

    gtk_widget_set_sensitive (menu_option_action, FALSE);

    action_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (action_window), "Amcl Action Center");
    gtk_signal_connect_object (GTK_OBJECT (action_window), "destroy",
                               GTK_SIGNAL_FUNC(action_close_window), NULL);
    gtk_widget_set_usize (action_window,450,320);

    vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (vbox), 5);
    gtk_container_add (GTK_CONTAINER (action_window), vbox);
    gtk_widget_show (vbox         );

    clist = gtk_clist_new_with_titles (2, titles);
    gtk_signal_connect_object (GTK_OBJECT (clist), "select_row",
                               GTK_SIGNAL_FUNC (action_selection_made),
                               (gpointer) clist);
    gtk_clist_column_titles_passive (GTK_CLIST (clist));
    gtk_clist_set_shadow_type (GTK_CLIST (clist), GTK_SHADOW_IN);

    gtk_clist_set_column_width (GTK_CLIST (clist), 0, 150);
    gtk_clist_set_column_width (GTK_CLIST (clist), 1, 200);
    gtk_clist_set_column_justification (GTK_CLIST (clist), 0, GTK_JUSTIFY_LEFT);
    gtk_clist_set_column_justification (GTK_CLIST (clist), 1, GTK_JUSTIFY_LEFT);
    gtk_clist_column_titles_show (GTK_CLIST (clist));
    gtk_box_pack_start (GTK_BOX (vbox), clist, TRUE, TRUE, 0);
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

    texttrigger   = gtk_entry_new ();
    textaction    = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (hbox2), texttrigger,   FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox2), textaction, FALSE, TRUE, 0);
    gtk_widget_show (texttrigger  );
    gtk_widget_show (textaction);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, TRUE, 5);
    gtk_widget_show (separator);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    button_add    = gtk_button_new_with_label ("  add   ");
    button_quit   = gtk_button_new_with_label (" close  ");
    button_delete = gtk_button_new_with_label (" delete ");
    button_save   = gtk_button_new_with_label ("  save  ");
    gtk_signal_connect (GTK_OBJECT (button_add), "clicked",
                               GTK_SIGNAL_FUNC (action_button_add),
                               (gpointer) clist);
    gtk_signal_connect (GTK_OBJECT (button_delete), "clicked",
                               GTK_SIGNAL_FUNC (action_button_delete),
                               (gpointer) clist);
    gtk_signal_connect (GTK_OBJECT (button_save), "clicked",
                               GTK_SIGNAL_FUNC (save_actions),
                               (gpointer) clist);
    gtk_signal_connect (GTK_OBJECT (button_quit), "clicked",
                               GTK_SIGNAL_FUNC (action_close_window),
                               NULL);
    gtk_box_pack_start (GTK_BOX (hbox), button_add,    TRUE, TRUE, 15);
    gtk_box_pack_start (GTK_BOX (hbox), button_delete, TRUE, TRUE, 15);
    gtk_box_pack_start (GTK_BOX (hbox), button_save,   TRUE, TRUE, 15);
    gtk_box_pack_start (GTK_BOX (hbox), button_quit,   TRUE, TRUE, 15);

    gtk_widget_show (button_add   );
    gtk_widget_show (button_quit  );
    gtk_widget_show (button_delete);
    gtk_widget_show (button_save  );

    g_list_foreach (action_list2, (GFunc) insert_actions, clist);
    gtk_clist_select_row (GTK_CLIST (clist), 0, 0);
    
    gtk_widget_show (action_window);

    return;
}

