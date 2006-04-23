#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>

#include <glade/glade-xml.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkcellrenderer.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcolorsel.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkfontsel.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreeviewcolumn.h>

#include "mud-preferences-window.h"
#include "mud-profile.h"

static char const rcsid[] = "$Id: ";

struct _MudPreferencesWindowPrivate
{
	MudProfile *profile;

	GtkWidget *treeview;
	GtkWidget *notebook;

	GtkWidget *cb_echo;
	GtkWidget *cb_keep;
	GtkWidget *cb_disable;
	GtkWidget *cb_scrollback;

	GtkWidget *entry_commdev;
	GtkWidget *entry_terminal;
	
	GtkWidget *sb_history;
	GtkWidget *sb_lines;

	GtkWidget *fp_font;

	GtkWidget *cp_foreground;
	GtkWidget *cp_background;
	GtkWidget *colors[C_MAX];

	gulong signal;

	gint notification_count;
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
	COLUMN_PREFERENCES,
	COLUMN_ALIASES
};

enum
{
	TAB_BLANK,
	TAB_PREFERENCES,
	TAB_ALIASES
};

static void mud_preferences_window_init               (MudPreferencesWindow *preferences);
static void mud_preferences_window_class_init         (MudPreferencesWindowClass *preferences);
static void mud_preferences_window_finalize           (GObject *object);
static void mud_preferences_window_tree_selection_cb  (GtkTreeSelection *selection, MudPreferencesWindow *window);
static void mud_preferences_window_show_tab           (MudPreferencesWindow *window, gint tab);
static void mud_preferences_window_connect_callbacks  (MudPreferencesWindow *window);
static gboolean mud_preferences_window_response_cb    (GtkWidget *dialog, GdkEvent *Event, MudPreferencesWindow *window);
static void mud_preferences_window_change_profile_from_name (MudPreferencesWindow *window, const gchar *profile);
static void mud_preferences_window_change_profile     (MudPreferencesWindow *window, MudProfile *profile);
static void mud_preferences_window_set_preferences    (MudPreferencesWindow *window);

static void mud_preferences_window_echo_cb            (GtkWidget *widget, MudPreferencesWindow *window);
static void mud_preferences_window_keeptext_cb        (GtkWidget *widget, MudPreferencesWindow *window);
static void mud_preferences_window_disablekeys_cb     (GtkWidget *widget, MudPreferencesWindow *window);
static void mud_preferences_window_scrolloutput_cb    (GtkWidget *widget, MudPreferencesWindow *window);
static void mud_preferences_window_commdev_cb         (GtkWidget *widget, MudPreferencesWindow *window);
static void mud_preferences_window_terminal_cb        (GtkWidget *widget, MudPreferencesWindow *window);
static void mud_preferences_window_history_cb         (GtkWidget *widget, MudPreferencesWindow *window);
static void mud_preferences_window_scrollback_cb      (GtkWidget *widget, MudPreferencesWindow *window);
static void mud_preferences_window_font_cb            (GtkWidget *widget, const gchar *fontname, MudPreferencesWindow *window);
static void mud_preferences_window_foreground_cb      (GtkWidget *widget, guint r, guint g, guint b, guint a, MudPreferencesWindow *window);
static void mud_preferences_window_background_cb      (GtkWidget *widget, guint r, guint g, guint b, guint a, MudPreferencesWindow *window);
static void mud_preferences_window_colors_cb          (GtkWidget *widget, guint r, guint g, guint b, guint a, MudPreferencesWindow *window);

static void mud_preferences_window_changed_cb         (MudProfile *profile, MudProfileMask *mask, MudPreferencesWindow *window);

static void mud_preferences_window_update_echotext    (MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_keeptext    (MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_disablekeys (MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_scrolloutput(MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_commdev     (MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_terminaltype(MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_history     (MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_scrollback  (MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_font        (MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_foreground  (MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_background  (MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_colors      (MudPreferencesWindow *window, MudPrefs *preferences);

#define RETURN_IF_CHANGING_PROFILES(window)	if (window->priv->notification_count) return

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
	gint i;
	
	preferences->priv = g_new0(MudPreferencesWindowPrivate, 1);
	preferences->priv->profile = NULL;
	preferences->priv->notification_count = 0;

	glade = glade_xml_new(GLADEDIR "/prefs.glade", "preferences_window", NULL);
	dialog = glade_xml_get_widget(glade, "preferences_window");
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

	// FIXME, rewrite this (check gossip)
	preferences->priv->treeview = glade_xml_get_widget(glade, "treeview");
	preferences->priv->notebook = glade_xml_get_widget(glade, "notebook");

	preferences->priv->cb_echo = glade_xml_get_widget(glade, "cb_echo");
	preferences->priv->cb_keep = glade_xml_get_widget(glade, "cb_keep");
	preferences->priv->cb_disable = glade_xml_get_widget(glade, "cb_system");
	preferences->priv->cb_scrollback = glade_xml_get_widget(glade, "cb_scrollback");

	preferences->priv->entry_commdev = glade_xml_get_widget(glade, "entry_commdev");
	preferences->priv->entry_terminal = glade_xml_get_widget(glade, "entry_terminal");
	
	preferences->priv->sb_history = glade_xml_get_widget(glade, "sb_history");
	preferences->priv->sb_lines = glade_xml_get_widget(glade, "sb_lines");

	preferences->priv->fp_font = glade_xml_get_widget(glade, "fp_font");

	preferences->priv->cp_foreground = glade_xml_get_widget(glade, "cp_foreground");
	preferences->priv->cp_background = glade_xml_get_widget(glade, "cp_background");
	for (i = 0; i < C_MAX; i++)
	{
		gchar buf[24];

		g_snprintf(buf, 24, "cp%d", i);
		preferences->priv->colors[i] = glade_xml_get_widget(glade, buf);
	}
	
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(preferences->priv->treeview), TRUE);
	gtk_widget_show_all(dialog);
	
	gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
	gtk_window_present(GTK_WINDOW(dialog));

	g_signal_connect(G_OBJECT(dialog), "response", 
					 G_CALLBACK(mud_preferences_window_response_cb), preferences);
	mud_preferences_window_connect_callbacks(preferences);
	
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
	g_signal_handler_disconnect(preferences->priv->profile,
								preferences->priv->signal);

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

	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter,
					   TITLE_COLUMN, _("Aliases"),
					   DATA_COLUMN, NULL,
					   TYPE_COLUMN, GINT_TO_POINTER(COLUMN_ALIASES),
					   -1);
	
	list = mud_profile_get_profiles();
	for (entry = (GList *) list; entry != NULL; entry = g_list_next(entry))
	{
		GtkTreeIter iter_child;
		MudProfile *profile = (MudProfile *) entry->data;
		
		/* Special hack for default profile */
		if (!strcmp(profile->name, "Default")) continue;

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
		gtk_tree_store_append(store, &iter_child, &iter);
		gtk_tree_store_set(store, &iter_child,
						   TITLE_COLUMN, _("Aliases"),
						   DATA_COLUMN, profile,
						   TYPE_COLUMN, GINT_TO_POINTER(COLUMN_ALIASES),
						   -1);
	}

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(window->priv->treeview));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	g_signal_connect(G_OBJECT(selection), "changed",
					 G_CALLBACK(mud_preferences_window_tree_selection_cb), window);
}

static void
mud_preferences_window_tree_selection_cb(GtkTreeSelection *selection, MudPreferencesWindow *window)
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

		if (profile == NULL)
		{
			mud_preferences_window_change_profile_from_name(window, "Default");
		}
		else
		{
			mud_preferences_window_change_profile(window, profile);
		}

		window->priv->notification_count++;
		mud_preferences_window_show_tab(window, type);
		window->priv->notification_count--;
	}
}

static void
mud_preferences_window_show_tab(MudPreferencesWindow *window, gint tab)
{
	GtkWidget *widget;
	
	gtk_notebook_set_current_page(GTK_NOTEBOOK(window->priv->notebook), tab);
	switch (tab)
	{
		case COLUMN_PREFERENCES:
			widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(window->priv->notebook), tab);
			gtk_notebook_set_current_page(GTK_NOTEBOOK(widget), 0);
			mud_preferences_window_set_preferences(window);
			break;
	}
}

static gboolean
mud_preferences_window_response_cb(GtkWidget *dialog, GdkEvent *event, MudPreferencesWindow *window)
{
	gtk_widget_destroy(dialog);
	g_object_unref(window);
	
	return FALSE;
}

static void
mud_preferences_window_change_profile_from_name(MudPreferencesWindow *window, const gchar *name)
{
	MudProfile *profile;

	profile = mud_profile_new(name);
	mud_preferences_window_change_profile(window, profile);
}

static void
mud_preferences_window_change_profile(MudPreferencesWindow *window, MudProfile *profile)
{
	g_message("changing prefs profile to %s", profile->name);
	if (window->priv->profile != NULL)
	{
		g_signal_handler_disconnect(window->priv->profile, window->priv->signal);
		g_object_unref(window->priv->profile);
	}

	window->priv->profile = profile;
	window->priv->signal = g_signal_connect(G_OBJECT(window->priv->profile), "changed",
						G_CALLBACK(mud_preferences_window_changed_cb),
						window);
	g_object_ref(G_OBJECT(window->priv->profile));
}

static void
mud_preferences_window_connect_callbacks(MudPreferencesWindow *window)
{
	gint i;

	g_signal_connect(G_OBJECT(window->priv->cb_echo), "toggled",
					 G_CALLBACK(mud_preferences_window_echo_cb),
					 window);
	g_signal_connect(G_OBJECT(window->priv->cb_keep), "toggled",
					 G_CALLBACK(mud_preferences_window_keeptext_cb),
					 window);
	g_signal_connect(G_OBJECT(window->priv->cb_disable), "toggled",
					 G_CALLBACK(mud_preferences_window_disablekeys_cb),
					 window);
	g_signal_connect(G_OBJECT(window->priv->cb_scrollback), "toggled",
					 G_CALLBACK(mud_preferences_window_scrolloutput_cb),
					 window);
	g_signal_connect(G_OBJECT(window->priv->entry_commdev), "changed",
					 G_CALLBACK(mud_preferences_window_commdev_cb),
					 window);
	g_signal_connect(G_OBJECT(window->priv->entry_terminal), "changed",
					 G_CALLBACK(mud_preferences_window_terminal_cb),
					 window);
	g_signal_connect(G_OBJECT(window->priv->sb_history), "changed",
					 G_CALLBACK(mud_preferences_window_history_cb),
					 window);
	g_signal_connect(G_OBJECT(window->priv->sb_lines), "changed",
					 G_CALLBACK(mud_preferences_window_scrollback_cb),
					 window);
	g_signal_connect(G_OBJECT(window->priv->fp_font), "font_set",
					 G_CALLBACK(mud_preferences_window_font_cb),
					 window);
	g_signal_connect(G_OBJECT(window->priv->cp_foreground), "color_set",
					 G_CALLBACK(mud_preferences_window_foreground_cb),
					 window);
	g_signal_connect(G_OBJECT(window->priv->cp_background), "color_set",
					 G_CALLBACK(mud_preferences_window_background_cb),
					 window);
	for (i = 0; i < C_MAX; i++)
	{
		g_signal_connect(G_OBJECT(window->priv->colors[i]), "color_set",
						 G_CALLBACK(mud_preferences_window_colors_cb),
						 window);
	}
}

static void
mud_preferences_window_set_preferences(MudPreferencesWindow *window)
{
	MudProfile *profile = window->priv->profile;

	mud_preferences_window_update_echotext(window, profile->preferences);
	mud_preferences_window_update_keeptext(window, profile->preferences);
	mud_preferences_window_update_disablekeys(window, profile->preferences);
	mud_preferences_window_update_scrolloutput(window, profile->preferences);
	mud_preferences_window_update_commdev(window, profile->preferences);
	mud_preferences_window_update_terminaltype(window, profile->preferences);
	mud_preferences_window_update_history(window, profile->preferences);
	mud_preferences_window_update_scrollback(window, profile->preferences);
	mud_preferences_window_update_font(window, profile->preferences);
	mud_preferences_window_update_foreground(window, profile->preferences);
	mud_preferences_window_update_background(window, profile->preferences);
	mud_preferences_window_update_colors(window, profile->preferences);
}

static void
mud_preferences_window_disablekeys_cb(GtkWidget *widget, MudPreferencesWindow *window)
{
	gboolean value = GTK_TOGGLE_BUTTON(widget)->active ? TRUE : FALSE;
	RETURN_IF_CHANGING_PROFILES(window);

	mud_profile_set_disablekeys(window->priv->profile, value);
}

static void
mud_preferences_window_scrolloutput_cb(GtkWidget *widget, MudPreferencesWindow *window)
{
	gboolean value = GTK_TOGGLE_BUTTON(widget)->active ? TRUE : FALSE;
	RETURN_IF_CHANGING_PROFILES(window);

	mud_profile_set_scrolloutput(window->priv->profile, value);
}

static void
mud_preferences_window_keeptext_cb(GtkWidget *widget, MudPreferencesWindow *window)
{	
	gboolean value = GTK_TOGGLE_BUTTON(widget)->active ? TRUE : FALSE;
	RETURN_IF_CHANGING_PROFILES(window);

	mud_profile_set_keeptext(window->priv->profile, value);
}

static void
mud_preferences_window_echo_cb(GtkWidget *widget, MudPreferencesWindow *window)
{
	gboolean value = GTK_TOGGLE_BUTTON(widget)->active ? TRUE : FALSE;
	RETURN_IF_CHANGING_PROFILES(window);

	mud_profile_set_echotext(window->priv->profile, value);
}

static void
mud_preferences_window_commdev_cb(GtkWidget *widget, MudPreferencesWindow *window)
{
	const gchar *s = gtk_entry_get_text(GTK_ENTRY(widget));
	RETURN_IF_CHANGING_PROFILES(window);

	mud_profile_set_commdev(window->priv->profile, s);
}

static void
mud_preferences_window_terminal_cb(GtkWidget *widget, MudPreferencesWindow *window)
{
	const gchar *s = gtk_entry_get_text(GTK_ENTRY(widget));
	RETURN_IF_CHANGING_PROFILES(window);

	mud_profile_set_terminal(window->priv->profile, s);
}

static void
mud_preferences_window_history_cb(GtkWidget *widget, MudPreferencesWindow *window)
{
	const gint value = (gint) gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
	RETURN_IF_CHANGING_PROFILES(window);

	mud_profile_set_history(window->priv->profile, value);
}

static void
mud_preferences_window_scrollback_cb(GtkWidget *widget, MudPreferencesWindow *window)
{
	const gint value = (gint) gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
	RETURN_IF_CHANGING_PROFILES(window);

	mud_profile_set_scrollback(window->priv->profile, value);
}

static void
mud_preferences_window_font_cb(GtkWidget *widget, const gchar *fontname, MudPreferencesWindow *window)
{
	RETURN_IF_CHANGING_PROFILES(window);
	mud_profile_set_font(window->priv->profile, fontname);
}

static void
mud_preferences_window_foreground_cb(GtkWidget *widget, guint r, guint g, guint b, guint a, MudPreferencesWindow *window)
{
	RETURN_IF_CHANGING_PROFILES(window);
	mud_profile_set_foreground(window->priv->profile, r, g, b);
}

static void
mud_preferences_window_background_cb(GtkWidget *widget, guint r, guint g, guint b, guint a, MudPreferencesWindow *window)
{
	RETURN_IF_CHANGING_PROFILES(window);
	mud_profile_set_background(window->priv->profile, r, g, b);
}

static void
mud_preferences_window_colors_cb(GtkWidget *widget, guint r, guint g, guint b, guint a, MudPreferencesWindow *window)
{
	gint i;

	RETURN_IF_CHANGING_PROFILES(window);
	
	for (i = 0; i < C_MAX; i++)
	{
		if (widget == window->priv->colors[i])
		{
			mud_profile_set_colors(window->priv->profile, i, r, g, b);
		}
	}
}

static void
mud_preferences_window_changed_cb(MudProfile *profile, MudProfileMask *mask, MudPreferencesWindow *window)
{
	g_message("and preferences-window knows about it too");

	if (mask->EchoText)
		mud_preferences_window_update_echotext(window, profile->preferences);
	if (mask->KeepText)
		mud_preferences_window_update_keeptext(window, profile->preferences);
	if (mask->DisableKeys)
		mud_preferences_window_update_disablekeys(window, profile->preferences);
	if (mask->ScrollOnOutput)
		mud_preferences_window_update_scrolloutput(window, profile->preferences);
	if (mask->CommDev)
		mud_preferences_window_update_commdev(window, profile->preferences);
	if (mask->TerminalType)
		mud_preferences_window_update_terminaltype(window, profile->preferences);
	if (mask->History)
		mud_preferences_window_update_history(window, profile->preferences);
	if (mask->Scrollback)
		mud_preferences_window_update_scrollback(window, profile->preferences);
	if (mask->FontName)
		mud_preferences_window_update_font(window, profile->preferences);
	if (mask->Foreground)
		mud_preferences_window_update_foreground(window, profile->preferences);
	if (mask->Background)
		mud_preferences_window_update_background(window, profile->preferences);
	if (mask->Colors)
		mud_preferences_window_update_colors(window, profile->preferences);
}

static void
mud_preferences_window_update_terminaltype(MudPreferencesWindow *window, MudPrefs *preferences)
{
	gtk_entry_set_text(GTK_ENTRY(window->priv->entry_terminal), preferences->TerminalType);
}

static void
mud_preferences_window_update_commdev(MudPreferencesWindow *window, MudPrefs *preferences)
{
	gtk_entry_set_text(GTK_ENTRY(window->priv->entry_commdev), preferences->CommDev);
}

static void
mud_preferences_window_update_scrolloutput(MudPreferencesWindow *window, MudPrefs *preferences)
{
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(window->priv->cb_scrollback), preferences->ScrollOnOutput);
}

static void
mud_preferences_window_update_disablekeys(MudPreferencesWindow *window, MudPrefs *preferences)
{
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(window->priv->cb_disable), preferences->DisableKeys);
}

static void
mud_preferences_window_update_keeptext(MudPreferencesWindow *window, MudPrefs *preferences)
{
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(window->priv->cb_keep), preferences->KeepText);
}

static void
mud_preferences_window_update_echotext(MudPreferencesWindow *window, MudPrefs *preferences)
{
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(window->priv->cb_echo), preferences->EchoText);
}

static void
mud_preferences_window_update_history(MudPreferencesWindow *window, MudPrefs *preferences)
{
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(window->priv->sb_history), preferences->History);
}

static void
mud_preferences_window_update_scrollback(MudPreferencesWindow *window, MudPrefs *preferences)
{
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(window->priv->sb_lines), preferences->Scrollback);
}

static void
mud_preferences_window_update_font(MudPreferencesWindow *window, MudPrefs *preferences)
{
	gtk_font_selection_set_font_name(GTK_FONT_SELECTION(window->priv->fp_font),
									preferences->FontName);
}

static void
mud_preferences_window_update_foreground(MudPreferencesWindow *window, MudPrefs *preferences)
{
	gtk_color_selection_set_color(GTK_COLOR_SELECTION(window->priv->cp_foreground),
							   preferences->Foreground);
}

static void
mud_preferences_window_update_background(MudPreferencesWindow *window, MudPrefs *preferences)
{
	gtk_color_selection_set_color(GTK_COLOR_SELECTION(window->priv->cp_background), preferences->Background);
}

static void
mud_preferences_window_update_colors(MudPreferencesWindow *window, MudPrefs *preferences)
{
	gint i;

	for (i = 0; i < C_MAX; i++)
	{
		gtk_color_selection_set_color (GTK_COLOR_SELECTION(window->priv->colors[i]),preferences->Colors[i]);
	}
}

MudPreferencesWindow*
mud_preferences_window_new (const gchar *profile)
{
	MudPreferencesWindow *prefs;

	prefs = g_object_new(MUD_TYPE_PREFERENCES_WINDOW, NULL);

	mud_preferences_window_change_profile_from_name(prefs, profile);
	mud_preferences_window_fill_profiles(prefs);

	return prefs;
}

