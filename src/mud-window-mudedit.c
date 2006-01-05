#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gconf/gconf-client.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <glade/glade-xml.h>

#include "gnome-mud.h"
#include "gconf-helper.h"
#include "mud-connection-view.h"
#include "mud-window.h"
#include "mud-window-mudlist.h"
#include "mud-window-mudedit.h"
#include "mud-profile.h"
#include "utils.h"

gchar *gmud;
gboolean gNewMud;
MudListWindow *gParent;

struct _MudEditWindowPrivate
{
	GConfClient *gconf_client;
	gchar *mud;
	gint CurrSelRow;
	gchar *CurrSelRowText;
	gchar *CurrSelMud;
	gchar *CurrIterStr;
	
	GtkWidget *dialog;
	
	GtkWidget *btnAdd;
	GtkWidget *btnProps;
	GtkWidget *btnCancel;
	GtkWidget *btnOK;
	GtkWidget *btnShowToggle;
	GtkWidget *btnDel;

	GtkWidget *MudCodeBaseCombo;
	GtkWidget *MudProfileCombo;
	GtkListStore *MudProfileComboStore;
	GtkCellRenderer *comborender;

	GtkWidget *EntryName;
	GtkWidget *EntryHost;
	GtkWidget *EntryPort;
	GtkWidget *EntryTheme;

	GtkWidget *MudDescTextView;
	GtkWidget *CharView;
	GtkTreeStore *CharStore;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
};

enum
{
	NAME_COLUMN,
	N_COLUMNS
};

GType mud_edit_window_get_type (void);
static void mud_edit_window_init (MudEditWindow *preferences);
static void mud_edit_window_class_init (MudEditWindowClass *klass);
static void mud_edit_window_finalize (GObject *object);

void mud_edit_window_query_gconf(MudEditWindow *mudedit);
void mud_edit_window_query_glade(MudEditWindow *mudedit);
void mud_edit_window_connect_signals(MudEditWindow *mudedit);

void mud_edit_window_add_cb(GtkWidget *widget, MudEditWindow *mudedit);
void mud_edit_window_props_cb(GtkWidget *widget, MudEditWindow *mudedit);
void mud_edit_window_ok_cb(GtkWidget *widget, MudEditWindow *mudedit);
void mud_edit_window_cancel_cb(GtkWidget *widget, MudEditWindow *mudedit);
void mud_edit_window_del_cb(GtkWidget *widget, MudEditWindow *mudedit);

void props_window_dialog(gchar *charname, MudEditWindow *mudedit, gboolean NewChar);
void populate_charview(MudEditWindow *mudedit);
void populate_profiles(MudEditWindow *mudedit);

gboolean mud_edit_window_tree_select_cb(GtkTreeSelection *selection,
                     			GtkTreeModel     *model,
                     			GtkTreePath      *path,
                   			gboolean        path_currently_selected,
                     			gpointer          userdata);
// MudEdit class functions
GType
mud_edit_window_get_type (void)
{
	static GType object_type = 0;

	g_type_init();

	if (!object_type)
	{
		static const GTypeInfo object_info =
		{
			sizeof (MudEditWindowClass),
			NULL,
			NULL,
			(GClassInitFunc) mud_edit_window_class_init,
			NULL,
			NULL,
			sizeof (MudEditWindow),
			0,
			(GInstanceInitFunc) mud_edit_window_init,
		};

		object_type = g_type_register_static(G_TYPE_OBJECT, "MudEditWindow", &object_info, 0);
	}

	return object_type;
}

static void
mud_edit_window_init (MudEditWindow *mudedit)
{
	mudedit->priv = g_new0(MudEditWindowPrivate, 1);

	mudedit->priv->mud = g_strdup(remove_whitespace(gmud));
	g_free(gmud);
	
	mud_edit_window_query_glade(mudedit);
	populate_profiles(mudedit);
	mud_edit_window_connect_signals(mudedit);
	mud_edit_window_query_gconf(mudedit);

	mudedit->priv->CharStore = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING);
  	gtk_tree_view_set_model(GTK_TREE_VIEW(mudedit->priv->CharView), GTK_TREE_MODEL(mudedit->priv->CharStore));
  	
  	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(mudedit->priv->CharView), TRUE);
  	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(mudedit->priv->CharView), FALSE);
  	mudedit->priv->col = gtk_tree_view_column_new();

  	gtk_tree_view_append_column(GTK_TREE_VIEW(mudedit->priv->CharView), mudedit->priv->col);
  	mudedit->priv->renderer = gtk_cell_renderer_text_new();
  	gtk_tree_view_column_pack_start(mudedit->priv->col, mudedit->priv->renderer, TRUE);
  	gtk_tree_view_column_add_attribute(mudedit->priv->col, mudedit->priv->renderer, "text", NAME_COLUMN);
  	
  	gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(GTK_TREE_VIEW(mudedit->priv->CharView)), mud_edit_window_tree_select_cb, mudedit, NULL);
  	
	populate_charview(mudedit);
	
	gtk_widget_show_all(mudedit->priv->dialog);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(mudedit->priv->dialog), TRUE);
	gtk_window_present(GTK_WINDOW(mudedit->priv->dialog));
}

static void
mud_edit_window_class_init (MudEditWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = mud_edit_window_finalize;
}

static void
mud_edit_window_finalize (GObject *object)
{
	MudEditWindow *mudedit;
	GObjectClass *parent_class;

	mudedit = MUD_EDIT_WINDOW(object);
	
	g_free(mudedit->priv);

	parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
	parent_class->finalize(object);
}

// MudEditWindow Utility Functions
void 
mud_edit_window_query_gconf(MudEditWindow *mudedit)
{
	GConfClient *client;
	GError *error = NULL;
	gchar keyname[2048];
	gchar buf[255];
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(mudedit->priv->MudDescTextView));
	gchar *desc;

	client = gconf_client_get_default();

	if(gNewMud)
	{
		gtk_entry_set_text(GTK_ENTRY(mudedit->priv->EntryName), mudedit->priv->mud);
	}
	else
	{
		g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/name", mudedit->priv->mud);
		gtk_entry_set_text(GTK_ENTRY(mudedit->priv->EntryName),gconf_client_get_string(client, keyname, &error));
	}

	g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/host", mudedit->priv->mud);
	gtk_entry_set_text(GTK_ENTRY(mudedit->priv->EntryHost),gconf_client_get_string(client, keyname, &error));
	
	g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/port", mudedit->priv->mud);
	g_snprintf(buf, 255, "%d", gconf_client_get_int(client, keyname, &error));
	gtk_entry_set_text(GTK_ENTRY(mudedit->priv->EntryPort), buf);

	g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/show", mudedit->priv->mud);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mudedit->priv->btnShowToggle),gconf_client_get_int(client, keyname, &error));

	g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/desc", mudedit->priv->mud);
	desc = g_strdup(gconf_client_get_string(client, keyname, &error));

	if(desc) 
	{
		gtk_text_buffer_set_text(buffer, desc, strlen(desc));
		g_free(desc);
	}

	g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/theme", mudedit->priv->mud);
	gtk_entry_set_text(GTK_ENTRY(mudedit->priv->EntryTheme), gconf_client_get_string(client, keyname, &error));

	g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/codebase", mudedit->priv->mud);
	gtk_combo_box_set_active(GTK_COMBO_BOX(mudedit->priv->MudCodeBaseCombo), gconf_client_get_int(client, keyname, &error));	

	g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/profile", mudedit->priv->mud);
	gtk_combo_box_set_active(GTK_COMBO_BOX(mudedit->priv->MudProfileCombo), gconf_client_get_int(client, keyname, &error));
	
}

void 
mud_edit_window_connect_signals(MudEditWindow *mudedit)
{
	g_signal_connect(G_OBJECT(mudedit->priv->btnProps), "clicked", G_CALLBACK(mud_edit_window_props_cb), mudedit);
	g_signal_connect(G_OBJECT(mudedit->priv->btnAdd), "clicked", G_CALLBACK(mud_edit_window_add_cb), mudedit);
	g_signal_connect(G_OBJECT(mudedit->priv->btnOK), "clicked", G_CALLBACK(mud_edit_window_ok_cb), mudedit);
	g_signal_connect(G_OBJECT(mudedit->priv->btnCancel), "clicked", G_CALLBACK(mud_edit_window_cancel_cb), mudedit);
	g_signal_connect(G_OBJECT(mudedit->priv->btnDel), "clicked", G_CALLBACK(mud_edit_window_del_cb), mudedit);
}

void 
mud_edit_window_query_glade(MudEditWindow *mudedit)
{
	GladeXML *glade;

	glade = glade_xml_new(GLADEDIR "/muds.glade", "mudedit_window", NULL);
	
	mudedit->priv->dialog = glade_xml_get_widget(glade, "mudedit_window");

	mudedit->priv->btnAdd = glade_xml_get_widget(glade, "btnAdd");
	mudedit->priv->btnProps = glade_xml_get_widget(glade, "btnProps");
	mudedit->priv->btnOK = glade_xml_get_widget(glade, "btnOK");
	mudedit->priv->btnCancel = glade_xml_get_widget(glade, "btnCancel");
	mudedit->priv->btnShowToggle = glade_xml_get_widget(glade, "btnShowToggle");
	mudedit->priv->btnDel = glade_xml_get_widget(glade, "btnDelete");

	mudedit->priv->MudCodeBaseCombo = glade_xml_get_widget(glade, "MudCodeBaseCombo");
	mudedit->priv->MudProfileCombo = glade_xml_get_widget(glade, "MudProfileCombo");

	mudedit->priv->MudProfileComboStore = gtk_list_store_new(1, G_TYPE_STRING);
	gtk_list_store_clear(GTK_LIST_STORE(mudedit->priv->MudProfileComboStore));
	gtk_combo_box_set_model(GTK_COMBO_BOX(mudedit->priv->MudProfileCombo), GTK_TREE_MODEL(mudedit->priv->MudProfileComboStore));
	mudedit->priv->comborender =gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(mudedit->priv->MudProfileCombo), mudedit->priv->comborender, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(mudedit->priv->MudProfileCombo), mudedit->priv->comborender, "text", 0, NULL);
	
	mudedit->priv->EntryName = glade_xml_get_widget(glade, "EntryName");
	mudedit->priv->EntryHost = glade_xml_get_widget(glade, "EntryHost");
	mudedit->priv->EntryPort = glade_xml_get_widget(glade, "EntryPort");
	mudedit->priv->EntryTheme = glade_xml_get_widget(glade, "EntryTheme");

	mudedit->priv->MudDescTextView = glade_xml_get_widget(glade, "MudDescTextView");
	
	mudedit->priv->CharView = glade_xml_get_widget(glade, "CharView");
	
	g_object_unref(glade);
}

void
props_window_dialog(gchar *charname, MudEditWindow *mudedit, gboolean NewChar)
{
	GtkWidget* dialog;
	GtkWidget* name;
	GtkWidget* connectString;
	GladeXML* glade;
	gint result;
	char keyname[2048];
	GConfClient *client;
	GError *error = NULL;
	GtkTextBuffer *buffer;
	gchar *namestr;
	gchar *connect;
	GtkTextIter start, end;
	GConfValue *strval;
	GSList *chars, *entry, *res;

	client = gconf_client_get_default();
	strval = gconf_value_new(GCONF_VALUE_STRING);
	res = NULL;
	
	chars = NULL;

	glade = glade_xml_new(GLADEDIR "/muds.glade", "charprops_window", NULL);

	dialog = glade_xml_get_widget(glade, "charprops_window");
	name = glade_xml_get_widget(glade, "CharNameEntry");
	connectString = glade_xml_get_widget(glade, "CharConnectStrTextView");
	
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(connectString));
	
	if(charname != NULL) {
		gtk_entry_set_text(GTK_ENTRY(name), charname);
		
		g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/chars/%s/connect", mudedit->priv->mud, charname);
		connect = g_strdup(gconf_client_get_string(client, keyname, &error));
		if(connect)
		{
			gtk_text_buffer_set_text(buffer, connect, strlen(connect));
			g_free(connect);
		}
	}

	result = gtk_dialog_run(GTK_DIALOG(dialog));
	if(result == GTK_RESPONSE_OK)
	{
		namestr = (gchar *)gtk_entry_get_text(GTK_ENTRY(name));
		if(!charname)
			charname = g_strdup(namestr);

		g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/chars/list", mudedit->priv->mud);

		chars = gconf_client_get_list(client, keyname, GCONF_VALUE_STRING, &error);

		if(NewChar)
		{
			chars = g_slist_append(chars, (void *)g_strdup(namestr));
			g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/chars/list", mudedit->priv->mud);
			gconf_client_set_list(client, keyname, GCONF_VALUE_STRING, chars, &error);	
		}
		
		if(strcmp(namestr, charname) != 0 && !NewChar)
		{
			for (entry = chars; entry != NULL; entry = g_slist_next(entry))
			{
				if(strcmp((gchar *)entry->data, charname) == 0)
				{
					entry->data = (void *)g_strdup(namestr);
				}
			}
			
			g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/chars/list", mudedit->priv->mud);
			gconf_client_set_list(client, keyname, GCONF_VALUE_STRING, chars, &error);	
		}

		g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/chars/%s/connect", mudedit->priv->mud, namestr);

		gtk_text_buffer_get_start_iter(buffer, &start);
		gtk_text_buffer_get_end_iter(buffer, &end);
	
		gconf_value_set_string(strval, gtk_text_buffer_get_text(buffer, &start, &end, FALSE));
		gconf_client_set(client, keyname, strval, &error);	
	}
	
	gconf_value_free(strval);
	gtk_widget_destroy(dialog);
	g_object_unref(glade);

	populate_charview(mudedit);
}

void 
populate_charview(MudEditWindow *mudedit)
{
	GtkTreeStore* store = GTK_TREE_STORE(mudedit->priv->CharStore);
	GtkTreeIter iter;
	GSList *chars, *entry;
	GError *error=NULL;
	gchar *cname;
	char keyname[2048];
	
	gtk_tree_store_clear(store);
	
	gtk_widget_set_sensitive(mudedit->priv->btnProps, FALSE);
	gtk_widget_set_sensitive(mudedit->priv->btnDel, FALSE);
	
	g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/chars/list", mudedit->priv->mud);

	chars = gconf_client_get_list(gconf_client_get_default(), keyname, GCONF_VALUE_STRING, &error);

	for (entry = chars; entry != NULL; entry = g_slist_next(entry))
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mudedit->priv->btnShowToggle), TRUE);
		gtk_widget_set_sensitive(mudedit->priv->btnShowToggle, FALSE);
		cname = g_strdup((gchar *) entry->data);
		gtk_tree_store_append(store, &iter, NULL);
		gtk_tree_store_set(store, &iter, NAME_COLUMN, cname, -1);
		g_free(cname);
	}	
}

void
populate_profiles(MudEditWindow *mudedit)
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
		gtk_list_store_append(GTK_LIST_STORE(mudedit->priv->MudProfileComboStore), &iter);
		gtk_list_store_set(GTK_LIST_STORE(mudedit->priv->MudProfileComboStore), &iter, 0, (gchar *)entry->data, -1);
	}
}

// MudEditWindow Callbacks
void 
mud_edit_window_props_cb(GtkWidget *widget, MudEditWindow *mudedit)
{
	props_window_dialog(mudedit->priv->CurrSelRowText, mudedit, FALSE);
}

void 
mud_edit_window_add_cb(GtkWidget *widget, MudEditWindow *mudedit)
{
	props_window_dialog(NULL, mudedit, TRUE);
}

void 
mud_edit_window_ok_cb(GtkWidget *widget, MudEditWindow *mudedit)
{
	gchar *name;
	gchar keyname[2048];
	GConfValue *strval;
	GConfValue *intval;
	GConfClient *client;
	GError *error = NULL;
	GSList *muds, *entry;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(mudedit->priv->MudDescTextView));
	GtkTextIter start, end;
	gint profileInt;
	
	client = gconf_client_get_default();
	strval = gconf_value_new(GCONF_VALUE_STRING);
	intval = gconf_value_new(GCONF_VALUE_INT);
	
	name = remove_whitespace((gchar *)gtk_entry_get_text(GTK_ENTRY(mudedit->priv->EntryName)));
	
	/* Add the mud name to the list if new */
	if(gNewMud)
	{
		muds = gconf_client_get_list(client, "/apps/gnome-mud/muds/list", GCONF_VALUE_STRING, NULL);
		muds = g_slist_append(muds, (void *)name);
		gconf_client_set_list(client, "/apps/gnome-mud/muds/list", GCONF_VALUE_STRING, muds, &error);		
	}
	
	/* Changing the name of a MUD entry requires some extra maintenance */
	if(strcmp(mudedit->priv->mud,name) != 0)
	{
		gconf_value_set_string(strval, (gchar *)gtk_entry_get_text(GTK_ENTRY(mudedit->priv->EntryName)));
	
		g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/name",name);
		gconf_client_set(client, keyname, strval, &error);

		muds = gconf_client_get_list(client, "/apps/gnome-mud/muds/list", GCONF_VALUE_STRING, NULL);

		for (entry = muds; entry != NULL; entry = g_slist_next(entry))
		{
			if(strcmp((gchar *)entry->data, mudedit->priv->mud) == 0)
			{
				entry->data = (void *)g_strdup(name);
			}
		}

		gconf_client_set_list(client, "/apps/gnome-mud/muds/list", GCONF_VALUE_STRING, muds, &error);

		g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/name", name);
		gconf_client_set(client, keyname, strval, &error);
	
		g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s", mudedit->priv->mud);
		g_free(mudedit->priv->mud);
		mudedit->priv->mud = g_strdup(name);

		// This seems to do nothing. WTF. -lh
		gconf_client_remove_dir(client, keyname, &error);
	}
	
	gconf_value_set_string(strval, (gchar *)gtk_entry_get_text(GTK_ENTRY(mudedit->priv->EntryHost)));
	g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/host", mudedit->priv->mud);
	gconf_client_set(client, keyname, strval, &error);

	gconf_value_set_int(intval, atoi((gchar *)gtk_entry_get_text(GTK_ENTRY(mudedit->priv->EntryPort))));
	g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/port", mudedit->priv->mud);
	gconf_client_set(client, keyname, intval, &error);
	
	gconf_value_set_int(intval, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mudedit->priv->btnShowToggle)));
	g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/show", mudedit->priv->mud);
	gconf_client_set(client, keyname, intval, &error);
	
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
	
	gconf_value_set_string(strval, gtk_text_buffer_get_text(buffer, &start, &end, FALSE));
	g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/desc", mudedit->priv->mud);
	gconf_client_set(client, keyname, strval, &error);

	gconf_value_set_string(strval, (gchar *)gtk_entry_get_text(GTK_ENTRY(mudedit->priv->EntryTheme)));
	g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/theme", mudedit->priv->mud);
	gconf_client_set(client, keyname, strval, &error);
	
	gconf_value_set_int(intval, gtk_combo_box_get_active(GTK_COMBO_BOX(mudedit->priv->MudCodeBaseCombo)));
	g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/codebase", mudedit->priv->mud);
	gconf_client_set(client, keyname, intval, &error);

	if (gtk_combo_box_get_active(GTK_COMBO_BOX(mudedit->priv->MudProfileCombo)) == -1)
		profileInt = 0;
	else
		profileInt = gtk_combo_box_get_active(GTK_COMBO_BOX(mudedit->priv->MudProfileCombo));
		
	gconf_value_set_int(intval, profileInt);
	g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/profile", mudedit->priv->mud);
	gconf_client_set(client, keyname, intval, &error);
	
	gconf_value_free(strval);
	gconf_value_free(intval);
	
	mud_list_window_populate_treeview(gParent);	
	gtk_widget_destroy(mudedit->priv->dialog);

	mud_edit_window_finalize(G_OBJECT(mudedit));
}

void 
mud_edit_window_cancel_cb(GtkWidget *widget, MudEditWindow *mudedit)
{
	gtk_widget_destroy(mudedit->priv->dialog);
}

gboolean 
mud_edit_window_tree_select_cb(GtkTreeSelection *selection,
			       GtkTreeModel     *model,
			       GtkTreePath      *path,
			       gboolean          path_currently_selected,
			       gpointer          userdata) 
{
	GtkTreeIter iter;
	MudEditWindow *mudedit = (MudEditWindow *)userdata;

	if (gtk_tree_model_get_iter(model, &iter, path)) 
	{		
		gtk_tree_model_get(model, &iter, 0, &mudedit->priv->CurrSelRowText, -1);
    	
		mudedit->priv->CurrSelRow = (gtk_tree_path_get_indices(path))[0];
		mudedit->priv->CurrIterStr = gtk_tree_model_get_string_from_iter(model, &iter);

		gtk_widget_set_sensitive(mudedit->priv->btnProps, TRUE);
		gtk_widget_set_sensitive(mudedit->priv->btnDel, TRUE);	
	}
	
	return TRUE;
}

void
mud_edit_window_del_cb(GtkWidget *widget, MudEditWindow *mudedit)
{
	GSList *chars, *entry, *rementry;
	GConfClient *client;
	GError *error = NULL;
	char keyname[2048];
	
	rementry = NULL;
	rementry = g_slist_append(rementry, NULL);

	client = gconf_client_get_default();
	g_snprintf(keyname, 2048, "/apps/gnome-mud/muds/%s/chars/list", mudedit->priv->mud);
	
	chars = gconf_client_get_list(client, keyname, GCONF_VALUE_STRING, &error);

	for (entry = chars; entry != NULL; entry = g_slist_next(entry))
	{
		if(strcmp((gchar *)entry->data, mudedit->priv->CurrSelRowText) == 0)
		{
			rementry->data = entry->data;
		}
	}
			
	
	chars = g_slist_remove(chars, rementry->data);
	 
	gconf_client_set_list(client, keyname, GCONF_VALUE_STRING, chars, &error);	
	if(!chars) 
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mudedit->priv->btnShowToggle), FALSE);
		gtk_widget_set_sensitive(mudedit->priv->btnShowToggle, TRUE);
	}
	
	populate_charview(mudedit);
}

// Instantiate MudEditWindow
MudEditWindow*
mud_window_mudedit_new(gchar *mud, MudListWindow *mudlist, gboolean NewMud)
{
	MudEditWindow *mudedit;

	gmud = g_strdup(mud); // Cheesy.
	gNewMud = NewMud;
	gParent = mudlist;
	mudedit = g_object_new(MUD_TYPE_EDIT_WINDOW, NULL);

	return mudedit;	
}
