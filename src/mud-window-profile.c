/* GNOME-Mud - A simple Mud Client
 * mud-window-profile.c
 * Copyright (C) 2008-2009 Les Harris <lharris@gnome.org>
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
#include <glade/glade-xml.h>
#include <glib/gi18n.h>
#include <string.h>

#include "gnome-mud.h"
#include "mud-connection-view.h"
#include "mud-window.h"
#include "mud-window-profile.h"
#include "utils.h"

struct _MudProfileWindowPrivate
{
	MudWindow *parent;

	GtkWidget *window;
	GtkWidget *treeview;

	GtkWidget *btnAdd;
	GtkWidget *btnDelete;

	gint CurrSelRow;
	gchar *CurrSelRowText;

	GtkTreeStore *treestore;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
};

enum
{
	NAME_COLUMN,
	N_COLUMNS
};

GType mud_profile_window_get_type (void);
static void mud_profile_window_init (MudProfileWindow *preferences);
static void mud_profile_window_class_init (MudProfileWindowClass *klass);
static void mud_profile_window_finalize (GObject *object);

static gint mud_profile_window_close_cb(GtkWidget *widget, MudProfileWindow *profwin);
static void mud_profile_window_add_cb(GtkWidget *widget, MudProfileWindow *profwin);
static void mud_profile_window_del_cb(GtkWidget *widget, MudProfileWindow *profwin);

static void mud_profile_window_populate_treeview(MudProfileWindow *profwin);

static gboolean mud_profile_window_tree_select_cb(GtkTreeSelection *selection,
                     			   GtkTreeModel     *model,
                     			   GtkTreePath      *path,
                   			   gboolean        path_currently_selected,
                     			   gpointer          userdata);

// MudProfile class functions
GType
mud_profile_window_get_type (void)
{
	static GType object_type = 0;

	g_type_init();

	if (!object_type)
	{
		static const GTypeInfo object_info =
		{
			sizeof (MudProfileWindowClass),
			NULL,
			NULL,
			(GClassInitFunc) mud_profile_window_class_init,
			NULL,
			NULL,
			sizeof (MudProfileWindow),
			0,
			(GInstanceInitFunc) mud_profile_window_init,
		};

		object_type = g_type_register_static(G_TYPE_OBJECT, "MudProfileWindow", &object_info, 0);
	}

	return object_type;
}

static void
mud_profile_window_init (MudProfileWindow *profwin)
{
	GladeXML *glade;

	profwin->priv = g_new0(MudProfileWindowPrivate, 1);

	glade = glade_xml_new(GLADEDIR "/prefs.glade", "profiles_window", NULL);

	profwin->priv->window = glade_xml_get_widget(glade, "profiles_window");

	profwin->priv->btnAdd = glade_xml_get_widget(glade, "btnAdd");
	profwin->priv->btnDelete = glade_xml_get_widget(glade, "btnDelete");

	profwin->priv->treeview = glade_xml_get_widget(glade, "profilesView");
  	profwin->priv->treestore = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING);

  	gtk_tree_view_set_model(GTK_TREE_VIEW(profwin->priv->treeview), GTK_TREE_MODEL(profwin->priv->treestore));

  	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(profwin->priv->treeview), TRUE);
  	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(profwin->priv->treeview), FALSE);
  	profwin->priv->col = gtk_tree_view_column_new();

  	gtk_tree_view_append_column(GTK_TREE_VIEW(profwin->priv->treeview), profwin->priv->col);
  	profwin->priv->renderer = gtk_cell_renderer_text_new();
  	gtk_tree_view_column_pack_start(profwin->priv->col, profwin->priv->renderer, TRUE);
  	gtk_tree_view_column_add_attribute(profwin->priv->col, profwin->priv->renderer, "text", NAME_COLUMN);

  	gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(GTK_TREE_VIEW(profwin->priv->treeview)), mud_profile_window_tree_select_cb, profwin, NULL);

	mud_profile_window_populate_treeview(profwin);

	g_signal_connect(profwin->priv->window, "destroy",
			G_CALLBACK(mud_profile_window_close_cb),
			profwin);
	g_signal_connect(profwin->priv->btnAdd, "clicked",
			G_CALLBACK(mud_profile_window_add_cb),
			profwin);
	g_signal_connect(profwin->priv->btnDelete, "clicked",
			G_CALLBACK(mud_profile_window_del_cb),
		       	profwin);

	g_object_unref(glade);
}

static void
mud_profile_window_class_init (MudProfileWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = mud_profile_window_finalize;
}

static void
mud_profile_window_finalize (GObject *object)
{
	MudProfileWindow *profwin;
	GObjectClass *parent_class;

	profwin = MUD_PROFILE_WINDOW(object);

	g_free(profwin->priv);

	parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
	parent_class->finalize(object);
}

// MudProfileWindow Callbacks
static void
mud_profile_window_add_cb(GtkWidget *widget, MudProfileWindow *profwin)
{
	GladeXML *glade;
	GtkWidget *window;
	GtkWidget *entry_profile;
	gchar *profile;
	gint result;
	MudProfile *def, *prof;

	glade = glade_xml_new(GLADEDIR "/prefs.glade", "newprofile_dialog", NULL);
	window = glade_xml_get_widget(glade, "newprofile_dialog");

	gtk_window_set_transient_for(
			GTK_WINDOW(window),
			GTK_WINDOW(profwin->priv->window));

	entry_profile = glade_xml_get_widget(glade, "entry_profile");

	result = gtk_dialog_run(GTK_DIALOG(window));
	if (result == GTK_RESPONSE_OK)
	{
		profile = utils_remove_whitespace((gchar *)gtk_entry_get_text(GTK_ENTRY(entry_profile)));

		def = get_profile("Default");
		prof = mud_profile_new((const gchar *)profile);

		mud_profile_copy_preferences(def, prof);

		mud_profile_window_populate_treeview(profwin);
		mud_window_populate_profiles_menu(profwin->priv->parent);
	}

	gtk_widget_destroy(window);
	g_object_unref(glade);
}

static void
mud_profile_window_del_cb(GtkWidget *widget, MudProfileWindow *profwin)
{

	if(strcmp("Default", profwin->priv->CurrSelRowText) != 0)
	{
		mud_profile_delete(profwin->priv->CurrSelRowText);

		mud_profile_window_populate_treeview(profwin);
		mud_window_populate_profiles_menu(profwin->priv->parent);
	}
}

static gint
mud_profile_window_close_cb(GtkWidget *widget, MudProfileWindow *profwin)
{
	g_object_unref(profwin);

	return TRUE;
}

static gboolean
mud_profile_window_tree_select_cb(GtkTreeSelection *selection,
                     		  GtkTreeModel     *model,
                     	          GtkTreePath      *path,
                   	          gboolean        path_currently_selected,
                     		  gpointer          userdata)
{
	GtkTreeIter iter;
	MudProfileWindow *profwin = (MudProfileWindow *)userdata;

	if (gtk_tree_model_get_iter(model, &iter, path))
	{
		gtk_tree_model_get(model, &iter, 0, &profwin->priv->CurrSelRowText, -1);

		profwin->priv->CurrSelRow = (gtk_tree_path_get_indices(path))[0];

		gtk_widget_set_sensitive(profwin->priv->btnDelete, TRUE);

	}

	return TRUE;
}

// MudProfileWindow Util Functions
static void
mud_profile_window_populate_treeview(MudProfileWindow *profwin)
{
	const GList *profiles;
	GList *entry;
	GtkTreeStore* store = GTK_TREE_STORE(profwin->priv->treestore);
	GtkTreeIter iter;

	gtk_tree_store_clear(store);

	gtk_widget_set_sensitive(profwin->priv->btnDelete, FALSE);

	profiles = mud_profile_get_profiles();

	for (entry = (GList *)profiles; entry != NULL; entry = g_list_next(entry))
	{
		gtk_tree_store_append(store, &iter, NULL);
		gtk_tree_store_set(store, &iter, NAME_COLUMN,(gchar *)MUD_PROFILE(entry->data)->name, -1);
	}
}

// Instantiate MudProfileWindow
MudProfileWindow*
mud_window_profile_new(MudWindow *window)
{
	MudProfileWindow *profwin;

	profwin = g_object_new(MUD_TYPE_PROFILE_WINDOW, NULL);
	profwin->priv->parent = window;

	gtk_window_set_transient_for(
			GTK_WINDOW(profwin->priv->window),
			GTK_WINDOW(mud_window_get_window(profwin->priv->parent)));
	gtk_widget_show_all(profwin->priv->window);

	return profwin;
}
