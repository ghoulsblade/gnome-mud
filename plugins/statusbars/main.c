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

static void init_plugin(PLUGIN_OBJECT *plugin, GModule *context);

GtkWidget *create_statusbars_win (void);
GtkWidget *create_statusbarsconfig_win (void);
GtkWidget *create_statusbarsabout_dlg (void);

void show_statusbar(void);
void show_statusbar_config(void);
void statusbar_update_bars(gdouble hp, gdouble sp, gdouble mp);
void statusbar_config_populate_treeview(void);
void statusbar_config_save(void);

void statusbar_destroy_cb(GtkWidget *widget, gpointer data);
void statusbar_config_destroy_cb(GtkWidget *widget, gpointer data);
void statusbar_config_about_cb(GtkWidget *widget, gpointer data);
void statusbar_config_add_cb(GtkWidget *widget, gpointer data);
void statusbar_config_delete_cb(GtkWidget *widget, gpointer data);
void statusbar_config_close_cb(GtkWidget *widget, gpointer data);
void statusbar_config_ok_cb(GtkWidget *widget, gpointer data);

gboolean statusbar_config_select_cb(GtkTreeSelection *selection,
                     	   GtkTreeModel     *model,
						   GtkTreePath      *path,
                   		   gboolean        path_currently_selected,
                     	   gpointer          userdata);
void statusbar_config_enabled_toggle_cb(GtkCellRendererToggle *cell_renderer,gchar *path, gpointer user_data);
gboolean statusbar_config_enabled_toggle_foreach(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data);

gboolean statusbar_move_cb (GtkWidget *widget, GdkEventConfigure *event, gpointer user_data);

enum
{
	ENABLED_COLUMN,
	NAME_COLUMN,
	N_COLUMNS
};

typedef struct TTreeViewRowInfo {
	gint row;
	gchar *text;
	gchar *iterstr;
} TTreeViewRowInfo;

typedef struct StatusbarInfo {
	gboolean statusbar_visible;
	gboolean statusbar_config_visible;

	gint curr_hpmax;
	gint curr_spmax;
	gint curr_mpmax;
	
	GtkWidget *statusbar_win;
	GtkWidget *statusbar_config;

	GtkProgressBar *hpmax;
	GtkProgressBar *spmax;
	GtkProgressBar *mpmax;
	
	GtkLabel *hpmax_lbl;
	GtkLabel *spmax_lbl;
	GtkLabel *mpmax_lbl;
	
	GtkWidget *delete;
	GtkWidget *save;
	
	GtkTreeView *view;
	GtkTreeStore *store;
	GtkTreeViewColumn *name_col;
	GtkTreeViewColumn *enabled_col;
	GtkCellRenderer *name_renderer;
	GtkCellRenderer *enabled_renderer;
	TTreeViewRowInfo treeview_info;
	
	GtkEntry *entry;
	
	GKeyFile *keyfile;
} StatusBarInfo;

PLUGIN_INFO gnomemud_plugin_info =
{
	"Statusbars Plugin",
    "Les Harris",
    "1.0",
    "Plugin that provides graphical status bars.",
    init_plugin,
};

StatusBarInfo sb_info;

/* Plugin<->Gmud Interface functions */
void init_plugin(PLUGIN_OBJECT *plugin, GModule *context)
{
	GError *error = NULL;
	gchar filename[1024];
	gint x;
	gint y;
	gint w;
	gint h;
	
	plugin_register_menu(context, "Configure Statusbars...", "menu_function");
	plugin_register_data_incoming(context, "data_in_function");
	
	sb_info.statusbar_visible = FALSE;
	sb_info.statusbar_config_visible = FALSE;
	
	show_statusbar();
	statusbar_update_bars(0.0, 0.0, 0.0);
	
	sb_info.keyfile = g_key_file_new();
	g_snprintf(filename, 1024, "%s/.gnome-mud/plugins/statusbar.cfg", g_get_home_dir());
	g_key_file_load_from_file(sb_info.keyfile, (const gchar *)filename, G_KEY_FILE_NONE, &error);

	error = NULL;
	x = g_key_file_get_integer(sb_info.keyfile, "window", "x", &error);

	error = NULL;
	y = g_key_file_get_integer(sb_info.keyfile, "window", "y", &error);

	error = NULL;
	w = g_key_file_get_integer(sb_info.keyfile, "window", "w", &error);

	error = NULL;
	h = g_key_file_get_integer(sb_info.keyfile, "window", "h", &error);

	if(x && y && w && h)
	{
		gtk_window_move(GTK_WINDOW(sb_info.statusbar_win), x, y);
		gtk_window_resize(GTK_WINDOW(sb_info.statusbar_win), w, h);
	}
}

void data_in_function(PLUGIN_OBJECT *plugin, gchar *data, MudConnectionView *view)
{
	gint hp;
	gint sp;
	gint mp;
	gint rc;
	gint errorcode;
	gint erroroffset;
	gint group_count;
	gint enabled;
	gint i;
	gint j;
	gchar *regex;
	gchar *stripped_data;
	const gchar *errors;
	const gchar **substrings;
	gchar **groups;
	GError *error = NULL;
	
	groups = g_key_file_get_groups(sb_info.keyfile, &group_count);
	stripped_data = strip_ansi((const gchar *)data);
	
	for(i = 0; i < group_count; i++)
	{
		enabled = g_key_file_get_integer(sb_info.keyfile, (const gchar *)groups[i], "enabled", &error);
  		
  		if(enabled)
  		{
  			regex = g_key_file_get_string(sb_info.keyfile, (const gchar *)groups[i], "regex", &error);
			substrings = mud_regex_test((const gchar *)stripped_data, (const gchar *)regex, &rc, &errors, &errorcode, &erroroffset);
			g_free(regex);

			if(rc > 0)
			{
				for(j = 1; j < rc; j++)
				{
					switch(j)
					{
						case 1:
							hp = g_strtod((const gchar *)substrings[j],NULL);
						break;
						
						case 2:
							sp = g_strtod((const gchar *)substrings[j],NULL);
						break;
						
						case 3:
							mp = g_strtod((const gchar *)substrings[j],NULL);
						break;
						
						default:
							g_warning("Too many captured substrings for statusbar use.");
					}
				}
				
				if(hp > sb_info.curr_hpmax) sb_info.curr_hpmax = hp;
				if(sp > sb_info.curr_spmax) sb_info.curr_spmax = sp;
				if(mp > sb_info.curr_mpmax) sb_info.curr_mpmax = mp;

				statusbar_update_bars((gdouble)hp/(gdouble)sb_info.curr_hpmax, (gdouble)sp/(gdouble)sb_info.curr_spmax, (gdouble)mp/(gdouble)sb_info.curr_mpmax);
				
				g_free(stripped_data);
				mud_regex_substring_clear(substrings);
			}
  		}
	}
	
	
	
	g_strfreev(groups);
}

void menu_function(GtkWidget *widget, gint data)
{
	show_statusbar_config();
};

/* Plugin Utility Functions */
void show_statusbar(void)
{
	if(!sb_info.statusbar_visible)
	{
		sb_info.statusbar_visible = TRUE;
		sb_info.statusbar_win = create_statusbars_win();
		
		gtk_widget_show_all(sb_info.statusbar_win);
	}
}

void show_statusbar_config(void)
{
	gchar filename[1024];
	GError *error = NULL;
	
	if(!sb_info.statusbar_config_visible)
	{
		sb_info.statusbar_config_visible = TRUE;
		sb_info.statusbar_config = create_statusbarsconfig_win();
		
		if(!sb_info.keyfile)
		{
			sb_info.keyfile = g_key_file_new();
			
			g_snprintf(filename, 1024, "%s/.gnome-mud/plugins/statusbar.cfg", g_get_home_dir());

			g_key_file_load_from_file(sb_info.keyfile, (const gchar *)filename, G_KEY_FILE_NONE, &error);

		}

		statusbar_config_populate_treeview();
		gtk_widget_show_all(sb_info.statusbar_config);
	}
}

void statusbar_config_populate_treeview(void)
{
	GError *error = NULL;
	gsize group_count;
	gint i;
	gchar **groups;
	GtkTreeIter iter;
	gint enabled;

	gtk_tree_store_clear(sb_info.store);
	gtk_widget_set_sensitive(sb_info.delete, FALSE);
	gtk_widget_set_sensitive(sb_info.save, FALSE);
	
	groups = g_key_file_get_groups(sb_info.keyfile, &group_count);
	
	for(i = 0; i < group_count; i++)
	{

		if(g_ascii_strcasecmp((const gchar *)groups[i], "window") != 0)
		{
			enabled = g_key_file_get_integer(sb_info.keyfile, (const gchar *)groups[i], "enabled", &error);
  			gtk_tree_store_append(sb_info.store, &iter, NULL);
			gtk_tree_store_set(sb_info.store, &iter, 
						   	   ENABLED_COLUMN, enabled,
						   	   NAME_COLUMN, (const gchar *)groups[i],
						       -1);
		}
	}
	
	g_strfreev(groups);
}

void 
statusbar_update_bars(gdouble hp, gdouble sp, gdouble mp)
{
	gchar *markup;

	if(hp < 0.0)
		hp = 0;
	if(hp > 1.0)
		hp = 1.0;
		
	if(sp < 0.0)
		sp = 0;
	if(sp > 1.0)
		sp = 1.0;
		
	if(mp < 0.0)
		mp = 0;
	if(mp > 1.0)
		mp = 1.0;
	
	if(!sb_info.statusbar_visible)
	{
		sb_info.statusbar_visible = TRUE;
		sb_info.statusbar_win = create_statusbars_win();
		
		gtk_widget_show_all(sb_info.statusbar_win);
	}
	
	markup = g_markup_printf_escaped ("<i>%d</i>", sb_info.curr_hpmax);
	gtk_label_set_markup(GTK_LABEL(sb_info.hpmax_lbl), markup);
	g_free(markup);
	
	markup = g_markup_printf_escaped ("<i>%d</i>", sb_info.curr_spmax);
	gtk_label_set_markup(GTK_LABEL(sb_info.spmax_lbl), markup);
	g_free(markup);

	markup = g_markup_printf_escaped ("<i>%d</i>", sb_info.curr_mpmax);
	gtk_label_set_markup(GTK_LABEL(sb_info.mpmax_lbl), markup);
	g_free(markup);
	
	gtk_progress_bar_set_fraction(sb_info.hpmax, hp);
	gtk_progress_bar_set_fraction(sb_info.spmax, sp);
	gtk_progress_bar_set_fraction(sb_info.mpmax, mp);
}

/* Callbacks */
void statusbar_destroy_cb(GtkWidget *widget, gpointer data)
{
	sb_info.statusbar_visible = FALSE;
}

void statusbar_config_destroy_cb(GtkWidget *widget, gpointer data)
{
	sb_info.statusbar_config_visible = FALSE;
	
	statusbar_config_save();
}

void statusbar_config_about_cb(GtkWidget *widget, gpointer data)
{
	GtkAboutDialog *about = GTK_ABOUT_DIALOG(create_statusbarsabout_dlg());
	
	gtk_dialog_run(GTK_DIALOG(about));
}

void statusbar_config_add_cb(GtkWidget *widget, gpointer data)
{
	gint result;
	GtkWidget *entry;	
	GtkWidget *dialog = gtk_dialog_new_with_buttons("Enter name...", GTK_WINDOW(sb_info.statusbar_config),
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

		if(strlen(name) > 0 && (g_ascii_strcasecmp(name, "window") != 0))
		{
			g_key_file_set_integer(sb_info.keyfile, name, "enabled", 0);
			g_key_file_set_string(sb_info.keyfile, name, "regex", "");

			statusbar_config_populate_treeview();
		}
	}
		
	gtk_widget_destroy(dialog);
}

void statusbar_config_delete_cb(GtkWidget *widget, gpointer data)
{
	GError *error = NULL;
	
	g_key_file_remove_group(sb_info.keyfile, (const gchar *)sb_info.treeview_info.text, &error);
	
	statusbar_config_populate_treeview();
}

void statusbar_config_close_cb(GtkWidget *widget, gpointer data)
{	
	gtk_widget_destroy(sb_info.statusbar_config);
}

void statusbar_config_ok_cb(GtkWidget *widget, gpointer data)
{
	const gchar *regex;
	
	regex = gtk_entry_get_text(sb_info.entry);
	g_key_file_set_string (sb_info.keyfile, (const gchar *)sb_info.treeview_info.text, "regex", regex);
	
	statusbar_config_save();
}

void statusbar_config_save(void)
{
	gchar *cfgdata;
	gchar filename[1024];
	gsize cfglen;
	FILE *cfgfile;
	GError *error = NULL;

	cfgdata = g_key_file_to_data(sb_info.keyfile, &cfglen, &error);
	
	g_snprintf(filename, 1024, "%s/.gnome-mud/plugins/statusbar.cfg", g_get_home_dir());

	cfgfile = fopen(filename, "w");
	
	if(cfgfile)
	{
		fwrite(cfgdata, 1, cfglen, cfgfile);
		fclose(cfgfile);
	}
	else
		g_warning("Could not save status bar configuration!");
		
}
gboolean 
statusbar_config_select_cb(GtkTreeSelection *selection,
                     	   GtkTreeModel     *model,
						   GtkTreePath      *path,
                   		   gboolean        path_currently_selected,
                     	   gpointer          userdata)
{
	GtkTreeIter iter;
	gchar *regex;
	GError *error = NULL;

	if (gtk_tree_model_get_iter(model, &iter, path)) 
	{
		gtk_tree_model_get(model, &iter, NAME_COLUMN, &sb_info.treeview_info.text, -1);
    	
		sb_info.treeview_info.row = (gtk_tree_path_get_indices(path))[0];
		sb_info.treeview_info.iterstr = gtk_tree_model_get_string_from_iter(model, &iter);
		
		regex = g_key_file_get_string(sb_info.keyfile, (const gchar *)sb_info.treeview_info.text, "regex", &error);
		gtk_entry_set_text(sb_info.entry, (const gchar *)regex);
		g_free(regex);

		gtk_widget_set_sensitive(sb_info.delete, TRUE);
		gtk_widget_set_sensitive(sb_info.save, TRUE);
	}
	
	return TRUE;
}

void statusbar_config_enabled_toggle_cb(GtkCellRendererToggle *cell_renderer,
                                            gchar *path,
                                            gpointer user_data)
{
	GtkTreeIter iter;
	gboolean active;
	gchar *name;
	
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(sb_info.store), &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(sb_info.store), &iter, ENABLED_COLUMN, &active, -1);
	gtk_tree_model_get(GTK_TREE_MODEL(sb_info.store), &iter, NAME_COLUMN, &name, -1);
	
	gtk_tree_store_set(sb_info.store, &iter, ENABLED_COLUMN, !active, -1);

	gtk_tree_model_get(GTK_TREE_MODEL(sb_info.store), &iter, ENABLED_COLUMN, &active, -1);
	g_key_file_set_integer(sb_info.keyfile, name, "enabled", active);

	gtk_tree_model_foreach(GTK_TREE_MODEL(sb_info.store), statusbar_config_enabled_toggle_foreach, path);
	
	g_free(name);
}

gboolean 
statusbar_config_enabled_toggle_foreach(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	gchar *group;
	GtkTreePath *path_comp = gtk_tree_path_new_from_string((const gchar *)data);
	
	if(gtk_tree_path_compare(path, path_comp) != 0)
	{
		gtk_tree_store_set(sb_info.store, iter, ENABLED_COLUMN, FALSE, -1);
		
		gtk_tree_model_get(GTK_TREE_MODEL(sb_info.store), iter, NAME_COLUMN, &group, -1);
		g_key_file_set_integer(sb_info.keyfile, (const gchar *)group, "enabled", FALSE);
	}

	return FALSE;
}

gboolean 
statusbar_move_cb (GtkWidget *widget, GdkEventConfigure *event, gpointer user_data)
{
	static gint count = 0;
	
	count++;
	
	if(count > 10)
	{
		g_key_file_set_integer(sb_info.keyfile, "window", "x", event->x);
		g_key_file_set_integer(sb_info.keyfile, "window", "y", event->y);
		g_key_file_set_integer(sb_info.keyfile, "window", "w", event->width);
		g_key_file_set_integer(sb_info.keyfile, "window", "h", event->height);

		count = 0;
		statusbar_config_save();
	}
	
	return FALSE;
}

/* UI Code */
GtkWidget*
create_statusbars_win (void)
{
	GtkWidget *statusbars_win;
	GtkWidget *hbox1;
	GtkWidget *vbox1;
	GtkWidget *label3;
	GtkWidget *hpmax_lbl;
	GtkWidget *hp_pbar;
	GtkWidget *label9;
	GtkWidget *vbox2;
	GtkWidget *label4;
	GtkWidget *spmax_lbl;
	GtkWidget *sp_pbar;
	GtkWidget *label12;
	GtkWidget *vbox3;
	GtkWidget *label5;
	GtkWidget *mpmax_lbl;
	GtkWidget *mp_pbar;
	GtkWidget *label11;
	GdkColor color;

	statusbars_win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_name (statusbars_win, "statusbars_win");
	gtk_window_set_title (GTK_WINDOW (statusbars_win), "Statusbars");

	hbox1 = gtk_hbox_new (FALSE, 0);
 	gtk_widget_set_name (hbox1, "hbox1");
	gtk_widget_show (hbox1);
	gtk_container_add (GTK_CONTAINER (statusbars_win), hbox1);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_set_name (vbox1, "vbox1");
	gtk_widget_show (vbox1);
	gtk_box_pack_start (GTK_BOX (hbox1), vbox1, TRUE, TRUE, 0);

	label3 = gtk_label_new ("<b>HP</b>");
	gtk_widget_set_name (label3, "label3");
	gtk_widget_show (label3);
	gtk_box_pack_start (GTK_BOX (vbox1), label3, FALSE, FALSE, 0);
	gtk_label_set_use_markup (GTK_LABEL (label3), TRUE);

	hpmax_lbl = gtk_label_new ("");
	gtk_widget_set_name (hpmax_lbl, "hpmax_lbl");
	gtk_widget_show (hpmax_lbl);
	gtk_box_pack_start (GTK_BOX (vbox1), hpmax_lbl, FALSE, FALSE, 0);
	gtk_label_set_use_markup (GTK_LABEL (hpmax_lbl), TRUE);
	sb_info.hpmax_lbl = (GtkLabel *)hpmax_lbl;

	hp_pbar = gtk_progress_bar_new ();
	gtk_widget_set_name (hp_pbar, "hp_pbar");
	gtk_widget_show (hp_pbar);
	gtk_box_pack_start (GTK_BOX (vbox1), hp_pbar, TRUE, TRUE, 0);
	gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (hp_pbar), GTK_PROGRESS_BOTTOM_TO_TOP);
	sb_info.hpmax = (GtkProgressBar *)hp_pbar;

	label9 = gtk_label_new ("<i>0</i>");
	gtk_widget_set_name (label9, "label9");
	gtk_widget_show (label9);
	gtk_box_pack_start (GTK_BOX (vbox1), label9, FALSE, FALSE, 0);
	gtk_label_set_use_markup (GTK_LABEL (label9), TRUE);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_widget_set_name (vbox2, "vbox2");
	gtk_widget_show (vbox2);
	gtk_box_pack_start (GTK_BOX (hbox1), vbox2, TRUE, TRUE, 0);

	label4 = gtk_label_new ("<b>SP</b>");
	gtk_widget_set_name (label4, "label4");
	gtk_widget_show (label4);
	gtk_box_pack_start (GTK_BOX (vbox2), label4, FALSE, FALSE, 0);
	gtk_label_set_use_markup (GTK_LABEL (label4), TRUE);

	spmax_lbl = gtk_label_new ("");
	gtk_widget_set_name (spmax_lbl, "spmax_lbl");
	gtk_widget_show (spmax_lbl);
	gtk_box_pack_start (GTK_BOX (vbox2), spmax_lbl, FALSE, FALSE, 0);
	gtk_label_set_use_markup (GTK_LABEL (spmax_lbl), TRUE);
	sb_info.spmax_lbl = (GtkLabel *)spmax_lbl;

	sp_pbar = gtk_progress_bar_new ();
	gtk_widget_set_name (sp_pbar, "sp_pbar");
	gtk_widget_show (sp_pbar);
	gtk_box_pack_start (GTK_BOX (vbox2), sp_pbar, TRUE, TRUE, 0);
	gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (sp_pbar), GTK_PROGRESS_BOTTOM_TO_TOP);
	sb_info.spmax = (GtkProgressBar *)sp_pbar;

	label12 = gtk_label_new ("<i>0</i>");
	gtk_widget_set_name (label12, "label12");
	gtk_widget_show (label12);
	gtk_box_pack_start (GTK_BOX (vbox2), label12, FALSE, FALSE, 0);
	gtk_label_set_use_markup (GTK_LABEL (label12), TRUE);

	vbox3 = gtk_vbox_new (FALSE, 0);
	gtk_widget_set_name (vbox3, "vbox3");
	gtk_widget_show (vbox3);
	gtk_box_pack_start (GTK_BOX (hbox1), vbox3, TRUE, TRUE, 0);

	label5 = gtk_label_new ("<b>MP</b>");
	gtk_widget_set_name (label5, "label5");
	gtk_widget_show (label5);
	gtk_box_pack_start (GTK_BOX (vbox3), label5, FALSE, FALSE, 0);
	gtk_label_set_use_markup (GTK_LABEL (label5), TRUE);

	mpmax_lbl = gtk_label_new ("");
	gtk_widget_set_name (mpmax_lbl, "mpmax_lbl");
	gtk_widget_show (mpmax_lbl);
	gtk_box_pack_start (GTK_BOX (vbox3), mpmax_lbl, FALSE, FALSE, 0);
	gtk_label_set_use_markup (GTK_LABEL (mpmax_lbl), TRUE);
	sb_info.mpmax_lbl = (GtkLabel *)mpmax_lbl;

	mp_pbar = gtk_progress_bar_new ();
	gtk_widget_set_name (mp_pbar, "mp_pbar");
	gtk_widget_show (mp_pbar);
	gtk_box_pack_start (GTK_BOX (vbox3), mp_pbar, TRUE, TRUE, 0);
	gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (mp_pbar), GTK_PROGRESS_BOTTOM_TO_TOP);
	sb_info.mpmax = (GtkProgressBar *)mp_pbar;

	label11 = gtk_label_new ("<i>0</i>");
	gtk_widget_set_name (label11, "label11");
	gtk_widget_show (label11);
	gtk_box_pack_start (GTK_BOX (vbox3), label11, FALSE, FALSE, 0);
	gtk_label_set_use_markup (GTK_LABEL (label11), TRUE);

	g_signal_connect(G_OBJECT(statusbars_win), "destroy", G_CALLBACK(statusbar_destroy_cb), NULL);
    
	gdk_color_parse("#880000", &color);
	gtk_widget_modify_base(GTK_WIDGET(sb_info.hpmax),GTK_STATE_NORMAL,&color);
	gdk_color_parse("#AA0000", &color);
	gtk_widget_modify_base(GTK_WIDGET(sb_info.hpmax),GTK_STATE_ACTIVE,&color);
	gdk_color_parse("#AA0000", &color);
	gtk_widget_modify_base(GTK_WIDGET(sb_info.hpmax),GTK_STATE_PRELIGHT,&color);
	gdk_color_parse("#AA0000", &color);
	gtk_widget_modify_base(GTK_WIDGET(sb_info.hpmax),GTK_STATE_SELECTED,&color);
	gdk_color_parse("#660000", &color);
	gtk_widget_modify_base(GTK_WIDGET(sb_info.hpmax),GTK_STATE_INSENSITIVE,&color);

	gdk_color_parse("#008800", &color);
	gtk_widget_modify_base(GTK_WIDGET(sb_info.spmax),GTK_STATE_NORMAL,&color);
	gdk_color_parse("#00AA00", &color);
	gtk_widget_modify_base(GTK_WIDGET(sb_info.spmax),GTK_STATE_ACTIVE,&color);
	gdk_color_parse("#00AA00", &color);
	gtk_widget_modify_base(GTK_WIDGET(sb_info.spmax),GTK_STATE_PRELIGHT,&color);
	gdk_color_parse("#00AA00", &color);
	gtk_widget_modify_base(GTK_WIDGET(sb_info.spmax),GTK_STATE_SELECTED,&color);
	gdk_color_parse("#006600", &color);
	gtk_widget_modify_base(GTK_WIDGET(sb_info.spmax),GTK_STATE_INSENSITIVE,&color);

	gdk_color_parse("#2222AA", &color);
	gtk_widget_modify_base(GTK_WIDGET(sb_info.mpmax),GTK_STATE_NORMAL,&color);
	gdk_color_parse("#4444CC", &color);
	gtk_widget_modify_base(GTK_WIDGET(sb_info.mpmax),GTK_STATE_ACTIVE,&color);
	gdk_color_parse("#4444CC", &color);
	gtk_widget_modify_base(GTK_WIDGET(sb_info.mpmax),GTK_STATE_PRELIGHT,&color);
	gdk_color_parse("#4444CC", &color);
	gtk_widget_modify_base(GTK_WIDGET(sb_info.mpmax),GTK_STATE_SELECTED,&color);
	gdk_color_parse("#222288", &color);
	gtk_widget_modify_base(GTK_WIDGET(sb_info.mpmax),GTK_STATE_INSENSITIVE,&color);

	g_signal_connect(G_OBJECT(statusbars_win), "configure-event", G_CALLBACK(statusbar_move_cb), NULL);
	return statusbars_win;
}

GtkWidget*
create_statusbarsconfig_win (void)
{
  GtkWidget *statusbarsconfig_win;
  GtkWidget *vbox4;
  GtkWidget *hbox2;
  GtkWidget *scrolledwindow1;
  GtkWidget *sbar_treeview;
  GtkWidget *vbuttonbox1;
  GtkWidget *sbar_add;
  GtkWidget *sbar_delete;
  GtkWidget *vbox5;
  GtkWidget *label14;
  GtkWidget *hbox4;
  GtkWidget *label13;
  GtkWidget *sbar_entry;
  GtkWidget *hbox3;
  GtkWidget *sbar_about;
  GtkWidget *hbuttonbox1;
  GtkWidget *sbar_close;
  GtkWidget *sbar_ok;

  statusbarsconfig_win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name (statusbarsconfig_win, "statusbarsconfig_win");
  gtk_window_set_title (GTK_WINDOW (statusbarsconfig_win), "Configure Statusbars");

  vbox4 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox4, "vbox4");
  gtk_widget_show (vbox4);
  gtk_container_add (GTK_CONTAINER (statusbarsconfig_win), vbox4);

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox2, "hbox2");
  gtk_widget_show (hbox2);
  gtk_box_pack_start (GTK_BOX (vbox4), hbox2, TRUE, TRUE, 0);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow1, "scrolledwindow1");
  gtk_widget_show (scrolledwindow1);
  gtk_box_pack_start (GTK_BOX (hbox2), scrolledwindow1, TRUE, TRUE, 0);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  sbar_treeview = gtk_tree_view_new ();
  gtk_widget_set_name (sbar_treeview, "sbar_treeview");
  gtk_widget_show (sbar_treeview);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), sbar_treeview);
  sb_info.view = GTK_TREE_VIEW(sbar_treeview);

  vbuttonbox1 = gtk_vbutton_box_new ();
  gtk_widget_set_name (vbuttonbox1, "vbuttonbox1");
  gtk_widget_show (vbuttonbox1);
  gtk_box_pack_start (GTK_BOX (hbox2), vbuttonbox1, FALSE, TRUE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (vbuttonbox1), GTK_BUTTONBOX_START);

  sbar_add = gtk_button_new_from_stock ("gtk-add");
  gtk_widget_set_name (sbar_add, "sbar_add");
  gtk_widget_show (sbar_add);
  gtk_container_add (GTK_CONTAINER (vbuttonbox1), sbar_add);
  GTK_WIDGET_SET_FLAGS (sbar_add, GTK_CAN_DEFAULT);

  sbar_delete = gtk_button_new_from_stock ("gtk-delete");
  gtk_widget_set_name (sbar_delete, "sbar_delete");
  gtk_widget_show (sbar_delete);
  gtk_container_add (GTK_CONTAINER (vbuttonbox1), sbar_delete);
  GTK_WIDGET_SET_FLAGS (sbar_delete, GTK_CAN_DEFAULT);
  gtk_widget_set_sensitive(sbar_delete, FALSE);
  sb_info.delete = sbar_delete;

  vbox5 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox5, "vbox5");
  gtk_widget_show (vbox5);
  gtk_box_pack_start (GTK_BOX (vbox4), vbox5, FALSE, TRUE, 0);

  label14 = gtk_label_new ("Be sure to enclose the hp/sp/mp max in ()!");
  gtk_widget_set_name (label14, "label14");
  gtk_widget_show (label14);
  gtk_box_pack_start (GTK_BOX (vbox5), label14, FALSE, FALSE, 0);

  hbox4 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox4, "hbox4");
  gtk_widget_show (hbox4);
  gtk_box_pack_start (GTK_BOX (vbox5), hbox4, TRUE, TRUE, 0);

  label13 = gtk_label_new ("Regex:");
  gtk_widget_set_name (label13, "label13");
  gtk_widget_show (label13);
  gtk_box_pack_start (GTK_BOX (hbox4), label13, FALSE, FALSE, 0);

  sbar_entry = gtk_entry_new ();
  gtk_widget_set_name (sbar_entry, "sbar_entry");
  gtk_widget_show (sbar_entry);
  gtk_box_pack_start (GTK_BOX (hbox4), sbar_entry, TRUE, TRUE, 0);
  sb_info.entry = GTK_ENTRY(sbar_entry);

  hbox3 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox3, "hbox3");
  gtk_widget_show (hbox3);
  gtk_box_pack_start (GTK_BOX (vbox4), hbox3, FALSE, TRUE, 0);

  sbar_about = gtk_button_new_from_stock ("gtk-about");
  gtk_widget_set_name (sbar_about, "sbar_about");
  gtk_widget_show (sbar_about);
  gtk_box_pack_start (GTK_BOX (hbox3), sbar_about, FALSE, FALSE, 0);

  hbuttonbox1 = gtk_hbutton_box_new ();
  gtk_widget_set_name (hbuttonbox1, "hbuttonbox1");
  gtk_widget_show (hbuttonbox1);
  gtk_box_pack_start (GTK_BOX (hbox3), hbuttonbox1, TRUE, TRUE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox1), GTK_BUTTONBOX_END);

  sbar_close = gtk_button_new_from_stock ("gtk-close");
  gtk_widget_set_name (sbar_close, "sbar_close");
  gtk_widget_show (sbar_close);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), sbar_close);
  GTK_WIDGET_SET_FLAGS (sbar_close, GTK_CAN_DEFAULT);

  sbar_ok = gtk_button_new_from_stock ("gtk-save");
  gtk_widget_set_name (sbar_ok, "sbar_ok");
  gtk_widget_show (sbar_ok);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), sbar_ok);
  GTK_WIDGET_SET_FLAGS (sbar_ok, GTK_CAN_DEFAULT);
  sb_info.save = sbar_ok;
  gtk_widget_set_sensitive(sb_info.save, FALSE);

  gtk_tree_view_set_rules_hint(sb_info.view, TRUE);
  gtk_tree_view_set_headers_visible(sb_info.view, TRUE);
  sb_info.store = gtk_tree_store_new(N_COLUMNS, G_TYPE_BOOLEAN, G_TYPE_STRING);
  gtk_tree_view_set_model(sb_info.view, GTK_TREE_MODEL(sb_info.store));
  sb_info.name_col = gtk_tree_view_column_new();
  sb_info.enabled_col = gtk_tree_view_column_new();
  gtk_tree_view_append_column(sb_info.view, sb_info.enabled_col);
  gtk_tree_view_append_column(sb_info.view, sb_info.name_col);
  gtk_tree_view_column_set_title(sb_info.name_col, "Name");
  gtk_tree_view_column_set_title(sb_info.enabled_col, "Enabled");
  sb_info.name_renderer = gtk_cell_renderer_text_new();
  sb_info.enabled_renderer = gtk_cell_renderer_toggle_new();
  gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(sb_info.enabled_renderer), TRUE);
  gtk_tree_view_column_pack_start(sb_info.name_col, sb_info.name_renderer, TRUE);
  gtk_tree_view_column_pack_start(sb_info.enabled_col, sb_info.enabled_renderer, TRUE);
  gtk_tree_view_column_add_attribute(sb_info.name_col, sb_info.name_renderer,
									   "text", NAME_COLUMN);
  gtk_tree_view_column_add_attribute(sb_info.enabled_col, sb_info.enabled_renderer,
									   "active", ENABLED_COLUMN);

  gtk_tree_store_clear(sb_info.store);
  g_signal_connect(G_OBJECT(sb_info.enabled_renderer), "toggled", G_CALLBACK(statusbar_config_enabled_toggle_cb), NULL);
  gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(sb_info.view), statusbar_config_select_cb, NULL, NULL);
  
  g_signal_connect(G_OBJECT(sbar_about), "clicked", G_CALLBACK(statusbar_config_about_cb), NULL);
  g_signal_connect(G_OBJECT(statusbarsconfig_win), "destroy", G_CALLBACK(statusbar_config_destroy_cb), NULL);
  g_signal_connect(G_OBJECT(sbar_add), "clicked", G_CALLBACK(statusbar_config_add_cb), NULL);
  g_signal_connect(G_OBJECT(sbar_delete), "clicked", G_CALLBACK(statusbar_config_delete_cb), NULL);
  g_signal_connect(G_OBJECT(sbar_close), "clicked", G_CALLBACK(statusbar_config_close_cb), NULL);
  g_signal_connect(G_OBJECT(sbar_ok), "clicked", G_CALLBACK(statusbar_config_ok_cb), NULL);
  
  gtk_window_resize(GTK_WINDOW(statusbarsconfig_win), 500,200);

  return statusbarsconfig_win;
}

GtkWidget*
create_statusbarsabout_dlg (void)
{
  GtkWidget *statusbarsabout_dlg;
  const gchar *authors[] = {
    "Les Harris <hl.harris@comcast.net>",
    NULL
  };

  statusbarsabout_dlg = gtk_about_dialog_new ();
  gtk_widget_set_name (statusbarsabout_dlg, "statusbarsabout_dlg");
  gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (statusbarsabout_dlg), VERSION);
  gtk_about_dialog_set_name (GTK_ABOUT_DIALOG (statusbarsabout_dlg), "Statusbars Plugin");
  gtk_about_dialog_set_comments (GTK_ABOUT_DIALOG (statusbarsabout_dlg), "A simple statusbar plugin for Gnome-MUD.");
  gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (statusbarsabout_dlg), authors);

  return statusbarsabout_dlg;
}
