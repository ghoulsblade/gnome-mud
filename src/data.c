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

#include <gconf/gconf-client.h>
#include <sys/types.h>
#include <gnome.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>

#include "gnome-mud.h"

static char const rcsid[] =
    "$Id$";

struct own_data {
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

#define DATA_LIST(data)	(data->z == 0 ? data->pd->alias : data->z == 1 ? data->pd->triggers : data->z == 2 ? data->pd->variables : NULL)
#define DATA_PLIST(data)	(data->z == 0 ? &data->pd->alias : data->z == 1 ? &data->pd->triggers : data->z == 2 ? &data->pd->variables : NULL)

extern SYSTEM_DATA  prefs;
extern GConfClient  *gconf_client;

/* Local functions */
gint	 get_size (GtkCList *);
static gint	 find_data (GList *, gchar *);
static void	 data_button_add (GtkWidget *, DDATA *);
static void	 data_button_delete (GtkWidget *, DDATA *);
static void	 data_button_close (GtkWidget *, DDATA *);
static gchar	 check_str (gchar *);
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

static gint find_data (GList *list, gchar *text)
{
	GList *entry;
    gint i = 0;

	for (entry = g_list_first(list); entry != NULL; entry = g_list_next(entry), i++)
	{
		if (!g_ascii_strncasecmp(((gchar **) entry->data)[0], text, strlen(text)))
		{
			return i;
		}
	}

	return -1;
}

static gchar *check_data (GList *list, gchar *incoming)
{
	GList *entry;
    gint i = find_data(list, incoming);

	if (i != -1)
	{
		entry = g_list_nth(list, i);

		return ((gchar **) entry->data)[1];
	}

	return NULL;
}

static void copy_data_pointer (GList **a, GList *b)
{
	*a = b;
}

gchar *check_actions (GList *list, gchar *incoming)
{
 	GList *entry;
	gchar *str;

	for (entry = g_list_first(list); entry != NULL; entry = g_list_next(entry))
	{
		str = ((gchar **) entry->data)[0];

		if (match_line(str, incoming))
		{
			return ((gchar **) entry->data)[1];
		}
	}
 
	return NULL;
}

gchar *check_alias (GList *list, gchar *incoming)
{
    return check_data(list, incoming);
}

gchar *check_vars (GList *list, gchar *str)
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

static void data_sync_gconf(DDATA *data)
{
	GSList *list = NULL, *elist;
	GList  *entry;
	gchar  *ldata;
	gchar  *pname = gconf_escape_key(data->pd->name, -1);
	gchar  *key = g_strdup_printf("/apps/gnome-mud/profiles/%s/%s", pname, data->file_name);

	for (entry = g_list_first(DATA_LIST(data)); entry != NULL; entry = g_list_next(entry))
	{
		ldata = g_strjoin("=", ((gchar **) entry->data)[0], ((gchar **) entry->data)[1], NULL);

		list = g_slist_append(list, (gpointer) ldata);
	}

	gconf_client_set_list(gconf_client, key, GCONF_VALUE_STRING, list, NULL);

	for (elist = g_slist_nth(list, 0); elist != NULL; elist = g_slist_next(elist))
	{
		g_free(elist->data);
	}

	g_slist_free(list);
	g_free(key);
	g_free(pname);
}

static void data_delete_entry(DDATA *data, const gchar *entry, GtkTreeSelection *selection)
{
	GList *lentry;
	GtkTreeIter iter;
	GtkTreeModel *model;

	for (lentry = g_list_first(DATA_LIST(data)); lentry != NULL; lentry = g_list_next(lentry))
	{
		if (!g_ascii_strncasecmp(((gchar **) lentry->data)[0], entry, strlen(entry)))
		{
			copy_data_pointer(DATA_PLIST(data), g_list_remove_link(DATA_LIST(data), lentry));
			g_strfreev(lentry->data);
			g_list_free_1(lentry);

			break;
		}
	}

	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_list_store_remove(g_object_get_data(G_OBJECT(data->button_delete), "list-store"), &iter);
	}
}

static void data_selection_made(GtkTreeSelection *selection, DDATA *data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	gchar *temp, *temp2;

	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, 0, &temp, 1, &temp2, -1);
	
		gtk_entry_set_text(GTK_ENTRY(data->textname), temp);
		gtk_entry_set_text(GTK_ENTRY(data->textvalue), temp2);
	}
}

static void data_button_add (GtkWidget *button, DDATA *data)
{
	GtkTreeIter iter;
    gchar buf[256], *text[3], a;
  
    text[0] = g_strdup( gtk_entry_get_text (GTK_ENTRY (data->textname)) );
    text[1] = g_strdup( gtk_entry_get_text (GTK_ENTRY (data->textvalue)) );
    text[2] = NULL;

    if (text[0][0] == '\0' || text[1][0] == '\0')
    {
        popup_window (_("No void characters allowed."));
		goto free;
    }
  
    if ( strcmp(data->title_name, _("Actions")) && (a = check_str(text[0])))
    {
        g_snprintf (buf, 255, _("Character '%c' not allowed."), a); 
        popup_window (buf);
    	goto free;
    }

    if (strlen (text[0]) > data->tam_name)
    {
        g_snprintf (buf, 255, _("%s too big."), data->title_name);
        popup_window (buf);
    	goto free;
    }
    
    if (strlen (text[1]) > data->tam_value)
    {
        g_snprintf (buf, 255, _("%s too big."), data->title_value);
        popup_window (buf);
    	goto free;
    }

    if (find_data (DATA_LIST(data), text[0]) != -1)
    {
        g_snprintf (buf, 255, _("Can't duplicate %s."), data->title_name);
        popup_window (buf);
    	goto free;
	}

	gtk_list_store_append(g_object_get_data(G_OBJECT(button), "list-store"), &iter);
	gtk_list_store_set(g_object_get_data(G_OBJECT(button), "list-store"), &iter, 0, text[0], 1, text[1], -1);

	gtk_tree_selection_select_iter(g_object_get_data(G_OBJECT(button), "list-select"), &iter);

	copy_data_pointer(DATA_PLIST(data), g_list_append(DATA_LIST(data), (gpointer) g_strdupv(text)));
	data_sync_gconf(data);

free:
	g_free(text[0]);
	g_free(text[1]);
}

static void data_button_delete (GtkWidget *button, DDATA *data)
{
	const gchar *entry = gtk_entry_get_text(GTK_ENTRY(data->textname));

	data_delete_entry(data, entry, g_object_get_data(G_OBJECT(button), "list-select"));
	data_sync_gconf(data);
}

static void data_button_close (GtkWidget *button, DDATA *data)
{
    if (button != data->window)
    {
        gtk_widget_destroy (data->window);
        return;
    }

    g_free(data->title_name);
    g_free(data->title_value);
    g_free(data);
}

static void data_populate_list_store(GtkListStore *store, GList *list)
{
	GtkTreeIter iter;
	GList *entry;

	for (entry = g_list_first(list); entry != NULL; entry = g_list_next(entry))
	{
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, ((gchar **) entry->data)[0], 1, ((gchar **) entry->data)[1], -1);
	}
}

void window_data (PROFILE_DATA *profile, gint z)
{
    GtkWidget *vbox, *a, *b, *tree;
	GtkListStore *store;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;
	GtkCellRenderer *renderer;
    DDATA     *data = g_new0(DDATA, 1);

    switch(z)
    {
		case(0): /* alias */
		   	data->title_name  = g_strdup(_("Alias"));
		   	data->title_value = g_strdup(_("Replacement"));
		   	data->tam_name    = 15;
		   	data->tam_value   = 80;
           	data->file_name   = "aliases";
			break;
			
		case(1): /* actions */
		   	data->title_name  = g_strdup(_("Actions"));
	   		data->title_value = g_strdup(_("Triggers"));
	   		data->tam_name    = 80;
	   		data->tam_value   = 80;
           	data->file_name   = "triggers";
			break;
			
		case(2): /* vars */
	   		data->title_name  = g_strdup(_("Variables"));
	   		data->title_value = g_strdup(_("Values"));
	   		data->tam_name    = 20;
	   		data->tam_value   = 50;
           	data->file_name   = "variables";
			break;
			
		default:
	   		g_warning(_("%s: trying to access to undefined data range: %d"), "window_data", z);
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

	store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	data_populate_list_store(store, DATA_LIST(data));

	tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(data->title_name, renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	column = gtk_tree_view_column_new_with_attributes(data->title_value, renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (vbox), 5);
    gtk_container_add  (GTK_CONTAINER (data->window), vbox);

    a = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (a),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add  (GTK_CONTAINER (a), tree);
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

	b = gtk_button_new_from_stock("gtk-add");
	g_object_set_data(G_OBJECT(b), "list-store", store);
	g_object_set_data(G_OBJECT(b), "list-select", select);
    gtk_signal_connect (GTK_OBJECT (b), "clicked", GTK_SIGNAL_FUNC (data_button_add), data);
    gtk_box_pack_start (GTK_BOX (a), b, TRUE, TRUE, 15);

	data->button_delete = gtk_button_new_from_stock("gtk-remove");
	g_object_set_data(G_OBJECT(data->button_delete), "list-store", store);
	g_object_set_data(G_OBJECT(data->button_delete), "list-select", select);
    gtk_box_pack_start (GTK_BOX (a), data->button_delete, TRUE, TRUE, 15);

	b = gtk_button_new_from_stock("gtk-close");
    gtk_signal_connect (GTK_OBJECT (b), "clicked", GTK_SIGNAL_FUNC (data_button_close), data);
    gtk_box_pack_start (GTK_BOX (a), b, TRUE, TRUE, 15);
	
	g_signal_connect(G_OBJECT(select), "changed", G_CALLBACK(data_selection_made), data);
    gtk_signal_connect (GTK_OBJECT (data->button_delete), "clicked", GTK_SIGNAL_FUNC (data_button_delete), data);
    gtk_signal_connect (GTK_OBJECT (data->window), "destroy", GTK_SIGNAL_FUNC(data_button_close), data);
    gtk_widget_show_all(data->window);
}
