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
#include <ctype.h>
#include <gnome.h>

#include "gnome-mud.h"

static char const rcsid[] =
    "$Id$";

WIZARD_DATA2 *connections_find_by_profilename(gchar *name);
gint 		  get_size(GtkCList *clist);

GList     *ProfilesData;
GList     *Profiles;
gint	   selected, pselected;
gint	   connection_selected;

GtkWidget *main_clist;

extern GConfClient	*gconf_client;

static void profile_gconf_sync_list()
{
	GList *entry;
	GSList *list = NULL;

	for (entry = g_list_first(ProfilesData); entry != NULL; entry = g_list_next(entry))
	{
		list = g_slist_append(list, ((PROFILE_DATA *) entry->data)->name);
	}

	gconf_client_set_list(gconf_client, "/apps/gnome-mud/profiles/list", GCONF_VALUE_STRING, list, NULL);
}

static void profile_gconf_sync_connection_list()
{
	GList *entry;
	GSList *list = NULL;

	for (entry = g_list_first(Profiles); entry != NULL; entry = g_list_next(entry))
	{
		WIZARD_DATA2 *wd = (WIZARD_DATA2 *) entry->data;

		if (strlen(wd->playername) > 0)
		{
			gchar buf[1024];
			
			g_snprintf(buf, 1024, "%s-at-%s", wd->playername, wd->name);
			list = g_slist_append(list, buf);
		}
		else
		{
			list = g_slist_append(list, wd->name);
		}
	}

	gconf_client_set_list(gconf_client, "/apps/gnome-mud/connections/list", GCONF_VALUE_STRING, list, NULL);
}

static void profile_gconf_sync_wizard(WIZARD_DATA2 *wd)
{
	gchar buf[1024] = "";
	gchar *cname, *pname;
	gchar *bufn;

	cname = gconf_escape_key(wd->name, -1);
	pname = gconf_escape_key(wd->playername, -1);

	if (strlen(wd->playername) > 0)
	{
		bufn = g_strdup_printf("%s-at-%s", pname, cname);
	}
	else
	{
		bufn = g_strdup(cname);
	}

#define SYNC(Name, Value) \
	g_snprintf(buf, 1024, "/apps/gnome-mud/connections/%s/%s", bufn, Name); \
	gconf_client_set_string(gconf_client, buf, Value, NULL);

	SYNC("name", wd->name);
	SYNC("hostname", wd->hostname);
	SYNC("port", wd->port);
	SYNC("playername", wd->playername);
	SYNC("password", wd->password);
	SYNC("profile", wd->profile);
#undef SYNC

	g_free(bufn);
	g_free(pname);
	g_free(cname);
}

static KEYBIND_DATA *profile_load_keybinds(gchar *profile_name)
{
	KEYBIND_DATA *k = NULL;
	GSList *slist, *lentry;
	gchar pname[255];

	snprintf(pname, 255, "/apps/gnome-mud/profiles/%s/keybinds", profile_name);
	slist = gconf_client_get_list(gconf_client, pname, GCONF_VALUE_STRING, NULL);
	for (lentry = slist; lentry != NULL; lentry = g_slist_next(lentry))
	{
		KEYBIND_DATA *kd = (KEYBIND_DATA *) g_malloc0(sizeof(KEYBIND_DATA));
		int KB_state = 0;
		gchar **a, *keyv_begin = NULL;
			
		a = g_strsplit((gchar *) lentry->data, "=", 2);

		if (strstr(a[0], "Control"))	KB_state |= 4;
		if (strstr(a[0], "Alt"))		KB_state |= 8;

		keyv_begin = a[0] + strlen(a[0]) - 2;

		while(!(keyv_begin == (a[0]-1) || keyv_begin[0] == '+')) keyv_begin--;
		keyv_begin++;

		kd->state	= KB_state;
		kd->keyv	= gdk_keyval_from_name(keyv_begin);
		kd->data	= g_strdup(a[1]);
		kd->next	= k;
		k 			= kd;

		g_strfreev(a);
	}

	return k;
}

static GList *profile_load_list(gchar *pn, gchar *pt)
{
	GList *list = NULL;
	GSList *slist;
	GSList *lentry;
	gchar pname[255];

	snprintf(pname, 255, "/apps/gnome-mud/profiles/%s/%s", pn, pt);
	slist = gconf_client_get_list(gconf_client, pname, GCONF_VALUE_STRING, NULL);
	for (lentry = slist; lentry != NULL; lentry = g_slist_next(lentry))
	{
		gchar **a;

		a = g_strsplit((gchar *) lentry->data, "=", 2);

		list = g_list_append(list, (gpointer) a);
	}

	return list;
}

void load_profiles()
{
	/*
	 * Profiles
	 */
	GSList *profiles, *connections;
	GSList *entry;
		
	profiles = gconf_client_get_list(gconf_client, "/apps/gnome-mud/profiles/list", GCONF_VALUE_STRING, NULL);

	for (entry = profiles; entry != NULL; entry = g_slist_next(entry))
	{
		gchar *pname;
		PROFILE_DATA *pd = g_malloc0(sizeof(PROFILE_DATA));

		pd->name = g_strdup((gchar *) entry->data);
		pname = gconf_escape_key(pd->name, -1);
		pd->alias = profile_load_list(pname, "aliases");
		pd->variables = profile_load_list(pname, "variables");
		pd->triggers = profile_load_list(pname, "triggers" );

		pd->kd = profile_load_keybinds(pname);
	
		ProfilesData = g_list_append(ProfilesData, (gpointer) pd);

		g_free(pname);
	}

	/*
	 * Wizard
	 */
	connections = gconf_client_get_list(gconf_client, "/apps/gnome-mud/connections/list", GCONF_VALUE_STRING, NULL);

#define LOAD(Name, Value) \
		g_snprintf(buf, 1024, "/apps/gnome-mud/connections/%s/%s", wname, Name); \
		Value = g_strdup(gconf_client_get_string(gconf_client, buf, NULL));

	for (entry = connections; entry != NULL; entry = g_slist_next(entry))
	{
		WIZARD_DATA2 *wd = g_malloc0(sizeof(WIZARD_DATA2));
		gchar buf[1024];
		gchar *wname;

		wname = gconf_escape_key((gchar *) entry->data, -1);
	
		LOAD("name", wd->name);
		LOAD("hostname", wd->hostname);
		LOAD("port", wd->port);
		LOAD("playername", wd->playername);
		LOAD("password", wd->password);
		LOAD("profile", wd->profile);
		
		Profiles = g_list_append(Profiles, (gpointer) wd);

		g_free(wname);
	}

#undef LOAD
}

PROFILE_DATA *profiledata_find(gchar *profile)
{
	GList *t;

	for (t = g_list_first(ProfilesData); t != NULL; t = t->next)
	{
		if (!g_strcasecmp(profile, ((PROFILE_DATA *)t->data)->name))
		{
			return ((PROFILE_DATA *) t->data);
		}
	}

	return NULL;
}

static void profilelist_new_profile(const gchar *string, gpointer data)
{
	GList *t;
	PROFILE_DATA *pd;
	gchar *pdir, *key, *text[2];
	gchar *pname = gconf_escape_key(string, -1);

	if (string == NULL)
	{
		return;
	}
	
	for (t = g_list_first(ProfilesData); t != NULL; t = g_list_next(t))
	{
		if (!g_strcasecmp(string, ((PROFILE_DATA *) t->data)->name))
		{
			return;
		}
	}

	pdir = gconf_concat_dir_and_key("/apps/gnome-mud/profiles", pname);

#define SET_LIST(name) \
	key = gconf_concat_dir_and_key(pdir, name); \
	gconf_client_set_list(gconf_client, key, GCONF_VALUE_STRING, NULL, NULL); \
	g_free(key)

	SET_LIST("aliases");
	SET_LIST("variables");
	SET_LIST("triggers");
	SET_LIST("keybinds");

	text[0] = g_strdup(string);
	gtk_clist_append(GTK_CLIST(data), text);

	pd = g_malloc0(sizeof(PROFILE_DATA));
	pd->name = g_strdup(string);
	ProfilesData = g_list_append(ProfilesData, (gpointer) pd);

	profile_gconf_sync_list();

	g_free(text[0]);
	g_free(pdir);
	g_free(pname);
}

static void profilelist_new_cb(GtkWidget *widget, gpointer data)
{
	GtkWidget *window = widget->parent->parent->parent;
	GtkWidget *label;
	GtkWidget *entry;
	gint       retval;
	
	GtkWidget *dialog = gnome_dialog_new(N_("New profile"), GNOME_STOCK_BUTTON_OK, GNOME_STOCK_BUTTON_CANCEL, NULL);
	gnome_dialog_set_parent(GNOME_DIALOG(dialog), GTK_WINDOW(window));

	label = gtk_label_new(_("Name of new profile:"));
	gtk_widget_show(label);

	entry = gtk_entry_new();
	gtk_widget_show(entry);

	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox), entry, FALSE, FALSE, 0);

	retval = gnome_dialog_run(GNOME_DIALOG(dialog));

	if (retval == 0)
	{
		profilelist_new_profile(gtk_entry_get_text(GTK_ENTRY(entry)), data);
	}

	if (retval != -1)
	{
		gnome_dialog_close(GNOME_DIALOG(dialog));
	}
}

static void profilelist_delete_cb(GtkWidget *widget, gpointer data)
{
	if (selected != -1)
	{
		gchar *name;

		gtk_clist_get_text(GTK_CLIST(data), selected, 0, &name);

		if (!g_strcasecmp(name, "Default"))
		{
			g_message("can't delete default profile");
			return;
		}
		
		if (connections_find_by_profilename(name) == NULL)
		{
			GList *remove;

			for (remove = g_list_first(ProfilesData); remove != NULL; remove = g_list_next(remove))
			{
				if (!g_strncasecmp(name, ((PROFILE_DATA *) remove->data)->name, strlen(name)))
				{
					ProfilesData = g_list_remove_link(ProfilesData, remove);
					
					g_free(((PROFILE_DATA *) remove->data)->name);
					g_free(remove->data);
					g_list_free_1(remove);
					break;
				}
			}

			gtk_clist_remove(GTK_CLIST(data), selected);
		}
		else
		{
			g_message("this profile is in use");
		}
	}

	profile_gconf_sync_list();
}

static void profilelist_alias_cb(GtkWidget *widget, gpointer data)
{
	PROFILE_DATA *pd;

	if (selected != -1)
	{
		gchar *name;

		gtk_clist_get_text(GTK_CLIST(data), selected, 0, &name);

		pd = profiledata_find(name);
		if (pd != NULL)
		{
			window_data(pd, 0);
		}
	}
}

static void profilelist_keybinds_cb(GtkWidget *widget, gpointer data)
{
	PROFILE_DATA *pd;

	if (selected != -1)
	{
		gchar *name;

		gtk_clist_get_text(GTK_CLIST(data), selected, 0, &name);

		pd = profiledata_find(name);
		if (pd != NULL)
		{
			window_keybind(pd);
		}
	}
}

static void profilelist_triggers_cb(GtkWidget *widget, gpointer data)
{
	PROFILE_DATA *pd;

	if (selected != -1)
	{
		gchar *name;

		gtk_clist_get_text(GTK_CLIST(data), selected, 0, &name);

		pd = profiledata_find(name);
		if (pd != NULL)
		{
			window_data(pd, 1);
		}
	}
}

static void profilelist_variables_cb(GtkWidget *widget, gpointer data)
{
	PROFILE_DATA *pd;

	if (selected != -1)
	{
		gchar *name;

		gtk_clist_get_text(GTK_CLIST(data), selected, 0, &name);

		pd = profiledata_find(name);
		if (pd != NULL)
		{
			window_data(pd, 2);
		}
	}
}

static void profilelist_button_ok_cb(GtkWidget *widget, GtkWidget *label)
{
	GtkWidget *clist = gtk_object_get_data(GTK_OBJECT(widget), "clist");
	gchar *text = NULL;

	gtk_clist_get_text(GTK_CLIST(clist), pselected, 0, &text);
	gtk_label_set_text(GTK_LABEL(label), g_strdup(text));
}

static void profilelist_unselect_row_cb(GtkCList *clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
	switch (GPOINTER_TO_INT(data))
	{
		case 0:
			selected = -1;
			break;

		case 1:
			pselected = -1;
			break;
	}
}

static void profilelist_select_row_cb(GtkCList *clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
	switch (GPOINTER_TO_INT(data))
	{
		case 0:
			selected = row;
			break;

		case 1:
			pselected = row;
			break;
	}
}

static void profilelist_clist_fill(PROFILE_DATA *pd, GtkCList *clist)
{
	gchar *text[1];
	
	text[0] = pd->name;
	gtk_clist_append(clist, text);
}

static void profilelist_cleanup (GtkWidget *widget, GtkWidget *data)
{
	if (GTK_IS_WIDGET(data))
	{
		gtk_widget_destroy(data);
	}
}

static void profilelist_dialog (GtkWidget *label)
{
	static GtkWidget *dialog;
	GtkWidget *dialog_vbox1;
	GtkWidget *scrolledwindow;
	GtkWidget *dialog_action_area;
	GtkWidget *button_ok;
	GtkWidget *button_cancel;
	GtkWidget *clist;

	if (dialog != NULL)
	{
		gtk_window_present (GTK_WINDOW (dialog));
		return;
	}

	selected = -1;
	
	dialog = gnome_dialog_new (NULL, NULL);
	gtk_object_set_data (GTK_OBJECT (dialog), "dialog", dialog);
	gtk_window_set_policy (GTK_WINDOW (dialog), FALSE, FALSE, FALSE);
	gtk_window_set_title (GTK_WINDOW(dialog), _("GNOME-Mud: Profilelist"));

	dialog_vbox1 = GNOME_DIALOG (dialog)->vbox;
	gtk_object_set_data (GTK_OBJECT (dialog), "dialog_vbox1", dialog_vbox1);
	gtk_widget_show (dialog_vbox1);

	scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_ref (scrolledwindow);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "scrolledwindow", scrolledwindow, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (scrolledwindow);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), scrolledwindow, TRUE, TRUE, 0);
	gtk_widget_set_usize (scrolledwindow, -2, 100);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	clist = gtk_clist_new (1);
	gtk_widget_ref (clist);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "clist", clist, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (clist);
	gtk_container_add (GTK_CONTAINER (scrolledwindow), clist);
	gtk_clist_set_column_width (GTK_CLIST (clist), 0, 80);
	gtk_clist_column_titles_hide (GTK_CLIST (clist));
	gtk_clist_set_shadow_type (GTK_CLIST (clist), GTK_SHADOW_ETCHED_IN);
	g_list_foreach(ProfilesData, (GFunc) profilelist_clist_fill, (gpointer) clist);

	dialog_action_area = GNOME_DIALOG (dialog)->action_area;
	gtk_object_set_data (GTK_OBJECT (dialog), "dialog_action_area", dialog_action_area);
	gtk_widget_show (dialog_action_area);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area), 8);

	gnome_dialog_append_button (GNOME_DIALOG (dialog), GNOME_STOCK_BUTTON_OK);
	button_ok = GTK_WIDGET (g_list_last (GNOME_DIALOG (dialog)->buttons)->data);
	gtk_widget_ref (button_ok);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "button_ok", button_ok, (GtkDestroyNotify) gtk_widget_unref);
	gtk_object_set_data(GTK_OBJECT(button_ok), "clist", clist);
	gtk_widget_show (button_ok);
	GTK_WIDGET_SET_FLAGS (button_ok, GTK_CAN_DEFAULT);
	gnome_dialog_append_button (GNOME_DIALOG (dialog), GNOME_STOCK_BUTTON_CANCEL);
	
	button_cancel = GTK_WIDGET (g_list_last (GNOME_DIALOG (dialog)->buttons)->data);
	gtk_widget_ref (button_cancel);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "button_cancel", button_cancel, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (button_cancel);
	GTK_WIDGET_SET_FLAGS (button_cancel, GTK_CAN_DEFAULT);

	gtk_signal_connect(GTK_OBJECT(clist), "select-row",   GTK_SIGNAL_FUNC(profilelist_select_row_cb), GINT_TO_POINTER(1));
	gtk_signal_connect(GTK_OBJECT(clist), "unselect-row", GTK_SIGNAL_FUNC(profilelist_unselect_row_cb), GINT_TO_POINTER(1));

	gtk_signal_connect(GTK_OBJECT(button_ok), "clicked", GTK_SIGNAL_FUNC(profilelist_button_ok_cb), (gpointer) label);
	gtk_signal_connect_object(GTK_OBJECT(button_ok), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer) dialog);
	gtk_signal_connect_object(GTK_OBJECT(button_cancel), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer) dialog);

	gtk_signal_connect(GTK_OBJECT(label->parent->parent), "destroy", GTK_SIGNAL_FUNC(profilelist_cleanup), (gpointer) dialog);
	gtk_signal_connect(GTK_OBJECT(dialog), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &dialog);
	
	gtk_widget_show(dialog);
}

WIZARD_DATA2 *connections_find(const gchar *name, const gchar *character)
{
	GList *t;

	for (t = g_list_first(Profiles); t != NULL; t = t->next)
	{
		if (!g_strcasecmp(name, ((WIZARD_DATA2 *)t->data)->name) && !g_strcasecmp(character, ((WIZARD_DATA2 *)t->data)->playername))
		{
			return ((WIZARD_DATA2 *)t->data);
		}
	}

	return NULL;
}

WIZARD_DATA2 *connections_find_by_profilename(gchar *name)
{
	GList *t;

	for (t = g_list_first(Profiles); t != NULL; t = t->next)
	{
		if (!g_strcasecmp(name, ((WIZARD_DATA2 *)t->data)->profile))
		{
			return ((WIZARD_DATA2 *)t->data);
		}
	}

	return NULL;
}

static void connections_entry_info_mud_port_cb(GtkEditable *editable, gchar *new_text, gint new_text_length, gint position, gpointer user_data)
{
	if (!isdigit(new_text[0]) || strlen(gtk_entry_get_text(GTK_ENTRY(editable))) > 4)
	{
		gtk_signal_emit_stop_by_name(GTK_OBJECT(editable), "insert-text");
	}
}

static void connections_button_info_prof_cb(GtkButton *button, GtkWidget *label)
{
	profilelist_dialog(label);
}

static void connections_button_info_apply_cb(GtkButton *button, GtkCList *clist)
{
	gchar *text[2];
	gchar *label;
	const gchar *name  = gtk_entry_get_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(button), "entry_info_mud_title")));
	const gchar *charc = gtk_entry_get_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(button), "entry_info_char_character")));
	WIZARD_DATA2 *wd = connections_find(name, charc);

	gboolean new_connection = TRUE;

	if (strlen(name) < 1)
	{
		/*
		 * No connection name
		 */
		return;
	}
	
	/*
	 * FIXME, need some more error checking here
	 */
	if (wd == NULL)
	{
		wd = g_malloc0(sizeof(WIZARD_DATA2));
	}
	else
	{
		g_free(wd->name);
		g_free(wd->hostname);
		g_free(wd->port);
		g_free(wd->playername);
		g_free(wd->password);

		new_connection = FALSE;
	}

	wd->name 		= g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(button), "entry_info_mud_title"))));
	wd->hostname	= g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(button), "entry_info_mud_host"))));
	wd->port		= g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(button), "entry_info_mud_port"))));
	wd->playername	= g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(button), "entry_info_char_character"))));
	wd->password	= g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(button), "entry_info_char_password"))));

	gtk_label_get(GTK_LABEL(gtk_object_get_data(GTK_OBJECT(button), "label_info_prof")), &label);
	wd->profile		= g_strdup(label);

	if (new_connection)
	{
		text[0] = g_strdup(name);
		text[1] = g_strdup(charc);
		gtk_clist_select_row(clist, gtk_clist_append(clist, text), 0);

		Profiles = g_list_append(Profiles, (gpointer) wd);

		g_free(text[0]);
		g_free(text[1]);
	}

	profile_gconf_sync_wizard(wd);
	profile_gconf_sync_connection_list();
}

static void connections_fields_clean(GtkWidget *widget)
{
	gtk_entry_set_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(widget), "entry_info_mud_title")),      "");
	gtk_entry_set_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(widget), "entry_info_mud_host")),       "");
	gtk_entry_set_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(widget), "entry_info_mud_port")),       "");
	gtk_entry_set_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(widget), "entry_info_char_character")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(widget), "entry_info_char_password")),  "");

	gtk_label_set_text(GTK_LABEL(gtk_object_get_data(GTK_OBJECT(widget), "label_info_prof")), _("Default"));
}

static void connections_button_info_cancel_cb(GtkWidget *button, gpointer data)
{
	connections_fields_clean(button);
}

static void connections_button_info_fetch_cb(GtkWidget *button, gpointer data)
{
	connections_fields_clean(button);

	window_mudlist(button, TRUE);
}

static void connections_button_connect_cb(GtkButton *button, gpointer data)
{
	if (connection_selected != -1)
	{
		gchar *name, *player;
		WIZARD_DATA2 *wd;
		CONNECTION_DATA *cd;

		gtk_clist_get_text(GTK_CLIST(main_clist), connection_selected, 0, &name);
		gtk_clist_get_text(GTK_CLIST(main_clist), connection_selected, 1, &player);

		wd = connections_find(name, player);

		cd = make_connection(wd->hostname, wd->port, wd->profile);

		if (cd && cd->connected)
		{
			if (strlen(wd->playername) > 0 && strlen(wd->password) > 0)
			{
				connection_send(cd, wd->playername);
				connection_send(cd, "\n");
				connection_send_secret(cd, wd->password);
				connection_send(cd, "\n");
			}

			gtk_widget_destroy((GtkWidget *) data);
		}
	}
}

static void connections_delete_menu_cb(GtkButton *button, gpointer data)
{
	if (connection_selected != -1)
	{
		WIZARD_DATA2 *wd;
		gchar *name, *character;
		gtk_clist_get_text(GTK_CLIST(main_clist), connection_selected, 0, &name);
		gtk_clist_get_text(GTK_CLIST(main_clist), connection_selected, 1, &character);
	
		wd = connections_find(name, character);
		Profiles = g_list_remove(Profiles, (gpointer) wd);

		gtk_clist_remove(GTK_CLIST(main_clist), connection_selected);
		profile_gconf_sync_connection_list();
	}
}

static void connections_clist_fill(WIZARD_DATA2 *wd, GtkCList *clist)
{
	gchar *text[2];
	
	text[0] = wd->name;
	text[1] = wd->playername;

	gtk_clist_append(clist, text);
}

static void connections_select_row_cb(GtkCList *clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
	gchar *name, *character;
	WIZARD_DATA2 *wd;

	gtk_clist_get_text(clist, row, 0, &name);
	gtk_clist_get_text(clist, row, 1, &character);

	connection_selected = row;

	wd = connections_find(name, character);
	if (wd != NULL)
	{
		gtk_entry_set_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(clist), "entry_info_mud_title")), 		wd->name);
		gtk_entry_set_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(clist), "entry_info_mud_host")),  		wd->hostname);
		gtk_entry_set_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(clist), "entry_info_mud_port")),			wd->port);
		gtk_entry_set_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(clist), "entry_info_char_character")),	wd->playername);
		gtk_entry_set_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(clist), "entry_info_char_password")),		wd->password);

		gtk_label_set_text(GTK_LABEL(gtk_object_get_data(GTK_OBJECT(clist), "label_info_prof")), wd->profile);
	}
}

static void connections_unselect_row_cb(GtkCList *clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
	connection_selected = -1;
}

static gboolean connections_button_press_cb(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	gint  row, col;
	gchar *row_name = NULL;

	switch (event->type)
	{
		case GDK_2BUTTON_PRESS:
			gtk_clist_get_selection_info(GTK_CLIST(widget), event->x, event->y, &row, &col);
			if (gtk_clist_get_text(GTK_CLIST(widget), row, col, &row_name))
			{
				WIZARD_DATA2 *wd;
				CONNECTION_DATA *cd;

				gchar *player = NULL;

				gtk_clist_get_text(GTK_CLIST(widget), row, 1, &player);

				wd = connections_find(row_name, player);
				cd = make_connection(wd->hostname, wd->port, wd->profile);

				if (cd && cd->connected)
				{
					if (strlen(wd->playername) > 0 && strlen(wd->password) > 0)
					{
						connection_send(cd, wd->playername);
						connection_send(cd, "\n");
						connection_send_secret(cd, wd->password);
						connection_send(cd, "\n");
					}

					gtk_widget_destroy((GtkWidget *) data);
				}

			}
			break;

		default:
			break;
	}

	return FALSE;
}

void window_profiles(void)
{
	static GtkWidget *dialog;
	GtkWidget *dialog_vbox1;
	GtkWidget *main_hbox;
	GtkWidget *scrolledwindow;
	GtkWidget *label1;
	GtkWidget *label2;
	GtkWidget *main_vbox;
	GtkWidget *frame_info_mud;
	GtkWidget *table_info_mud;
	GtkWidget *label_info_mud_title;
	GtkWidget *label_info_mud_host;
	GtkWidget *label_info_mud_port;
	GtkWidget *frame_info_char;
	GtkWidget *table2;
	GtkWidget *label_info_char_character;
	GtkWidget *label_info_char_password;
	GtkWidget *frame_info_prof;
	GtkWidget *table3;
	GtkWidget *button_info_prof;
	GtkWidget *table_buttons;
	GtkWidget *button_info_apply;
	GtkWidget *button_info_cancel;
	GtkWidget *button_info_fetch;
	GtkWidget *dialog_action_area;
	GtkWidget *button_connect;
	GtkWidget *button_cancel;
	GtkWidget *extra_menu;
	GtkWidget *entry_info_mud_title;
	GtkWidget *entry_info_mud_host;
	GtkWidget *entry_info_mud_port;
	GtkWidget *entry_info_char_character;
	GtkWidget *entry_info_char_password;
	GtkWidget *label_info_prof;

	static GnomeUIInfo rightbutton_menu[] = {
		GNOMEUIINFO_ITEM_STOCK(N_("Delete"),	NULL,	connections_delete_menu_cb,		GNOME_STOCK_MENU_CLOSE),
		GNOMEUIINFO_END
	};

	if (dialog != NULL)
	{
		gtk_window_present (GTK_WINDOW (dialog));
		return;
	}
 
	connection_selected = -1;
	extra_menu = gnome_popup_menu_new(rightbutton_menu);
	
	dialog = gnome_dialog_new (NULL, NULL);
	gtk_object_set_data (GTK_OBJECT (dialog), "dialog", dialog);
	gtk_window_set_policy (GTK_WINDOW (dialog), FALSE, FALSE, FALSE);
	gtk_window_set_title(GTK_WINDOW(dialog), _("GNOME-Mud Connections"));
	
	dialog_vbox1 = GNOME_DIALOG (dialog)->vbox;
	gtk_object_set_data (GTK_OBJECT (dialog), "dialog_vbox1", dialog_vbox1);
	gtk_widget_show (dialog_vbox1);

	main_hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_ref (main_hbox);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "main_hbox", main_hbox, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (main_hbox);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), main_hbox, TRUE, TRUE, 0);

	scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_ref (scrolledwindow);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "scrolledwindow", scrolledwindow, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (scrolledwindow);
	gtk_box_pack_start (GTK_BOX (main_hbox), scrolledwindow, FALSE, TRUE, 0);
	gtk_widget_set_usize (scrolledwindow, 200, -2);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	main_clist = gtk_clist_new (2);
	gtk_widget_ref (main_clist);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "main_clist", main_clist, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (main_clist);
	gtk_container_add (GTK_CONTAINER (scrolledwindow), main_clist);
	gtk_clist_set_column_width (GTK_CLIST (main_clist), 0, 98);
	gtk_clist_set_column_width (GTK_CLIST (main_clist), 1, 80);
	gtk_clist_column_titles_show (GTK_CLIST (main_clist));
	g_list_foreach(Profiles, (GFunc) connections_clist_fill, main_clist);
	
	gtk_signal_connect(GTK_OBJECT(main_clist), "select-row", 			GTK_SIGNAL_FUNC(connections_select_row_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(main_clist), "unselect-row", 			GTK_SIGNAL_FUNC(connections_unselect_row_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(main_clist), "button_press_event", 	GTK_SIGNAL_FUNC(connections_button_press_cb), dialog);
	
	label1 = gtk_label_new (_("Mud"));
	gtk_widget_ref (label1);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "label1", label1, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label1);
	gtk_clist_set_column_widget (GTK_CLIST (main_clist), 0, label1);

	/* Translators: this is the name of your player */
	label2 = gtk_label_new (_("Character"));
	gtk_widget_ref (label2);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "label2", label2, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label2);
	gtk_clist_set_column_widget (GTK_CLIST (main_clist), 1, label2);
	gtk_label_set_justify (GTK_LABEL (label2), GTK_JUSTIFY_LEFT);

	main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_ref (main_vbox);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "main_vbox", main_vbox, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (main_vbox);
	gtk_box_pack_start (GTK_BOX (main_hbox), main_vbox, TRUE, TRUE, 0);

	frame_info_mud = gtk_frame_new (_("Mud information"));
	gtk_widget_ref (frame_info_mud);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "frame_info_mud", frame_info_mud, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (frame_info_mud);
	gtk_box_pack_start (GTK_BOX (main_vbox), frame_info_mud, FALSE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame_info_mud), 4);

	table_info_mud = gtk_table_new (2, 4, FALSE);
	gtk_widget_ref (table_info_mud);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "table_info_mud", table_info_mud, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (table_info_mud);
	gtk_container_add (GTK_CONTAINER (frame_info_mud), table_info_mud);
	gtk_container_set_border_width (GTK_CONTAINER (table_info_mud), 3);
	gtk_table_set_row_spacings (GTK_TABLE (table_info_mud), 2);
	gtk_table_set_col_spacings (GTK_TABLE (table_info_mud), 3);

	label_info_mud_title = gtk_label_new (_("Title:"));
	gtk_widget_ref (label_info_mud_title);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "label_info_mud_title", label_info_mud_title, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label_info_mud_title);
	gtk_table_attach (GTK_TABLE (table_info_mud), label_info_mud_title, 0, 1, 0, 1, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_info_mud_title), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (label_info_mud_title), 4, 0);

	label_info_mud_host = gtk_label_new (_("Host:"));
	gtk_widget_ref (label_info_mud_host);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "label_info_mud_host", label_info_mud_host, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label_info_mud_host);
	gtk_table_attach (GTK_TABLE (table_info_mud), label_info_mud_host, 0, 1, 1, 2, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_info_mud_host), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (label_info_mud_host), 4, 0);

	entry_info_mud_title = gtk_entry_new ();
	gtk_widget_ref (entry_info_mud_title);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "entry_info_mud_title", entry_info_mud_title, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (entry_info_mud_title);
	gtk_table_attach (GTK_TABLE (table_info_mud), entry_info_mud_title, 1, 4, 0, 1, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);

	entry_info_mud_host = gtk_entry_new ();
	gtk_widget_ref (entry_info_mud_host);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "entry_info_mud_host", entry_info_mud_host, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (entry_info_mud_host);
	gtk_table_attach (GTK_TABLE (table_info_mud), entry_info_mud_host, 1, 2, 1, 2, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);

	label_info_mud_port = gtk_label_new (_("Port:"));
	gtk_widget_ref (label_info_mud_port);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "label_info_mud_port", label_info_mud_port, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label_info_mud_port);
	gtk_table_attach (GTK_TABLE (table_info_mud), label_info_mud_port, 2, 3, 1, 2, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_info_mud_port), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (label_info_mud_port), 3, 0);

	entry_info_mud_port = gtk_entry_new ();
	gtk_widget_ref (entry_info_mud_port);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "entry_info_mud_port", entry_info_mud_port, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (entry_info_mud_port);
	gtk_table_attach (GTK_TABLE (table_info_mud), entry_info_mud_port, 3, 4, 1, 2, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_widget_set_usize (entry_info_mud_port, 40, -2);
	gtk_signal_connect(GTK_OBJECT(entry_info_mud_port), "insert-text", GTK_SIGNAL_FUNC(connections_entry_info_mud_port_cb), NULL);
			
	frame_info_char = gtk_frame_new (_("Character information"));
	gtk_widget_ref (frame_info_char);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "frame_info_char", frame_info_char, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (frame_info_char);
	gtk_box_pack_start (GTK_BOX (main_vbox), frame_info_char, FALSE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame_info_char), 4);

	table2 = gtk_table_new (2, 2, FALSE);
	gtk_widget_ref (table2);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "table2", table2, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (table2);
	gtk_container_add (GTK_CONTAINER (frame_info_char), table2);
	gtk_container_set_border_width (GTK_CONTAINER (table2), 3);
	gtk_table_set_row_spacings (GTK_TABLE (table2), 2);
	gtk_table_set_col_spacings (GTK_TABLE (table2), 3);

	label_info_char_character = gtk_label_new (_("Character:"));
	gtk_widget_ref (label_info_char_character);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "label_info_char_character", label_info_char_character, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label_info_char_character);
	gtk_table_attach (GTK_TABLE (table2), label_info_char_character, 0, 1, 0, 1, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_info_char_character), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (label_info_char_character), 3, 0);

	label_info_char_password = gtk_label_new (_("Password:"));
	gtk_widget_ref (label_info_char_password);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "label_info_char_password", label_info_char_password, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label_info_char_password);
	gtk_table_attach (GTK_TABLE (table2), label_info_char_password, 0, 1, 1, 2, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_info_char_password), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (label_info_char_password), 3, 0);

	entry_info_char_character = gtk_entry_new ();
	gtk_widget_ref (entry_info_char_character);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "entry_info_char_character", entry_info_char_character, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (entry_info_char_character);
	gtk_table_attach (GTK_TABLE (table2), entry_info_char_character, 1, 2, 0, 1, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);

	entry_info_char_password = gtk_entry_new ();
	gtk_widget_ref (entry_info_char_password);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "entry_info_char_password", entry_info_char_password, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (entry_info_char_password);
	gtk_table_attach (GTK_TABLE (table2), entry_info_char_password, 1, 2, 1, 2, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_entry_set_visibility(GTK_ENTRY(entry_info_char_password), FALSE);

	frame_info_prof = gtk_frame_new (_("Profile information"));
	gtk_widget_ref (frame_info_prof);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "frame_info_prof", frame_info_prof, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (frame_info_prof);
	gtk_box_pack_start (GTK_BOX (main_vbox), frame_info_prof, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame_info_prof), 4);

	table3 = gtk_table_new (1, 2, FALSE);
	gtk_widget_ref (table3);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "table3", table3, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (table3);
	gtk_container_add (GTK_CONTAINER (frame_info_prof), table3);
	gtk_container_set_border_width (GTK_CONTAINER (table3), 3);
	gtk_table_set_row_spacings (GTK_TABLE (table3), 2);
	gtk_table_set_col_spacings (GTK_TABLE (table3), 3);

	label_info_prof = gtk_label_new (_("Default"));
	gtk_widget_ref (label_info_prof);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "label_info_prof", label_info_prof, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label_info_prof);
	gtk_table_attach (GTK_TABLE (table3), label_info_prof, 0, 1, 0, 1, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_widget_set_usize (label_info_prof, 152, -2);
	gtk_label_set_justify (GTK_LABEL (label_info_prof), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment (GTK_MISC (label_info_prof), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (label_info_prof), 3, 0);

	button_info_prof = gtk_button_new_with_label (_("Select Profile"));
	gtk_widget_ref (button_info_prof);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "button_info_prof", button_info_prof, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (button_info_prof);
	gtk_table_attach (GTK_TABLE (table3), button_info_prof, 1, 2, 0, 1, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_signal_connect(GTK_OBJECT(button_info_prof), "clicked", GTK_SIGNAL_FUNC(connections_button_info_prof_cb), (gpointer) label_info_prof);

	table_buttons = gtk_table_new (1, 4, FALSE);
	gtk_widget_ref (table_buttons);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "table_buttons", table_buttons, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (table_buttons);
	gtk_box_pack_start (GTK_BOX (main_vbox), table_buttons, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (table_buttons), 5);
	gtk_table_set_row_spacings (GTK_TABLE (table_buttons), 2);
	gtk_table_set_col_spacings (GTK_TABLE (table_buttons), 3);
	
	button_info_fetch = gtk_button_new_with_label(_("Fetch from mudlist"));
	gtk_widget_show(button_info_fetch);
	gtk_table_attach(GTK_TABLE(table_buttons), button_info_fetch, 0, 1, 0, 1, GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 0);
	gtk_object_set_data(GTK_OBJECT(button_info_fetch), "entry_info_mud_title", entry_info_mud_title);
	gtk_object_set_data(GTK_OBJECT(button_info_fetch), "entry_info_mud_host",  entry_info_mud_host);
	gtk_object_set_data(GTK_OBJECT(button_info_fetch), "entry_info_mud_port",  entry_info_mud_port);
	gtk_object_set_data(GTK_OBJECT(button_info_fetch), "entry_info_char_character", entry_info_char_character);
	gtk_object_set_data(GTK_OBJECT(button_info_fetch), "entry_info_char_password", entry_info_char_password);
	gtk_object_set_data(GTK_OBJECT(button_info_fetch), "label_info_prof",      label_info_prof);
	gtk_signal_connect(GTK_OBJECT(button_info_fetch), "clicked", GTK_SIGNAL_FUNC(connections_button_info_fetch_cb), NULL);

	button_info_cancel = gtk_button_new_from_stock("gtk-cancel");
	gtk_widget_ref (button_info_cancel);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "button_info_cancel", button_info_cancel, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (button_info_cancel);
	gtk_table_attach (GTK_TABLE (table_buttons), button_info_cancel, 1, 2, 0, 1, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_signal_connect(GTK_OBJECT(button_info_cancel), "clicked", GTK_SIGNAL_FUNC(connections_button_info_cancel_cb), NULL);

	gtk_object_set_data(GTK_OBJECT(button_info_cancel), "entry_info_mud_title", entry_info_mud_title);
	gtk_object_set_data(GTK_OBJECT(button_info_cancel), "entry_info_mud_host",  entry_info_mud_host);
	gtk_object_set_data(GTK_OBJECT(button_info_cancel), "entry_info_mud_port",  entry_info_mud_port);
	gtk_object_set_data(GTK_OBJECT(button_info_cancel), "entry_info_char_character", entry_info_char_character);
	gtk_object_set_data(GTK_OBJECT(button_info_cancel), "entry_info_char_password", entry_info_char_password);
	gtk_object_set_data(GTK_OBJECT(button_info_cancel), "label_info_prof", label_info_prof);

	button_info_apply = gtk_button_new_from_stock ("gtk-apply");
	gtk_widget_ref (button_info_apply);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "button_info_apply", button_info_apply, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (button_info_apply);
	gtk_table_attach (GTK_TABLE (table_buttons), button_info_apply, 2, 3, 0, 1, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 5, 0);
	gtk_signal_connect(GTK_OBJECT(button_info_apply), "clicked", GTK_SIGNAL_FUNC(connections_button_info_apply_cb), (gpointer) main_clist);
	
	gtk_object_set_data(GTK_OBJECT(button_info_apply), "entry_info_mud_title", entry_info_mud_title);
	gtk_object_set_data(GTK_OBJECT(button_info_apply), "entry_info_mud_host",  entry_info_mud_host);
	gtk_object_set_data(GTK_OBJECT(button_info_apply), "entry_info_mud_port",  entry_info_mud_port);
	gtk_object_set_data(GTK_OBJECT(button_info_apply), "entry_info_char_character", entry_info_char_character);
	gtk_object_set_data(GTK_OBJECT(button_info_apply), "entry_info_char_password", entry_info_char_password);
	gtk_object_set_data(GTK_OBJECT(button_info_apply), "label_info_prof", label_info_prof);

	dialog_action_area = GNOME_DIALOG (dialog)->action_area;
	gtk_object_set_data (GTK_OBJECT (dialog), "dialog_action_area", dialog_action_area);
	gtk_widget_show (dialog_action_area);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area), 8);
 
	gnome_dialog_append_button (GNOME_DIALOG (dialog), GNOME_STOCK_BUTTON_CLOSE);
	button_cancel = GTK_WIDGET (g_list_last (GNOME_DIALOG (dialog)->buttons)->data);
	gtk_widget_ref (button_cancel);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "button_cancel", button_cancel, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (button_cancel);
	GTK_WIDGET_SET_FLAGS (button_cancel, GTK_CAN_DEFAULT);
  
  	gnome_dialog_append_button_with_pixmap (GNOME_DIALOG (dialog), _("Connect"), GNOME_STOCK_PIXMAP_JUMP_TO);
	button_connect = GTK_WIDGET (g_list_last (GNOME_DIALOG (dialog)->buttons)->data);
	gtk_widget_ref (button_connect);
	gtk_object_set_data_full (GTK_OBJECT (dialog), "button_connect", button_connect, (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (button_connect);
	GTK_WIDGET_SET_FLAGS (button_connect, GTK_CAN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(button_connect), "clicked", GTK_SIGNAL_FUNC(connections_button_connect_cb), (gpointer) dialog);

	gtk_signal_connect_object(GTK_OBJECT(button_cancel), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer) dialog);
	gtk_signal_connect(GTK_OBJECT(dialog), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &dialog);

	gtk_object_set_data(GTK_OBJECT(main_clist), "entry_info_mud_title", entry_info_mud_title);
	gtk_object_set_data(GTK_OBJECT(main_clist), "entry_info_mud_host",  entry_info_mud_host);
	gtk_object_set_data(GTK_OBJECT(main_clist), "entry_info_mud_port",  entry_info_mud_port);
	gtk_object_set_data(GTK_OBJECT(main_clist), "entry_info_char_character", entry_info_char_character);
	gtk_object_set_data(GTK_OBJECT(main_clist), "entry_info_char_password", entry_info_char_password);
	gtk_object_set_data(GTK_OBJECT(main_clist), "label_info_prof", label_info_prof);

	gtk_widget_show(dialog);

	gnome_popup_menu_attach(extra_menu, main_clist, NULL);
}

void window_profile_edit(void)
{
	static GtkWidget *profile_window = NULL;
	GtkWidget *vbox;
	GtkWidget *toolbar;
	GtkWidget *tmp_toolbar_icon;
	GtkWidget *button_new;
	GtkWidget *button_delete;
	GtkWidget *button_alias;
	GtkWidget *button_variables;
	GtkWidget *button_triggers;
	GtkWidget *button_keybinds;
	GtkWidget *button_close;
	GtkWidget *scrolledwindow;
	GtkWidget *profile_list;
	GtkWidget *vseparator;
	gchar *titles[1] = {_("Profiles")};

	if (profile_window != NULL)
	{
		gtk_window_present (GTK_WINDOW (profile_window));
		return;
	}
	
	profile_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(profile_window), _("GNOME-Mud Profiles"));

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(profile_window), vbox);

	toolbar = gtk_toolbar_new ();
	gtk_widget_show(toolbar);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	
	/*
	 * Button new
	 */
	tmp_toolbar_icon = gtk_image_new_from_stock ("gtk-new", GTK_ICON_SIZE_LARGE_TOOLBAR);
	button_new = gtk_toolbar_append_item (GTK_TOOLBAR(toolbar), _("New"),
			_("Create a new profile"), NULL, tmp_toolbar_icon, NULL,NULL);
	gtk_widget_show(button_new);

	/*
	 * Button delete
	 */
	tmp_toolbar_icon = gtk_image_new_from_stock ("gtk-new", GTK_ICON_SIZE_LARGE_TOOLBAR);
	button_delete = gtk_toolbar_append_item (GTK_TOOLBAR(toolbar), _("Delete"), _("Delete a profile"), NULL, tmp_toolbar_icon, NULL, NULL);
	gtk_widget_show(button_delete);

	/*
	 * Vertical separator
	 */
	vseparator = gtk_vseparator_new();
	gtk_widget_show(vseparator);
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), vseparator, NULL, NULL);
	gtk_widget_set_usize(vseparator, -2, 19);
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

	/*
	 * Button alias
	 */
	tmp_toolbar_icon = gtk_image_new_from_stock ("gtk-preferences", GTK_ICON_SIZE_LARGE_TOOLBAR);
	button_alias = gtk_toolbar_append_item (GTK_TOOLBAR(toolbar), _("Aliases"), _("Set aliases"), NULL, tmp_toolbar_icon, NULL, NULL);
	gtk_widget_show(button_alias);

	/*
	 * Button variables
	 */
	tmp_toolbar_icon = gtk_image_new_from_stock ("gtk-spell-check", GTK_ICON_SIZE_LARGE_TOOLBAR);
	button_variables = gtk_toolbar_append_item (GTK_TOOLBAR(toolbar), _("Variables"), _("Set variables"), NULL, tmp_toolbar_icon, NULL, NULL);
	gtk_widget_show(button_variables);

	/*
	 * Button triggers
	 */
	tmp_toolbar_icon = gtk_image_new_from_stock ("gtk-index", GTK_ICON_SIZE_LARGE_TOOLBAR);
	button_triggers = gtk_toolbar_append_item (GTK_TOOLBAR(toolbar), _("Triggers"), _("Set triggers"), NULL, tmp_toolbar_icon, NULL, NULL);
	gtk_widget_show(button_triggers);

	/*
	 * Vertical separator
	 */
	vseparator = gtk_vseparator_new();
	gtk_widget_show(vseparator);
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), vseparator, NULL, NULL);
	gtk_widget_set_usize(vseparator, -2, 19);
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	
	/*
	 * Button keybinds
	 */
	tmp_toolbar_icon = gtk_image_new_from_stock ("gtk-select-font", GTK_ICON_SIZE_LARGE_TOOLBAR);
	button_keybinds = gtk_toolbar_append_item (GTK_TOOLBAR(toolbar), _("Keybindings"), _("Set keybindings"), NULL, tmp_toolbar_icon, NULL, NULL);
	gtk_widget_show(button_keybinds);

	/*
	 * Vertical separator
	 */
	vseparator = gtk_vseparator_new();
	gtk_widget_show(vseparator);
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), vseparator, NULL, NULL);
	gtk_widget_set_usize(vseparator, -2, 19);
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
	
	/*
	 *
	 */
	tmp_toolbar_icon = gtk_image_new_from_stock("gtk-close", GTK_ICON_SIZE_LARGE_TOOLBAR);
	button_close = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), _("Close"), _("Close the window"), NULL, tmp_toolbar_icon, NULL, NULL);
	gtk_widget_show(button_close);
	
	/*
	 * CList
	 */
	scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_box_pack_start(GTK_BOX(vbox), scrolledwindow, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

	profile_list = gtk_clist_new_with_titles(1, titles);
	gtk_widget_show(profile_list);
	gtk_container_add(GTK_CONTAINER(scrolledwindow), profile_list);
	gtk_widget_set_usize(profile_list, 250, 180);
	gtk_clist_column_titles_show(GTK_CLIST(profile_list));
	gtk_clist_column_title_passive(GTK_CLIST(profile_list), 0);
	gtk_object_set_data(GTK_OBJECT(profile_list), "main_window", profile_window);

	g_list_foreach(ProfilesData, (GFunc) profilelist_clist_fill, (gpointer) profile_list);

	/*
	 * Signals
	 */
	gtk_signal_connect(GTK_OBJECT(profile_list), "select-row", GTK_SIGNAL_FUNC(profilelist_select_row_cb), GINT_TO_POINTER(0));
	gtk_signal_connect(GTK_OBJECT(profile_list), "unselect-row", GTK_SIGNAL_FUNC(profilelist_unselect_row_cb), GINT_TO_POINTER(0));

	gtk_signal_connect(GTK_OBJECT(button_new),    "clicked", GTK_SIGNAL_FUNC(profilelist_new_cb), profile_list);
	gtk_signal_connect(GTK_OBJECT(button_delete), "clicked", GTK_SIGNAL_FUNC(profilelist_delete_cb), profile_list);
	
	gtk_signal_connect(GTK_OBJECT(button_alias),     "clicked", GTK_SIGNAL_FUNC(profilelist_alias_cb), profile_list);
	gtk_signal_connect(GTK_OBJECT(button_triggers),  "clicked", GTK_SIGNAL_FUNC(profilelist_triggers_cb), profile_list);
	gtk_signal_connect(GTK_OBJECT(button_variables), "clicked", GTK_SIGNAL_FUNC(profilelist_variables_cb), profile_list);
	gtk_signal_connect(GTK_OBJECT(button_keybinds),  "clicked", GTK_SIGNAL_FUNC(profilelist_keybinds_cb), profile_list);

	gtk_signal_connect_object(GTK_OBJECT(button_close), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer) profile_window);
	gtk_signal_connect(GTK_OBJECT(profile_window), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &profile_window);
	
	gtk_widget_show(profile_window);
}
