#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glade/glade.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkcellrenderer.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreeviewcolumn.h>
#include <glib-object.h>
#include <glib/gi18n.h>

#include "mud-preferences-window.h"
#include "mud-profile.h"

static char const rcsid[] = "$Id: ";

struct _MudPreferencesWindowPrivate
{
	MudPreferences	*prefs;

	GtkWidget *treeview;
	GtkWidget *notebook;
};

enum
{
	TITLE_COLUMN,
	DATA_COLUMN,
	TYPE_COLUMN,
	N_COLUMNS
};

enum
{
	COLUMN_NODE,
	COLUMN_PREFERENCES
};

enum
{
	TAB_BLANK,
	TAB_PREFERENCES
};

static void mud_preferences_window_init           (MudPreferencesWindow *preferences);
static void mud_preferences_window_class_init     (MudPreferencesWindowClass *preferences);
static void mud_preferences_window_finalize       (GObject *object);
static void mud_preferences_tree_selection_cb     (GtkTreeSelection *selection, MudPreferencesWindow *window);
static void mud_preferences_show_tab              (MudPreferencesWindow *window, gint tab);
static gboolean mud_preferences_response_cb       (GtkWidget *dialog, GdkEvent *Event, MudPreferencesWindow *window);

GType
mud_preferences_window_get_type (void)
{
	static GType object_type = 0;

	g_type_init();

	if (!object_type)
	{
		static const GTypeInfo object_info =
		{
			sizeof (MudPreferencesWindowClass),
			NULL,
			NULL,
			(GClassInitFunc) mud_preferences_window_class_init,
			NULL,
			NULL,
			sizeof (MudPreferencesWindow),
			0,
			(GInstanceInitFunc) mud_preferences_window_init,
		};

		object_type = g_type_register_static(G_TYPE_OBJECT, "MudPreferencesWindow", &object_info, 0);
	}

	return object_type;
}

static void
mud_preferences_window_init (MudPreferencesWindow *preferences)
{
	GladeXML *glade;
	GtkWidget *dialog;
	
	preferences->priv = g_new0(MudPreferencesWindowPrivate, 1);

	glade = glade_xml_new(GLADEDIR "main.glade", "preferences_window", "preferences_window");
	dialog = glade_xml_get_widget(glade, "preferences_window");
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

	preferences->priv->treeview = glade_xml_get_widget(glade, "treeview");
	preferences->priv->notebook = glade_xml_get_widget(glade, "notebook");
	
	gtk_widget_show_all(dialog);
	
	gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
	gtk_window_present(GTK_WINDOW(dialog));

	g_signal_connect(G_OBJECT(dialog), "response", 
					 G_CALLBACK(mud_preferences_response_cb), preferences);
	
	g_object_unref(G_OBJECT(glade));
}

static void
mud_preferences_window_class_init (MudPreferencesWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = mud_preferences_window_finalize;
}

static void
mud_preferences_window_finalize (GObject *object)
{
	MudPreferencesWindow *preferences;
	GObjectClass *parent_class;

	preferences = MUD_PREFERENCES_WINDOW(object);

	g_free(preferences->priv);

	g_message("finalizing preferences window...\n");
	parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
	parent_class->finalize(object);
}

void
mud_preferences_window_fill_profiles (MudPreferencesWindow *window)
{
	const GList *list;
	GList *entry;
	GtkTreeStore *store;
	GtkTreeIter iter;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

	store = gtk_tree_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_INT);
	gtk_tree_view_set_model(GTK_TREE_VIEW(window->priv->treeview), GTK_TREE_MODEL(store));

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Title",
													  renderer,
													  "text", TITLE_COLUMN,
													  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(window->priv->treeview), column);

	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter,
					   TITLE_COLUMN, _("Preferences"),
					   DATA_COLUMN, NULL,
					   TYPE_COLUMN, GINT_TO_POINTER(COLUMN_PREFERENCES),
					   -1);
	
	list = mud_preferences_get_profiles(window->priv->prefs);
	for (entry = (GList *) list; entry != NULL; entry = g_list_next(entry))
	{
		GtkTreeIter iter_child;
		MudProfile *profile = (MudProfile *) entry->data;
		
		gtk_tree_store_append(store, &iter, NULL);
		gtk_tree_store_set(store, &iter, 
						   TITLE_COLUMN, profile->name,
						   DATA_COLUMN, profile,
						   TYPE_COLUMN, GINT_TO_POINTER(COLUMN_NODE),
						   -1);
		gtk_tree_store_append(store, &iter_child, &iter);
		gtk_tree_store_set(store, &iter_child,
						   TITLE_COLUMN, _("Preferences"),
						   DATA_COLUMN, profile,
						   TYPE_COLUMN, GINT_TO_POINTER(COLUMN_PREFERENCES),
						   -1);
	}

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(window->priv->treeview));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	g_signal_connect(G_OBJECT(selection), "changed",
					 G_CALLBACK(mud_preferences_tree_selection_cb), window);
}

static void
mud_preferences_tree_selection_cb(GtkTreeSelection *selection, MudPreferencesWindow *window)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	MudProfile *profile = NULL;
	gint type;
	
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		if (gtk_tree_model_iter_has_child(model, &iter))
		{
			GtkTreeIter iter_child;

			if (gtk_tree_model_iter_children(model, &iter_child, &iter))
			{
				gtk_tree_view_expand_to_path(GTK_TREE_VIEW(window->priv->treeview),
											 gtk_tree_model_get_path(model, &iter));
				gtk_tree_selection_select_iter(selection, &iter_child);
				gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(window->priv->treeview),
											 gtk_tree_model_get_path(model, &iter_child),
											 NULL, TRUE, 1.0f, 0.5f);

				return;
			}
			
		}
		
		gtk_tree_model_get(model, &iter, DATA_COLUMN, &profile, TYPE_COLUMN, &type, -1);

		mud_preferences_show_tab(window, TAB_PREFERENCES);
	}
}

static void
mud_preferences_show_tab(MudPreferencesWindow *window, gint tab)
{
	GtkWidget *widget;
	
	gtk_notebook_set_current_page(GTK_NOTEBOOK(window->priv->notebook), tab);
	switch (tab)
	{
		case COLUMN_PREFERENCES:
			widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window->priv->notebook), COLUMN_PREFERENCES);
			gtk_notebook_set_current_page(GTK_NOTEBOOK(widget), 0);
			break;
	}
}

static gboolean
mud_preferences_response_cb(GtkWidget *dialog, GdkEvent *event, MudPreferencesWindow *window)
{
	gtk_widget_destroy(dialog);
	g_object_unref(window);
	
	return FALSE;
}

MudPreferencesWindow*
mud_preferences_window_new (MudPreferences *preferences)
{
	MudPreferencesWindow *prefs;

	prefs = g_object_new(MUD_TYPE_PREFERENCES_WINDOW, NULL);
	prefs->priv->prefs = preferences;

	mud_preferences_window_fill_profiles(prefs);

	return prefs;
}

