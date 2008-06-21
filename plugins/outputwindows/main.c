/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 2006 Robin Ericsson <lobbin@localhost.nu>
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

#define __MODULE__
#include <stdio.h>
#include <gmodule.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <string.h>
#include <math.h>

#include "../../src/modules_api.h"
#include "../../src/mud-regex.h"
#include "../../src/utils.h"

#define VERSION "1.0"

enum
{
	CONFIG_ENABLED_COLUMN,
	CONFIG_NAME_COLUMN,
	N_COLUMNS
};

enum
{
	CHAN_ENABLED_COLUMN,
	CHAN_GAG_COLUMN,
	CHAN_NAME_COLUMN,
	CHAN_N_COLUMNS
};

typedef struct TermEntry {
	gboolean visible;
	gchar *name;
	GtkWidget *win;
	GtkWidget *term;
} TermEntry;

typedef struct TTreeViewRowInfo {
	gint row;
	gchar *text;
	gchar *iterstr;
} TTreeViewRowInfo;

typedef struct WindowOutputInfo {
	gboolean config_visible;

	GtkWidget *config_win;
	
	GtkTreeView *config_view;
	GtkTreeStore *config_store;
	GtkTreeViewColumn *config_name_col;
	GtkTreeViewColumn *config_enabled_col;
	GtkCellRenderer *config_name_renderer;
	GtkCellRenderer *config_enabled_renderer;
	TTreeViewRowInfo config_treeview_info;
	
	GtkTreeView *chan_view;
	GtkTreeStore *chan_store;
	GtkTreeViewColumn *chan_name_col;
	GtkTreeViewColumn *chan_enabled_col;
	GtkTreeViewColumn *chan_gag_col;
	GtkCellRenderer *chan_name_renderer;
	GtkCellRenderer *chan_gag_renderer;
	GtkCellRenderer *chan_enabled_renderer;
	TTreeViewRowInfo chan_treeview_info;

	GtkEntry *entry;
	GtkLabel *lbl;
	GtkButton *config_del;
	GtkButton *chan_add;
	GtkButton *chan_del;
	
	gchar *chankeys;
	
	GKeyFile *keyfile;
	GHashTable *termhash;
} WindowOutputInfo;

/* Plugin Functions */
static void init_plugin(PLUGIN_OBJECT *plugin, GModule *context);

void ow_show_config(void);
void ow_config_populate_treeview(void);
void ow_chan_populate_treeview(void);
TermEntry *show_terminal(gchar *name, gchar *group);

/* Callback Functions */
void ow_config_destroy_cb(GtkWidget *widget, gpointer data);

void ow_config_about_cb(GtkWidget *widget, gpointer data);
void ow_config_save_cb(GtkWidget *widget, gpointer data);
void ow_config_close_cb(GtkWidget *widget, gpointer data);
void ow_config_add_cb(GtkWidget *widget, gpointer data);
void ow_config_del_cb(GtkWidget *widget, gpointer data);
void ow_config_chan_add_cb(GtkWidget *widget, gpointer data);
void ow_config_chan_del_cb(GtkWidget *widget, gpointer data);

gboolean terminal_move_cb (GtkWidget *widget, GdkEventConfigure *event, gpointer user_data);

void hash_key_destroy_cb(gpointer data);
void hash_value_destroy_cb(gpointer data);

gboolean config_select_cb(GtkTreeSelection *selection,
                     	   GtkTreeModel     *model,
						   GtkTreePath      *path,
                   		   gboolean        path_currently_selected,
                     	   gpointer          userdata);
void config_enabled_toggle_cb(GtkCellRendererToggle *cell_renderer,gchar *path, gpointer user_data);

gboolean chan_select_cb(GtkTreeSelection *selection,
                     	   GtkTreeModel     *model,
						   GtkTreePath      *path,
                   		   gboolean        path_currently_selected,
                     	   gpointer          userdata);
void chan_enabled_toggle_cb(GtkCellRendererToggle *cell_renderer,gchar *path, gpointer user_data);
void chan_gag_toggle_cb(GtkCellRendererToggle *cell_renderer,gchar *path, gpointer user_data);
gboolean config_enabled_toggle_foreach(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data);

gboolean  hash_find_value_cb(gpointer key, gpointer value, gpointer user_data);

/* UI Functions */
GtkWidget* create_windowoutput_dlg (void);
GtkWidget* create_ow_config_window (void);
GtkWidget* create_terminal_window(gchar *name, gchar *group, GtkWidget **term);

WindowOutputInfo info;

PLUGIN_INFO gnomemud_plugin_info =
{
	"WindowOutput Plugin",
    "Les Harris",
    "1.0",
    "Plugin that provides multiple\nterminal output windows.",
    init_plugin,
};

/* Plugin<->Gmud Interface functions */
static void init_plugin(PLUGIN_OBJECT *plugin, GModule *context)
{
	gchar filename[1024];
	GError *error = NULL;
	gsize group_count;
	gint i;
	gchar **groups;
	gint enabled;
	
	plugin_register_menu(context, "Configure Output Windows...", "menu_function");
	plugin_register_data_incoming(context, "data_in_function");
	
	info.config_visible = FALSE;

	info.keyfile = g_key_file_new();
	g_snprintf(filename, 1024, "%s/.gnome-mud/plugins/outputwindows.cfg", g_get_home_dir());
	g_key_file_load_from_file(info.keyfile, (const gchar *)filename, G_KEY_FILE_NONE, &error);

	info.termhash = g_hash_table_new_full(g_str_hash, g_str_equal,hash_key_destroy_cb,hash_value_destroy_cb);

	g_clear_error(&error);
	
	groups = g_key_file_get_groups(info.keyfile, &group_count);

	info.chankeys = NULL;
	
	for(i = 0; i < group_count; i++)
	{
		enabled = g_key_file_get_integer(info.keyfile, (const gchar *)groups[i], "enabled", &error);

		if(enabled)
		{
			gchar **chankeys;
			gsize chancount;
			
			g_clear_error(&error);
			chankeys = g_key_file_get_string_list(info.keyfile, (const gchar *)groups[i], "chanlist", &chancount, &error);
			
			if(!error)
			{
				gint j;
				
				for(j = 0; j < chancount; j++)
				{
					gint chan_enabled;
					GString *key;
					TermEntry *te;
					gint x, y, w, h;
					
					key = g_string_new(NULL);
					g_clear_error(&error);
					
					g_string_append_printf(key, "%s_%s", chankeys[j], "enabled");
					chan_enabled = g_key_file_get_integer(info.keyfile, (const gchar *)groups[i], key->str, &error);
					
					if(chan_enabled)
					{
						GString *chankey;
						
						chankey = g_string_new(NULL);
						
						te = show_terminal(chankeys[j], groups[i]);
						
						g_clear_error(&error);
						g_string_append_printf(chankey, "%s_x", chankeys[j]);
						x = g_key_file_get_integer(info.keyfile, groups[i], chankey->str, &error);

						g_clear_error(&error);
						chankey = g_string_erase(chankey, 0, -1);
						g_string_append_printf(chankey, "%s_y", chankeys[j]);
						y = g_key_file_get_integer(info.keyfile, groups[i], chankey->str, &error);

						g_clear_error(&error);
						chankey = g_string_erase(chankey, 0, -1);
						g_string_append_printf(chankey, "%s_w", chankeys[j]);
						w = g_key_file_get_integer(info.keyfile, groups[i], chankey->str, &error);

						g_clear_error(&error);
						chankey = g_string_erase(chankey, 0, -1);
						g_string_append_printf(chankey, "%s_h", chankeys[j]);
						h = g_key_file_get_integer(info.keyfile, groups[i], chankey->str, &error);

						if(x && y && w && h)
						{
							gtk_window_move(GTK_WINDOW(te->win), x, y);
							gtk_window_resize(GTK_WINDOW(te->win), w, h);
						}
					}

					g_string_free(key, TRUE);
				}
			}
			
			if(chankeys)
				g_strfreev(chankeys);
		}
	}

	
}

void data_in_function(PLUGIN_OBJECT *plugin, gchar *data, guint length, MudConnectionView *view)
{
	GError *error = NULL;
	gchar **groups;
	gchar *stripped_data;
	gchar *regex;
	gsize group_count;
	gint i, enabled;
	
	groups = g_key_file_get_groups(info.keyfile, &group_count);
	stripped_data = strip_ansi((const gchar *)data);

	for(i = 0; i < group_count; i++)
	{
		enabled = g_key_file_get_integer(info.keyfile, (const gchar *)groups[i], "enabled", &error);

		if(enabled)
		{
			gchar **chans;
			gsize count;
			
			chans = g_key_file_get_string_list(info.keyfile, (const gchar *)groups[i], "chanlist", &count, &error);
			
			if(!error)
			{
				gint j;
				
				for(j = 0; j < count; j++)
				{
					gint chan_enabled;
					gint chan_gag;
					TermEntry *te;
					GString *key;
					
					key = g_string_new(NULL);

					g_clear_error(&error);
					key = g_string_erase(key, 0, -1);
					g_string_append_printf(key, "%s_enabled", chans[j]);
					chan_enabled = g_key_file_get_integer(info.keyfile, (const gchar *)groups[i], key->str,&error);

					if(chan_enabled)
					{
						g_clear_error(&error);
						g_string_printf(key, "%s_regex", chans[j]);
						regex = g_key_file_get_string(info.keyfile, (const gchar *)groups[i], key->str,&error);

						g_clear_error(&error);
						g_string_printf(key, "%s_gag", chans[j]);
						chan_gag = g_key_file_get_integer(info.keyfile, (const gchar *)groups[i], key->str,&error);
					
						te = g_hash_table_find(info.termhash, hash_find_value_cb ,chans[j]);
					
						if(regex)
						{
							const gchar **substrings;
							const gchar *errors;
							gint rc, errorcode, erroroffset;
						
							substrings = mud_regex_test((const gchar *)stripped_data, strlen(stripped_data),(const gchar *)regex, &rc, &errors, &errorcode, &erroroffset);
							g_free(regex);
						
							if(rc > 0)
							{
								if(te)
								{
									if(te->visible)
										vte_terminal_feed(VTE_TERMINAL(te->term), data, strlen(data));
									else
									{
										te->win = create_terminal_window(te->name, groups[i], &te->term);
										gtk_widget_show(te->win);
										vte_terminal_feed(VTE_TERMINAL(te->term), data, strlen(data));
									}
								}			
								else
								{
									te = show_terminal(chans[j], groups[i]);
									vte_terminal_feed(VTE_TERMINAL(te->term), data, strlen(data));
								}

								if(chan_gag)
								{
									plugin_gag_flag();
								}
							}
							if(substrings)
								mud_regex_substring_clear(substrings);
						}
					}

					g_string_free(key, TRUE);
				}
			}

			if(chans)
				g_strfreev(chans);
		}	
	}

	if(groups)	
		g_strfreev(groups);
		
	if(stripped_data)
		g_free(stripped_data);
}

void menu_function(GtkWidget *widget, gint data)
{	
	ow_show_config();
};

/* Plugin Utility Functions */
TermEntry * 
show_terminal(gchar *name, gchar *group)
{
	TermEntry *te = NULL;
	
	te = g_hash_table_find(info.termhash, hash_find_value_cb ,name);
	
	if(te)
	{
		if(te->visible)
		{
			gtk_widget_destroy(te->win);
			te->visible = FALSE;
		}
		else
		{
			te->visible = TRUE;
			te->win = create_terminal_window(te->name, group, &te->term);
			gtk_widget_show(te->win);
		}
	}
	else 
	{
		te = g_new(TermEntry, 1);
	
		te->visible = TRUE;
		te->name = g_strdup(name);
		te->win = create_terminal_window(te->name, group, &te->term);

		gtk_widget_show(te->win);
	
		g_hash_table_insert(info.termhash, g_strdup(name), (gpointer)te);
	}

	return te;
}

void 
ow_show_config(void)
{
	gchar filename[1024];
	GError *error = NULL;
	
	if(!info.config_visible)
	{
		info.config_visible = TRUE;
		info.config_win = create_ow_config_window();
		
		if(!info.keyfile)
		{
			info.keyfile = g_key_file_new();
			
			g_snprintf(filename, 1024, "%s/.gnome-mud/plugins/outputwindows.cfg", g_get_home_dir());

			g_key_file_load_from_file(info.keyfile, (const gchar *)filename, G_KEY_FILE_NONE, &error);

		}

		ow_config_populate_treeview();
		gtk_widget_show_all(info.config_win);
	}
}

void 
ow_config_populate_treeview(void)
{
	GError *error = NULL;
	gsize group_count;
	gint i;
	gchar **groups;
	GtkTreeIter iter;
	gint enabled;

	gtk_tree_store_clear(info.config_store);
	gtk_tree_store_clear(info.chan_store);

	gtk_widget_set_sensitive(GTK_WIDGET(info.chan_view), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(info.chan_add), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(info.entry), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(info.lbl), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(info.config_del), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(info.chan_del), FALSE);
	
	gtk_entry_set_text(GTK_ENTRY(info.entry), "");
	
	groups = g_key_file_get_groups(info.keyfile, &group_count);
	
	for(i = 0; i < group_count; i++)
	{

		enabled = g_key_file_get_integer(info.keyfile, (const gchar *)groups[i], "enabled", &error);
		gtk_tree_store_append(info.config_store, &iter, NULL);
		gtk_tree_store_set(info.config_store, &iter, 
						   CONFIG_ENABLED_COLUMN, enabled,
					       CONFIG_NAME_COLUMN, (const gchar *)groups[i],
						       -1);
	}
	
	g_strfreev(groups);
}

void 
ow_chan_populate_treeview(void)
{
	GError *error = NULL;
	gsize chan_count;
	gint i;
	gchar **chans;
	GString *key;
	GtkTreeIter iter;
	gint enabled;
	gint gag;

	gtk_tree_store_clear(info.chan_store);
	gtk_widget_set_sensitive(GTK_WIDGET(info.chan_del), FALSE);

	if(info.chankeys)
	{
		gsize chan_keycount;
		gchar **chan_keys;
		
		chan_keys = g_strsplit(info.chankeys, ";", -1);
		chan_keycount = g_strv_length(chan_keys);
		
		g_key_file_set_string_list(info.keyfile, info.config_treeview_info.text, "chanlist", (const gchar **)chan_keys, chan_keycount);
		g_free(info.chankeys);
		
		if(chan_keys)
			g_free(chan_keys);
	}
			
	chans = g_key_file_get_string_list(info.keyfile, info.config_treeview_info.text, "chanlist", &chan_count, &error);
	key = g_string_new(NULL);
	
	if(!error)
	{
		info.chankeys = g_strjoinv(";", chans);

		for(i = 0; i < chan_count; i++)
		{

			g_clear_error(&error);
			key = g_string_erase(key, 0, -1);
			g_string_append_printf(key, "%s_%s", chans[i], "enabled");
			enabled = g_key_file_get_integer(info.keyfile, info.config_treeview_info.text, key->str, &error);

			g_clear_error(&error);
			key = g_string_erase(key, 0, -1);
			g_string_append_printf(key, "%s_%s", chans[i], "gag");
			gag = g_key_file_get_integer(info.keyfile, info.config_treeview_info.text, key->str, &error);
		
			gtk_tree_store_append(info.chan_store, &iter, NULL);
			gtk_tree_store_set(info.chan_store, &iter, 
							   CHAN_ENABLED_COLUMN, enabled,
							   CHAN_GAG_COLUMN, gag,
						       CHAN_NAME_COLUMN, chans[i],
						       -1);
		}
	}
	
	if(chans)
		g_strfreev(chans);
		
	g_string_free(key, TRUE);
}

void
ow_config_save(void)
{
	gchar *cfgdata;
	gchar filename[1024];
	gsize cfglen;
	FILE *cfgfile;
	GError *error = NULL;

	cfgdata = g_key_file_to_data(info.keyfile, &cfglen, &error);
	
	g_snprintf(filename, 1024, "%s/.gnome-mud/plugins/outputwindows.cfg", g_get_home_dir());

	cfgfile = fopen(filename, "w");
	
	if(cfgfile)
	{
		fwrite(cfgdata, 1, cfglen, cfgfile);
		fclose(cfgfile);
	}
	else
		g_warning("Could not save output windows configuration!");
}

/* Plugin Callback Functions */
void 
ow_config_destroy_cb(GtkWidget *widget, gpointer data)
{
	info.config_visible = FALSE;
	ow_config_save();
}

void
ow_config_about_cb(GtkWidget *widget, gpointer data)
{
	GtkWidget *about = create_windowoutput_dlg();
	
	gtk_dialog_run(GTK_DIALOG(about));
}

void 
ow_config_save_cb(GtkWidget *widget, gpointer data)
{
	const gchar *regex;
	GString *key;
	
	key = g_string_new(NULL);
	
	regex = gtk_entry_get_text(GTK_ENTRY(info.entry));

	if(strlen(regex) > 0 && info.chan_treeview_info.text)
	{
		g_string_append_printf(key, "%s_regex", info.chan_treeview_info.text);
		g_key_file_set_string(info.keyfile, info.config_treeview_info.text, key->str, regex);
	}
	
	g_string_free(key, TRUE);
	
	ow_config_save();
}

void 
ow_config_close_cb(GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy(info.config_win);
}

void 
ow_config_add_cb(GtkWidget *widget, gpointer data)
{
	gint result;
	GtkWidget *entry;	
	GtkWidget *dialog = gtk_dialog_new_with_buttons("Enter name...", GTK_WINDOW(info.config_win),
													GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
													GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
													GTK_STOCK_OK, GTK_RESPONSE_OK,
													NULL);

	entry = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), entry);
	gtk_widget_show(entry);
	
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	
	if(result == GTK_RESPONSE_OK)
	{
		const gchar *name = gtk_entry_get_text(GTK_ENTRY(entry));

		g_key_file_set_integer(info.keyfile, name, "enabled", 0);

		ow_config_populate_treeview();
	}
		
	gtk_widget_destroy(dialog);
}

void 
ow_config_del_cb(GtkWidget *widget, gpointer data)
{
	GError *error = NULL;
	
	g_key_file_remove_group(info.keyfile, (const gchar *)info.config_treeview_info.text, &error);
	
	ow_config_populate_treeview();
}

void 
ow_config_chan_add_cb(GtkWidget *widget, gpointer data)
{
	gint result;
	GtkWidget *entry;
	GString *key;
	GtkWidget *dialog = gtk_dialog_new_with_buttons("Enter name...", GTK_WINDOW(info.config_win),
													GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
													GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
													GTK_STOCK_OK, GTK_RESPONSE_OK,
													NULL);

	entry = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), entry);
	gtk_widget_show(entry);
	
	key = g_string_new(NULL);
	
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	
	if(result == GTK_RESPONSE_OK)
	{
		const gchar *name = gtk_entry_get_text(GTK_ENTRY(entry));

		if(strlen(name) > 0)
		{
			if(info.chankeys)
				info.chankeys = g_strjoin(";", info.chankeys, name, NULL);
			else
				info.chankeys = g_strdup(name);

			key = g_string_erase(key, 0, -1);
			g_string_append_printf(key, "%s_%s", name, "enabled");
			g_key_file_set_integer(info.keyfile, info.config_treeview_info.text, key->str, 0);

			key = g_string_erase(key, 0, -1);
			g_string_append_printf(key, "%s_%s", name, "gag");
			g_key_file_set_integer(info.keyfile, info.config_treeview_info.text, key->str, 0);

			ow_chan_populate_treeview();
		}
	}
	
	g_string_free(key, TRUE);
	gtk_widget_destroy(dialog);
}

void 
ow_config_chan_del_cb(GtkWidget *widget, gpointer data)
{
	GString *key;
	GError *error = NULL;
	gchar **chan_keys;
	gchar *replace = NULL;
	gint chan_count;
	gint i;
	
	key = g_string_new(NULL);
	
	chan_keys = g_strsplit(info.chankeys, ";", -1);
	chan_count = g_strv_length(chan_keys);
	
	for(i = 0; i < chan_count; i++)
	{
		if(g_ascii_strcasecmp(chan_keys[i], info.chan_treeview_info.text) != 0)
		{
			if(!replace)
				replace = g_strdup(chan_keys[i]);
			else
				replace = g_strjoin(";",replace, chan_keys[i] ,NULL);
		}
	}
	
	g_free(info.chankeys);
	
	if(replace)
	{
		info.chankeys = g_strdup(replace);
		g_free(replace);
	}
	else
	{
		g_clear_error(&error);
		key = g_string_erase(key, 0, -1);
		g_string_append_printf(key, "%s", "chanlist");
		g_key_file_remove_key(info.keyfile, info.config_treeview_info.text, key->str, &error);
		info.chankeys = NULL;
	}
	
	g_clear_error(&error);
	key = g_string_erase(key, 0, -1);
	g_string_append_printf(key, "%s_%s", info.chan_treeview_info.text, "enabled");
	g_key_file_remove_key(info.keyfile, info.config_treeview_info.text, key->str, &error);

	g_clear_error(&error);
	key = g_string_erase(key, 0, -1);
	g_string_append_printf(key, "%s_%s", info.chan_treeview_info.text, "gag");
	g_key_file_remove_key(info.keyfile, info.config_treeview_info.text, key->str, &error);

	g_clear_error(&error);
	key = g_string_erase(key, 0, -1);
	g_string_append_printf(key, "%s_%s", info.chan_treeview_info.text, "regex");
	g_key_file_remove_key(info.keyfile, info.config_treeview_info.text, key->str, &error);

	g_clear_error(&error);
	key = g_string_erase(key, 0, -1);
	g_string_append_printf(key, "%s_%s", info.chan_treeview_info.text, "w");
	g_key_file_remove_key(info.keyfile, info.config_treeview_info.text, key->str, &error);

	g_clear_error(&error);
	key = g_string_erase(key, 0, -1);
	g_string_append_printf(key, "%s_%s", info.chan_treeview_info.text, "h");
	g_key_file_remove_key(info.keyfile, info.config_treeview_info.text, key->str, &error);

	g_clear_error(&error);
	key = g_string_erase(key, 0, -1);
	g_string_append_printf(key, "%s_%s", info.chan_treeview_info.text, "x");
	g_key_file_remove_key(info.keyfile, info.config_treeview_info.text, key->str, &error);

	g_clear_error(&error);
	key = g_string_erase(key, 0, -1);
	g_string_append_printf(key, "%s_%s", info.chan_treeview_info.text, "y");
	g_key_file_remove_key(info.keyfile, info.config_treeview_info.text, key->str, &error);

	ow_chan_populate_treeview();
}

gboolean 
config_select_cb(GtkTreeSelection *selection,
                 GtkTreeModel     *model,
				 GtkTreePath      *path,
                 gboolean        path_currently_selected,
                 gpointer          userdata)
{
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter(model, &iter, path)) 
	{
		gtk_tree_model_get(model, &iter, CONFIG_NAME_COLUMN, &info.config_treeview_info.text, -1);
    	
		info.config_treeview_info.row = (gtk_tree_path_get_indices(path))[0];
		info.config_treeview_info.iterstr = gtk_tree_model_get_string_from_iter(model, &iter);
		
		gtk_widget_set_sensitive(GTK_WIDGET(info.chan_view), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(info.chan_add), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(info.entry), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(info.lbl), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(info.config_del), TRUE);

		gtk_tree_store_clear(info.chan_store);
		ow_chan_populate_treeview();
	}
	
	return TRUE;
}

void 
config_enabled_toggle_cb(GtkCellRendererToggle *cell_renderer,gchar *path, gpointer user_data)
{
	GtkTreeIter iter;
	gboolean active;
	gchar *name;
	
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(info.config_store), &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(info.config_store), &iter, CONFIG_ENABLED_COLUMN, &active, -1);
	gtk_tree_model_get(GTK_TREE_MODEL(info.config_store), &iter, CONFIG_NAME_COLUMN, &name, -1);
	
	gtk_tree_store_set(info.config_store, &iter, CONFIG_ENABLED_COLUMN, !active, -1);

	gtk_tree_model_get(GTK_TREE_MODEL(info.config_store), &iter, CHAN_ENABLED_COLUMN, &active, -1);
	g_key_file_set_integer(info.keyfile, name, "enabled", active);

	gtk_tree_model_foreach(GTK_TREE_MODEL(info.config_store), config_enabled_toggle_foreach, path);
	
	g_free(name);
}

gboolean 
config_enabled_toggle_foreach(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	gchar *group;
	GtkTreePath *path_comp = gtk_tree_path_new_from_string((const gchar *)data);
	
	if(gtk_tree_path_compare(path, path_comp) != 0)
	{
		gtk_tree_store_set(info.config_store, iter, CONFIG_ENABLED_COLUMN, FALSE, -1);
		
		gtk_tree_model_get(GTK_TREE_MODEL(info.config_store), iter, CONFIG_NAME_COLUMN, &group, -1);
		g_key_file_set_integer(info.keyfile, (const gchar *)group, "enabled", FALSE);
	}

	return FALSE;
}

gboolean 
chan_select_cb(GtkTreeSelection *selection,
               GtkTreeModel     *model,
			   GtkTreePath      *path,
               gboolean        path_currently_selected,
               gpointer          userdata)
{
	GtkTreeIter iter;
	gchar *regex;
	GString *key;
	GError *error = NULL;
	
	key = g_string_new(NULL);

	if (gtk_tree_model_get_iter(model, &iter, path)) 
	{
		gtk_tree_model_get(model, &iter, CHAN_NAME_COLUMN, &info.chan_treeview_info.text, -1);
    	
		info.chan_treeview_info.row = (gtk_tree_path_get_indices(path))[0];
		info.chan_treeview_info.iterstr = gtk_tree_model_get_string_from_iter(model, &iter);
		
		gtk_widget_set_sensitive(GTK_WIDGET(info.chan_del), TRUE);

		g_string_append_printf(key, "%s_%s", info.chan_treeview_info.text, "regex");
		regex = g_key_file_get_string(info.keyfile, info.config_treeview_info.text, key->str, &error);

		if(regex)
			gtk_entry_set_text(GTK_ENTRY(info.entry),regex);
	}
	
	if(regex)
		g_free(regex);
	g_string_free(key, TRUE);

	return TRUE;
}

void 
chan_enabled_toggle_cb(GtkCellRendererToggle *cell_renderer,gchar *path, gpointer user_data)
{
	GtkTreeIter iter;
	gboolean active;
	gchar *name;
	GString *key;
	
	key = g_string_new(NULL);
	
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(info.chan_store), &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(info.chan_store), &iter, CHAN_ENABLED_COLUMN, &active, -1);
	gtk_tree_model_get(GTK_TREE_MODEL(info.chan_store), &iter, CHAN_NAME_COLUMN, &name, -1);
	
	gtk_tree_store_set(info.chan_store, &iter, CHAN_ENABLED_COLUMN, !active, -1);

	g_string_append_printf(key, "%s_%s", name, "enabled");
	
	gtk_tree_model_get(GTK_TREE_MODEL(info.chan_store), &iter, CHAN_ENABLED_COLUMN, &active, -1);
	g_key_file_set_integer(info.keyfile, info.config_treeview_info.text, key->str, active);
	
	show_terminal(name, info.config_treeview_info.text);
	
	g_string_free(key, TRUE);
	g_free(name);
}

void 
chan_gag_toggle_cb(GtkCellRendererToggle *cell_renderer,gchar *path, gpointer user_data)
{
	GtkTreeIter iter;
	gboolean active;
	gchar *name;
	GString *key;
	
	key = g_string_new(NULL);
	
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(info.chan_store), &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(info.chan_store), &iter, CHAN_GAG_COLUMN, &active, -1);
	gtk_tree_model_get(GTK_TREE_MODEL(info.chan_store), &iter, CHAN_NAME_COLUMN, &name, -1);
	
	gtk_tree_store_set(info.chan_store, &iter, CHAN_GAG_COLUMN, !active, -1);

	g_string_append_printf(key, "%s_%s", name, "gag");
	
	gtk_tree_model_get(GTK_TREE_MODEL(info.chan_store), &iter, CHAN_GAG_COLUMN, &active, -1);
	g_key_file_set_integer(info.keyfile, info.config_treeview_info.text, key->str, active);
	
	g_string_free(key, TRUE);
	g_free(name);
}

gboolean 
terminal_move_cb (GtkWidget *widget, GdkEventConfigure *event, gpointer user_data)
{
	gchar **items;
	GString *key;
	
	key = g_string_new(NULL);
	
	items = g_strsplit(gtk_widget_get_name(GTK_WIDGET(user_data)),":", -1);

	g_string_append_printf(key, "%s_x", items[1]);
	g_key_file_set_integer(info.keyfile, items[0], key->str, event->x);
	
	key = g_string_erase(key, 0, -1);
	g_string_append_printf(key, "%s_y", items[1]);
	g_key_file_set_integer(info.keyfile, items[0], key->str, event->y);

	key = g_string_erase(key, 0, -1);
	g_string_append_printf(key, "%s_w", items[1]);
	g_key_file_set_integer(info.keyfile, items[0], key->str, event->width);

	key = g_string_erase(key, 0, -1);
	g_string_append_printf(key, "%s_h", items[1]);
	g_key_file_set_integer(info.keyfile, items[0], key->str, event->height);

	ow_config_save();
	
	g_string_free(key, TRUE);
	g_strfreev(items);
	
	return FALSE;
}

void 
terminal_destroy_cb(GtkWidget *widget, gpointer data)
{
	TermEntry *te = NULL;
	
	te = g_hash_table_find(info.termhash, hash_find_value_cb ,(gpointer)gtk_widget_get_name(GTK_WIDGET(data)));

	if(te)
		te->visible = FALSE;
}

gboolean  
hash_find_value_cb(gpointer key, gpointer value, gpointer user_data)
{
	gchar *curr_key = (gchar *)key;
	gchar *name = (gchar *)user_data;
	
	if(g_ascii_strcasecmp(curr_key,name) == 0)
		return TRUE;
		
	return FALSE;
}

void 
hash_key_destroy_cb(gpointer data)
{
	gchar *key = (gchar *)data;
	
	if(key)
		g_free(data);
}

void 
hash_value_destroy_cb(gpointer data)
{
	TermEntry *to = (TermEntry *)data;
	
	if(to->name)
		g_free(to->name);
	
	g_free(to);
}

/* UI Code */
GtkWidget*
create_terminal_window(gchar *name, gchar *group, GtkWidget **term)
{
	GtkWidget *box;
	GtkWidget *terminal;
	GtkWidget *scrollbar;
	GtkWidget *window;
	GString *n;
	
	n = g_string_new(NULL);

   	GdkGeometry hints;
   	gint xpad, ypad;
   	gint char_width, char_height;
	
	box = gtk_hbox_new(FALSE, 0);
	
	terminal = vte_terminal_new();
	gtk_box_pack_start(GTK_BOX(box), terminal, TRUE, TRUE, 0);

	scrollbar = gtk_vscrollbar_new(NULL);
	gtk_range_set_adjustment(GTK_RANGE(scrollbar), VTE_TERMINAL(terminal)->adjustment);
	gtk_box_pack_start(GTK_BOX(box), scrollbar, FALSE, FALSE, 0);
 					
	gtk_widget_show_all(box);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), name);
	g_string_append_printf(n, "%s:%s", group, name); 
	gtk_widget_set_name(window, n->str);
	g_string_free(n, TRUE);

	gtk_container_add(GTK_CONTAINER(window), box);

	vte_terminal_set_emulation(VTE_TERMINAL(terminal), "xterm");
	vte_terminal_set_font_from_string(VTE_TERMINAL(terminal), "Monospace 10");
	
	vte_terminal_get_padding(VTE_TERMINAL(terminal), &xpad, &ypad);
	char_width = VTE_TERMINAL(terminal)->char_width;
	char_height = VTE_TERMINAL(terminal)->char_height;
  
	hints.base_width = xpad;
	hints.base_height = ypad;
	hints.width_inc = char_width;
	hints.height_inc = char_height;

	hints.min_width =  hints.base_width + hints.width_inc * 4;
	hints.min_height = hints.base_height+ hints.height_inc * 2;

	gtk_window_set_geometry_hints(GTK_WINDOW(window),
					GTK_WIDGET(terminal),
  					&hints,
  					GDK_HINT_RESIZE_INC |
  					GDK_HINT_MIN_SIZE |
  					GDK_HINT_BASE_SIZE);

	gtk_window_resize(GTK_WINDOW(window), 400, 55);

	g_signal_connect(G_OBJECT(window), "configure-event", G_CALLBACK(terminal_move_cb), window);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(terminal_destroy_cb), window);
	
	*term = terminal;
	
	return window;
	
}

GtkWidget*
create_ow_config_window (void)
{
	GtkWidget *ow_config_window;
	GtkWidget *hbox1;
	GtkWidget *vbox4;
	GtkWidget *hbuttonbox2;
	GtkWidget *ow_config_add;
	GtkWidget *ow_config_del;
	GtkWidget *scrolledwindow3;
	GtkWidget *ow_config_treeview;
	GtkWidget *vbox2;
	GtkWidget *ow_config_name_lbl;
	GtkWidget *hbox2;
	GtkWidget *scrolledwindow2;
	GtkWidget *ow_config_chan_treeview;
	GtkWidget *vbuttonbox1;
	GtkWidget *ow_config_chan_add;
	GtkWidget *ow_config_chan_del;
	GtkWidget *vbox3;
	GtkWidget *vbox5;
	GtkWidget *hseparator1;
	GtkWidget *hbox4;
	GtkWidget *label3;
	GtkWidget *ow_config_chan_entry;
	GtkWidget *hseparator2;
	GtkWidget *hbox3;
	GtkWidget *ow_config_about;
	GtkWidget *hbuttonbox3;
	GtkWidget *ow_config_close;
	GtkWidget *ow_config_save;

	ow_config_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (ow_config_window), "Configure Output Windows...");

	hbox1 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox1);
	gtk_container_add (GTK_CONTAINER (ow_config_window), hbox1);

	vbox4 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox4);
	gtk_box_pack_start (GTK_BOX (hbox1), vbox4, FALSE, TRUE, 0);

	hbuttonbox2 = gtk_hbutton_box_new ();
	gtk_widget_show (hbuttonbox2);
	gtk_box_pack_start (GTK_BOX (vbox4), hbuttonbox2, FALSE, TRUE, 0);

	ow_config_add = gtk_button_new_from_stock ("gtk-add");
	gtk_widget_show (ow_config_add);
	gtk_container_add (GTK_CONTAINER (hbuttonbox2), ow_config_add);
	GTK_WIDGET_SET_FLAGS (ow_config_add, GTK_CAN_DEFAULT);

	ow_config_del = gtk_button_new_from_stock ("gtk-delete");
	gtk_widget_show (ow_config_del);
	gtk_container_add (GTK_CONTAINER (hbuttonbox2), ow_config_del);
	gtk_widget_set_sensitive (ow_config_del, FALSE);
	GTK_WIDGET_SET_FLAGS (ow_config_del, GTK_CAN_DEFAULT);
	info.config_del = GTK_BUTTON(ow_config_del);

	scrolledwindow3 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow3);
	gtk_box_pack_start (GTK_BOX (vbox4), scrolledwindow3, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow3), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow3), GTK_SHADOW_IN);

	ow_config_treeview = gtk_tree_view_new ();
	gtk_widget_show (ow_config_treeview);
	gtk_container_add (GTK_CONTAINER (scrolledwindow3), ow_config_treeview);
	info.config_view = GTK_TREE_VIEW(ow_config_treeview);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox2);
	gtk_box_pack_start (GTK_BOX (hbox1), vbox2, TRUE, TRUE, 0);

	ow_config_name_lbl = gtk_label_new ("");
	gtk_widget_show (ow_config_name_lbl);
	gtk_box_pack_start (GTK_BOX (vbox2), ow_config_name_lbl, FALSE, FALSE, 0);
	gtk_label_set_use_markup (GTK_LABEL (ow_config_name_lbl), TRUE);
	info.lbl = GTK_LABEL(ow_config_name_lbl);

	hbox2 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox2);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);

	scrolledwindow2 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow2);
	gtk_box_pack_start (GTK_BOX (hbox2), scrolledwindow2, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow2), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow2), GTK_SHADOW_IN);

	ow_config_chan_treeview = gtk_tree_view_new ();
	gtk_widget_show (ow_config_chan_treeview);
	gtk_container_add (GTK_CONTAINER (scrolledwindow2), ow_config_chan_treeview);
	gtk_widget_set_sensitive (ow_config_chan_treeview, FALSE);
	info.chan_view = GTK_TREE_VIEW(ow_config_chan_treeview);

	vbuttonbox1 = gtk_vbutton_box_new ();
	gtk_widget_show (vbuttonbox1);
	gtk_box_pack_start (GTK_BOX (hbox2), vbuttonbox1, FALSE, TRUE, 0);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (vbuttonbox1), GTK_BUTTONBOX_START);

	ow_config_chan_add = gtk_button_new_from_stock ("gtk-add");
	gtk_widget_show (ow_config_chan_add);
	gtk_container_add (GTK_CONTAINER (vbuttonbox1), ow_config_chan_add);
	gtk_widget_set_sensitive (ow_config_chan_add, FALSE);
	GTK_WIDGET_SET_FLAGS (ow_config_chan_add, GTK_CAN_DEFAULT);
	info.chan_add = GTK_BUTTON(ow_config_chan_add);

	ow_config_chan_del = gtk_button_new_from_stock ("gtk-delete");
	gtk_widget_show (ow_config_chan_del);
	gtk_container_add (GTK_CONTAINER (vbuttonbox1), ow_config_chan_del);
	gtk_widget_set_sensitive (ow_config_chan_del, FALSE);
	GTK_WIDGET_SET_FLAGS (ow_config_chan_del, GTK_CAN_DEFAULT);
	info.chan_del = GTK_BUTTON(ow_config_chan_del);
	
	vbox3 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox3);
	gtk_box_pack_start (GTK_BOX (vbox2), vbox3, FALSE, TRUE, 0);

	vbox5 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox5);
	gtk_box_pack_start (GTK_BOX (vbox3), vbox5, FALSE, TRUE, 0);

	hseparator1 = gtk_hseparator_new ();
	gtk_widget_show (hseparator1);
	gtk_box_pack_start (GTK_BOX (vbox5), hseparator1, TRUE, TRUE, 3);

	hbox4 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox4);
	gtk_box_pack_start (GTK_BOX (vbox5), hbox4, TRUE, TRUE, 0);

	label3 = gtk_label_new_with_mnemonic ("  _Regex:");
	gtk_widget_show (label3);
	gtk_box_pack_start (GTK_BOX (hbox4), label3, FALSE, FALSE, 0);

	ow_config_chan_entry = gtk_entry_new ();
	gtk_widget_show (ow_config_chan_entry);
	gtk_box_pack_start (GTK_BOX (hbox4), ow_config_chan_entry, TRUE, TRUE, 0);
	gtk_widget_set_sensitive (ow_config_chan_entry, FALSE);
	info.entry = GTK_ENTRY(ow_config_chan_entry);

	hseparator2 = gtk_hseparator_new ();
	gtk_widget_show (hseparator2);
	gtk_box_pack_start (GTK_BOX (vbox5), hseparator2, TRUE, TRUE, 3);

	hbox3 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox3);
	gtk_box_pack_start (GTK_BOX (vbox3), hbox3, FALSE, TRUE, 0);

	ow_config_about = gtk_button_new_from_stock ("gtk-about");
	gtk_widget_show (ow_config_about);
	gtk_box_pack_start (GTK_BOX (hbox3), ow_config_about, FALSE, FALSE, 0);

	hbuttonbox3 = gtk_hbutton_box_new ();
	gtk_widget_show (hbuttonbox3);
	gtk_box_pack_start (GTK_BOX (hbox3), hbuttonbox3, TRUE, TRUE, 0);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox3), GTK_BUTTONBOX_END);

	ow_config_close = gtk_button_new_from_stock ("gtk-close");
	gtk_widget_show (ow_config_close);
	gtk_container_add (GTK_CONTAINER (hbuttonbox3), ow_config_close);
	GTK_WIDGET_SET_FLAGS (ow_config_close, GTK_CAN_DEFAULT);

	ow_config_save = gtk_button_new_from_stock ("gtk-save");
	gtk_widget_show (ow_config_save);
	gtk_container_add (GTK_CONTAINER (hbuttonbox3), ow_config_save);
	GTK_WIDGET_SET_FLAGS (ow_config_save, GTK_CAN_DEFAULT);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label3), ow_config_chan_entry);

	gtk_tree_view_set_rules_hint(info.config_view, TRUE);
	gtk_tree_view_set_headers_visible(info.config_view, TRUE);
	info.config_store = gtk_tree_store_new(N_COLUMNS, G_TYPE_BOOLEAN, G_TYPE_STRING);
	gtk_tree_view_set_model(info.config_view, GTK_TREE_MODEL(info.config_store));
	info.config_name_col = gtk_tree_view_column_new();
	info.config_enabled_col = gtk_tree_view_column_new();
	gtk_tree_view_append_column(info.config_view, info.config_enabled_col);
	gtk_tree_view_append_column(info.config_view, info.config_name_col);
	gtk_tree_view_column_set_title(info.config_name_col, "Name");
	gtk_tree_view_column_set_title(info.config_enabled_col, "Enabled");
	info.config_name_renderer = gtk_cell_renderer_text_new();
	info.config_enabled_renderer = gtk_cell_renderer_toggle_new();
	gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(info.config_enabled_renderer), TRUE);
	gtk_tree_view_column_pack_start(info.config_name_col, info.config_name_renderer, TRUE);
	gtk_tree_view_column_pack_start(info.config_enabled_col, info.config_enabled_renderer, TRUE);
	gtk_tree_view_column_add_attribute(info.config_name_col, info.config_name_renderer,
									   "text", CONFIG_NAME_COLUMN);
	gtk_tree_view_column_add_attribute(info.config_enabled_col, info.config_enabled_renderer,
									   "active", CONFIG_ENABLED_COLUMN);

	gtk_tree_view_set_rules_hint(info.chan_view, TRUE);
	gtk_tree_view_set_headers_visible(info.chan_view, TRUE);
	info.chan_store = gtk_tree_store_new(CHAN_N_COLUMNS, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_STRING);
	gtk_tree_view_set_model(info.chan_view, GTK_TREE_MODEL(info.chan_store));
	info.chan_name_col = gtk_tree_view_column_new();
	info.chan_enabled_col = gtk_tree_view_column_new();
	info.chan_gag_col = gtk_tree_view_column_new();
	gtk_tree_view_append_column(info.chan_view, info.chan_enabled_col);
	gtk_tree_view_append_column(info.chan_view, info.chan_gag_col);
	gtk_tree_view_append_column(info.chan_view, info.chan_name_col);
	gtk_tree_view_column_set_title(info.chan_name_col, "Name");
	gtk_tree_view_column_set_title(info.chan_gag_col, "Gag");
	gtk_tree_view_column_set_title(info.chan_enabled_col, "Enabled");
	info.chan_name_renderer = gtk_cell_renderer_text_new();
	info.chan_enabled_renderer = gtk_cell_renderer_toggle_new();
	info.chan_gag_renderer = gtk_cell_renderer_toggle_new();
	gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(info.chan_enabled_renderer), FALSE);
	gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(info.chan_gag_renderer), FALSE);
	gtk_tree_view_column_pack_start(info.chan_name_col, info.chan_name_renderer, TRUE);
	gtk_tree_view_column_pack_start(info.chan_gag_col, info.chan_gag_renderer, TRUE);
	gtk_tree_view_column_pack_start(info.chan_enabled_col, info.chan_enabled_renderer, TRUE);
	gtk_tree_view_column_add_attribute(info.chan_name_col, info.chan_name_renderer,
									   "text", CHAN_NAME_COLUMN);
	gtk_tree_view_column_add_attribute(info.chan_enabled_col, info.chan_enabled_renderer,
									   "active", CHAN_ENABLED_COLUMN);
	gtk_tree_view_column_add_attribute(info.chan_gag_col, info.chan_gag_renderer,
									   "active", CHAN_GAG_COLUMN);

	gtk_tree_store_clear(info.config_store);
	gtk_tree_store_clear(info.chan_store);

	g_signal_connect(G_OBJECT(info.config_enabled_renderer), "toggled", G_CALLBACK(config_enabled_toggle_cb), NULL);
	gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(info.config_view), config_select_cb, NULL, NULL);

	g_signal_connect(G_OBJECT(info.chan_enabled_renderer), "toggled", G_CALLBACK(chan_enabled_toggle_cb), NULL);
	g_signal_connect(G_OBJECT(info.chan_gag_renderer), "toggled", G_CALLBACK(chan_gag_toggle_cb), NULL);
	gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(info.chan_view), chan_select_cb, NULL, NULL);
	
	g_signal_connect(G_OBJECT(ow_config_window), "destroy", G_CALLBACK(ow_config_destroy_cb), NULL);
	
	g_signal_connect(G_OBJECT(ow_config_about), "clicked", G_CALLBACK(ow_config_about_cb), NULL);
	g_signal_connect(G_OBJECT(ow_config_save), "clicked", G_CALLBACK(ow_config_save_cb), NULL);
	g_signal_connect(G_OBJECT(ow_config_close), "clicked", G_CALLBACK(ow_config_close_cb), NULL);
	g_signal_connect(G_OBJECT(ow_config_add), "clicked", G_CALLBACK(ow_config_add_cb), NULL);
	g_signal_connect(G_OBJECT(ow_config_del), "clicked", G_CALLBACK(ow_config_del_cb), NULL);
	g_signal_connect(G_OBJECT(ow_config_chan_add), "clicked", G_CALLBACK(ow_config_chan_add_cb), NULL);
	g_signal_connect(G_OBJECT(ow_config_chan_del), "clicked", G_CALLBACK(ow_config_chan_del_cb), NULL);

	gtk_window_resize(GTK_WINDOW(ow_config_window), 500,200);

	return ow_config_window;
}

GtkWidget*
create_windowoutput_dlg (void)
{
	GtkWidget *windowoutput_dlg;
	const gchar *authors[] = {
		"Les Harris <hl.harris@comcast.net>",
		NULL
	};

	windowoutput_dlg = gtk_about_dialog_new ();
	gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (windowoutput_dlg), VERSION);
	gtk_about_dialog_set_name (GTK_ABOUT_DIALOG (windowoutput_dlg), "WindowOutput Plugin");
	gtk_about_dialog_set_comments (GTK_ABOUT_DIALOG (windowoutput_dlg), "Plugin that provides multiple terminal output windows.");
	gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (windowoutput_dlg), authors);

	return windowoutput_dlg;
}
