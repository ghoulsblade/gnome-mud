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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


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

GList      *alias_list2;
GtkWidget  *alias_button_save;
GtkWidget  *alias_button_delete;
GtkWidget  *textalias;
GtkWidget  *textreplace;
GtkWidget  *alias_window;
gint        alias_selected_row    = -1;

/* Local functions */
static void		 add_alias (gchar *, gchar *);
static void		 alias_button_add_cb (GtkWidget *, gpointer);
static void		 alias_button_delete_cb (GtkWidget *, gpointer);
static void	 	 alias_close_window (GtkWindow *);
static ALIAS_DATA       *alias_get_alias_data (gchar *, GList *);
static void		 alias_selection_made (GtkWidget *, gint, gint,
					       GdkEventButton *, gpointer);
static void		 alias_unselection_made (GtkWidget *, gint, gint,
						 GdkEventButton *, gpointer);
static void		 insert_aliases (ALIAS_DATA *, GtkCList *);
static void		 save_aliases (GtkWidget *, gpointer);



static void add_alias (gchar *alias, gchar *replace)
{
    ALIAS_DATA *a;

    a = (ALIAS_DATA *) g_malloc0 (sizeof (ALIAS_DATA) );
    a->alias   = g_strdup (alias);
    a->replace = g_strdup (replace);

    if (!alias_list2) alias_list2 = g_list_alloc ();
    alias_list2    = g_list_append (alias_list2, a);
}

static void save_aliases (GtkWidget *button, gpointer data)
{
    GList *tmp;
    ALIAS_DATA *a;
    FILE *fp;

    if (!(fp = open_file ("aliases", "w"))) return;
    for ( tmp = alias_list2; tmp != NULL; tmp = tmp->next )
	if ( tmp->data )
	{
	    a = (ALIAS_DATA *) tmp->data;
	    if ( a->alias[0] == '\0' ) continue;
	    fprintf (fp, "%s %s\n", a->alias, a->replace);
	}

    if ( fp )
        fclose (fp);

    gtk_widget_set_sensitive (button, FALSE);
    return;
}

void load_aliases ( void )
{
    FILE *fp;
    gchar line[255] = "", alias[15], replace[80];
    
    if (!(fp = open_file ("aliases", "r"))) return;
    while ( fgets (line, 80+15+5, fp) != NULL )
    {
        sscanf (line, "%s %[^\n]", alias, replace);
	add_alias(alias, replace);
    }

    if (fp) fclose (fp);
}

static ALIAS_DATA *alias_get_alias_data (gchar *text, GList *alias_list2)
{
    GList       *tmp;
    ALIAS_DATA  *a;

    for (tmp = g_list_first (alias_list2); tmp != NULL; tmp = tmp->next)
	if ( tmp->data != NULL)
	{
	    a = (ALIAS_DATA *) tmp->data;
	    if ( !strcmp (a->alias, text) )
	 	return a;
	}
    return NULL;
}

static void insert_aliases (ALIAS_DATA *a, GtkCList *clist)
{
    if ( a )
    {
	gchar *text[2];

	text[0] = a->alias;
	text[1] = a->replace;

	gtk_clist_append (GTK_CLIST (clist), text);
    }
}

static void alias_selection_made (GtkWidget *clist, gint row, gint column,
			   GdkEventButton *event, gpointer data)
{
    gchar *text;

    alias_selected_row = row;

    if ( (GtkCList*) data )
    {
	gtk_clist_get_text ((GtkCList*) data, row, 0, &text);
	gtk_entry_set_text (GTK_ENTRY (textalias), text);
	gtk_clist_get_text ((GtkCList*) data, row, 1, &text);
	gtk_entry_set_text (GTK_ENTRY (textreplace), text);
     }

     gtk_widget_set_sensitive (alias_button_delete, TRUE);
     return;
}

static void alias_unselection_made (GtkWidget *clist, gint row, gint column,
			     	    GdkEventButton *event, gpointer data)
{
    alias_selected_row = -1;
    gtk_widget_set_sensitive (alias_button_delete, FALSE);
}

static void alias_button_add_cb (GtkWidget *button, gpointer data)
{
    gchar *text[2];

    text[0]   = gtk_entry_get_text (GTK_ENTRY (textalias  ));
    text[1]   = gtk_entry_get_text (GTK_ENTRY (textreplace));

    if ( text[0][0] == '\0' || text[1][0] == '\0' )
    {
        popup_window ("Please complete the alias first.");
        return;
    }
    
    if (strstr(text[0], " "))
    {
        popup_window ("I can't not make an alias of that.");
        return;
    }

    if ( strlen (text[0]) > 15)
    {
        popup_window ("Alias to big.");
        return;
    }
    
    if ( strlen (text[1]) > 80)
    {
        popup_window ("Replacement to big.");
        return;
    }

    if ( alias_get_alias_data (text[0], alias_list2) )
    {
        popup_window ("Can't add an existing alias.");
        return;
    }

    gtk_clist_append (GTK_CLIST (data), text);

    add_alias(text[0], text[1]);
    gtk_widget_set_sensitive (alias_button_delete, TRUE);
    gtk_widget_set_sensitive (alias_button_save,   TRUE);

    if ( alias_selected_row < 0 )
        gtk_clist_select_row (GTK_CLIST (data), 0, 0);
}

static void alias_button_delete_cb (GtkWidget *button, gpointer data)
{
    ALIAS_DATA *alias;
    gchar *word;
    
    if ( alias_selected_row == -1 )
    {
        popup_window ("No selection made.");
        return;
    }

    gtk_clist_get_text ((GtkCList*) data, alias_selected_row, 0, &word);

    alias = alias_get_alias_data (word, alias_list2);

    alias_list2 = g_list_remove (alias_list2, alias);

    gtk_clist_remove ((GtkCList*) data, alias_selected_row);
    alias_selected_row = -1;
    if ( alias_list2 == NULL )
        gtk_widget_set_sensitive (alias_button_delete, FALSE);
    gtk_widget_set_sensitive (alias_button_save, TRUE);
}

static void alias_close_window (GtkWindow *button)
{
    if ( prefs.AutoSave )
        save_aliases (NULL, NULL);

    gtk_widget_set_sensitive (menu_option_alias, TRUE);
    gtk_widget_destroy (alias_window);
}

void window_alias (GtkWidget *widget, gpointer data)
{
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *hbox2;
    GtkWidget *hbox3;
    GtkWidget *clist;
    GtkWidget *scrolled_win;
    GtkWidget *alias_button_add;
    GtkWidget *alias_button_quit;
    GtkWidget *label;
    GtkWidget *separator;
    gchar     *titles[2] = { "Alias", "Replacement" };

    gtk_widget_set_sensitive (menu_option_alias, FALSE);

    alias_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (alias_window), "Amcl Alias Center");
    gtk_signal_connect_object (GTK_OBJECT (alias_window), "destroy",
                               GTK_SIGNAL_FUNC(alias_close_window), NULL );
    gtk_widget_set_usize (alias_window,450,320);

    vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (vbox), 5);
    gtk_container_add (GTK_CONTAINER (alias_window), vbox);
    gtk_widget_show (vbox);

    clist = gtk_clist_new_with_titles (2, titles);
    gtk_signal_connect_object (GTK_OBJECT (clist), "select_row",
                               GTK_SIGNAL_FUNC (alias_selection_made),
                               (gpointer) clist);
    gtk_signal_connect_object (GTK_OBJECT (clist), "unselect_row",
                               GTK_SIGNAL_FUNC (alias_unselection_made),
                               NULL);
    gtk_clist_column_titles_passive (GTK_CLIST (clist));
    gtk_clist_set_shadow_type (GTK_CLIST (clist), GTK_SHADOW_IN);
    gtk_clist_set_column_width (GTK_CLIST (clist), 0, 100);
    gtk_clist_set_column_width (GTK_CLIST (clist), 1, 250);
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

    label = gtk_label_new ("Alias");
    gtk_box_pack_start (GTK_BOX (hbox3), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    label = gtk_label_new ("Replacement");
    gtk_box_pack_start (GTK_BOX (hbox3), label, FALSE, TRUE, 0);
    gtk_widget_show (label);

    hbox2 = gtk_hbox_new (TRUE, 15);
    gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 0);
    gtk_widget_show (hbox2);

    textalias   = gtk_entry_new ();
    textreplace = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (hbox2), textalias,   FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox2), textreplace, FALSE, TRUE, 0);
    gtk_widget_show (textalias  );
    gtk_widget_show (textreplace);
    
    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, TRUE, 5);
    gtk_widget_show (separator);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    alias_button_add    = gtk_button_new_with_label ("  add   ");
    alias_button_quit   = gtk_button_new_with_label (" close  ");
    alias_button_delete = gtk_button_new_with_label (" delete ");
    alias_button_save   = gtk_button_new_with_label ("  save  ");
    gtk_signal_connect (GTK_OBJECT (alias_button_add), "clicked",
                               GTK_SIGNAL_FUNC (alias_button_add_cb),
                               (gpointer) clist);
    gtk_signal_connect (GTK_OBJECT (alias_button_delete), "clicked",
                               GTK_SIGNAL_FUNC (alias_button_delete_cb),
                               (gpointer) clist);
    gtk_signal_connect (GTK_OBJECT (alias_button_save), "clicked",
                               GTK_SIGNAL_FUNC (save_aliases),
                               NULL);
    gtk_signal_connect (GTK_OBJECT (alias_button_quit), "clicked",
                               GTK_SIGNAL_FUNC (alias_close_window),
                               NULL);
                               
    gtk_box_pack_start (GTK_BOX (hbox), alias_button_add,    TRUE, TRUE, 15);
    gtk_box_pack_start (GTK_BOX (hbox), alias_button_delete, TRUE, TRUE, 15);
    gtk_box_pack_start (GTK_BOX (hbox), alias_button_save,   TRUE, TRUE, 15);
    gtk_box_pack_start (GTK_BOX (hbox), alias_button_quit,   TRUE, TRUE, 15);

    gtk_widget_show (alias_button_add   );
    gtk_widget_show (alias_button_quit  );
    gtk_widget_show (alias_button_delete);
    gtk_widget_show (alias_button_save  );

    gtk_widget_set_sensitive (alias_button_delete, FALSE);
    gtk_widget_set_sensitive (alias_button_save,   FALSE);

    g_list_foreach (alias_list2, (GFunc) insert_aliases, clist);
    gtk_clist_select_row (GTK_CLIST (clist), 0, 0);
    gtk_widget_show (alias_window );

    return;
}
