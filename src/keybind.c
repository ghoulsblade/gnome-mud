/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1999-2001 Robin Ericsson <lobbin@localhost.nu>
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gconf/gconf-client.h>
#include <gtk/gtk.h>
#include <libintl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>

#include <gdk/gdkkeysyms.h>

#include "gnome-mud.h"

#define _(string)	gettext(string)

static char const rcsid[] =
    "$Id$";

extern SYSTEM_DATA  prefs;
extern GConfClient	*gconf_client;

gint KB_state;
gint KB_keyv;    

gint bind_list_row_counter  =  0;
gint bind_list_selected_row = -1;

static gchar *keybind_key_to_string(gint state, gint keyval)
{
	gchar *buf = g_malloc0(80);
	if (state&4)	strcat(buf, "Control+");
	if (state&8)	strcat(buf, "Alt+");

	strcat(buf, gdk_keyval_name(keyval));

	return buf;
}

static gboolean keybind_capture_entry_keypress_event (GtkWidget *widget, GdkEventKey *event, gpointer comm_entry)
{
	gint keyv = event->keyval;
	gint state = event->state;
	keyv = gdk_keyval_to_upper(keyv);

	if ((state&12)!=0)
    {
		if (keyv < 65500)
		{
			gchar *buf;
		
			gtk_widget_grab_focus(GTK_WIDGET(comm_entry));
			gtk_entry_select_region(GTK_ENTRY(comm_entry),0, GTK_ENTRY(comm_entry)->text_length);
			gtk_entry_set_position(GTK_ENTRY(comm_entry),0);

			GTK_WIDGET_UNSET_FLAGS (GTK_ENTRY(widget), GTK_CAN_FOCUS);

			buf = keybind_key_to_string(state, keyv);
			gtk_entry_set_text(GTK_ENTRY(widget), buf);
			g_free(buf);

			KB_keyv = keyv;
			KB_state = state;
		} 
	}
	else
	{
		if (keyv > 255 && keyv < 65500) 
		{
			gtk_widget_grab_focus(GTK_WIDGET(comm_entry));
			gtk_entry_select_region(GTK_ENTRY(comm_entry),0, GTK_ENTRY(comm_entry)->text_length);
			gtk_entry_set_position(GTK_ENTRY(comm_entry),0);

			GTK_WIDGET_UNSET_FLAGS (GTK_ENTRY(widget), GTK_CAN_FOCUS);
	
			gtk_entry_append_text(GTK_ENTRY(widget),gdk_keyval_name(keyv));

			KB_keyv = keyv;
			KB_state = state;
		}
    }

	return FALSE;
}

static void keybind_button_capture_clicked_cb (GtkButton *button, gpointer user_data)
{
	GtkWidget *capt_entry = gtk_object_get_data(GTK_OBJECT(button), "capt_entry");
	
	gtk_entry_set_text(GTK_ENTRY(capt_entry),"");
	GTK_WIDGET_SET_FLAGS (capt_entry, GTK_CAN_FOCUS);
	gtk_widget_grab_focus(GTK_WIDGET(capt_entry));
}

static void keybind_gconf_sync(PROFILE_DATA *pd)
{
	KEYBIND_DATA *entry;
	GSList *list = NULL;
	gchar *data, *key = g_strdup_printf("/apps/gnome-mud/profiles/%s/keybinds", pd->name);

	for (entry = pd->kd; entry != NULL; entry = entry->next)
	{
		gchar *buf;

		buf = keybind_key_to_string(entry->state, entry->keyv);
	
		data = g_strjoin("=", buf, entry->data, NULL);
		list = g_slist_append(list, (gpointer) data);
		g_free(buf);
	}

	gconf_client_set_list(gconf_client, key, GCONF_VALUE_STRING, list, NULL);

	g_free(key);
}

static void keybind_button_add_clicked_cb (GtkButton *button, PROFILE_DATA *pd)
{
	KEYBIND_DATA *k;
	GtkWidget *capt_entry = gtk_object_get_data(GTK_OBJECT(button), "capt_entry");
	GtkWidget *comm_entry = gtk_object_get_data(GTK_OBJECT(button), "comm_entry");
	gchar *list[2];

	if (KB_keyv == 0)
	{
		popup_window(_("You must use capture first!"));
		return;
	}

	list[0] = g_strdup( gtk_entry_get_text(GTK_ENTRY(capt_entry)) );
	list[1] = g_strdup( gtk_entry_get_text(GTK_ENTRY(comm_entry)) );

	for (k = pd->kd; k != NULL; k = k->next)
	{
		gchar *buf = keybind_key_to_string(k->state, k->keyv);
		
		if (!g_strncasecmp(list[0], buf, strlen(list[0])))
		{
			popup_window(_("Can't add an existing key."));
		    	g_free (buf);
			goto free;
		}
		g_free (buf);
	}

	if (list[0][0] && list[1][0])
	{
		GtkTreeIter iter;
		KEYBIND_DATA *tmp = (KEYBIND_DATA *)g_malloc0(sizeof(KEYBIND_DATA));
		tmp->state = KB_state;
		tmp->keyv = KB_keyv;
		tmp->data = g_strdup(list[1]); 
		tmp->next = pd->kd;
		pd->kd = tmp;

		gtk_list_store_append(g_object_get_data(G_OBJECT(button), "list-store"), &iter);
		gtk_list_store_set(g_object_get_data(G_OBJECT(button), "list-store"), &iter, 0, list[0], 1, list[1], -1);
		gtk_tree_selection_select_iter(g_object_get_data(G_OBJECT(button), "list-select"), &iter);

		gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(button), "KB_button_delete"), TRUE);

		keybind_gconf_sync(pd);
	}
	else
	{
		popup_window (_("Incomplete fields."));
	}

free:
	g_free(list[0]);
	g_free(list[1]);
}

static void keybind_button_delete_clicked_cb (GtkButton *button, GtkTreeSelection *selection)
{
	PROFILE_DATA *p = g_object_get_data(G_OBJECT(button), "profile");
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *t1;

	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gint i;
		gchar *buf;
		KEYBIND_DATA *k, *kp = NULL;
		
		gtk_tree_model_get(model, &iter, 0, &t1, -1);

		for (i = 0, k = p->kd; k != NULL; k = k->next, i++)
		{
			buf = keybind_key_to_string(k->state, k->keyv);
			
			if (!g_strncasecmp(buf, t1, strlen(buf)))
			{
				KEYBIND_DATA *tmp;
				
				tmp = k;
				
				if (i == 0)
				{
					p->kd = k->next;
				}
			
				if (kp != NULL)
				{
					kp->next = k->next;
				}

				g_free(tmp->data);
				g_free(tmp);

				gtk_list_store_remove(g_object_get_data(G_OBJECT(selection), "list-store"), &iter);
			}
		
			g_free(buf);
			kp = k;
		}
	}

	keybind_gconf_sync(p);
}

static void keybind_selection_made(GtkTreeSelection *selection, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	gchar *t1, *t2;
	
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, 0, &t1, 1, &t2, -1);

		gtk_entry_set_text(GTK_ENTRY(g_object_get_data(G_OBJECT(selection), "capt_entry")), t1);
		gtk_entry_set_text(GTK_ENTRY(g_object_get_data(G_OBJECT(selection), "comm_entry")), t2);
	}

	gtk_widget_set_sensitive(g_object_get_data(G_OBJECT(selection), "KB_button_delete"), TRUE);
}

static void keybind_populate_list_store(GtkListStore *store, KEYBIND_DATA *k)
{
	KEYBIND_DATA *e;
	GtkTreeIter iter;

	for (e = k; e != NULL; e = e->next)
	{
		gchar *buf;

		buf = keybind_key_to_string(e->state, e->keyv);

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, buf, 1, e->data, -1);
	
		g_free(buf);
	}
}

void window_keybind (PROFILE_DATA *pd)
{
	static GtkWidget *window_key_bind;
	GtkWidget *vbox, *a, *tree;
	GtkListStore *store;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;
	GtkCellRenderer *renderer;
	GtkWidget *label3, *label4;
	GtkWidget *hbox2, *hbox;
	GtkWidget *hseparator;
	GtkWidget *hbuttonbox;
	GtkWidget *KB_button_capt, *KB_button_add, *KB_button_close, *KB_button_delete;
	GtkTooltips *tooltips;
	GtkWidget *capt_entry;
	GtkWidget *comm_entry;

	if (window_key_bind != NULL)
	{
		gtk_window_present (GTK_WINDOW (window_key_bind));
		return;
	}

	tooltips = gtk_tooltips_new ();

	window_key_bind = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_usize (window_key_bind, 450, 320);

	gtk_container_set_border_width (GTK_CONTAINER (window_key_bind), 5);
	gtk_window_set_title (GTK_WINDOW (window_key_bind), _("GNOME-Mud Keybinding Center"));
	gtk_window_set_position (GTK_WINDOW (window_key_bind), GTK_WIN_POS_CENTER);
	
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_ref (vbox);
	gtk_widget_show (vbox);
 	gtk_container_add (GTK_CONTAINER (window_key_bind), vbox);

	store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	keybind_populate_list_store(store, pd->kd);

	tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Key"), renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	column = gtk_tree_view_column_new_with_attributes(_("Command"), renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	a = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (a), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_container_add(GTK_CONTAINER(a), tree);
	gtk_box_pack_start (GTK_BOX (vbox), a, TRUE, TRUE, 0);

	hbox2 = gtk_hbox_new (TRUE, 0);
	gtk_widget_ref (hbox2);
	gtk_widget_show (hbox2);
	gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, TRUE, 0);

	label3 = gtk_label_new (_("Bind"));
	gtk_widget_show (label3);
	gtk_box_pack_start (GTK_BOX (hbox2), label3, FALSE, TRUE, 0);

	label4 = gtk_label_new (_("Command"));
	gtk_widget_show (label4);
	gtk_box_pack_start (GTK_BOX (hbox2), label4, FALSE, TRUE, 0);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

	capt_entry = gtk_entry_new_with_max_length (30);
	gtk_widget_show (capt_entry);
	gtk_box_pack_start (GTK_BOX (hbox), capt_entry, FALSE, TRUE, 5);
	GTK_WIDGET_UNSET_FLAGS (capt_entry, GTK_CAN_FOCUS);
	gtk_tooltips_set_tip (tooltips, capt_entry, _("Capture"), NULL);
	gtk_entry_set_editable (GTK_ENTRY (capt_entry), FALSE);

	comm_entry = gtk_entry_new ();
	gtk_widget_show (comm_entry);
	gtk_box_pack_start (GTK_BOX (hbox), comm_entry, TRUE, TRUE, 5);
	gtk_tooltips_set_tip (tooltips, comm_entry, _("Command"), NULL);

	hseparator = gtk_hseparator_new ();
	gtk_widget_show (hseparator);
	gtk_box_pack_start (GTK_BOX (vbox), hseparator, FALSE, TRUE, 0);
	gtk_widget_set_usize (hseparator, -2, 22);

	hbuttonbox = gtk_hbutton_box_new ();
	gtk_widget_show (hbuttonbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbuttonbox, FALSE, FALSE, 0);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox), GTK_BUTTONBOX_SPREAD);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbuttonbox), 0);
	gtk_button_box_set_child_size (GTK_BUTTON_BOX (hbuttonbox), 85, 0);
	gtk_button_box_set_child_ipadding (GTK_BUTTON_BOX (hbuttonbox), 0, 0);

	KB_button_capt = gtk_button_new_with_label (_("Capture"));
	gtk_widget_show (KB_button_capt);
	gtk_container_add (GTK_CONTAINER (hbuttonbox), KB_button_capt);

	KB_button_add = gtk_button_new_from_stock("gtk-add");
	gtk_widget_ref (KB_button_add);
	gtk_object_set_data_full (GTK_OBJECT (window_key_bind), "KB_button_add", KB_button_add, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (KB_button_add);
	gtk_container_add (GTK_CONTAINER (hbuttonbox), KB_button_add);

	KB_button_delete = gtk_button_new_from_stock("gtk-remove");
	gtk_widget_show (KB_button_delete);
	gtk_container_add (GTK_CONTAINER (hbuttonbox), KB_button_delete);

	KB_button_close = gtk_button_new_from_stock("gtk-close");
	gtk_widget_show (KB_button_close);
	gtk_container_add (GTK_CONTAINER (hbuttonbox), KB_button_close);

	gtk_object_set_data(GTK_OBJECT(KB_button_capt), "capt_entry", capt_entry);

	gtk_object_set_data(GTK_OBJECT(KB_button_add), "capt_entry", capt_entry);
	gtk_object_set_data(GTK_OBJECT(KB_button_add), "comm_entry", comm_entry);
	g_object_set_data(G_OBJECT(KB_button_add), "list-store", store);
	g_object_set_data(G_OBJECT(KB_button_add), "list-select", select);
	g_object_set_data(G_OBJECT(KB_button_add), "KB_button_delete", KB_button_delete);

	g_object_set_data(G_OBJECT(select), "capt_entry", capt_entry);
	g_object_set_data(G_OBJECT(select), "comm_entry", comm_entry);
	g_object_set_data(G_OBJECT(select), "KB_button_delete", KB_button_delete);
	g_object_set_data(G_OBJECT(select), "list-store", store);

	gtk_object_set_data(GTK_OBJECT(KB_button_delete), "profile", pd);
  
	gtk_signal_connect (GTK_OBJECT(window_key_bind), "destroy", GTK_SIGNAL_FUNC (gtk_widget_destroyed), &window_key_bind);
	gtk_signal_connect (GTK_OBJECT (capt_entry), "key_press_event", GTK_SIGNAL_FUNC (keybind_capture_entry_keypress_event), comm_entry);
	gtk_signal_connect (GTK_OBJECT (KB_button_capt), "clicked", GTK_SIGNAL_FUNC (keybind_button_capture_clicked_cb), NULL);
	gtk_signal_connect (GTK_OBJECT (KB_button_add), "clicked", GTK_SIGNAL_FUNC (keybind_button_add_clicked_cb), pd);
	gtk_signal_connect_object(GTK_OBJECT (KB_button_close), "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (window_key_bind));
	gtk_signal_connect(GTK_OBJECT (KB_button_close), "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroyed), &window_key_bind);

	g_signal_connect(G_OBJECT(select), "changed", G_CALLBACK(keybind_selection_made), NULL);
	g_signal_connect(G_OBJECT(KB_button_delete), "clicked", G_CALLBACK(keybind_button_delete_clicked_cb), select);

	gtk_object_set_data (GTK_OBJECT (window_key_bind), "tooltips", tooltips);
  
	gtk_widget_set_sensitive ( KB_button_delete, FALSE);
	
	gtk_widget_show_all(window_key_bind);    
}
