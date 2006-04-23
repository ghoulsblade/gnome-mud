#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gconf/gconf-client.h>
#include <glib/gi18n.h>
#include <glade/glade-xml.h>

#include "gnome-mud.h"
#include "mud-connection-view.h"
#include "mud-window.h"
#include "mud-tray.h"
#include "mud-window-mconnect.h"
#include "mud-profile.h"
#include "utils.h"

struct _MudMConnectWindowPrivate
{
	gint CurrSelRow;
	gchar *CurrSelRowText;
	gchar *CurrSelMud;
	gchar *CurrIterStr;

	gchar *SelHost;
	gchar *SelChar;
	gchar *SelConnect;
	gint SelPort;
	
	GtkWidget *dialog;

	GtkWidget *mudView;
	
	GtkWidget *btnConnect;
	GtkWidget *btnClose;

	GtkWidget *profileCombo;
	GtkListStore *comboStore;
	GtkCellRenderer *comborender;

	GtkTreeStore *store;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	MudConnectionView *view;

	MudWindow *window;
	GtkWidget *winwidget;
	
	MudTray *tray;
};

enum
{
	NAME_COLUMN,
	N_COLUMNS
};

GType mud_mconnect_window_get_type (void);
static void mud_mconnect_window_init (MudMConnectWindow *preferences);
static void mud_mconnect_window_class_init (MudMConnectWindowClass *klass);
static void mud_mconnect_window_finalize (GObject *object);

void mud_mconnect_window_connect_cb(GtkWidget *widget, MudMConnectWindow *mconnect);
void mud_mconnect_window_close_cb(GtkWidget *widget, MudMConnectWindow *mconnect);

void  mud_mconnect_window_populate_treeview(MudMConnectWindow *mconnect);
void mud_mconnect_populate_profiles(MudMConnectWindow *mconnect);

gboolean mud_mconnect_select_cb(GtkTreeSelection *selection,
                     			GtkTreeModel     *model,
                     			GtkTreePath      *path,
                   			gboolean        path_currently_selected,
                     			gpointer          userdata);
// MudMConnect class functions
GType
mud_mconnect_window_get_type (void)
{
	static GType object_type = 0;

	g_type_init();

	if (!object_type)
	{
		static const GTypeInfo object_info =
		{
			sizeof (MudMConnectWindowClass),
			NULL,
			NULL,
			(GClassInitFunc) mud_mconnect_window_class_init,
			NULL,
			NULL,
			sizeof (MudMConnectWindow),
			0,
			(GInstanceInitFunc) mud_mconnect_window_init,
		};

		object_type = g_type_register_static(G_TYPE_OBJECT, "MudMConnectWindow", &object_info, 0);
	}

	return object_type;
}

static void
mud_mconnect_window_init (MudMConnectWindow *mconnect)
{
	GladeXML *glade;

	mconnect->priv = g_new0(MudMConnectWindowPrivate, 1);
	
	glade = glade_xml_new(GLADEDIR "/connect.glade", "main_connect", NULL);
	
	mconnect->priv->dialog = glade_xml_get_widget(glade, "main_connect");
	
	mconnect->priv->mudView = glade_xml_get_widget(glade, "mudView");
	
	mconnect->priv->btnConnect = glade_xml_get_widget(glade, "btnConnect");
	mconnect->priv->btnClose = glade_xml_get_widget(glade, "btnClose");

	mconnect->priv->profileCombo = glade_xml_get_widget(glade, "comboProfiles");

	mconnect->priv->comboStore = gtk_list_store_new(1, G_TYPE_STRING);
	gtk_list_store_clear(GTK_LIST_STORE(mconnect->priv->comboStore));
	gtk_combo_box_set_model(GTK_COMBO_BOX(mconnect->priv->profileCombo), GTK_TREE_MODEL(mconnect->priv->comboStore));
	mconnect->priv->comborender = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(mconnect->priv->profileCombo), mconnect->priv->comborender, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(mconnect->priv->profileCombo), mconnect->priv->comborender, "text", 0, NULL);

	mud_mconnect_populate_profiles(mconnect);
	
	mconnect->priv->store = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING);
  	gtk_tree_view_set_model(GTK_TREE_VIEW(mconnect->priv->mudView), GTK_TREE_MODEL(mconnect->priv->store));
  	
  	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(mconnect->priv->mudView), TRUE);
  	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(mconnect->priv->mudView), FALSE);
  	mconnect->priv->col = gtk_tree_view_column_new();
 	
  	gtk_tree_view_append_column(GTK_TREE_VIEW(mconnect->priv->mudView), mconnect->priv->col);
  	mconnect->priv->renderer = gtk_cell_renderer_text_new();
  	gtk_tree_view_column_pack_start(mconnect->priv->col, mconnect->priv->renderer, TRUE);
  	gtk_tree_view_column_add_attribute(mconnect->priv->col, mconnect->priv->renderer, "text", NAME_COLUMN);
  	
  	gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(GTK_TREE_VIEW(mconnect->priv->mudView)), mud_mconnect_select_cb, mconnect, NULL);

	mud_mconnect_window_populate_treeview(mconnect);

	g_signal_connect(G_OBJECT(mconnect->priv->btnConnect), "clicked", G_CALLBACK(mud_mconnect_window_connect_cb), mconnect);
	g_signal_connect(G_OBJECT(mconnect->priv->btnClose), "clicked", G_CALLBACK(mud_mconnect_window_close_cb), mconnect);

	gtk_window_resize(GTK_WINDOW(mconnect->priv->dialog), 400,200);
	
	gtk_widget_show_all(mconnect->priv->dialog);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(mconnect->priv->dialog), TRUE);
	gtk_window_present(GTK_WINDOW(mconnect->priv->dialog));

	g_object_unref(glade);
}

static void
mud_mconnect_window_class_init (MudMConnectWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = mud_mconnect_window_finalize;
}

static void
mud_mconnect_window_finalize (GObject *object)
{
	MudMConnectWindow *MudMConnect;
	GObjectClass *parent_class;

	MudMConnect = MUD_MCONNECT_WINDOW(object);
	
	g_free(MudMConnect->priv);

	parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
	parent_class->finalize(object);
}

void 
mud_mconnect_populate_profiles(MudMConnectWindow *mconnect)
{
	GSList *profiles, *entry;
	GConfClient *client;
	GError *error = NULL;
	gchar keyname[2048];
	GtkTreeIter iter;

	client = gconf_client_get_default();
	
	g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/list");
	
	profiles = gconf_client_get_list(client, keyname, GCONF_VALUE_STRING, &error);
	for (entry = profiles; entry != NULL; entry = g_slist_next(entry))
	{
		gtk_list_store_append(GTK_LIST_STORE(mconnect->priv->comboStore), &iter);
		gtk_list_store_set(GTK_LIST_STORE(mconnect->priv->comboStore), &iter, 0, (gchar *)entry->data, -1);
	}
}

void
mud_mconnect_window_populate_treeview(MudMConnectWindow *mconnect)
{
	GtkTreeStore* store = GTK_TREE_STORE(mconnect->priv->store);
	GtkTreeIter iter, child;
	GSList *muds, *entry, *chars, *centry;
	gchar *mname;
	GConfClient *client;
	GError *error = NULL;
	gchar keyname[2048];
	int show;
	
	gtk_tree_store_clear(store);
	
	client = gconf_client_get_default();
	
	muds = gconf_client_get_list(client, "/apps/gnome-mud/muds/list", GCONF_VALUE_STRING, &error);

	gtk_widget_set_sensitive(mconnect->priv->btnConnect,FALSE);
	
	for (entry = muds; entry != NULL; entry = g_slist_next(entry))
	{
		mname = g_strdup((gchar *) entry->data);
		
		g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/show", mname);
		
		show = gconf_client_get_int(client, keyname, &error);

		if(show)
		{
			g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/name", mname);
			
			gtk_tree_store_append(store, &iter, NULL);
			gtk_tree_store_set(store, &iter, NAME_COLUMN, gconf_client_get_string(client, keyname, &error), -1);

			g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/chars/list", mname);
			chars = gconf_client_get_list(client, keyname, GCONF_VALUE_STRING, &error);
			
			for(centry = chars; centry != NULL; centry = g_slist_next(centry))
			{
				gtk_tree_store_append(store, &child, &iter);
				gtk_tree_store_set(store, &child, NAME_COLUMN, (gchar *)centry->data,-1);
			}
		}

		g_free(mname);
	}	
}
// MudMConnectWindow Callbacks
void 
mud_mconnect_window_connect_cb(GtkWidget *widget, MudMConnectWindow *mconnect)
{
	if(mconnect->priv->SelPort < 1)
		mconnect->priv->SelPort = 23;
		
	mud_tray_update_icon(mconnect->priv->tray, offline);
	
	mconnect->priv->view = mud_connection_view_new("Default", mconnect->priv->SelHost, mconnect->priv->SelPort, mconnect->priv->winwidget, (GtkWidget *)mconnect->priv->tray, mconnect->priv->CurrSelMud);
	
	mud_window_add_connection_view(mconnect->priv->window, mconnect->priv->view, mconnect->priv->CurrSelMud);

	mud_connection_view_set_profile(mconnect->priv->view, get_profile((const gchar *)mud_profile_from_number(gtk_combo_box_get_active(GTK_COMBO_BOX(mconnect->priv->profileCombo)))));
	mud_window_profile_menu_set_active(mud_profile_from_number(gtk_combo_box_get_active(GTK_COMBO_BOX(mconnect->priv->profileCombo))),mconnect->priv->window);

	if(mconnect->priv->SelConnect)
	{
		mud_connection_view_set_connect_string(mconnect->priv->view, mconnect->priv->SelConnect);
	}

	gtk_widget_destroy(mconnect->priv->dialog);
}

void 
mud_mconnect_window_close_cb(GtkWidget *widget, MudMConnectWindow *mconnect)
{
	gtk_widget_destroy(mconnect->priv->dialog);
}

gboolean 
mud_mconnect_select_cb(GtkTreeSelection *selection,
                      GtkTreeModel     *model,
                      GtkTreePath      *path,
                      gboolean          path_currently_selected,
                      gpointer          userdata)
{
	GtkTreeIter iter, top;
	GtkTreePath *apath;
	MudMConnectWindow *mconnect = (MudMConnectWindow *)userdata;
	GConfClient *client;
	GError *error = NULL;
	char keyname[2048];
	char *name;

	
	client = gconf_client_get_default();

	mconnect->priv->SelChar = NULL;
	mconnect->priv->SelConnect = NULL;
		
	if (gtk_tree_model_get_iter(model, &iter, path)) 
	{		
		gtk_tree_model_get(model, &iter, 0, &mconnect->priv->CurrSelRowText, -1);
    	
		mconnect->priv->CurrSelRow = (gtk_tree_path_get_indices(path))[0];
		mconnect->priv->CurrIterStr = gtk_tree_model_get_string_from_iter(model, &iter);	
		
		apath = gtk_tree_path_new_from_indices(mconnect->priv->CurrSelRow,-1);
		
		gtk_tree_model_get_iter(model, &top, apath);
		
		gtk_tree_model_get(model, &top, 0, &mconnect->priv->CurrSelMud,-1);
		
		name = remove_whitespace(mconnect->priv->CurrSelMud);
		
		g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/host", name);
		mconnect->priv->SelHost = gconf_client_get_string(client, keyname, &error);
		
		g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/port",name);
		mconnect->priv->SelPort = gconf_client_get_int(client, keyname, &error);
		if(strcmp(mconnect->priv->CurrSelRowText,mconnect->priv->CurrSelMud) != 0)
		{
			mconnect->priv->SelChar = g_strdup(mconnect->priv->CurrSelRowText);
			g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/chars/%s/connect", name, mconnect->priv->SelChar);
			mconnect->priv->SelConnect = gconf_client_get_string(client, keyname, &error);
		}
		
		g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/profile", name);
		gtk_combo_box_set_active(GTK_COMBO_BOX(mconnect->priv->profileCombo),gconf_client_get_int(client, keyname, &error));

		gtk_widget_set_sensitive(mconnect->priv->btnConnect,TRUE);
	}
	
	return TRUE;
}

// Instantiate MudMConnectWindow
MudMConnectWindow*
mud_window_mconnect_new(MudWindow *win, GtkWidget *winwidget, MudTray *tray)
{
	MudMConnectWindow *MudMConnect;
	
	MudMConnect = g_object_new(MUD_TYPE_MCONNECT_WINDOW, NULL);

	MudMConnect->priv->window = win;
	MudMConnect->priv->winwidget = winwidget;
	MudMConnect->priv->tray = tray;
	
	return MudMConnect;	
}
