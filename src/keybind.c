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

#include <gtk/gtk.h>
#include <libintl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>

#include <gdk/gdkkeysyms.h>

#include "gnome-mud.h"

#define _(string) gettext(string)

static char const rcsid[] =
    "$Id$";

/* Local functions */
static void	Command_list_fill(GtkCList *, PROFILE_DATA *);
static void	on_KB_button_add_clicked (GtkButton *, PROFILE_DATA *);
static void	on_KB_button_capt_clicked (GtkButton *, gpointer);
static void	on_KB_button_delete_clicked (GtkButton *, gpointer);
static gboolean	on_capt_entry_key_press_event (GtkWidget *, GdkEventKey *,
					       gpointer);
static void	on_clist_select_row (GtkCList *, gint, gint, GdkEvent *, PROFILE_DATA *);
static void	on_clist_unselect_row (GtkCList *, gint, gint, GdkEvent *,
				       gpointer);
    
extern SYSTEM_DATA  prefs;

gint KB_state;
gint KB_keyv;    

gint bind_list_row_counter  =  0;
gint bind_list_selected_row = -1;


static gboolean on_capt_entry_key_press_event (GtkWidget *widget,
					       GdkEventKey *event,
					       gpointer comm_entry)
{

gint keyv = event->keyval;
gint state = event->state;
keyv = gdk_keyval_to_upper(keyv);

if ((state&12)!=0)
    {
        if (keyv < 65500)
	{
         gtk_widget_grab_focus(GTK_WIDGET(comm_entry));
	 gtk_entry_select_region(GTK_ENTRY(comm_entry),0,
	     GTK_ENTRY(comm_entry)->text_length);
	 gtk_entry_set_position(GTK_ENTRY(comm_entry),0);

	 GTK_WIDGET_UNSET_FLAGS (GTK_ENTRY(widget), GTK_CAN_FOCUS);

         if((state&4)!=0) gtk_entry_append_text(GTK_ENTRY(widget),"Control+");
	 if((state&8)!=0) gtk_entry_append_text(GTK_ENTRY(widget),"Alt+");
	 gtk_entry_append_text(GTK_ENTRY(widget),gdk_keyval_name(keyv));


	 KB_keyv = keyv;
	 KB_state = state;
	} 
    }
    else
    {
        if (keyv > 255 && keyv < 65500) 
	{
	
	 gtk_widget_grab_focus(GTK_WIDGET(comm_entry));
	 gtk_entry_select_region(GTK_ENTRY(comm_entry),0,
	     GTK_ENTRY(comm_entry)->text_length);
	 gtk_entry_set_position(GTK_ENTRY(comm_entry),0);

	 GTK_WIDGET_UNSET_FLAGS (GTK_ENTRY(widget), GTK_CAN_FOCUS);
	
	 gtk_entry_append_text(GTK_ENTRY(widget),gdk_keyval_name(keyv));

	 KB_keyv = keyv;
	 KB_state = state;
	}

    }

  return FALSE;
}

static void on_KB_button_capt_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *capt_entry = gtk_object_get_data(GTK_OBJECT(button), "capt_entry");
	
	gtk_entry_set_text(GTK_ENTRY(capt_entry),"");
	GTK_WIDGET_SET_FLAGS (capt_entry, GTK_CAN_FOCUS);
	gtk_widget_grab_focus(GTK_WIDGET(capt_entry));
}

static void on_KB_button_add_clicked (GtkButton *button, PROFILE_DATA *pd)
{
	GtkWidget *capt_entry = gtk_object_get_data(GTK_OBJECT(button), "capt_entry");
	GtkWidget *comm_entry = gtk_object_get_data(GTK_OBJECT(button), "comm_entry");
	GtkWidget *main_clist = gtk_object_get_data(GTK_OBJECT(button), "clist");
	gchar *list[2];
	gint i = 0;

	list[0] = gtk_entry_get_text(GTK_ENTRY(capt_entry));
	list[1] = gtk_entry_get_text(GTK_ENTRY(comm_entry));

	for(; i<bind_list_row_counter ; i++)
	{
		gchar *text;
		gtk_clist_get_text(GTK_CLIST(main_clist), i, 0, &text);
		
		if (strcmp(list[0],text) == 0)
		{
			popup_window(_("Can't add an existing key."));
			return;
		}
	}

	if (list[0][0] && list[1][0])
	{
		KEYBIND_DATA *tmp = (KEYBIND_DATA *)g_malloc0(sizeof(KEYBIND_DATA));
		tmp->state = KB_state;
		tmp->keyv = KB_keyv;
		tmp->data = g_strdup(list[1]); 
		tmp->next = pd->kd;
		pd->kd = tmp;

		bind_list_row_counter++;
		gtk_clist_prepend(GTK_CLIST(main_clist),list);
		gtk_clist_select_row (GTK_CLIST(main_clist), 0,0); 
	}
	else
	{
		popup_window (_("Incomplete fields."));
	}
}

static void on_KB_button_delete_clicked (GtkButton *button, gpointer clist)
{
	PROFILE_DATA *profile = gtk_object_get_data(button, "profile");
	
    gint i;
    KEYBIND_DATA *tmp = profile->kd, *tmp2 = NULL;

    if(bind_list_selected_row >= 0)
    {
		if (bind_list_selected_row == 0)
    	{
			profile->kd = profile->kd->next;
			g_free(tmp->data);
			g_free(tmp);
		}
		else
		{
			for(i=1;i<bind_list_selected_row;i++, tmp = tmp->next);
				
			tmp2 = tmp->next;
			tmp->next = tmp2->next;
			g_free(tmp2->data);
			g_free(tmp2);
		}
        
		gtk_clist_remove(GTK_CLIST(clist),bind_list_selected_row);
	}
}

static void on_clist_select_row (GtkCList *clist, gint row, gint column, GdkEvent *event, PROFILE_DATA *pd)
{
  	GtkWidget *capt_entry = gtk_object_get_data(GTK_OBJECT(clist), "capt_entry");
	GtkWidget *comm_entry = gtk_object_get_data(GTK_OBJECT(clist), "comm_entry");
    GtkWidget *KB_button_delete = gtk_object_get_data(GTK_OBJECT(clist), "KB_button_delete");
	gchar *text;
    gint i = 0;
    KEYBIND_DATA *scroll = pd->kd;
	

    bind_list_selected_row = row;
    
    gtk_clist_get_text (clist, row, 0, &text);
    gtk_entry_set_text (GTK_ENTRY(capt_entry), text);
    gtk_clist_get_text (clist, row, 1, &text);
    gtk_entry_set_text (GTK_ENTRY(comm_entry), text);
    
    for (; i != row ; i++ ,scroll = scroll->next);
    
    KB_keyv  = scroll->keyv;
    KB_state = scroll->state;
    
    gtk_widget_set_sensitive ( KB_button_delete, TRUE);
}

static void on_clist_unselect_row (GtkCList *clist, gint row, gint column,
				   GdkEvent *event, gpointer user_data)
{
	GtkWidget *KB_button_delete = gtk_object_get_data(GTK_OBJECT(clist), "KB_button_delete");
	
    bind_list_selected_row=-1;
    gtk_widget_set_sensitive ( KB_button_delete, FALSE);

}

static void Command_list_fill(GtkCList *clist, PROFILE_DATA *pd)
{
	KEYBIND_DATA *scroll = pd->kd; 
	gchar *str[2];
	str[0] = g_malloc0(80);
	
	for (; scroll != NULL ; scroll = scroll->next)
	{
		str[0][0] = 0;
		if ((scroll->state)&4) strcat(str[0],"Control+");
		if ((scroll->state)&8) strcat(str[0],"Alt+");
		
		strcat(str[0],gdk_keyval_name(scroll->keyv));
	 
		str[1] = scroll->data;
		gtk_clist_append(clist,str);
	}
	
	g_free(str[0]);
}

void window_keybind (PROFILE_DATA *pd)
{
  static GtkWidget *window_key_bind;
  GtkWidget *vbox;
  GtkWidget *scrolledwindow;
  GtkWidget *clist;
  GtkWidget *label1;
  GtkWidget *label2;
  GtkWidget *hbox2;
  GtkWidget *label3;
  GtkWidget *label4;
  GtkWidget *hbox;
  GtkWidget *hseparator;
  GtkWidget *hbuttonbox;
  GtkWidget *KB_button_capt;
  GtkWidget *KB_button_add;
  GtkWidget *KB_button_close;
  GtkTooltips *tooltips;
  GtkWidget *capt_entry;
  GtkWidget *comm_entry;
  GtkWidget *KB_button_delete;

  if (window_key_bind != NULL) {
    gdk_window_raise(window_key_bind->window);
    gdk_window_show(window_key_bind->window);
    return;
  }

  tooltips = gtk_tooltips_new ();

  window_key_bind = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_usize (window_key_bind, 450, 320);
  gtk_container_set_border_width (GTK_CONTAINER (window_key_bind), 5);
  gtk_window_set_title (GTK_WINDOW (window_key_bind), _("GNOME-Mud Key Binding Center"));
  gtk_window_set_position (GTK_WINDOW (window_key_bind), GTK_WIN_POS_CENTER);
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (window_key_bind), vbox);

  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow);
  gtk_object_set_data_full (GTK_OBJECT (window_key_bind), "scrolledwindow", scrolledwindow,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow);
  gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);
  gtk_widget_set_usize (scrolledwindow, 60, 60);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

  clist = gtk_clist_new (2);
  gtk_widget_ref (clist);
  gtk_widget_show (clist);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), clist);
  gtk_clist_set_column_width (GTK_CLIST (clist), 0, 104);
  gtk_clist_set_column_width (GTK_CLIST (clist), 1, 80);
  gtk_clist_column_titles_show (GTK_CLIST (clist));

  label1 = gtk_label_new (_("Key"));
  gtk_widget_show (label1);
  gtk_clist_set_column_widget (GTK_CLIST (clist), 0, label1);

  label2 = gtk_label_new (_("Command"));
  gtk_widget_show (label2);
  gtk_clist_set_column_widget (GTK_CLIST (clist), 1, label2);

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

  KB_button_add = gtk_button_new_with_label (_("Add"));
  gtk_widget_ref (KB_button_add);
  gtk_object_set_data_full (GTK_OBJECT (window_key_bind), "KB_button_add", KB_button_add,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (KB_button_add);
  gtk_container_add (GTK_CONTAINER (hbuttonbox), KB_button_add);

  KB_button_delete = gtk_button_new_with_label (_("Delete"));
  gtk_widget_show (KB_button_delete);
  gtk_container_add (GTK_CONTAINER (hbuttonbox), KB_button_delete);

  KB_button_close = gtk_button_new_with_label (_("Close"));
  gtk_widget_show (KB_button_close);
  gtk_container_add (GTK_CONTAINER (hbuttonbox), KB_button_close);

  gtk_object_set_data(GTK_OBJECT(clist), "capt_entry", capt_entry);
  gtk_object_set_data(GTK_OBJECT(KB_button_capt), "capt_entry", capt_entry);
  gtk_object_set_data(GTK_OBJECT(KB_button_add), "capt_entry", capt_entry);
  
  gtk_object_set_data(GTK_OBJECT(clist), "comm_entry", comm_entry);
  gtk_object_set_data(GTK_OBJECT(KB_button_add), "comm_entry", comm_entry);
  
  gtk_object_set_data(GTK_OBJECT(clist), "KB_button_delete", KB_button_delete);

  gtk_object_set_data(GTK_OBJECT(KB_button_add), "clist", clist);

  gtk_object_set_data(GTK_OBJECT(KB_button_delete), "profile", pd);
  
  gtk_signal_connect(GTK_OBJECT(window_key_bind), "destroy", gtk_widget_destroyed, &window_key_bind);
  gtk_signal_connect (GTK_OBJECT (clist), "select_row", GTK_SIGNAL_FUNC (on_clist_select_row), pd);
  gtk_signal_connect (GTK_OBJECT (clist), "unselect_row", GTK_SIGNAL_FUNC (on_clist_unselect_row), NULL);
  gtk_signal_connect (GTK_OBJECT (capt_entry), "key_press_event", GTK_SIGNAL_FUNC (on_capt_entry_key_press_event), comm_entry);
  gtk_signal_connect (GTK_OBJECT (KB_button_capt), "clicked", GTK_SIGNAL_FUNC (on_KB_button_capt_clicked), NULL);
  gtk_signal_connect (GTK_OBJECT (KB_button_add), "clicked", GTK_SIGNAL_FUNC (on_KB_button_add_clicked), pd);
  gtk_signal_connect (GTK_OBJECT (KB_button_delete), "clicked", GTK_SIGNAL_FUNC (on_KB_button_delete_clicked), GTK_OBJECT(clist));
  gtk_signal_connect_object(GTK_OBJECT (KB_button_close), "clicked", gtk_widget_destroy, GTK_OBJECT (window_key_bind));
  gtk_signal_connect(GTK_OBJECT (KB_button_close), "clicked", gtk_widget_destroyed, &window_key_bind);

  gtk_object_set_data (GTK_OBJECT (window_key_bind), "tooltips", tooltips);
  
  gtk_widget_set_sensitive ( KB_button_delete, FALSE);

  Command_list_fill(GTK_CLIST(clist), pd);  

  gtk_widget_show(window_key_bind);    
}
