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

#include <gnome.h>
#include <sys/types.h>
#include <libintl.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>

#include "gnome-mud.h"

static char const rcsid[] =
    "$Id$";

struct own_data {
	GtkCList		*list;
	GtkWidget		*button_delete,  
					*textname,
					*textvalue,
					*window;
    gchar    	 	*title_name,
					*title_value,
					*file_name;
	gint      		 tam_name, 
					 tam_value, 
					 row, 
					 z;
	PROFILE_DATA	*pd;
};

typedef struct own_data DDATA;

extern SYSTEM_DATA  prefs;

/* Local functions */
gint	 get_size (GtkCList *);
static gint	 find_data (GtkCList *, gchar *);
static void	 data_button_add (GtkWidget *, DDATA *);
static void	 data_button_delete (GtkWidget *, DDATA *);
static void	 data_button_close (GtkWidget *, DDATA *);
static void	 data_selection_made (GtkCList *, gint, gint, GdkEventButton *, DDATA *);
static void	 data_unselection_made (GtkCList *, gint, gint, GdkEventButton *, DDATA *);
static gchar	 check_str (gchar *);
static gchar	*check_data (GtkCList *, gchar *);
static gint      match_line (gchar *, gchar *);

static gchar check_str (gchar *str)
{
    gint i, j;
    gchar *a = " %;";

    for (i = 0; i < strlen (str); i++)
        for (j = 0; j < strlen (a); j++)
             if (str[i] == a[j]) 
                 return a[j];
    return 0;
}

gint get_size (GtkCList *clist)
{
    gint i;
    gchar *a[2] = { "", "" };

    i = gtk_clist_append (clist, a);
    gtk_clist_remove (clist, i);
    return i;
}

static gint find_data (GtkCList *list, gchar *text)
{
    gint i;
    gchar *aux;

    for (i = 0; i < get_size(list); i++)
    {
        gtk_clist_get_text (list, i, 0, &aux);
        if (!strcmp (aux, text)) return i;
    }
    return -1;
}

static gchar *check_data (GtkCList *list, gchar *incoming)
{
    gint i = find_data(list, incoming);
    gchar *r = 0;

    if (i != -1) gtk_clist_get_text (list, i, 1, &r);
    return r;
}

gchar *check_actions (GtkCList *list, gchar *incoming)
{
    gint i;
    gchar *str;
  
    for (i=0; i < get_size(list); i++)
    {
        gtk_clist_get_text (list, i, 0, &str);
        if (match_line(str, incoming))
        {
            gtk_clist_get_text (list, i, 1, &str);
            return str;
        }
    }
    return 0;
}

gchar *check_alias (GtkCList *list, gchar *incoming)
{
    return check_data(list, incoming);
}

gchar *check_vars (GtkCList *list, gchar *str)
{
    gchar **var = g_strsplit(str, " ", 0), **c, *r;


    c = var;
    while (*c)
    {
        if (**c == '%' && (r = check_data (list, ((*c)+1))))
        {
            g_free(*c);
            *c = g_strdup(r);
        }
        c++;
    }
    r = g_strjoinv(" ", var);
    g_strfreev(var);
    return r;
}

static gint match_line (gchar *trigger, gchar *incoming)
{
    gint reg_return;
    regex_t preg;  

    regcomp (&preg, (char*) trigger, REG_NOSUB);
    reg_return = regexec (&preg, incoming, 1, NULL, 0);
    regfree (&preg);
  
    if (reg_return == 0) return 1;
    return 0;
}

static void data_selection_made (GtkCList *list, gint row, gint column,
			   GdkEventButton *event, DDATA *data)
{
    gchar *text;
  
    data->row = row;
    gtk_clist_get_text (GTK_CLIST (list), row, 0, &text);
    gtk_entry_set_text (GTK_ENTRY (data->textname), text);
    gtk_clist_get_text (GTK_CLIST (list), row, 1, &text);
    gtk_entry_set_text (GTK_ENTRY (data->textvalue), text);
    gtk_widget_set_sensitive (data->button_delete, TRUE);
}

static void data_unselection_made (GtkCList *list, gint row, gint column,
			     	    GdkEventButton *event, DDATA *data)
{
    data->row = -1;
    gtk_entry_set_text (GTK_ENTRY (data->textname), "");
    gtk_entry_set_text (GTK_ENTRY (data->textvalue), "");
    gtk_widget_set_sensitive (data->button_delete, FALSE);
}

static void data_button_add (GtkWidget *button, DDATA *data)
{
    gchar buf[256], *text[2], a;
  
    text[0] = gtk_entry_get_text (GTK_ENTRY (data->textname));
    text[1] = gtk_entry_get_text (GTK_ENTRY (data->textvalue));

    if (text[0][0] == '\0' || text[1][0] == '\0')
    {
        popup_window (_("No void characters allowed."));
        return;
    }
  
    if ( strcmp(gtk_clist_get_column_title(data->list, 0),_("Actions")) && (a = check_str(text[0])))
    {
        g_snprintf (buf, 255, _("Character '%c' not allowed."), a); 
        popup_window (buf);
        return;
    }

    if (strlen (text[0]) > data->tam_name)
    {
        g_snprintf (buf, 255, _("%s too big."), data->title_name);
        popup_window (buf);
        return;
    }
    
    if (strlen (text[1]) > data->tam_value)
    {
        g_snprintf (buf, 255, _("%s too big."), data->title_value);
        popup_window (buf);
        return;
    }

    if (find_data (data->list, text[0]) != -1)
    {
        g_snprintf (buf, 255, _("Can't duplicate %s."), data->title_name);
        popup_window (buf);
        return;
    }
    gtk_clist_append (data->list, text);

    gtk_widget_set_sensitive (data->button_delete, TRUE);
}

static void data_button_delete (GtkWidget *button, DDATA *data)
{
    if (data->row == -1) return;

    gtk_clist_remove (data->list, data->row);
    gtk_widget_set_sensitive (data->button_delete, FALSE);
}

static void data_button_close (GtkWidget *button, DDATA *data)
{
    gint i;
    gchar *aux[2];

    if (button != data->window)
    {
        gtk_widget_destroy (data->window);
        return;
    }

	switch (data->z)
	{
		case 0: 
			data->pd->alias = (GtkCList *) gtk_clist_new(2);
			break;

		case 1:
			data->pd->triggers = (GtkCList *) gtk_clist_new(2);
			break;

		case 2:
			data->pd->variables = (GtkCList *) gtk_clist_new(2);
			break;
	}
	
    for (i = 0; i < get_size(data->list); i++)
    {
        gtk_clist_get_text (data->list, i, 0, &aux[0]);
        gtk_clist_get_text (data->list, i, 1, &aux[1]);
		switch (data->z)
		{
			case 0: gtk_clist_append(data->pd->alias, aux);		break;
			case 1: gtk_clist_append(data->pd->triggers, aux);	break;
			case 2: gtk_clist_append(data->pd->variables, aux);	break;
		}
    }

    g_free(data->title_name);
    g_free(data->title_value);
    g_free(data);
}

void window_data (PROFILE_DATA *profile, gint z)
{
    GtkWidget *vbox, *a, *b;
    DDATA     *data = g_new0(DDATA, 1);

    switch(z)
    {
		case(0): /* alias */
		   	data->title_name  = g_strdup(_("Alias"));
		   	data->title_value = g_strdup(_("Replacement"));
		   	data->tam_name    = 15;
		   	data->tam_value   = 80;
           	data->file_name   = "aliases";
			data->list		  = profile->alias;
			break;
			
		case(1): /* actions */
		   	data->title_name  = g_strdup(_("Actions"));
	   		data->title_value = g_strdup(_("Triggers"));
	   		data->tam_name    = 80;
	   		data->tam_value   = 80;
           	data->file_name   = "actions";
			data->list		  = profile->triggers;
			break;
			
		case(2): /* vars */
	   		data->title_name  = g_strdup(_("Variables"));
	   		data->title_value = g_strdup(_("Values"));
	   		data->tam_name    = 20;
	   		data->tam_value   = 50;
           	data->file_name   = "vars";
			data->list		  = profile->variables;
			break;
			
		default:
	   		g_warning(_("window_data: trying to access to undefined data range: %d"), z);
			return;
    }
	
    data->z      = z;
    data->row    = -1;
    data->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	data->pd	 = profile;
	
    data->textname  = gtk_entry_new ();
    data->textvalue = gtk_entry_new ();
  
    gtk_window_set_title (GTK_WINDOW (data->window), _("GNOME-Mud Configuration Center"));
    gtk_widget_set_usize (data->window,450,320);

    gtk_clist_set_column_title         (data->list, 0, data->title_name);
    gtk_clist_set_column_title         (data->list, 1, data->title_value);
    gtk_clist_column_titles_passive    (data->list);
    gtk_clist_column_titles_show       (data->list);
    gtk_clist_set_shadow_type          (data->list, GTK_SHADOW_IN);
    gtk_clist_set_column_width         (data->list, 0, 100);
    gtk_clist_set_column_width         (data->list, 1, 250);
    gtk_clist_set_column_justification (data->list, 0, GTK_JUSTIFY_LEFT);
    gtk_clist_set_column_justification (data->list, 1, GTK_JUSTIFY_LEFT);

    vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (vbox), 5);
    gtk_container_add  (GTK_CONTAINER (data->window), vbox);

    a = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (a),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add  (GTK_CONTAINER (a), (GtkWidget *)data->list);
    gtk_box_pack_start (GTK_BOX (vbox), a, TRUE, TRUE, 0);

    a = gtk_hbox_new (TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), a, FALSE, FALSE, 0);

    b = gtk_label_new (data->title_name); 
    gtk_box_pack_start (GTK_BOX (a), b, FALSE, TRUE, 0);

    b = gtk_label_new (data->title_value);
    gtk_box_pack_start (GTK_BOX (a), b, FALSE, TRUE, 0);

    a = gtk_hbox_new (TRUE, 15);
    gtk_box_pack_start (GTK_BOX (vbox), a, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (a), data->textname, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (a), data->textvalue, FALSE, TRUE, 0);

    a = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), a, FALSE, TRUE, 5);

    a = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), a, FALSE, FALSE, 0);

    b = gtk_button_new_with_label (_("Add"));
    gtk_signal_connect (GTK_OBJECT (b), "clicked",
                        GTK_SIGNAL_FUNC (data_button_add), data);
    gtk_box_pack_start (GTK_BOX (a), b, TRUE, TRUE, 15);

    data->button_delete = gtk_button_new_with_label (_("Delete"));
    gtk_box_pack_start (GTK_BOX (a), data->button_delete, TRUE, TRUE, 15);
    gtk_widget_set_sensitive (data->button_delete, FALSE);

    b = gtk_button_new_with_label (_("Close"));
    gtk_signal_connect (GTK_OBJECT (b), "clicked",
                        GTK_SIGNAL_FUNC (data_button_close), data);
    gtk_box_pack_start (GTK_BOX (a), b, TRUE, TRUE, 15);

    gtk_signal_connect (GTK_OBJECT (data->list), "select_row",
                        GTK_SIGNAL_FUNC (data_selection_made), data);
    gtk_signal_connect (GTK_OBJECT (data->list), "unselect_row",
                        GTK_SIGNAL_FUNC (data_unselection_made), data);
    gtk_signal_connect (GTK_OBJECT (data->button_delete), "clicked",
                        GTK_SIGNAL_FUNC (data_button_delete), data);
    gtk_signal_connect (GTK_OBJECT (data->window), "destroy",
                        GTK_SIGNAL_FUNC(data_button_close), data);
    gtk_widget_show_all(data->window);
}
