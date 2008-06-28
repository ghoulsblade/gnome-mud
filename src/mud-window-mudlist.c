/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1998-2006 Robin Ericsson <lobbin@localhost.nu>
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
#include "mud-window-mudlist.h"
#include "mud-window-mudedit.h"
#include "utils.h"

struct _MudListWindowPrivate
{
	GList *mudList;

	gint CurrSelRow;
	gchar *CurrSelRowText;
	gchar *CurrSelMud;
	gchar *CurrIterStr;

	GtkWidget *dialog;

	GtkWidget *btnAdd;
	GtkWidget *btnEdit;
	GtkWidget *btnDel;
	GtkWidget *btnClose;

	GtkWidget *MudListTreeView;
	GtkWidget *MudDetailTextView;

	GtkTreeStore *MudListTreeStore;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
};

enum
{
	NAME_COLUMN,
	N_COLUMNS
};

GType mud_list_window_get_type (void);
static void mud_list_window_init (MudListWindow *preferences);
static void mud_list_window_class_init (MudListWindowClass *klass);
static void mud_list_window_finalize (GObject *object);

void mud_list_window_add_cb(GtkWidget *widget, MudListWindow *mudlist);
void mud_list_window_edit_cb(GtkWidget *widget, MudListWindow *mudlist);
void mud_list_window_del_cb(GtkWidget *widget, MudListWindow *mudlist);
void mud_list_window_close_cb(GtkWidget *widget, MudListWindow *mudlist);

gboolean mud_list_window_tree_select_cb(GtkTreeSelection *selection,
                     			GtkTreeModel     *model,
                     			GtkTreePath      *path,
                   			gboolean        path_currently_selected,
                     			gpointer          userdata);

// MudList class functions
GType
mud_list_window_get_type (void)
{
	static GType object_type = 0;

	g_type_init();

	if (!object_type)
	{
		static const GTypeInfo object_info =
		{
			sizeof (MudListWindowClass),
			NULL,
			NULL,
			(GClassInitFunc) mud_list_window_class_init,
			NULL,
			NULL,
			sizeof (MudListWindow),
			0,
			(GInstanceInitFunc) mud_list_window_init,
		};

		object_type = g_type_register_static(G_TYPE_OBJECT, "MudListWindow", &object_info, 0);
	}

	return object_type;
}

static void
mud_list_window_init (MudListWindow *mudlist)
{
	GladeXML *glade;

	mudlist->priv = g_new0(MudListWindowPrivate, 1);

	mudlist->priv->mudList = NULL;
	mudlist->priv->CurrSelRow = -1; // Nothing selected.

	glade = glade_xml_new(GLADEDIR "/muds.glade", "mudlist_window", NULL);
	mudlist->priv->dialog = glade_xml_get_widget(glade, "mudlist_window");
	mudlist->priv->btnAdd = glade_xml_get_widget(glade, "btnAdd");
	mudlist->priv->btnEdit = glade_xml_get_widget(glade, "btnEdit");
	mudlist->priv->btnDel = glade_xml_get_widget(glade, "btnDelete");
	mudlist->priv->btnClose = glade_xml_get_widget(glade, "btnClose");
	mudlist->priv->MudListTreeView = glade_xml_get_widget(glade, "MudListTreeView");
	mudlist->priv->MudDetailTextView = glade_xml_get_widget(glade, "MudDetailTextView");

	g_signal_connect(G_OBJECT(mudlist->priv->btnAdd), "clicked", G_CALLBACK(mud_list_window_add_cb), mudlist);
	g_signal_connect(G_OBJECT(mudlist->priv->btnEdit), "clicked", G_CALLBACK(mud_list_window_edit_cb), mudlist);
	g_signal_connect(G_OBJECT(mudlist->priv->btnDel), "clicked", G_CALLBACK(mud_list_window_del_cb), mudlist);
	g_signal_connect(G_OBJECT(mudlist->priv->btnClose), "clicked", G_CALLBACK(mud_list_window_close_cb), mudlist);

	/* I hate GtkTreeView and its loathsome ilk */
  	mudlist->priv->MudListTreeStore = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING);
  	gtk_tree_view_set_model(GTK_TREE_VIEW(mudlist->priv->MudListTreeView), GTK_TREE_MODEL(mudlist->priv->MudListTreeStore));

  	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(mudlist->priv->MudListTreeView), TRUE);
  	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(mudlist->priv->MudListTreeView), FALSE);
  	mudlist->priv->col = gtk_tree_view_column_new();

  	gtk_tree_view_append_column(GTK_TREE_VIEW(mudlist->priv->MudListTreeView), mudlist->priv->col);
  	mudlist->priv->renderer = gtk_cell_renderer_text_new();
  	gtk_tree_view_column_pack_start(mudlist->priv->col, mudlist->priv->renderer, TRUE);
  	gtk_tree_view_column_add_attribute(mudlist->priv->col, mudlist->priv->renderer, "text", NAME_COLUMN);

  	gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(GTK_TREE_VIEW(mudlist->priv->MudListTreeView)), mud_list_window_tree_select_cb, mudlist, NULL);

	mud_list_window_populate_treeview(mudlist);

	gtk_window_resize(GTK_WINDOW(mudlist->priv->dialog), 400,200);

	gtk_widget_show_all(mudlist->priv->dialog);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(mudlist->priv->dialog), TRUE);
	gtk_window_present(GTK_WINDOW(mudlist->priv->dialog));

	g_object_unref(glade);
}

static void
mud_list_window_class_init (MudListWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = mud_list_window_finalize;
}

static void
mud_list_window_finalize (GObject *object)
{
	MudListWindow *mudlist;
	GObjectClass *parent_class;

	mudlist = MUD_LIST_WINDOW(object);

	g_free(mudlist->priv);

	parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
	parent_class->finalize(object);
}

void
mud_list_window_populate_treeview(MudListWindow *mudlist)
{
	GtkTreeStore* store = GTK_TREE_STORE(mudlist->priv->MudListTreeStore);
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(mudlist->priv->MudDetailTextView));
	GtkTreeIter iter;
	GSList *muds, *entry;
	gchar *mname;
	GConfClient *client;
	GError *error = NULL;
	gchar keyname[2048];

	client = gconf_client_get_default();

	gtk_tree_store_clear(store);

	gtk_widget_set_sensitive(mudlist->priv->btnEdit, FALSE);
	gtk_widget_set_sensitive(mudlist->priv->btnDel, FALSE);

	gtk_text_buffer_set_text(buffer, "", 0);

	muds = gconf_client_get_list(gconf_client_get_default(), "/apps/gnome-mud/muds/list", GCONF_VALUE_STRING, NULL);

	for (entry = muds; entry != NULL; entry = g_slist_next(entry))
	{
		mname = g_strdup((gchar *) entry->data);

		g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/name", mname);

		gtk_tree_store_append(store, &iter, NULL);
		gtk_tree_store_set(store, &iter, NAME_COLUMN, gconf_client_get_string(client, keyname, &error), -1);
		g_free(mname);
	}
	g_slist_free(muds);
}

// Mudlist Callbacks
void
mud_list_window_add_cb(GtkWidget *widget, MudListWindow *mudlist)
{
	gchar name[1024];

	g_snprintf(name, 1024, "New MUD %d", gtk_tree_model_iter_n_children(GTK_TREE_MODEL(mudlist->priv->MudListTreeStore),NULL) + 1);

	mud_window_mudedit_new(name, mudlist,TRUE);
}

void
mud_list_window_edit_cb(GtkWidget *widget, MudListWindow *mudlist)
{
	mud_window_mudedit_new(mudlist->priv->CurrSelRowText, mudlist, FALSE);
}

void
mud_list_window_del_cb(GtkWidget *widget, MudListWindow *mudlist)
{
	GSList *muds, *entry, *rementry;
	GConfClient *client;
	GError *error = NULL;

	rementry = NULL;
	rementry = g_slist_append(rementry, NULL);

	client = gconf_client_get_default();

	muds = gconf_client_get_list(client, "/apps/gnome-mud/muds/list", GCONF_VALUE_STRING, &error);

	for (entry = muds; entry != NULL; entry = g_slist_next(entry))
	{
		if(strcmp((gchar *)entry->data, mudlist->priv->CurrSelRowText) == 0)
		{
			rementry->data = entry->data;
		}
	}


	muds = g_slist_remove(muds, rementry->data);

	gconf_client_set_list(client, "/apps/gnome-mud/muds/list", GCONF_VALUE_STRING, muds, &error);

	mud_list_window_populate_treeview(mudlist);
}

void
mud_list_window_close_cb(GtkWidget *widget, MudListWindow *mudlist)
{
	gtk_widget_destroy(mudlist->priv->dialog);
	mud_list_window_finalize(G_OBJECT(mudlist));
}

gboolean
mud_list_window_tree_select_cb(GtkTreeSelection *selection,
			       GtkTreeModel     *model,
			       GtkTreePath      *path,
			       gboolean          path_currently_selected,
			       gpointer          userdata)
{
	GtkTreeIter iter;
	MudListWindow *mudlist = (MudListWindow *)userdata;
	GConfClient *client;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(mudlist->priv->MudDetailTextView));
	gchar keyname[2048];
	gchar *desc;
	GError *error = NULL;

	gtk_text_buffer_set_text(buffer, "", 0);

	if (gtk_tree_model_get_iter(model, &iter, path))
	{
		client = gconf_client_get_default();

		gtk_tree_model_get(model, &iter, 0, &mudlist->priv->CurrSelRowText, -1);

		mudlist->priv->CurrSelRow = (gtk_tree_path_get_indices(path))[0];
		mudlist->priv->CurrIterStr = gtk_tree_model_get_string_from_iter(model, &iter);

		gtk_widget_set_sensitive(mudlist->priv->btnEdit, TRUE);
		gtk_widget_set_sensitive(mudlist->priv->btnDel, TRUE);

		g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/desc", remove_whitespace(mudlist->priv->CurrSelRowText));

		desc = gconf_client_get_string(client, keyname, &error);
		if(desc)
		{
			gtk_text_buffer_set_text(buffer, desc, strlen(desc));
			g_free(desc);
		}

	}

	return TRUE;
}

// Instantiate MudListWindow
MudListWindow*
mud_window_mudlist_new(void)
{
	MudListWindow *mudlist;

	mudlist = g_object_new(MUD_TYPE_LIST_WINDOW, NULL);

	return mudlist;
}
