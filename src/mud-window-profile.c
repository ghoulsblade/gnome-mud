#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glade/glade.h>
#include <gtk/gtk.h>
#include <libgnome/gnome-i18n.h>
#include <stdlib.h>
#include <gconf/gconf-client.h>

#include "gnome-mud.h"
#include "mud-connection-view.h"
#include "mud-window.h"
#include "mud-window-profile.h"
#include "utils.h"

struct _MudProfileWindowPrivate
{
	MudWindow *window;
	
	GtkWidget *dialog;
	GtkWidget *treeview;
	
	GtkWidget *btnAdd;
	GtkWidget *btnDelete;
	GtkWidget *btnClose;
	
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

void mud_profile_window_close_cb(GtkWidget *widget, MudProfileWindow *profwin);
void mud_profile_window_add_cb(GtkWidget *widget, MudProfileWindow *profwin);
void mud_profile_window_del_cb(GtkWidget *widget, MudProfileWindow *profwin);

void mud_profile_window_populate_treeview(MudProfileWindow *profwin);

gboolean mud_profile_window_tree_select_cb(GtkTreeSelection *selection,
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
	
	glade = glade_xml_new(GLADEDIR "/prefs.glade", "profiles_window", "profiles_window");

	profwin->priv->dialog = glade_xml_get_widget(glade, "profiles_window");

	profwin->priv->btnAdd = glade_xml_get_widget(glade, "btnAdd");
	profwin->priv->btnDelete = glade_xml_get_widget(glade, "btnDelete");
	profwin->priv->btnClose = glade_xml_get_widget(glade, "btnClose");
				
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

	g_signal_connect(G_OBJECT(profwin->priv->btnClose), "clicked", G_CALLBACK(mud_profile_window_close_cb), profwin);
	g_signal_connect(G_OBJECT(profwin->priv->btnAdd), "clicked", G_CALLBACK(mud_profile_window_add_cb), profwin);
	g_signal_connect(G_OBJECT(profwin->priv->btnDelete), "clicked", G_CALLBACK(mud_profile_window_del_cb), profwin);
	
	gtk_widget_show_all(profwin->priv->dialog);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(profwin->priv->dialog), TRUE);
	gtk_window_present(GTK_WINDOW(profwin->priv->dialog));

	gtk_window_resize(GTK_WINDOW(profwin->priv->dialog), 300,300);
	
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
void 
mud_profile_window_add_cb(GtkWidget *widget, MudProfileWindow *profwin)
{
	GladeXML *glade;
	GtkWidget *dialog;
	GtkWidget *entry_profile;
	gchar *profile;
	gint result;
	MudProfile *def, *prof;

	glade = glade_xml_new(GLADEDIR "/prefs.glade", "newprofile_dialog", "newprofile_dialog");
	dialog = glade_xml_get_widget(glade, "newprofile_dialog");
	
	entry_profile = glade_xml_get_widget(glade, "entry_profile");

	result = gtk_dialog_run(GTK_DIALOG(dialog));
	if (result == GTK_RESPONSE_OK)
	{
		profile = remove_whitespace((gchar *)gtk_entry_get_text(GTK_ENTRY(entry_profile)));
		
		def = get_profile("Default");
		prof = mud_profile_new((const gchar *)profile);
			
		mud_profile_copy_preferences(def, prof);

		mud_profile_window_populate_treeview(profwin);
		mud_window_populate_profiles_menu(profwin->priv->window);
	}

	gtk_widget_destroy(dialog);
	g_object_unref(glade);
}

void 
mud_profile_window_del_cb(GtkWidget *widget, MudProfileWindow *profwin)
{

	if(strcmp("Default", profwin->priv->CurrSelRowText) != 0)
	{
		mud_profile_delete(profwin->priv->CurrSelRowText);

		mud_profile_window_populate_treeview(profwin);
		mud_window_populate_profiles_menu(profwin->priv->window);
	}
}

void 
mud_profile_window_close_cb(GtkWidget *widget, MudProfileWindow *profwin)
{
	gtk_widget_destroy(profwin->priv->dialog);
	g_object_unref(profwin);
}

gboolean 
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
void 
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

	profwin->priv->window = window;

	return profwin;	
}
