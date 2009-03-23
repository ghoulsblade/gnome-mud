/* GNOME-Mud - A simple Mud Client
 * mud-preferences-window.c
 * Copyright (C) 1998-2005 Robin Ericsson <lobbin@localhost.nu>
 * Copyright (C) 2005-2009 Les Harris <lharris@gnome.org>
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

#include <string.h>

#include <glade/glade-xml.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glib.h>

#include "mud-preferences-window.h"
#include "mud-profile.h"
#include "mud-regex.h"
#include "mud-window.h"
#include "utils.h"

typedef struct TTreeViewRowInfo {
    gint row;
    gchar *text;
    gchar *iterstr;
} TTreeViewRowInfo;

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

    GtkWidget *encoding_combo;
    GtkWidget *encoding_check;
    GtkWidget *proxy_check;
    GtkWidget *proxy_combo;
    GtkWidget *proxy_entry;

    GtkWidget *msp_check;

    GtkWidget *sb_lines;

    GtkWidget *fp_font;

    GtkWidget *cp_foreground;
    GtkWidget *cp_background;
    GtkWidget *colors[C_MAX];

    GtkWidget *alias_treeview;
    GtkWidget *alias_del;
    GtkWidget *alias_entry;
    GtkWidget *alias_textview;
    GtkWidget *alias_regex_textview;
    GtkWidget *alias_ok;
    GtkWidget *alias_match_treeview;
    GtkWidget *alias_match_label;
    GtkWidget *alias_match_test;
    GtkWidget *alias_match_entry;

    GtkWidget *trigger_treeview;
    GtkWidget *trigger_del;
    GtkWidget *trigger_name_entry;
    GtkWidget *trigger_regex_textview;
    GtkWidget *trigger_action_textview;
    GtkWidget *trigger_match_label;
    GtkWidget *trigger_match_entry;
    GtkWidget *trigger_match_button;
    GtkWidget *trigger_match_treeview;
    GtkWidget *trigger_ok;

    GtkTreeStore *trigger_match_store;
    GtkTreeViewColumn *trigger_match_register_col;
    GtkTreeViewColumn *trigger_match_text_col;
    GtkCellRenderer *trigger_match_register_renderer;
    GtkCellRenderer *trigger_match_text_renderer;

    GtkTreeStore *trigger_store;
    GtkTreeViewColumn *trigger_name_col;
    GtkTreeViewColumn *trigger_enabled_col;
    GtkTreeViewColumn *trigger_gag_col;
    GtkCellRenderer *trigger_name_renderer;
    GtkCellRenderer *trigger_enabled_renderer;
    GtkCellRenderer *trigger_gag_renderer;
    TTreeViewRowInfo trigger_info;

    GtkTreeStore *alias_store;
    GtkTreeViewColumn *alias_name_col;
    GtkTreeViewColumn *alias_enabled_col;
    GtkCellRenderer *alias_name_renderer;
    GtkCellRenderer *alias_enabled_renderer;
    TTreeViewRowInfo alias_info;

    GtkTreeStore *alias_match_store;
    GtkTreeViewColumn *alias_match_register_col;
    GtkTreeViewColumn *alias_match_text_col;
    GtkCellRenderer *alias_match_register_renderer;
    GtkCellRenderer *alias_match_text_renderer;

    gulong signal;

    gint notification_count;

    gchar *current_encoding;

    MudWindow *parent;
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
    COLUMN_ALIASES,
    COLUMN_TRIGGERS
};

enum
{
    TRIGGER_MATCH_REGISTER_COLUMN,
    TRIGGER_MATCH_TEXT_COLUMN,
    TRIGGER_MATCH_N_COLUMNS
};

enum
{
    TRIGGER_ENABLED_COLUMN,
    TRIGGER_GAG_COLUMN,
    TRIGGER_NAME_COLUMN,
    TRIGGER_N_COLUMNS
};

enum
{
    ALIAS_ENABLED_COLUMN,
    ALIAS_NAME_COLUMN,
    ALIAS_N_COLUMNS
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
static void mud_preferences_window_scrollback_cb      (GtkWidget *widget, MudPreferencesWindow *window);
static void mud_preferences_window_font_cb            (GtkWidget *widget, MudPreferencesWindow *window);
static void mud_preferences_window_foreground_cb      (GtkWidget *widget, MudPreferencesWindow *window);
static void mud_preferences_window_background_cb      (GtkWidget *widget, MudPreferencesWindow *window);
static void mud_preferences_window_colors_cb          (GtkWidget *widget, MudPreferencesWindow *window);

static void mud_preferences_window_changed_cb         (MudProfile *profile, MudProfileMask *mask, MudPreferencesWindow *window);

static void mud_preferences_window_encoding_combo_cb(GtkWidget *widget, MudPreferencesWindow *window);
static void mud_preferences_window_encoding_check_cb(GtkWidget *widget, MudPreferencesWindow *window);
static void mud_preferences_window_proxy_check_cb(GtkWidget *widget, MudPreferencesWindow *window);
static void mud_preferences_window_proxy_combo_cb(GtkWidget *widget, MudPreferencesWindow *window);
static void mud_preferences_window_proxy_entry_cb(GtkWidget *widget, MudPreferencesWindow *window);
static void mud_preferences_window_msp_check_cb(GtkWidget *widget, MudPreferencesWindow *window);

static void mud_preferences_window_update_echotext    (MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_keeptext    (MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_disablekeys (MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_scrolloutput(MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_commdev     (MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_scrollback  (MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_font        (MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_foreground  (MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_background  (MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_colors      (MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_proxy_check(MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_proxy_combo(MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_proxy_entry(MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_encoding_check(MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_encoding_combo(MudPreferencesWindow *window, MudPrefs *preferences);
static void mud_preferences_window_update_msp_check(MudPreferencesWindow *window, MudPrefs *preferences);

void mud_preferences_window_populate_trigger_treeview(MudPreferencesWindow *window);
void mud_preferences_window_populate_alias_treeview(MudPreferencesWindow *window);

static void mud_preferences_window_trigger_del_cb(GtkWidget *widget, MudPreferencesWindow *window);
static void mud_preferences_window_trigger_ok_cb(GtkWidget *widget, MudPreferencesWindow *window);

static void mud_preferences_window_alias_del_cb(GtkWidget *widget, MudPreferencesWindow *window);
static void mud_preferences_window_alias_ok_cb(GtkWidget *widget, MudPreferencesWindow *window);

gboolean mud_preferences_window_trigger_select_cb(GtkTreeSelection *selection,
                     			GtkTreeModel     *model,
                     			GtkTreePath      *path,
                   				gboolean        path_currently_selected,
                     			gpointer          userdata);
gboolean mud_preferences_window_alias_select_cb(GtkTreeSelection *selection,
                     			GtkTreeModel     *model,
                     			GtkTreePath      *path,
                   				gboolean        path_currently_selected,
                     			gpointer          userdata);
void mud_preferences_window_trigger_enabled_toggle_cb(GtkCellRendererToggle *cell_renderer,
                                            gchar *path,
                                            gpointer user_data);
void mud_preferences_window_trigger_gag_toggle_cb(GtkCellRendererToggle *cell_renderer,
                                            gchar *path,
                                            gpointer user_data);
void mud_preferences_window_alias_enabled_toggle_cb(GtkCellRendererToggle *cell_renderer,
                                            gchar *path,
                                            gpointer user_data);

void trigger_match_cb(GtkWidget *widget, MudPreferencesWindow *prefs);
void alias_match_cb(GtkWidget *widget, MudPreferencesWindow *prefs);

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

    preferences->priv->sb_lines = glade_xml_get_widget(glade, "sb_lines");

    preferences->priv->encoding_combo = glade_xml_get_widget(glade, "encoding_combo");
    preferences->priv->encoding_check = glade_xml_get_widget(glade, "encoding_check");
    preferences->priv->proxy_check = glade_xml_get_widget(glade, "proxy_check");
    preferences->priv->proxy_combo = glade_xml_get_widget(glade, "proxy_combo");
    preferences->priv->proxy_entry = glade_xml_get_widget(glade, "proxy_entry");
    preferences->priv->msp_check = glade_xml_get_widget(glade, "msp_check");

    preferences->priv->fp_font = glade_xml_get_widget(glade, "fp_font");

    preferences->priv->cp_foreground = glade_xml_get_widget(glade, "cp_foreground");
    preferences->priv->cp_background = glade_xml_get_widget(glade, "cp_background");
    for (i = 0; i < C_MAX; i++)
    {
        gchar buf[24];

        g_snprintf(buf, 24, "cp%d", i);
        preferences->priv->colors[i] = glade_xml_get_widget(glade, buf);
    }

    preferences->priv->alias_treeview = glade_xml_get_widget(glade, "alias_treeview");
    preferences->priv->alias_del = glade_xml_get_widget(glade, "alias_del");
    preferences->priv->alias_entry = glade_xml_get_widget(glade, "alias_entry");
    preferences->priv->alias_textview = glade_xml_get_widget(glade, "alias_textview");
    preferences->priv->alias_ok = glade_xml_get_widget(glade, "alias_ok");
    preferences->priv->alias_regex_textview = glade_xml_get_widget(glade, "alias_regex_textview");
    preferences->priv->alias_match_treeview = glade_xml_get_widget(glade, "alias_match_treeview");
    preferences->priv->alias_match_label = glade_xml_get_widget(glade, "alias_match_label");
    preferences->priv->alias_match_test = glade_xml_get_widget(glade, "alias_match_test");
    preferences->priv->alias_match_entry = glade_xml_get_widget(glade, "alias_match_entry");

    preferences->priv->trigger_treeview = glade_xml_get_widget(glade, "trigger_treeview");
    preferences->priv->trigger_del = glade_xml_get_widget(glade, "trigger_del");
    preferences->priv->trigger_name_entry = glade_xml_get_widget(glade, "trigger_name_entry");
    preferences->priv->trigger_regex_textview = glade_xml_get_widget(glade, "trigger_regex_textview");
    preferences->priv->trigger_action_textview = glade_xml_get_widget(glade, "trigger_action_textview");
    preferences->priv->trigger_match_label = glade_xml_get_widget(glade, "trigger_match_label");
    preferences->priv->trigger_match_entry = glade_xml_get_widget(glade, "trigger_match_entry");
    preferences->priv->trigger_match_button = glade_xml_get_widget(glade, "trigger_match_button");
    preferences->priv->trigger_match_treeview = glade_xml_get_widget(glade, "trigger_match_treeview");
    preferences->priv->trigger_ok = glade_xml_get_widget(glade, "trigger_ok");

    //... Okay seriously.... GtkTreeView API.... WTF. lh

    // Setup alias treeview
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(preferences->priv->alias_treeview), TRUE);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(preferences->priv->alias_treeview), TRUE);
    preferences->priv->alias_store = gtk_tree_store_new(ALIAS_N_COLUMNS, G_TYPE_BOOLEAN, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(preferences->priv->alias_treeview), GTK_TREE_MODEL(preferences->priv->alias_store));
    preferences->priv->alias_name_col = gtk_tree_view_column_new();
    preferences->priv->alias_enabled_col = gtk_tree_view_column_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(preferences->priv->alias_treeview), preferences->priv->alias_enabled_col);
    gtk_tree_view_append_column(GTK_TREE_VIEW(preferences->priv->alias_treeview), preferences->priv->alias_name_col);
    gtk_tree_view_column_set_title(preferences->priv->alias_name_col, _("Name"));
    gtk_tree_view_column_set_title(preferences->priv->alias_enabled_col, _("Enabled"));
    preferences->priv->alias_name_renderer = gtk_cell_renderer_text_new();
    preferences->priv->alias_enabled_renderer = gtk_cell_renderer_toggle_new();
    gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(preferences->priv->alias_enabled_renderer), FALSE);
    gtk_tree_view_column_pack_start(preferences->priv->alias_name_col, preferences->priv->alias_name_renderer, TRUE);
    gtk_tree_view_column_pack_start(preferences->priv->alias_enabled_col, preferences->priv->alias_enabled_renderer, TRUE);
    gtk_tree_view_column_add_attribute(preferences->priv->alias_name_col, preferences->priv->alias_name_renderer,
            "text", ALIAS_NAME_COLUMN);
    gtk_tree_view_column_add_attribute(preferences->priv->alias_enabled_col, preferences->priv->alias_enabled_renderer,
            "active", ALIAS_ENABLED_COLUMN);
    gtk_tree_store_clear(preferences->priv->alias_store);
    g_signal_connect(G_OBJECT(preferences->priv->alias_enabled_renderer), "toggled", G_CALLBACK(mud_preferences_window_alias_enabled_toggle_cb), preferences);
    gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(GTK_TREE_VIEW(preferences->priv->alias_treeview)), mud_preferences_window_alias_select_cb, preferences, NULL);

    // Setup trigger treeview
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(preferences->priv->trigger_treeview), TRUE);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(preferences->priv->trigger_treeview), TRUE);
    preferences->priv->trigger_store = gtk_tree_store_new(TRIGGER_N_COLUMNS, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(preferences->priv->trigger_treeview), GTK_TREE_MODEL(preferences->priv->trigger_store));
    preferences->priv->trigger_name_col = gtk_tree_view_column_new();
    preferences->priv->trigger_enabled_col = gtk_tree_view_column_new();
    preferences->priv->trigger_gag_col = gtk_tree_view_column_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(preferences->priv->trigger_treeview), preferences->priv->trigger_enabled_col);
    gtk_tree_view_append_column(GTK_TREE_VIEW(preferences->priv->trigger_treeview), preferences->priv->trigger_gag_col);
    gtk_tree_view_append_column(GTK_TREE_VIEW(preferences->priv->trigger_treeview), preferences->priv->trigger_name_col);
    gtk_tree_view_column_set_title(preferences->priv->trigger_name_col, _("Name"));
    gtk_tree_view_column_set_title(preferences->priv->trigger_enabled_col, _("Enabled"));
    gtk_tree_view_column_set_title(preferences->priv->trigger_gag_col, _("Gag"));
    preferences->priv->trigger_name_renderer = gtk_cell_renderer_text_new();
    preferences->priv->trigger_enabled_renderer = gtk_cell_renderer_toggle_new();
    preferences->priv->trigger_gag_renderer = gtk_cell_renderer_toggle_new();
    gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(preferences->priv->trigger_enabled_renderer), FALSE);
    gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(preferences->priv->trigger_gag_renderer), FALSE);
    gtk_tree_view_column_pack_start(preferences->priv->trigger_name_col, preferences->priv->trigger_name_renderer, TRUE);
    gtk_tree_view_column_pack_start(preferences->priv->trigger_enabled_col, preferences->priv->trigger_enabled_renderer, TRUE);
    gtk_tree_view_column_pack_start(preferences->priv->trigger_gag_col, preferences->priv->trigger_gag_renderer, TRUE);
    gtk_tree_view_column_add_attribute(preferences->priv->trigger_name_col, preferences->priv->trigger_name_renderer,
            "text", TRIGGER_NAME_COLUMN);
    gtk_tree_view_column_add_attribute(preferences->priv->trigger_enabled_col, preferences->priv->trigger_enabled_renderer,
            "active", TRIGGER_ENABLED_COLUMN);
    gtk_tree_view_column_add_attribute(preferences->priv->trigger_gag_col, preferences->priv->trigger_gag_renderer,
            "active", TRIGGER_GAG_COLUMN);

    gtk_tree_store_clear(preferences->priv->trigger_store);
    g_signal_connect(G_OBJECT(preferences->priv->trigger_enabled_renderer), "toggled", G_CALLBACK(mud_preferences_window_trigger_enabled_toggle_cb), preferences);
    g_signal_connect(G_OBJECT(preferences->priv->trigger_gag_renderer), "toggled", G_CALLBACK(mud_preferences_window_trigger_gag_toggle_cb), preferences);
    gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(GTK_TREE_VIEW(preferences->priv->trigger_treeview)), mud_preferences_window_trigger_select_cb, preferences, NULL);

    // Setup trigger match treeview
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(preferences->priv->trigger_match_treeview), TRUE);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(preferences->priv->trigger_match_treeview), FALSE);
    preferences->priv->trigger_match_store = gtk_tree_store_new(TRIGGER_MATCH_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(preferences->priv->trigger_match_treeview), GTK_TREE_MODEL(preferences->priv->trigger_match_store));
    preferences->priv->trigger_match_register_col = gtk_tree_view_column_new();
    preferences->priv->trigger_match_text_col = gtk_tree_view_column_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(preferences->priv->trigger_match_treeview), preferences->priv->trigger_match_register_col);
    gtk_tree_view_append_column(GTK_TREE_VIEW(preferences->priv->trigger_match_treeview), preferences->priv->trigger_match_text_col);
    preferences->priv->trigger_match_register_renderer = gtk_cell_renderer_text_new();
    preferences->priv->trigger_match_text_renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(preferences->priv->trigger_match_register_col, preferences->priv->trigger_match_register_renderer, TRUE);
    gtk_tree_view_column_add_attribute(preferences->priv->trigger_match_register_col,  preferences->priv->trigger_match_register_renderer,
            "text", TRIGGER_MATCH_REGISTER_COLUMN);
    gtk_tree_view_column_pack_start(preferences->priv->trigger_match_text_col, preferences->priv->trigger_match_text_renderer, TRUE);
    gtk_tree_view_column_add_attribute(preferences->priv->trigger_match_text_col,  preferences->priv->trigger_match_text_renderer,
            "text", TRIGGER_MATCH_TEXT_COLUMN);

    // Setup alias match treeview
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(preferences->priv->alias_match_treeview), TRUE);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(preferences->priv->alias_match_treeview), FALSE);
    preferences->priv->alias_match_store = gtk_tree_store_new(TRIGGER_MATCH_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(preferences->priv->alias_match_treeview), GTK_TREE_MODEL(preferences->priv->alias_match_store));
    preferences->priv->alias_match_register_col = gtk_tree_view_column_new();
    preferences->priv->alias_match_text_col = gtk_tree_view_column_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(preferences->priv->alias_match_treeview), preferences->priv->alias_match_register_col);
    gtk_tree_view_append_column(GTK_TREE_VIEW(preferences->priv->alias_match_treeview), preferences->priv->alias_match_text_col);
    preferences->priv->alias_match_register_renderer = gtk_cell_renderer_text_new();
    preferences->priv->alias_match_text_renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(preferences->priv->alias_match_register_col, preferences->priv->alias_match_register_renderer, TRUE);
    gtk_tree_view_column_add_attribute(preferences->priv->alias_match_register_col,  preferences->priv->alias_match_register_renderer,
            "text", TRIGGER_MATCH_REGISTER_COLUMN);
    gtk_tree_view_column_pack_start(preferences->priv->alias_match_text_col, preferences->priv->alias_match_text_renderer, TRUE);
    gtk_tree_view_column_add_attribute(preferences->priv->alias_match_text_col,  preferences->priv->alias_match_text_renderer,
            "text", TRIGGER_MATCH_TEXT_COLUMN);


    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(preferences->priv->treeview), TRUE);

    g_signal_connect(G_OBJECT(preferences->priv->trigger_match_button), "clicked", G_CALLBACK(trigger_match_cb), preferences);
    g_signal_connect(G_OBJECT(preferences->priv->alias_match_test), "clicked", G_CALLBACK(alias_match_cb), preferences);

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

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

void
mud_preferences_window_fill_profiles (MudPreferencesWindow *window)
{
    const GSList *list;
    GSList *entry;
    GtkTreeStore *store;
    GtkTreeIter iter;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *selection;
    GtkTreePath *path;

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
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter,
            TITLE_COLUMN, _("Triggers"),
            DATA_COLUMN, NULL,
            TYPE_COLUMN, GINT_TO_POINTER(COLUMN_TRIGGERS),
            -1);

    list = mud_profile_manager_get_profiles(window->priv->parent->profile_manager);
    for (entry = (GSList *) list; entry != NULL; entry = g_slist_next(entry))
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

        gtk_tree_store_append(store, &iter_child, &iter);
        gtk_tree_store_set(store, &iter_child,
                TITLE_COLUMN, _("Triggers"),
                DATA_COLUMN, profile,
                TYPE_COLUMN, GINT_TO_POINTER(COLUMN_TRIGGERS),
                -1);

    }

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(window->priv->treeview));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
    g_signal_connect(G_OBJECT(selection), "changed",
            G_CALLBACK(mud_preferences_window_tree_selection_cb), window);

    path = gtk_tree_path_new_first();
    gtk_tree_selection_select_path(selection, path);
}

gboolean
mud_preferences_window_trigger_select_cb(GtkTreeSelection *selection,
                     			GtkTreeModel     *model,
                     			GtkTreePath      *path,
                   				gboolean        path_currently_selected,
                     			gpointer          userdata)
{
    GtkTreeIter iter;
    MudPreferencesWindow *prefs = (MudPreferencesWindow *)userdata;
    GConfClient *client;
    gchar *profile_name;
    GError *error = NULL;
    gchar keyname[2048];
    gchar *regex;
    gchar *actions;
    GtkTextBuffer *regex_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(prefs->priv->trigger_regex_textview));
    GtkTextBuffer *action_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(prefs->priv->trigger_action_textview));

    client = gconf_client_get_default();

    g_object_get(prefs->priv->profile, "name", &profile_name, NULL);

    if (gtk_tree_model_get_iter(model, &iter, path))
    {
        gtk_entry_set_text(GTK_ENTRY(prefs->priv->trigger_match_entry), "");
        gtk_label_set_text(GTK_LABEL(prefs->priv->trigger_match_label), "");
        gtk_tree_store_clear(prefs->priv->trigger_match_store);

        gtk_tree_model_get(model, &iter, TRIGGER_NAME_COLUMN, &prefs->priv->trigger_info.text, -1);

        prefs->priv->trigger_info.row = (gtk_tree_path_get_indices(path))[0];
        prefs->priv->trigger_info.iterstr = gtk_tree_model_get_string_from_iter(model, &iter);

        g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/triggers/%s/name", profile_name, prefs->priv->trigger_info.text);
        gtk_entry_set_text(GTK_ENTRY(prefs->priv->trigger_name_entry),gconf_client_get_string(client, keyname, &error));

        g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/triggers/%s/regex", profile_name, prefs->priv->trigger_info.text);
        regex = gconf_client_get_string(client, keyname, &error);

        if(regex)
        {
            gtk_text_buffer_set_text(regex_buffer, regex, strlen(regex));
            g_free(regex);
        }

        g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/triggers/%s/actions", profile_name, prefs->priv->trigger_info.text);
        actions = gconf_client_get_string(client, keyname, &error);

        if(actions)
        {
            gtk_text_buffer_set_text(action_buffer, actions, strlen(actions));
            g_free(actions);
        }

        gtk_widget_set_sensitive(prefs->priv->trigger_del, TRUE);
    }

    g_free(profile_name);

    g_object_unref(client);

    return TRUE;
}

gboolean
mud_preferences_window_alias_select_cb(GtkTreeSelection *selection,
                     			GtkTreeModel     *model,
                     			GtkTreePath      *path,
                   				gboolean        path_currently_selected,
                     			gpointer          userdata)
{
    GtkTreeIter iter;
    MudPreferencesWindow *prefs = (MudPreferencesWindow *)userdata;
    GConfClient *client;
    gchar *profile_name;
    GError *error = NULL;
    gchar keyname[2048];
    gchar *actions;
    gchar *regex;
    GtkTextBuffer *action_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(prefs->priv->alias_textview));
    GtkTextBuffer *regex_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(prefs->priv->alias_regex_textview));

    client = gconf_client_get_default();

    g_object_get(prefs->priv->profile, "name", &profile_name, NULL);

    if (gtk_tree_model_get_iter(model, &iter, path))
    {
        gtk_entry_set_text(GTK_ENTRY(prefs->priv->alias_entry), "");

        gtk_tree_model_get(model, &iter, ALIAS_NAME_COLUMN, &prefs->priv->alias_info.text, -1);
        prefs->priv->alias_info.row = (gtk_tree_path_get_indices(path))[0];
        prefs->priv->alias_info.iterstr = gtk_tree_model_get_string_from_iter(model, &iter);

        g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/aliases/%s/name", profile_name, prefs->priv->alias_info.text);
        gtk_entry_set_text(GTK_ENTRY(prefs->priv->alias_entry),gconf_client_get_string(client, keyname, &error));

        g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/aliases/%s/actions", profile_name, prefs->priv->alias_info.text);
        actions = gconf_client_get_string(client, keyname, &error);

        if(actions)
        {
            gtk_text_buffer_set_text(action_buffer, actions, strlen(actions));
            g_free(actions);
        }

        g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/aliases/%s/regex", profile_name, prefs->priv->alias_info.text);
        regex = gconf_client_get_string(client, keyname, &error);

        if(regex)
        {
            gtk_text_buffer_set_text(regex_buffer, regex, strlen(regex));
            g_free(regex);
        }

        gtk_widget_set_sensitive(prefs->priv->alias_del, TRUE);
    }

    g_free(profile_name);
    g_object_unref(client);

    return TRUE;
}

void mud_preferences_window_trigger_enabled_toggle_cb(GtkCellRendererToggle *cell_renderer,
                                            gchar *path,
                                            gpointer user_data)
{
    GtkTreeIter iter;
    MudPreferencesWindow *prefs = (MudPreferencesWindow *)user_data;
    GtkTreeStore *model = prefs->priv->trigger_store;
    gboolean active;
    gchar *profile_name;
    gchar keyname[2048];
    gchar *name;
    GConfValue *intval;
    GConfClient *client;
    GError *error = NULL;

    client = gconf_client_get_default();

    g_object_get(prefs->priv->profile, "name", &profile_name, NULL);

    intval = gconf_value_new(GCONF_VALUE_INT);

    gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(model), &iter, path);
    gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, TRIGGER_ENABLED_COLUMN, &active, -1);
    gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, TRIGGER_NAME_COLUMN, &name, -1);

    gconf_value_set_int(intval, !active);
    g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/triggers/%s/enabled", profile_name, name);
    gconf_client_set(client, keyname, intval, &error);

    gtk_tree_store_set(model, &iter, TRIGGER_ENABLED_COLUMN, !active, -1);

    g_free(profile_name);
    g_free(name);
    gconf_value_free(intval);
    g_object_unref(client);
}

void mud_preferences_window_trigger_gag_toggle_cb(GtkCellRendererToggle *cell_renderer,
                                            gchar *path,
                                            gpointer user_data)
{
    GtkTreeIter iter;
    MudPreferencesWindow *prefs = (MudPreferencesWindow *)user_data;
    GtkTreeStore *model = prefs->priv->trigger_store;
    gboolean active;
    gchar *profile_name;
    gchar keyname[2048];
    gchar *name;
    GConfValue *intval;
    GConfClient *client;
    GError *error = NULL;

    client = gconf_client_get_default();

    g_object_get(prefs->priv->profile, "name", &profile_name, NULL);

    intval = gconf_value_new(GCONF_VALUE_INT);

    gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(model), &iter, path);
    gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, TRIGGER_GAG_COLUMN, &active, -1);
    gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, TRIGGER_NAME_COLUMN, &name, -1);

    gconf_value_set_int(intval, !active);
    g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/triggers/%s/gag", profile_name, name);
    gconf_client_set(client, keyname, intval, &error);

    gtk_tree_store_set(model, &iter, TRIGGER_GAG_COLUMN, !active, -1);

    g_free(profile_name);
    g_free(name);
    gconf_value_free(intval);
    g_object_unref(client);
}

void mud_preferences_window_alias_enabled_toggle_cb(GtkCellRendererToggle *cell_renderer,
                                            gchar *path,
                                            gpointer user_data)
{
    GtkTreeIter iter;
    MudPreferencesWindow *prefs = (MudPreferencesWindow *)user_data;
    GtkTreeStore *model = prefs->priv->alias_store;
    gboolean active;
    gchar *profile_name;
    gchar keyname[2048];
    gchar *name;
    GConfValue *intval;
    GConfClient *client;
    GError *error = NULL;

    client = gconf_client_get_default();

    g_object_get(prefs->priv->profile, "name", &profile_name, NULL);

    intval = gconf_value_new(GCONF_VALUE_INT);

    gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(model), &iter, path);
    gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, ALIAS_ENABLED_COLUMN, &active, -1);
    gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, ALIAS_NAME_COLUMN, &name, -1);

    gconf_value_set_int(intval, !active);
    g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/aliases/%s/enabled", profile_name, name);
    gconf_client_set(client, keyname, intval, &error);

    gtk_tree_store_set(model, &iter, ALIAS_ENABLED_COLUMN, !active, -1);

    gconf_value_free(intval);
    g_free(profile_name);
    g_free(name);
    g_object_unref(client);
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

        mud_preferences_window_populate_trigger_treeview(window);
        mud_preferences_window_populate_alias_treeview(window);

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

    profile = mud_profile_manager_get_profile_by_name(window->priv->parent->profile_manager, name);
    mud_preferences_window_change_profile(window, profile);
}

static void
mud_preferences_window_change_profile(MudPreferencesWindow *window, MudProfile *profile)
{
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

    g_signal_connect(G_OBJECT(window->priv->encoding_combo), "changed",
            G_CALLBACK(mud_preferences_window_encoding_combo_cb),
            window);
    g_signal_connect(G_OBJECT(window->priv->encoding_check), "toggled",
            G_CALLBACK(mud_preferences_window_encoding_check_cb),
            window);

    g_signal_connect(G_OBJECT(window->priv->proxy_check), "toggled",
            G_CALLBACK(mud_preferences_window_proxy_check_cb),
            window);
    g_signal_connect(G_OBJECT(window->priv->proxy_combo), "changed",
            G_CALLBACK(mud_preferences_window_proxy_combo_cb),
            window);
    g_signal_connect(G_OBJECT(window->priv->proxy_entry), "changed",
            G_CALLBACK(mud_preferences_window_proxy_entry_cb),
            window);

    g_signal_connect(G_OBJECT(window->priv->msp_check), "toggled",
            G_CALLBACK(mud_preferences_window_msp_check_cb),
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

    g_signal_connect(G_OBJECT(window->priv->trigger_del), "clicked",
            G_CALLBACK(mud_preferences_window_trigger_del_cb), window);
    g_signal_connect(G_OBJECT(window->priv->trigger_ok), "clicked",
            G_CALLBACK(mud_preferences_window_trigger_ok_cb), window);

    g_signal_connect(G_OBJECT(window->priv->alias_del), "clicked",
            G_CALLBACK(mud_preferences_window_alias_del_cb), window);
    g_signal_connect(G_OBJECT(window->priv->alias_ok), "clicked",
            G_CALLBACK(mud_preferences_window_alias_ok_cb), window);
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
    mud_preferences_window_update_scrollback(window, profile->preferences);
    mud_preferences_window_update_font(window, profile->preferences);
    mud_preferences_window_update_foreground(window, profile->preferences);
    mud_preferences_window_update_background(window, profile->preferences);
    mud_preferences_window_update_colors(window, profile->preferences);
    mud_preferences_window_update_proxy_check(window, profile->preferences);
    mud_preferences_window_update_proxy_combo(window, profile->preferences);
    mud_preferences_window_update_proxy_entry(window, profile->preferences);
    mud_preferences_window_update_encoding_check(window, profile->preferences);
    mud_preferences_window_update_encoding_combo(window, profile->preferences);
    mud_preferences_window_update_msp_check(window, profile->preferences);
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
mud_preferences_window_encoding_combo_cb(GtkWidget *widget, MudPreferencesWindow *window)
{
    const gchar *s = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
    RETURN_IF_CHANGING_PROFILES(window);

    mud_profile_set_encoding_combo(window->priv->profile, s);
}

static void
mud_preferences_window_encoding_check_cb(GtkWidget *widget, MudPreferencesWindow *window)
{
    gboolean value = GTK_TOGGLE_BUTTON(widget)->active ? TRUE : FALSE;
    RETURN_IF_CHANGING_PROFILES(window);

    mud_profile_set_encoding_check(window->priv->profile, value);
}

static void
mud_preferences_window_proxy_check_cb(GtkWidget *widget, MudPreferencesWindow *window)
{
    gboolean value = GTK_TOGGLE_BUTTON(widget)->active ? TRUE : FALSE;

    gtk_widget_set_sensitive(window->priv->proxy_entry, value);
    gtk_widget_set_sensitive(window->priv->proxy_combo, value);

    RETURN_IF_CHANGING_PROFILES(window);

    mud_profile_set_proxy_check(window->priv->profile, value);
}

static void
mud_preferences_window_msp_check_cb(GtkWidget *widget, MudPreferencesWindow *window)
{
    gboolean value = GTK_TOGGLE_BUTTON(widget)->active ? TRUE : FALSE;
    RETURN_IF_CHANGING_PROFILES(window);

    mud_profile_set_msp_check(window->priv->profile, value);
}

static void
mud_preferences_window_proxy_combo_cb(GtkWidget *widget, MudPreferencesWindow *window)
{
    RETURN_IF_CHANGING_PROFILES(window);

    mud_profile_set_proxy_combo(window->priv->profile, GTK_COMBO_BOX(widget));
}

static void
mud_preferences_window_proxy_entry_cb(GtkWidget *widget, MudPreferencesWindow *window)
{
    const gchar *s = gtk_entry_get_text(GTK_ENTRY(widget));
    RETURN_IF_CHANGING_PROFILES(window);

    if(s)
        mud_profile_set_proxy_entry(window->priv->profile, s);
}

static void
mud_preferences_window_scrollback_cb(GtkWidget *widget, MudPreferencesWindow *window)
{
    const gint value = (gint) gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
    RETURN_IF_CHANGING_PROFILES(window);

    mud_profile_set_scrollback(window->priv->profile, value);
}

static void
mud_preferences_window_font_cb(GtkWidget *widget, MudPreferencesWindow *window)
{
    const gchar *fontname = gtk_font_button_get_font_name(GTK_FONT_BUTTON(widget));

    RETURN_IF_CHANGING_PROFILES(window);
    mud_profile_set_font(window->priv->profile, fontname);
}

static void
mud_preferences_window_foreground_cb(GtkWidget *widget, MudPreferencesWindow *window)
{
    GdkColor color;

    RETURN_IF_CHANGING_PROFILES(window);

    gtk_color_button_get_color(GTK_COLOR_BUTTON(widget), &color);
    mud_profile_set_foreground(window->priv->profile, color.red, color.green, color.blue);
}

static void
mud_preferences_window_background_cb(GtkWidget *widget, MudPreferencesWindow *window)
{
    GdkColor color;

    RETURN_IF_CHANGING_PROFILES(window);

    gtk_color_button_get_color(GTK_COLOR_BUTTON(widget), &color);
    mud_profile_set_background(window->priv->profile, color.red, color.green, color.blue);
}

static void
mud_preferences_window_colors_cb(GtkWidget *widget, MudPreferencesWindow *window)
{
    gint i;
    GdkColor color;

    RETURN_IF_CHANGING_PROFILES(window);

    for (i = 0; i < C_MAX; i++)
    {
        if (widget == window->priv->colors[i])
        {
            gtk_color_button_get_color(GTK_COLOR_BUTTON(widget), &color);
            mud_profile_set_colors(window->priv->profile, i,
                    color.red, color.green, color.blue);
        }
    }
}

static void
mud_preferences_window_trigger_del_cb(GtkWidget *widget, MudPreferencesWindow *window)
{
    GSList *triggers, *entry, *rementry;
    GConfClient *client;
    GError *error = NULL;
    gchar *profile_name;
    gchar keyname[2048];

    rementry = NULL;
    rementry = g_slist_append(rementry, NULL);

    g_object_get(window->priv->profile, "name", &profile_name, NULL);

    client = gconf_client_get_default();

    g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/triggers/list", profile_name);
    triggers = gconf_client_get_list(client, keyname, GCONF_VALUE_STRING, &error);

    for (entry = triggers; entry != NULL; entry = g_slist_next(entry))
    {
        if(strcmp((gchar *)entry->data, window->priv->trigger_info.text) == 0)
        {
            rementry->data = entry->data;
        }
    }

    triggers = g_slist_remove(triggers, rementry->data);

    gconf_client_set_list(client, keyname, GCONF_VALUE_STRING, triggers, &error);

    mud_preferences_window_populate_trigger_treeview(window);

    g_free(profile_name);
    g_object_unref(client);
}

static void
mud_preferences_window_alias_del_cb(GtkWidget *widget, MudPreferencesWindow *window)
{
    GSList *aliases, *entry, *rementry;
    GConfClient *client;
    GError *error = NULL;
    gchar *profile_name;
    gchar keyname[2048];

    rementry = NULL;
    rementry = g_slist_append(rementry, NULL);

    g_object_get(window->priv->profile, "name", &profile_name, NULL);

    client = gconf_client_get_default();

    g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/aliases/list", profile_name);
    aliases = gconf_client_get_list(client, keyname, GCONF_VALUE_STRING, &error);

    for (entry = aliases; entry != NULL; entry = g_slist_next(entry))
    {
        if(strcmp((gchar *)entry->data, window->priv->alias_info.text) == 0)
        {
            rementry->data = entry->data;
        }
    }


    aliases = g_slist_remove(aliases, rementry->data);

    gconf_client_set_list(client, keyname, GCONF_VALUE_STRING, aliases, &error);

    mud_preferences_window_populate_alias_treeview(window);

    g_free(profile_name);
    g_free(aliases);
    g_object_unref(client);
}

static void
mud_preferences_window_trigger_ok_cb(GtkWidget *widget, MudPreferencesWindow *window)
{
    gchar *name;
    gchar *text = NULL;
    gchar *profile_name;
    gchar keyname[2048];
    GConfValue *strval;
    GConfValue *intval;
    GConfClient *client;
    GError *error = NULL;
    gboolean newtrig = TRUE;
    GtkTextIter start, end;
    GSList *triggers, *entry;
    GtkTextBuffer *buffer_regex = gtk_text_view_get_buffer(GTK_TEXT_VIEW(window->priv->trigger_regex_textview));
    GtkTextBuffer *buffer_actions = gtk_text_view_get_buffer(GTK_TEXT_VIEW(window->priv->trigger_action_textview));

    client = gconf_client_get_default();
    strval = gconf_value_new(GCONF_VALUE_STRING);
    intval = gconf_value_new(GCONF_VALUE_INT);

    text = (gchar *)gtk_entry_get_text(GTK_ENTRY(window->priv->trigger_name_entry));

    if(!strlen(text))
        return;

    name = utils_remove_whitespace(text);

    g_object_get(window->priv->profile, "name", &profile_name, NULL);

    g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/triggers/list",profile_name);
    triggers = gconf_client_get_list(client, keyname, GCONF_VALUE_STRING, NULL);

    for(entry = triggers; entry != NULL; entry = g_slist_next(entry))
        if(g_ascii_strcasecmp((gchar *)entry->data,name) == 0)
            newtrig = FALSE;

    if(newtrig)
    {
        triggers = g_slist_append(triggers, (void *)name);
        gconf_client_set_list(client, keyname, GCONF_VALUE_STRING, triggers, &error);
    }

    gconf_value_set_string(strval, name);
    g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/triggers/%s/name", profile_name, name);
    gconf_client_set(client, keyname, strval, &error);

    gtk_text_buffer_get_start_iter(buffer_regex, &start);
    gtk_text_buffer_get_end_iter(buffer_regex, &end);

    gconf_value_set_string(strval, gtk_text_buffer_get_text(buffer_regex, &start, &end, FALSE));
    g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/triggers/%s/regex", profile_name, name);
    gconf_client_set(client, keyname, strval, &error);

    gtk_text_buffer_get_start_iter(buffer_actions, &start);
    gtk_text_buffer_get_end_iter(buffer_actions, &end);

    gconf_value_set_string(strval, gtk_text_buffer_get_text(buffer_actions, &start, &end, FALSE));
    g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/triggers/%s/actions", profile_name, name);
    gconf_client_set(client, keyname, strval, &error);

    gconf_value_set_int(intval, 1);
    g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/triggers/%s/enabled", profile_name, name);
    gconf_client_set(client, keyname, intval, &error);

    gconf_value_set_int(intval, 0);
    g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/triggers/%s/gag", profile_name, name);
    gconf_client_set(client, keyname, intval, &error);

    gconf_value_free(strval);
    gconf_value_free(intval);

    mud_preferences_window_populate_trigger_treeview(window);

    g_free(profile_name);

    g_object_unref(client);
}

static void
mud_preferences_window_alias_ok_cb(GtkWidget *widget, MudPreferencesWindow *window)
{
    gchar *name;
    gchar *text = NULL;
    gchar *profile_name;
    gchar keyname[2048];
    GConfValue *strval;
    GConfValue *intval;
    GConfClient *client;
    gboolean newalias = TRUE;
    GError *error = NULL;
    GtkTextIter start, end;
    GSList *aliases, *entry;
    GtkTextBuffer *buffer_actions = gtk_text_view_get_buffer(GTK_TEXT_VIEW(window->priv->alias_textview));
    GtkTextBuffer *buffer_regex = gtk_text_view_get_buffer(GTK_TEXT_VIEW(window->priv->alias_regex_textview));

    client = gconf_client_get_default();
    strval = gconf_value_new(GCONF_VALUE_STRING);
    intval = gconf_value_new(GCONF_VALUE_INT);

    text = (gchar *)gtk_entry_get_text(GTK_ENTRY(window->priv->alias_entry));

    if(!strlen(text))
        return;

    name = utils_remove_whitespace(text);

    g_object_get(window->priv->profile, "name", &profile_name, NULL);

    g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/aliases/list",profile_name);
    aliases = gconf_client_get_list(client, keyname, GCONF_VALUE_STRING, NULL);

    for(entry = aliases; entry != NULL; entry = g_slist_next(entry))
        if(g_ascii_strcasecmp((gchar *)entry->data,name) == 0)
            newalias = FALSE;

    if(newalias)
    {
        aliases = g_slist_append(aliases, (void *)name);
        gconf_client_set_list(client, keyname, GCONF_VALUE_STRING, aliases, &error);
    }

    gconf_value_set_string(strval, name);
    g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/aliases/%s/name", profile_name, name);
    gconf_client_set(client, keyname, strval, &error);

    gtk_text_buffer_get_start_iter(buffer_regex, &start);
    gtk_text_buffer_get_end_iter(buffer_regex, &end);

    gconf_value_set_string(strval, gtk_text_buffer_get_text(buffer_regex, &start, &end, FALSE));
    g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/aliases/%s/regex", profile_name, name);
    gconf_client_set(client, keyname, strval, &error);

    gtk_text_buffer_get_start_iter(buffer_actions, &start);
    gtk_text_buffer_get_end_iter(buffer_actions, &end);

    gconf_value_set_string(strval, gtk_text_buffer_get_text(buffer_actions, &start, &end, FALSE));
    g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/aliases/%s/actions", profile_name, name);
    gconf_client_set(client, keyname, strval, &error);

    gconf_value_set_int(intval, 1);
    g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/aliases/%s/enabled", profile_name, name);
    gconf_client_set(client, keyname, intval, &error);

    gconf_value_free(strval);
    gconf_value_free(intval);

    mud_preferences_window_populate_alias_treeview(window);

    g_free(profile_name);
    g_object_unref(client);
}

void
mud_preferences_window_populate_trigger_treeview(MudPreferencesWindow *window)
{
    gint enabled_active;
    gint gag_active;
    gchar *profile_name;
    gchar keyname[2048];
    GConfClient *client;
    GError *error = NULL;
    GSList *triggers, *entry;
    GtkTreeIter iter;
    GtkTextIter start, end;
    GtkTextBuffer *buffer_regex = gtk_text_view_get_buffer(GTK_TEXT_VIEW(window->priv->trigger_regex_textview));
    GtkTextBuffer *buffer_action = gtk_text_view_get_buffer(GTK_TEXT_VIEW(window->priv->trigger_action_textview));

    client = gconf_client_get_default();
    g_object_get(window->priv->profile, "name", &profile_name, NULL);

    gtk_entry_set_text(GTK_ENTRY(window->priv->trigger_match_entry), "");
    gtk_entry_set_text(GTK_ENTRY(window->priv->trigger_name_entry), "");
    gtk_label_set_text(GTK_LABEL(window->priv->trigger_match_label), "");

    gtk_tree_store_clear(window->priv->trigger_store);
    gtk_tree_store_clear(window->priv->trigger_match_store);

    gtk_text_buffer_get_start_iter(buffer_regex, &start);
    gtk_text_buffer_get_end_iter(buffer_regex, &end);

    gtk_text_buffer_delete(buffer_regex ,&start, &end);

    gtk_text_buffer_get_start_iter(buffer_action, &start);
    gtk_text_buffer_get_end_iter(buffer_action, &end);

    gtk_text_buffer_delete(buffer_action ,&start, &end);

    gtk_widget_set_sensitive(window->priv->trigger_del, FALSE);

    g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/triggers/list", profile_name);

    triggers = gconf_client_get_list(client, keyname, GCONF_VALUE_STRING, &error);
    for (entry = triggers; entry != NULL; entry = g_slist_next(entry))
    {
        g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/triggers/%s/enabled", profile_name, (gchar *)entry->data);
        enabled_active = gconf_client_get_int(client, keyname, &error);

        g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/triggers/%s/gag", profile_name, (gchar *)entry->data);
        gag_active = gconf_client_get_int(client, keyname, &error);

        gtk_tree_store_append(window->priv->trigger_store, &iter, NULL);
        gtk_tree_store_set(window->priv->trigger_store, &iter,
                TRIGGER_ENABLED_COLUMN, enabled_active,
                TRIGGER_GAG_COLUMN, gag_active,
                TRIGGER_NAME_COLUMN, (gchar *)entry->data,
                -1);
    }

    g_free(profile_name);
    g_object_unref(client);
}

void
mud_preferences_window_populate_alias_treeview(MudPreferencesWindow *window)
{
    gint enabled_active;
    gchar *profile_name;
    gchar keyname[2048];
    GConfClient *client;
    GError *error = NULL;
    GSList *aliases, *entry;
    GtkTreeIter iter;
    GtkTextIter start, end;
    GtkTextBuffer *buffer_action = gtk_text_view_get_buffer(GTK_TEXT_VIEW(window->priv->alias_textview));
    GtkTextBuffer *buffer_regex = gtk_text_view_get_buffer(GTK_TEXT_VIEW(window->priv->alias_regex_textview));

    client = gconf_client_get_default();

    g_object_get(window->priv->profile, "name", &profile_name, NULL);

    gtk_entry_set_text(GTK_ENTRY(window->priv->alias_entry), "");
    gtk_entry_set_text(GTK_ENTRY(window->priv->alias_match_entry), "");
    gtk_label_set_text(GTK_LABEL(window->priv->alias_match_label), "");

    gtk_tree_store_clear(window->priv->alias_store);
    gtk_tree_store_clear(window->priv->alias_match_store);

    gtk_text_buffer_get_start_iter(buffer_action, &start);
    gtk_text_buffer_get_end_iter(buffer_action, &end);

    gtk_text_buffer_delete(buffer_action ,&start, &end);

    gtk_text_buffer_get_start_iter(buffer_regex, &start);
    gtk_text_buffer_get_end_iter(buffer_regex, &end);

    gtk_text_buffer_delete(buffer_regex ,&start, &end);

    gtk_widget_set_sensitive(window->priv->alias_del, FALSE);

    g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/aliases/list", profile_name);

    aliases = gconf_client_get_list(client, keyname, GCONF_VALUE_STRING, &error);
    for (entry = aliases; entry != NULL; entry = g_slist_next(entry))
    {
        g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/aliases/%s/enabled", profile_name, (gchar *)entry->data);
        enabled_active = gconf_client_get_int(client, keyname, &error);

        gtk_tree_store_append(window->priv->alias_store, &iter, NULL);
        gtk_tree_store_set(window->priv->alias_store, &iter,
                ALIAS_ENABLED_COLUMN, enabled_active,
                ALIAS_NAME_COLUMN, (gchar *)entry->data,
                -1);
    }

    g_free(profile_name);
    g_object_unref(client);
}

static void
mud_preferences_window_changed_cb(MudProfile *profile, MudProfileMask *mask, MudPreferencesWindow *window)
{

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
    if (mask->UseProxy)
        mud_preferences_window_update_proxy_check(window, profile->preferences);
    if (mask->UseRemoteEncoding)
        mud_preferences_window_update_encoding_check(window, profile->preferences);
    if (mask->ProxyHostname)
        mud_preferences_window_update_proxy_entry(window, profile->preferences);
    if (mask->ProxyVersion)
        mud_preferences_window_update_proxy_combo(window, profile->preferences);
    if (mask->Encoding)
        mud_preferences_window_update_encoding_combo(window, profile->preferences);
    if (mask->UseRemoteDownload)
        mud_preferences_window_update_msp_check(window, profile->preferences);
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
mud_preferences_window_update_proxy_check(MudPreferencesWindow *window, MudPrefs *preferences)
{
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(window->priv->proxy_check), preferences->UseProxy);

}

static void
mud_preferences_window_update_msp_check(MudPreferencesWindow *window, MudPrefs *preferences)
{
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(window->priv->msp_check), preferences->UseRemoteDownload);

}

static void
mud_preferences_window_update_proxy_combo(MudPreferencesWindow *window, MudPrefs *preferences)
{
    gchar *profile_name;
    GConfClient *client;

    gchar buf[2048];
    gchar extra_path[512] = "";
    gchar *version;
    gint active;
    gint current;

    g_object_get(window->priv->profile, "name", &profile_name, NULL);

    if (!g_str_equal(profile_name, "Default"))
        g_snprintf(extra_path, 512, "profiles/%s/", profile_name);
    g_free(profile_name);

    g_snprintf(buf, 2048, "/apps/gnome-mud/%s%s", extra_path, "functionality/proxy_version");
    client = gconf_client_get_default();
    version = gconf_client_get_string(client, buf, NULL);

    if(version)
    {
        current = gtk_combo_box_get_active(GTK_COMBO_BOX(window->priv->proxy_combo));

        if(strcmp(version,"4") == 0)
            active = 0;
        else
            active = 1;


        if(current != active)
            gtk_combo_box_set_active(GTK_COMBO_BOX(window->priv->proxy_combo), active);

        current = gtk_combo_box_get_active(GTK_COMBO_BOX(window->priv->proxy_combo));
    }

    g_object_unref(client);
}

static void
mud_preferences_window_update_proxy_entry(MudPreferencesWindow *window, MudPrefs *preferences)
{
    if(preferences->ProxyHostname)
        gtk_entry_set_text(GTK_ENTRY(window->priv->proxy_entry), preferences->ProxyHostname);

}

static void
mud_preferences_window_update_encoding_combo(MudPreferencesWindow *window, MudPrefs *preferences)
{
    GtkTreeModel *encodings = gtk_combo_box_get_model(GTK_COMBO_BOX(window->priv->encoding_combo));
    GtkTreeIter iter;
    gboolean valid;
    gint count = 0;

    valid = gtk_tree_model_get_iter_first(encodings, &iter);

    if(!preferences->Encoding)
        return;

    while(valid)
    {
        gchar *encoding;

        gtk_tree_model_get(encodings, &iter, 0, &encoding, -1);

        if(!encoding)
            continue;

        if(strcmp(encoding, preferences->Encoding) == 0)
            break;

        count++;

        valid = gtk_tree_model_iter_next(encodings, &iter);
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX(window->priv->encoding_combo), count);
}

static void
mud_preferences_window_update_encoding_check(MudPreferencesWindow *window, MudPrefs *preferences)
{
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(window->priv->encoding_check), preferences->UseRemoteEncoding);

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
mud_preferences_window_update_scrollback(MudPreferencesWindow *window, MudPrefs *preferences)
{
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(window->priv->sb_lines), preferences->Scrollback);
}

static void
mud_preferences_window_update_font(MudPreferencesWindow *window, MudPrefs *preferences)
{
    gtk_font_button_set_font_name(GTK_FONT_BUTTON(window->priv->fp_font),
            preferences->FontName);
}

static void
mud_preferences_window_update_foreground(MudPreferencesWindow *window, MudPrefs *preferences)
{
    GdkColor color;

    color.red = preferences->Foreground.red;
    color.green = preferences->Foreground.green;
    color.blue = preferences->Foreground.blue;

    gtk_color_button_set_color(GTK_COLOR_BUTTON(window->priv->cp_foreground), &color);
}

static void
mud_preferences_window_update_background(MudPreferencesWindow *window, MudPrefs *preferences)
{
    GdkColor color;

    color.red = preferences->Background.red;
    color.green = preferences->Background.green;
    color.blue = preferences->Background.blue;

    gtk_color_button_set_color(GTK_COLOR_BUTTON(window->priv->cp_background), &color);
}

static void
mud_preferences_window_update_colors(MudPreferencesWindow *window, MudPrefs *preferences)
{
    gint i;
    GdkColor color;

    for (i = 0; i < C_MAX; i++)
    {
        color.red = preferences->Colors[i].red;
        color.green = preferences->Colors[i].green;
        color.blue = preferences->Colors[i].blue;

        gtk_color_button_set_color(GTK_COLOR_BUTTON(window->priv->colors[i]), &color);
    }
}

void
trigger_match_cb(GtkWidget *widget, MudPreferencesWindow *prefs)
{
    gint i;
    gint rc;
    const gchar **matched_strings;
    const gchar *error;
    const gchar *regex;
    const gchar *test_string;
    gint errorcode = 0;
    gint erroroffset;
    GtkTreeIter iter;
    GtkTextIter start, end;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(prefs->priv->trigger_regex_textview));
    gchar buf[512];

    gtk_label_set_text(GTK_LABEL(prefs->priv->trigger_match_label), "");

    gtk_tree_store_clear(prefs->priv->trigger_match_store);

    test_string = gtk_entry_get_text(GTK_ENTRY(prefs->priv->trigger_match_entry));

    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    regex = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    matched_strings = mud_regex_test(test_string, strlen(test_string),regex, &rc, &error, &errorcode, &erroroffset);

    if(errorcode)
    {
        GladeXML *glade;
        GtkWidget *dialog;
        GtkWidget *errcode_lbl;
        GtkWidget *errstring_lbl;
        GtkWidget *regex_lbl;
        gchar buf[2048];
        gchar buf2[2048];
        gchar buf3[2048];
        gchar *markup;
        gint result;
        gint i,j;

        glade = glade_xml_new(GLADEDIR "/prefs.glade", "regex_error_dialog", NULL);
        dialog = glade_xml_get_widget(glade, "regex_error_dialog");

        errcode_lbl = glade_xml_get_widget(glade, "errcode_label");
        errstring_lbl = glade_xml_get_widget(glade, "errorstring_label");
        regex_lbl = glade_xml_get_widget(glade, "regex_label");

        markup = g_markup_printf_escaped ("<b>%d</b>", errorcode);
        gtk_label_set_markup (GTK_LABEL(errcode_lbl), markup);
        g_free(markup);

        markup = g_markup_printf_escaped("<b>%s</b>", _("Error in Regex."));
        gtk_label_set_markup (GTK_LABEL(prefs->priv->trigger_match_label), markup);
        g_free(markup);

        gtk_label_set_text(GTK_LABEL(errstring_lbl), error);

        for(i = 0; i < erroroffset - 1; ++i)
            buf[i] = regex[i];
        buf[erroroffset - 1] = '\0';

        g_snprintf(buf2, 2048, "%s<b>%c</b>", buf, regex[erroroffset-1]);

        j = 0;
        for(i = erroroffset; i < strlen(regex); i++, j++)
            buf[j] = regex[i];
        buf[j] = '\0';

        g_snprintf(buf3, 2048, "%s%s", buf2, buf);

        gtk_label_set_markup (GTK_LABEL(regex_lbl), buf3);

        result = gtk_dialog_run(GTK_DIALOG(dialog));

        gtk_widget_destroy(dialog);
        g_object_unref(glade);

        return;
    }

    if(rc > -1)
    {
        for(i = 0; i < rc; ++i)
        {
            g_snprintf(buf, 512, "%%%d", i);

            gtk_tree_store_append(prefs->priv->trigger_match_store, &iter, NULL);
            gtk_tree_store_set(prefs->priv->trigger_match_store, &iter,
                    TRIGGER_MATCH_REGISTER_COLUMN, buf,
                    TRIGGER_MATCH_TEXT_COLUMN, matched_strings[i],
                    -1);
        }
    }
    else
    {
        gchar *markup;
        markup = g_markup_printf_escaped ("<b>%s</b>", _("No match."));
        gtk_label_set_markup (GTK_LABEL(prefs->priv->trigger_match_label), markup);

        g_free(markup);
    }
}

void
alias_match_cb(GtkWidget *widget, MudPreferencesWindow *prefs)
{
    gint i;
    gint rc;
    const gchar **matched_strings;
    const gchar *error;
    const gchar *regex;
    const gchar *test_string;
    gint errorcode = 0;
    gint erroroffset;
    GtkTreeIter iter;
    GtkTextIter start, end;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(prefs->priv->alias_regex_textview));
    gchar buf[512];

    gtk_label_set_text(GTK_LABEL(prefs->priv->alias_match_label), "");

    gtk_tree_store_clear(prefs->priv->alias_match_store);

    test_string = gtk_entry_get_text(GTK_ENTRY(prefs->priv->alias_match_entry));

    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    regex = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    matched_strings = mud_regex_test(test_string, strlen(test_string), regex, &rc, &error, &errorcode, &erroroffset);

    if(errorcode)
    {
        GladeXML *glade;
        GtkWidget *dialog;
        GtkWidget *errcode_lbl;
        GtkWidget *errstring_lbl;
        GtkWidget *regex_lbl;
        gchar buf[2048];
        gchar buf2[2048];
        gchar buf3[2048];
        gchar *markup;
        gint result;
        gint i,j;

        glade = glade_xml_new(GLADEDIR "/prefs.glade", "regex_error_dialog", NULL);
        dialog = glade_xml_get_widget(glade, "regex_error_dialog");

        errcode_lbl = glade_xml_get_widget(glade, "errcode_label");
        errstring_lbl = glade_xml_get_widget(glade, "errorstring_label");
        regex_lbl = glade_xml_get_widget(glade, "regex_label");

        markup = g_markup_printf_escaped ("<b>%d</b>", errorcode);
        gtk_label_set_markup (GTK_LABEL(errcode_lbl), markup);
        g_free(markup);

        markup = g_markup_printf_escaped("<b>%s</b>", _("Error in Regex."));
        gtk_label_set_markup (GTK_LABEL(prefs->priv->alias_match_label), markup);
        g_free(markup);

        gtk_label_set_text(GTK_LABEL(errstring_lbl), error);

        for(i = 0; i < erroroffset - 1; ++i)
            buf[i] = regex[i];
        buf[erroroffset - 1] = '\0';

        g_snprintf(buf2, 2048, "%s<b>%c</b>", buf, regex[erroroffset-1]);

        j = 0;
        for(i = erroroffset; i < strlen(regex); i++, j++)
            buf[j] = regex[i];
        buf[j] = '\0';

        g_snprintf(buf3, 2048, "%s%s", buf2, buf);

        gtk_label_set_markup (GTK_LABEL(regex_lbl), buf3);

        result = gtk_dialog_run(GTK_DIALOG(dialog));

        gtk_widget_destroy(dialog);
        g_object_unref(glade);

        return;
    }

    if(rc > -1)
    {
        for(i = 0; i < rc; ++i)
        {
            g_snprintf(buf, 512, "%%%d", i);

            gtk_tree_store_append(prefs->priv->alias_match_store, &iter, NULL);
            gtk_tree_store_set(prefs->priv->alias_match_store, &iter,
                    TRIGGER_MATCH_REGISTER_COLUMN, buf,
                    TRIGGER_MATCH_TEXT_COLUMN, matched_strings[i],
                    -1);
        }
    }
    else
    {
        gchar *markup;
        markup = g_markup_printf_escaped ("<b>%s</b>", _("No match."));
        gtk_label_set_markup (GTK_LABEL(prefs->priv->alias_match_label), markup);

        g_free(markup);
    }
}

MudPreferencesWindow*
mud_preferences_window_new (const gchar *profile, MudWindow *window)
{
    MudPreferencesWindow *prefs;

    prefs = g_object_new(MUD_TYPE_PREFERENCES_WINDOW, NULL);

    prefs->priv->parent = window;
    mud_preferences_window_change_profile_from_name(prefs, profile);
    mud_preferences_window_fill_profiles(prefs);

    return prefs;
}

