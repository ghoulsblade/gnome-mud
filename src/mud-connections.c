/* GNOME-Mud - A simple Mud Client
 * mud-connections.c
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

#include <glib.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <glade/glade-xml.h>
#include <string.h>

#include "gnome-mud.h"
#include "mud-connections.h"
#include "mud-window.h"
#include "mud-connection-view.h"
#include "mud-tray.h"
#include "mud-profile.h"
#include "utils.h"

struct _MudConnectionsPrivate
{
    // Main Window
    GtkWidget *winwidget;
    MudTray *tray;

    GtkWidget *window;
    GtkWidget *iconview;
    GtkWidget *popup;

    GtkTreeModel *icon_model;
    GtkTreeModel *profile_model;

    GtkWidget *qconnect_host_entry;
    GtkWidget *qconnect_port_entry;
    GtkWidget *qconnect_connect_button;

    // Properties Dialog
    GtkWidget *properties_window;
    GtkWidget *name_entry;
    GtkWidget *host_entry;
    GtkWidget *port_entry;

    GtkWidget *icon_button;
    GtkWidget *icon_image;

    GtkWidget *profile_combo;
    GtkCellRenderer *profile_combo_renderer;

    GtkWidget *character_name_entry;
    GtkWidget *logon_textview;

    gboolean changed;

    gchar *original_name;
    gchar *original_char_name;
	
    guint connection;

    // Iconview Dialog
    GtkWidget *icon_dialog;
    GtkWidget *icon_dialog_view;
    GtkTreeModel *icon_dialog_view_model;
    GtkWidget *icon_dialog_chooser;
    gchar *icon_current;
};

typedef enum MudConnectionsModelColumns
{
    MODEL_COLUMN_STRING,
    MODEL_COLUMN_PIXBUF,
    MODEL_COLUMN_N
} MudConnectionsModelColumns;

/* Property Identifiers */
enum
{
    PROP_MUD_CONNECTIONS_0,
    PROP_PARENT_WINDOW
};

/* Create the Type */
G_DEFINE_TYPE(MudConnections, mud_connections, G_TYPE_OBJECT);

/* Class Functions */
static void mud_connections_init (MudConnections *conn);
static void mud_connections_class_init (MudConnectionsClass *klass);
static void mud_connections_finalize (GObject *object);
static GObject *mud_connections_constructor (GType gtype,
                                             guint n_properties,
                                             GObjectConstructParam *properties);
static void mud_connections_set_property(GObject *object,
                                         guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec);
static void mud_connections_get_property(GObject *object,
                                         guint prop_id,
                                         GValue *value,
                                         GParamSpec *pspec);

/* Callbacks */
static gint mud_connections_close_cb(GtkWidget *widget,
				     MudConnections *conn);
static void mud_connections_connect_cb(GtkWidget *widget,
				       MudConnections *conn);
static void mud_connections_add_cb(GtkWidget *widget,
				   MudConnections *conn);
static void mud_connections_delete_cb(GtkWidget *widget,
				      MudConnections *conn);
static void mud_connections_properties_cb(GtkWidget *widget,
					  MudConnections *conn);
static void mud_connections_property_cancel_cb(GtkWidget *widget,
					       MudConnections *conn);
static void mud_connections_property_save_cb(GtkWidget *widget,
					     MudConnections *conn);
static void mud_connections_property_icon_cb(GtkWidget *widget,
					     MudConnections *conn);
static void mud_connections_qconnect_cb(GtkWidget *widget,
                                        MudConnections *conn);
static gboolean mud_connections_property_changed_cb(GtkWidget *widget,
						    GdkEventKey *event,
						    MudConnections *conn);
static void mud_connections_property_combo_changed_cb(GtkWidget *widget,
						      MudConnections *conn);
static gboolean mud_connections_property_delete_cb(GtkWidget *widget,
						   GdkEvent *event,
						   MudConnections *conn);
static gint mud_connections_compare_func(GtkTreeModel *model,
                                         GtkTreeIter *a,
                                         GtkTreeIter *b,
                                         gpointer user_data);

/* Private Methods */
static void mud_connections_populate_iconview(MudConnections *conn);
static void mud_connections_show_properties(MudConnections *conn,
					    gchar *mud);
static gboolean mud_connections_property_save(MudConnections *conn);
static gint mud_connections_property_confirm(void);
static void mud_connections_property_populate_profiles(
    MudConnections *conn);
static void mud_connections_property_combo_get_index(MudConnections *conn,
						     gchar *profile);
static void mud_connections_refresh_iconview(MudConnections *conn);
static gboolean mud_connections_delete_confirm(gchar *name);
static gboolean mud_connections_button_press_cb(GtkWidget *widget,
						GdkEventButton *event,
						MudConnections *conn);
static void mud_connections_popup(MudConnections *conn,
				  GdkEventButton *event);

/* IconDialog Prototypes */
static void mud_connections_show_icon_dialog(MudConnections *conn);

/* IconDialog callbacks */
static void mud_connections_icon_fileset_cb(GtkFileChooserButton *widget,
					    MudConnections *conn);
static void mud_connections_icon_select_cb(GtkIconView *view,
					   MudConnections *conn);
void mud_connections_iconview_activate_cb(GtkIconView *iconview,
					  GtkTreePath *path,
					  MudConnections *conn);
void mud_connections_gconf_notify_cb(GConfClient *client,
				     guint cnxn_id, GConfEntry *entry,
				     gpointer *data);

// MudConnections class functions
static void
mud_connections_class_init (MudConnectionsClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Overide base object's finalize */
    object_class->finalize = mud_connections_finalize;

    /* Override base object constructor */
    object_class->constructor = mud_connections_constructor;

    /* Override base object property methods */
    object_class->set_property = mud_connections_set_property;
    object_class->get_property = mud_connections_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(MudConnectionsPrivate));

    /* Install Properties */
    g_object_class_install_property(object_class,
            PROP_PARENT_WINDOW,
            g_param_spec_object("parent-window",
                "parent mud window",
                "the mud window the connections is attached to",
                MUD_TYPE_WINDOW,
                G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));
}

static void
mud_connections_init (MudConnections *conn)
{
    conn->priv = MUD_CONNECTIONS_GET_PRIVATE(conn);

    conn->parent_window = NULL;
}

static GObject *
mud_connections_constructor (GType gtype,
                             guint n_properties,
                             GObjectConstructParam *properties)
{
    MudConnections *conn;

    GladeXML *glade;
    GConfClient *client;
    GtkWidget *main_window;

    GObject *obj;
    MudConnectionsClass *klass;
    GObjectClass *parent_class;

    /* Chain up to parent constructor */
    klass = MUD_CONNECTIONS_CLASS( g_type_class_peek(MUD_TYPE_CONNECTIONS) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    conn = MUD_CONNECTIONS(obj);

    if(!conn->parent_window)
        g_error("Tried to instantiate MudConnections without passing parent MudWindow\n");

    g_object_get(conn->parent_window,
                 "window", &conn->priv->winwidget,
                 "tray",   &conn->priv->tray,
                 NULL);

    glade = glade_xml_new(GLADEDIR "/muds.glade", "mudviewwindow", NULL);

    conn->priv->window = glade_xml_get_widget(glade, "mudviewwindow");
    conn->priv->iconview = glade_xml_get_widget(glade, "iconview");

    conn->priv->qconnect_host_entry =
        glade_xml_get_widget(glade, "qconnect_host_entry");
    conn->priv->qconnect_port_entry =
        glade_xml_get_widget(glade, "qconnect_port_entry");
    conn->priv->qconnect_connect_button =
        glade_xml_get_widget(glade, "qconnect_connect_button");

    conn->priv->icon_model =
        GTK_TREE_MODEL(gtk_list_store_new(MODEL_COLUMN_N,
                    G_TYPE_STRING,
                    GDK_TYPE_PIXBUF));

    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(conn->priv->icon_model),
                                         MODEL_COLUMN_STRING,
                                         GTK_SORT_ASCENDING);

    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(conn->priv->icon_model),
                                    MODEL_COLUMN_STRING,
                                    mud_connections_compare_func,
                                    NULL,
                                    NULL);

    conn->priv->original_name = NULL;
    conn->priv->original_char_name = NULL;

    gtk_icon_view_set_model(GTK_ICON_VIEW(conn->priv->iconview),
            conn->priv->icon_model);
    g_object_unref(conn->priv->icon_model);

    gtk_icon_view_set_text_column(GTK_ICON_VIEW(conn->priv->iconview),
            MODEL_COLUMN_STRING);
    gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(conn->priv->iconview),
            MODEL_COLUMN_PIXBUF);

    client = gconf_client_get_default();
    gconf_client_add_dir(client, "/apps/gnome-mud/muds",
            GCONF_CLIENT_PRELOAD_NONE, NULL);

    conn->priv->connection =
        gconf_client_notify_add(client, "/apps/gnome-mud/muds",
                (GConfClientNotifyFunc)
                mud_connections_gconf_notify_cb,
                conn, NULL, NULL);

    g_signal_connect(conn->priv->iconview, "item-activated",
            G_CALLBACK(mud_connections_iconview_activate_cb),
            conn);
    g_signal_connect(conn->priv->iconview, "button-press-event",
            G_CALLBACK(mud_connections_button_press_cb),
            conn);
    g_signal_connect(conn->priv->window, "destroy",
            G_CALLBACK(mud_connections_close_cb), conn);
    g_signal_connect(glade_xml_get_widget(glade, "connect_button"),
            "clicked", G_CALLBACK(mud_connections_connect_cb),
            conn);
    g_signal_connect(glade_xml_get_widget(glade, "add_button"),
            "clicked", G_CALLBACK(mud_connections_add_cb), conn);
    g_signal_connect(glade_xml_get_widget(glade, "delete_button"),
            "clicked", G_CALLBACK(mud_connections_delete_cb),
            conn);
    g_signal_connect(glade_xml_get_widget(glade, "properties_button"),
            "clicked", G_CALLBACK(mud_connections_properties_cb),
            conn);
    g_signal_connect(conn->priv->qconnect_connect_button, "clicked",
            G_CALLBACK(mud_connections_qconnect_cb), conn);

    mud_connections_populate_iconview(conn);

    g_object_get(conn->parent_window, "window", &main_window, NULL);

    g_object_set(conn->priv->window,
                 "transient-for", GTK_WINDOW(main_window),
                 NULL);

    gtk_widget_show_all(conn->priv->window);
    g_object_unref(glade);
    g_object_unref(client);

    return obj;
}

static void
mud_connections_finalize (GObject *object)
{
    MudConnections *conn;
    GObjectClass *parent_class;
    GConfClient *client = gconf_client_get_default();

    conn = MUD_CONNECTIONS(object);

    gconf_client_notify_remove(client, conn->priv->connection);
	
    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);

    g_object_unref(client);
}

static void
mud_connections_set_property(GObject *object,
                             guint prop_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
    MudConnections *self;

    self = MUD_CONNECTIONS(object);

    switch(prop_id)
    {
        case PROP_PARENT_WINDOW:
            self->parent_window = MUD_WINDOW(g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
mud_connections_get_property(GObject *object,
                             guint prop_id,
                             GValue *value,
                             GParamSpec *pspec)
{
    MudConnections *self;

    self = MUD_CONNECTIONS(object);

    switch(prop_id)
    {
        case PROP_PARENT_WINDOW:
            g_value_take_object(value, self->parent_window);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

// MudConnections Private Methods
static gint
mud_connections_compare_func(GtkTreeModel *model,
                             GtkTreeIter *a,
                             GtkTreeIter *b,
                             gpointer user_data)
{
    gchar *item_one, *item_two;
    gboolean item_one_haschar, item_two_haschar;
    gint ret = 0;

    gtk_tree_model_get(model,
                       a,
                       MODEL_COLUMN_STRING, &item_one,
                       -1);

    gtk_tree_model_get(model,
                       b,
                       MODEL_COLUMN_STRING, &item_two,
                       -1);

    item_one_haschar = (g_strrstr(item_one, "\n") != NULL);
    item_two_haschar = (g_strrstr(item_two, "\n") != NULL);

    if(item_one_haschar && item_two_haschar)
    {
        gchar **item_onev, **item_twov;

        item_onev = g_strsplit(item_one, "\n", -1);
        item_twov = g_strsplit(item_two, "\n", -1);

        ret = strcmp(item_onev[1], item_twov[1]);

        g_strfreev(item_onev);
        g_strfreev(item_twov);
    } 
    else if(item_one_haschar && !item_two_haschar)
        ret = -1;
    else if(!item_one_haschar && item_two_haschar)
        ret = 1;
    else
        ret = strcmp(item_one, item_two);

    g_free(item_one);
    g_free(item_two);

    return ret;
}

static gint
mud_connections_close_cb(GtkWidget *widget, MudConnections *conn)
{
    g_object_unref(conn);

    return TRUE;
}

static void
mud_connections_connect_cb(GtkWidget *widget, MudConnections *conn)
{
    GList *selected =
	gtk_icon_view_get_selected_items(
	    GTK_ICON_VIEW(conn->priv->iconview));
    GtkTreeIter iter;
    gchar *buf, *mud_name, *key, *strip_name,
	*profile, *host, *logon, *char_name;
    gint port;
    gchar **mud_tuple;
    gint len;
    GConfClient *client = gconf_client_get_default();
    MudConnectionView *view;

    if(g_list_length(selected) == 0)
	return;

    logon = char_name = NULL;

    gtk_tree_model_get_iter(conn->priv->icon_model, &iter,
			    (GtkTreePath *)selected->data);
    gtk_tree_model_get(conn->priv->icon_model, &iter, 0, &buf, -1);
	
    mud_tuple = g_strsplit(buf, "\n", -1);

    len = g_strv_length(mud_tuple);

    switch(len)
    {
    case 1:
	mud_name = g_strdup(mud_tuple[0]);
	break;

    case 2:
	char_name = gconf_escape_key(mud_tuple[0], -1);
	mud_name = g_strdup(mud_tuple[1]);
	break;

    default:
	g_warning("Malformed mud name passed to delete.");
	return;
    }

    g_strfreev(mud_tuple);
    g_free(buf);

    strip_name = gconf_escape_key(mud_name, -1);

    key = g_strdup_printf("/apps/gnome-mud/muds/%s/host", strip_name);
    host = gconf_client_get_string(client, key, NULL);
    g_free(key);

    key = g_strdup_printf("/apps/gnome-mud/muds/%s/profile", strip_name);
    profile = gconf_client_get_string(client, key, NULL);
    g_free(key);

    key = g_strdup_printf("/apps/gnome-mud/muds/%s/port", strip_name);
    port = gconf_client_get_int(client, key, NULL);
    g_free(key);

    if(char_name && strlen(char_name) > 0)
    {
        key = g_strdup_printf(
                "/apps/gnome-mud/muds/%s/characters/%s/logon", 
                strip_name, char_name);
        logon = gconf_client_get_string(client, key, NULL);
        g_free(key);
    }

    mud_tray_update_icon(conn->priv->tray, offline);

    view = g_object_new(MUD_TYPE_CONNECTION_VIEW,
                        "hostname", host,
                        "port", port,
                        "profile-name", profile,
                        "mud-name", mud_name,
                        "connect-string", (logon && strlen(logon) != 0) ? logon : NULL,
                        "window", conn->parent_window,
                        NULL);

    mud_window_add_connection_view(conn->parent_window, G_OBJECT(view), mud_name);
    mud_connection_view_set_profile(view,
                                    mud_profile_manager_get_profile_by_name(conn->parent_window->profile_manager,
                                                                            profile));
    mud_window_profile_menu_set_active(conn->parent_window, profile);

    g_free(mud_name);
    g_free(strip_name);
    g_object_unref(client);

    g_list_foreach(selected, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(selected);

    gtk_widget_destroy(conn->priv->window);
}

static void
mud_connections_qconnect_cb(GtkWidget *widget, MudConnections *conn)
{
    MudConnectionView *view;
    const gchar *host;
    gint port =
        gtk_spin_button_get_value_as_int(
                GTK_SPIN_BUTTON(conn->priv->qconnect_port_entry));

    host = gtk_entry_get_text(GTK_ENTRY(conn->priv->qconnect_host_entry));

    if(strlen(host) != 0)
    {

        mud_tray_update_icon(conn->priv->tray, offline);
        view = g_object_new(MUD_TYPE_CONNECTION_VIEW,
                "hostname", host,
                "port", port,
                "profile-name", "Default",
                "mud-name", (gchar *)host, 
                "connect-string", NULL,
                "window", conn->parent_window,
                NULL);
        mud_window_add_connection_view(conn->parent_window, G_OBJECT(view), (gchar *)host);

        gtk_widget_destroy(conn->priv->window);
    }
}

static void
mud_connections_add_cb(GtkWidget *widget, MudConnections *conn)
{
    mud_connections_show_properties(conn, NULL);
}

static void
mud_connections_delete_cb(GtkWidget *widget, MudConnections *conn)
{
    GList *selected =
	gtk_icon_view_get_selected_items(
	    GTK_ICON_VIEW(conn->priv->iconview));
    GtkTreeIter iter;
    gchar *buf, *mud_name, *key, *strip_name,
          *strip_char_name,  *char_name;
    gchar **mud_tuple;
    gint len;
    GConfClient *client = gconf_client_get_default();

    if(g_list_length(selected) == 0)
	return;

    char_name = strip_name = NULL;

    gtk_tree_model_get_iter(conn->priv->icon_model, &iter,
			    (GtkTreePath *)selected->data);
    gtk_tree_model_get(conn->priv->icon_model, &iter, 0, &buf, -1);
	
    mud_tuple = g_strsplit(buf, "\n", -1);
    g_free(buf);

    len = g_strv_length(mud_tuple);

    switch(len)
    {
        /* Delete Mud */
        case 1:
            mud_name = g_strdup(mud_tuple[0]);
            break;

        /* Delete Character */
        case 2:
            char_name = g_strdup(mud_tuple[0]);
            mud_name = g_strdup(mud_tuple[1]);
            break;

        default:
            g_warning("Malformed mud name passed to delete.");
            return;
    }

    if(!mud_connections_delete_confirm(mud_tuple[0]))
    {
        g_free(mud_name);

        if(char_name)
            g_free(char_name);

        g_strfreev(mud_tuple);
        g_object_unref(client);

        g_list_foreach(selected, (GFunc)gtk_tree_path_free, NULL);
        g_list_free(selected);

        return;
    }

    g_strfreev(mud_tuple);

    if(len == 1)
    {
        strip_name = gconf_escape_key(mud_name, -1);
        
        key = g_strdup_printf("/apps/gnome-mud/muds/%s", strip_name);
        gconf_client_recursive_unset(client, key, 0, NULL);
        
        g_free(key);
        
        gconf_client_suggest_sync(client, NULL);
    }
    else if(len == 2)
    {
        strip_name = gconf_escape_key(mud_name, -1);
        strip_char_name = gconf_escape_key(char_name, -1);

        key = g_strdup_printf("/apps/gnome-mud/muds/%s/characters/%s",
                strip_name, strip_char_name);

        gconf_client_recursive_unset(client, key, 0, NULL);

        g_free(key);
        g_free(strip_char_name);
        g_free(char_name);

        gconf_client_suggest_sync(client, NULL);
    }

    g_free(mud_name);
    g_free(strip_name);
    g_object_unref(client);

    g_list_foreach(selected, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(selected);
}

static void
mud_connections_properties_cb(GtkWidget *widget, MudConnections *conn)
{
    GList *selected =
	gtk_icon_view_get_selected_items(
	    GTK_ICON_VIEW(conn->priv->iconview));
    GtkTreeIter iter;
    gchar *buf;

    if(g_list_length(selected) == 0)
	return;

    gtk_tree_model_get_iter(conn->priv->icon_model, &iter,
			    (GtkTreePath *)selected->data);
    gtk_tree_model_get(conn->priv->icon_model, &iter, 0, &buf, -1);

    mud_connections_show_properties(conn, buf);
}

static void
mud_connections_populate_iconview(MudConnections *conn)
{
    GSList *muds, *characters, *mud_entry, *char_entry;
    gchar *key, *mud_name, *char_name, *display_name,
          *name_strip, *char_strip, *buf;
    GConfClient *client = gconf_client_get_default();
    GtkTreeIter iter;
    GdkPixbuf *icon;

    key = g_strdup("/apps/gnome-mud/muds");
    muds = gconf_client_all_dirs(client, key, NULL);
    g_free(key);

    for(mud_entry = muds; mud_entry != NULL;
            mud_entry = g_slist_next(mud_entry))
    {
        mud_name = g_path_get_basename((gchar *)mud_entry->data);
        name_strip = NULL;

        key = g_strdup_printf("/apps/gnome-mud/muds/%s/name", mud_name);
        name_strip = gconf_client_get_string(client, key, NULL);
        g_free(key);

        key = g_strdup_printf("/apps/gnome-mud/muds/%s/characters",
                mud_name);
        characters = gconf_client_all_dirs(client, key, NULL);
        g_free(key);

        char_entry = characters;

        if(char_entry == NULL) // No Characters
        {
            key = g_strdup_printf("/apps/gnome-mud/muds/%s/icon", mud_name);
            buf = gconf_client_get_string(client, key, NULL);
            g_free(key);

            if(buf && strcmp(buf, "gnome-mud") != 0)
            {
                icon = gdk_pixbuf_new_from_file_at_size(
                        buf, 48, 48, NULL);
                g_free(buf);
            }
            else
                icon =
                    gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                            "gnome-mud", 48, 0, NULL);

            gtk_list_store_append(
                    GTK_LIST_STORE(conn->priv->icon_model), &iter);
            gtk_list_store_set(
                    GTK_LIST_STORE(conn->priv->icon_model), &iter,
                    MODEL_COLUMN_STRING, name_strip,
                    MODEL_COLUMN_PIXBUF, icon,
                    -1);

            g_object_unref(icon);
            continue;
        }

        for(char_entry = characters; char_entry != NULL;
                char_entry = g_slist_next(char_entry))
        {
            char_strip = NULL;
            char_name = g_path_get_basename((gchar *)char_entry->data);

            key = g_strdup_printf(
                    "/apps/gnome-mud/muds/%s/characters/%s/name",
                    mud_name, char_name);
            char_strip = gconf_client_get_string(client, key, NULL);
            g_free(key);

            display_name = g_strconcat(char_strip, "\n", name_strip, NULL);

            key = g_strdup_printf("/apps/gnome-mud/muds/%s/icon", mud_name);
            buf = gconf_client_get_string(client, key, NULL);
            g_free(key);

            if(buf && strcmp(buf, "gnome-mud") != 0)
            {
                icon = gdk_pixbuf_new_from_file_at_size(
                        buf, 48, 48, NULL);
                g_free(buf);
            }
            else
                icon =
                    gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                            "gnome-mud", 48, 0, NULL);

            gtk_list_store_append(GTK_LIST_STORE(conn->priv->icon_model),
                    &iter);
            gtk_list_store_set(GTK_LIST_STORE(conn->priv->icon_model),
                    &iter,
                    MODEL_COLUMN_STRING, display_name,
                    MODEL_COLUMN_PIXBUF, icon,
                    -1);

            g_object_unref(icon);
            g_free(char_name);
            g_free(char_strip);
            g_free(display_name);
        }

        for(char_entry = characters; char_entry != NULL;
                char_entry = g_slist_next(char_entry))
            if(char_entry->data)
                g_free(char_entry->data);

        if(characters)
            g_slist_free(characters);

        g_free(mud_name);
        g_free(name_strip);
    }

    for(mud_entry = muds; mud_entry != NULL;
            mud_entry = g_slist_next(mud_entry))
        if(mud_entry->data)
            g_free(mud_entry->data);

    if(muds)
        g_slist_free(muds);
}

static void
mud_connections_refresh_iconview(MudConnections *conn)
{
    GConfClient *client = gconf_client_get_default();

    gtk_list_store_clear(GTK_LIST_STORE(conn->priv->icon_model));

    gconf_client_suggest_sync(client, NULL);

    mud_connections_populate_iconview(conn);
}

void
mud_connections_gconf_notify_cb(GConfClient *client, guint cnxn_id,
				GConfEntry *entry, gpointer *conn)
{
    mud_connections_refresh_iconview((MudConnections *)conn);
}

void
mud_connections_iconview_activate_cb(GtkIconView *iconview,
				     GtkTreePath *path,
				     MudConnections *conn)
{
    mud_connections_connect_cb(GTK_WIDGET(iconview), conn);
}


static gboolean
mud_connections_button_press_cb(GtkWidget *widget,
				GdkEventButton *event,
				MudConnections *conn)
{
    if ((event->button == 3) &&
            !(event->state & (GDK_SHIFT_MASK |
                    GDK_CONTROL_MASK |
                    GDK_MOD1_MASK)))
    {
        mud_connections_popup(conn, event);
        return TRUE;
    }

    return FALSE;
}

static void
mud_connections_popup(MudConnections *conn, GdkEventButton *event)
{
    GladeXML *glade;
    GtkWidget *popup;
    GList *selected =
	gtk_icon_view_get_selected_items(
	    GTK_ICON_VIEW(conn->priv->iconview));

    if(g_list_length(selected) == 0)
	return;

    glade = glade_xml_new(GLADEDIR "/muds.glade", "popupmenu", NULL);
    popup = glade_xml_get_widget(glade, "popupmenu");

    g_signal_connect(glade_xml_get_widget(glade, "connect"), "activate",
		     G_CALLBACK(mud_connections_connect_cb),
		     conn);
    g_signal_connect(glade_xml_get_widget(glade, "add"), "activate",
		     G_CALLBACK(mud_connections_add_cb),
		     conn);
    g_signal_connect(glade_xml_get_widget(glade, "delete"), "activate",
		     G_CALLBACK(mud_connections_delete_cb),
		     conn);
    g_signal_connect(glade_xml_get_widget(glade, "properties"), "activate",
		     G_CALLBACK(mud_connections_properties_cb),
		     conn);

    g_object_unref(glade);

    gtk_menu_attach_to_widget(GTK_MENU(popup), conn->priv->iconview,
			      NULL);
    gtk_widget_show_all(popup);
    gtk_menu_popup(GTK_MENU(popup), NULL, NULL, NULL, NULL,
		   event ? event->button : 0,
		   event ? event->time : gtk_get_current_event_time());

    g_list_foreach(selected, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(selected);
}

static gboolean
mud_connections_delete_confirm(gchar *name)
{
    GladeXML *glade;
    GtkWidget *dialog;
    GtkWidget *label;
    gint result;
    gchar *message;
    gchar *title;

    glade = glade_xml_new(
	GLADEDIR "/muds.glade", "mudviewdelconfirm", NULL);
    dialog = glade_xml_get_widget(glade, "mudviewdelconfirm");
    label = glade_xml_get_widget(glade, "message");
    g_object_unref(glade);

    message =
	g_strdup_printf(_("Are you sure you want to delete %s?"), name);
    title = g_strdup_printf(_("Delete %s?"), name);

    gtk_label_set_text(GTK_LABEL(label), message);
    gtk_window_set_title(GTK_WINDOW(dialog), title);

    g_free(message);
    g_free(title);

    result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    return (result == GTK_RESPONSE_YES);
}

static void
mud_connections_show_properties(MudConnections *conn, gchar *mud)
{
    GladeXML *glade;
    GConfClient *client;
    GtkTextBuffer *buffer;
    gchar *key, *buf, *name_strip, *char_strip;
    gint port;
    gchar **mud_tuple;

    glade =
	glade_xml_new(GLADEDIR "/muds.glade", "mudviewproperties", NULL);

    conn->priv->properties_window =
	glade_xml_get_widget(glade, "mudviewproperties");
    conn->priv->name_entry = glade_xml_get_widget(glade, "name_entry");
    conn->priv->host_entry = glade_xml_get_widget(glade, "host_entry");
    conn->priv->port_entry = glade_xml_get_widget(glade, "port_entry");
    conn->priv->icon_button = glade_xml_get_widget(glade, "icon_button");
    conn->priv->icon_image = glade_xml_get_widget(glade, "icon_image");
    conn->priv->profile_combo =
	glade_xml_get_widget(glade, "profile_combo");
    conn->priv->character_name_entry =
	glade_xml_get_widget(glade, "character_name_entry");
    conn->priv->logon_textview =
	glade_xml_get_widget(glade, "character_logon_textview");

    if(conn->priv->icon_current)
	g_free(conn->priv->icon_current);
    conn->priv->icon_current = NULL;

    mud_connections_property_populate_profiles(conn);
    gtk_combo_box_set_model(
	GTK_COMBO_BOX(conn->priv->profile_combo),
	conn->priv->profile_model);
    g_object_unref(conn->priv->profile_model);

    conn->priv->profile_combo_renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(conn->priv->profile_combo),
			       conn->priv->profile_combo_renderer, TRUE);
    gtk_cell_layout_set_attributes(
	GTK_CELL_LAYOUT(conn->priv->profile_combo),
	conn->priv->profile_combo_renderer, "text", 0, NULL);

    g_signal_connect(conn->priv->properties_window, "delete-event",
		     G_CALLBACK(mud_connections_property_delete_cb), conn);
    g_signal_connect(glade_xml_get_widget(glade, "cancel_button"),
		     "clicked",
		     G_CALLBACK(mud_connections_property_cancel_cb),
		     conn);
    g_signal_connect(glade_xml_get_widget(glade, "save_button"),
		     "clicked",
		     G_CALLBACK(mud_connections_property_save_cb),
		     conn);
    g_signal_connect(glade_xml_get_widget(glade, "icon_button"),
		     "clicked",
		     G_CALLBACK(mud_connections_property_icon_cb),
		     conn);
    g_signal_connect(glade_xml_get_widget(glade, "name_entry"),
		     "key-press-event",
		     G_CALLBACK(mud_connections_property_changed_cb),
		     conn);
    g_signal_connect(glade_xml_get_widget(glade, "host_entry"),
		     "key-press-event",
		     G_CALLBACK(mud_connections_property_changed_cb),
		     conn);
    g_signal_connect(glade_xml_get_widget(glade, "port_entry"),
		     "key-press-event",
		     G_CALLBACK(mud_connections_property_changed_cb),
		     conn);
    g_signal_connect(glade_xml_get_widget(glade, "character_name_entry"),
		     "key-press-event",
		     G_CALLBACK(mud_connections_property_changed_cb),
		     conn);
    g_signal_connect(
	glade_xml_get_widget(glade, "character_logon_textview"),
	"key-press-event",
	G_CALLBACK(mud_connections_property_changed_cb),
	conn);
    g_signal_connect(glade_xml_get_widget(glade, "profile_combo"),
		     "changed",
		     G_CALLBACK(mud_connections_property_combo_changed_cb),
		     conn);

    g_object_unref(glade);

    if(conn->priv->original_name != NULL)
	g_free(conn->priv->original_name);
    conn->priv->original_name = NULL;

    if(conn->priv->original_char_name != NULL)
	g_free(conn->priv->original_char_name);
    conn->priv->original_char_name = NULL;

    if(mud != NULL)
    {
	gint len;
	mud_tuple = g_strsplit(mud, "\n", -1);
	g_free(mud);
	len = g_strv_length(mud_tuple);

	switch(len)
	{
	case 1:
	    conn->priv->original_name = g_strdup(mud_tuple[0]);
	    break;

	case 2:
	    conn->priv->original_char_name = g_strdup(mud_tuple[0]);
	    conn->priv->original_name = g_strdup(mud_tuple[1]);
	    break;
	}

	g_strfreev(mud_tuple);
    } else
	return;

    name_strip = gconf_escape_key(conn->priv->original_name, -1);

    gtk_entry_set_text(
	GTK_ENTRY(conn->priv->name_entry), conn->priv->original_name);

    if(conn->priv->original_char_name)
	gtk_entry_set_text(GTK_ENTRY(conn->priv->character_name_entry),
			   conn->priv->original_char_name);

    client = gconf_client_get_default();

    key = g_strdup_printf("/apps/gnome-mud/muds/%s/host", name_strip);
    buf = gconf_client_get_string(client, key, NULL);
    g_free(key);

    if(buf)
    {
	gtk_entry_set_text(GTK_ENTRY(conn->priv->host_entry), buf);
	g_free(buf);
    }

    key = g_strdup_printf("/apps/gnome-mud/muds/%s/port", name_strip);
    port = gconf_client_get_int(client, key, NULL);
    g_free(key);

    if(port != 0)
	gtk_spin_button_set_value(
	    GTK_SPIN_BUTTON(conn->priv->port_entry), port);

    key = g_strdup_printf("/apps/gnome-mud/muds/%s/profile", name_strip);
    buf = gconf_client_get_string(client, key, NULL);
    g_free(key);

    if(buf)
    {
	mud_connections_property_combo_get_index(conn, buf);
	g_free(buf);
    }

    key = g_strdup_printf("/apps/gnome-mud/muds/%s/icon", name_strip);
    buf = gconf_client_get_string(client, key, NULL);
    g_free(key);

    if(buf && strcmp(buf, "gnome-mud") != 0)
    {
	GdkPixbuf *icon;

	conn->priv->icon_current = g_strdup(buf);
	g_free(buf);

	icon = gdk_pixbuf_new_from_file_at_size(
	    conn->priv->icon_current, 48, 48, NULL);
	gtk_image_set_from_pixbuf(GTK_IMAGE(conn->priv->icon_image),
				  icon);

	g_object_unref(icon);
    }
    else
	conn->priv->icon_current = g_strdup("gnome-mud");

    if(conn->priv->original_char_name != NULL)
    {
	char_strip = gconf_escape_key(conn->priv->original_char_name, -1);
		
	key = g_strdup_printf("/apps/gnome-mud/muds/%s/characters/%s/logon",
			      name_strip, char_strip);
	buf = gconf_client_get_string(client, key, NULL);
	g_free(key);

	if(buf)
	{
	    buffer =
		gtk_text_view_get_buffer(
		    GTK_TEXT_VIEW(conn->priv->logon_textview));
	    gtk_text_buffer_set_text(buffer, buf, strlen(buf));
	    g_free(buf);
	}

	g_free(char_strip);
    }

    g_free(name_strip);

    g_object_unref(client);

    conn->priv->changed = FALSE;
}

static void
mud_connections_property_cancel_cb(GtkWidget *widget, MudConnections *conn)
{
    gtk_widget_destroy(conn->priv->properties_window);
}

static void
mud_connections_property_save_cb(GtkWidget *widget, MudConnections *conn)
{
    if(mud_connections_property_save(conn))
	gtk_widget_destroy(conn->priv->properties_window);
}
static void
mud_connections_property_icon_cb(GtkWidget *widget, MudConnections *conn)
{
    mud_connections_show_icon_dialog(conn);
}

static gboolean
mud_connections_property_changed_cb(GtkWidget *widget,
				    GdkEventKey *event,
				    MudConnections *conn)
{
    conn->priv->changed = TRUE;

    return FALSE;
}

static void
mud_connections_property_combo_changed_cb(GtkWidget *widget,
					  MudConnections *conn)
{
    conn->priv->changed = TRUE;
}

static gboolean
mud_connections_property_delete_cb(GtkWidget *widget,
				   GdkEvent *event,
				   MudConnections *conn)
{
    if(conn->priv->changed)
	switch(mud_connections_property_confirm())
	{
	case -1:
	    return FALSE;
	    break;	
	case 1:
	    return mud_connections_property_save(conn);
	    break;
	case 0:
	    return TRUE;
	    break;
	}

    return FALSE;
}

static gint
mud_connections_property_confirm(void)
{
    GladeXML *glade;
    GtkWidget *dialog;
    gint result;

    glade = glade_xml_new(GLADEDIR "/muds.glade", "mudviewconfirm", NULL);
    dialog = glade_xml_get_widget(glade, "mudviewconfirm");
    g_object_unref(glade);

    result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    switch(result)
    {
    case GTK_RESPONSE_OK:
	return 1;
	break;

    case GTK_RESPONSE_CLOSE:
	return -1;
	break;

    default:
	return 0;
	break;
    }
}

static gboolean
mud_connections_property_save(MudConnections *conn)
{
    GConfClient *client;
    const gchar *name =
	gtk_entry_get_text(GTK_ENTRY(conn->priv->name_entry));
    const gchar *host =
	gtk_entry_get_text(GTK_ENTRY(conn->priv->host_entry));
    const gchar *character_name
	= gtk_entry_get_text(GTK_ENTRY(conn->priv->character_name_entry));
    gchar *logon, *profile, *key, *buf, *char_name,
	*stripped_name, *strip_name_new;
    GtkTextIter start, end;
    GtkTextBuffer *buffer;
    GtkTreeIter iter;
    GSList *chars, *entry;

    buffer =
	gtk_text_view_get_buffer(GTK_TEXT_VIEW(conn->priv->logon_textview));
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    logon = gtk_text_iter_get_text(&start, &end);

    if(gtk_combo_box_get_active_iter(
	   GTK_COMBO_BOX(conn->priv->profile_combo), &iter))
	gtk_tree_model_get(
	    GTK_TREE_MODEL(conn->priv->profile_model),
	    &iter, 0, &profile, -1);
    else
	profile = g_strdup("Default");

    if(strlen(name) == 0)
    {
	utils_error_message(conn->priv->properties_window, _("Error"),
			    "%s", _("No mud name specified."));

	if(logon)
	    g_free(logon);

	if(profile)
	    g_free(profile);

	return FALSE;
    }
       
    client = gconf_client_get_default();

    /* If the user renames the mud we need to 
     * transfer over the old character information
     * and do some cleanup */
    if(conn->priv->original_name &&
       strcmp(conn->priv->original_name, name) != 0)
    {
	stripped_name = gconf_escape_key(conn->priv->original_name, -1);
	strip_name_new = gconf_escape_key(name, -1);

	key = g_strdup_printf("/apps/gnome-mud/muds/%s/characters",
			      stripped_name);
	chars = gconf_client_all_dirs(client, key, NULL);
	g_free(key);

	for(entry = chars; entry != NULL; entry = g_slist_next(entry))
	{
	    char_name = g_path_get_basename((gchar *)entry->data);

	    key = g_strdup_printf(
		"/apps/gnome-mud/muds/%s/characters/%s/logon",
		stripped_name, char_name);
	    buf = gconf_client_get_string(client, key, NULL);
	    g_free(key);

	    key = g_strdup_printf(
		"/apps/gnome-mud/muds/%s/characters/%s/logon",
		strip_name_new, char_name);
	    gconf_client_set_string(client, key, buf, NULL);
	    g_free(key);

	    g_free(char_name);
	    g_free(buf);
	}

	for(entry = chars; entry != NULL; entry = g_slist_next(entry))
	    if(entry->data)
		g_free(entry->data);
	g_slist_free(chars);

	key = g_strdup_printf(
	    "/apps/gnome-mud/muds/%s", stripped_name);
	gconf_client_recursive_unset(client, key, 0, NULL);
	g_free(key);

	g_free(stripped_name);
	g_free(strip_name_new);
    }

    stripped_name = gconf_escape_key(name, -1);
    key = g_strdup_printf("/apps/gnome-mud/muds/%s/name", stripped_name);
    gconf_client_set_string(client, key, name, NULL);
    g_free(key);

    key = g_strdup_printf("/apps/gnome-mud/muds/%s/host", stripped_name);
    gconf_client_set_string(client, key, host, NULL);
    g_free(key);

    key = g_strdup_printf("/apps/gnome-mud/muds/%s/port", stripped_name);
    gconf_client_set_int(client, key, 
			 gtk_spin_button_get_value_as_int(
			     GTK_SPIN_BUTTON(conn->priv->port_entry)),
			 NULL);
    g_free(key);

    key = g_strdup_printf("/apps/gnome-mud/muds/%s/icon", stripped_name);
    if(conn->priv->icon_current &&
       strcmp(conn->priv->icon_current, "gnome-mud") != 0)
	gconf_client_set_string(
	    client, key, conn->priv->icon_current, NULL);
    else
	gconf_client_set_string(client, key, "gnome-mud", NULL);
    g_free(key);

    key = g_strdup_printf("/apps/gnome-mud/muds/%s/profile", stripped_name);
    gconf_client_set_string(client, key, profile, NULL);
    g_free(key);

    if(conn->priv->original_char_name && 
       strcmp(conn->priv->original_char_name, character_name) != 0)
    {
	strip_name_new = gconf_escape_key(conn->priv->original_char_name, -1);
	key = g_strdup_printf("/apps/gnome-mud/muds/%s/characters/%s",
			      stripped_name, strip_name_new);
	gconf_client_recursive_unset(client, key, 0, NULL);
	g_free(key);
	g_free(strip_name_new);
    }

    strip_name_new = gconf_escape_key(character_name, -1);

    if(strlen(strip_name_new) > 0)
    {
	key = g_strdup_printf("/apps/gnome-mud/muds/%s/characters/%s/logon",
			      stripped_name, strip_name_new);
	gconf_client_set_string(client, key, logon, NULL);
	g_free(key);

	key = g_strdup_printf("/apps/gnome-mud/muds/%s/characters/%s/name",
			      stripped_name, strip_name_new);
	gconf_client_set_string(client, key, character_name, NULL);
	g_free(key);
    }

    g_object_unref(client);

    if(logon)
	g_free(logon);

    if(profile)
	g_free(profile);

    return TRUE;
}

static void
mud_connections_property_populate_profiles(MudConnections *conn)
{
    const GSList *profiles, *entry;
    GtkTreeIter iter;

    profiles = mud_profile_manager_get_profiles(conn->parent_window->profile_manager);

    conn->priv->profile_model =
	GTK_TREE_MODEL(gtk_list_store_new(1, G_TYPE_STRING));

    for(entry = profiles; entry != NULL; entry = g_slist_next(entry))
    {
        MudProfile *profile = MUD_PROFILE(entry->data);
	gtk_list_store_append(
	    GTK_LIST_STORE(conn->priv->profile_model), &iter);
	gtk_list_store_set(GTK_LIST_STORE(conn->priv->profile_model), &iter,
			   0, profile->name, -1);
    }
}

static void
mud_connections_property_combo_get_index(MudConnections *conn,
					 gchar *profile)
{
    gchar *buf;
    GtkTreeIter iter;
	
    gtk_tree_model_get_iter_first(conn->priv->profile_model, &iter);

    do
    {
	gtk_tree_model_get(conn->priv->profile_model, &iter, 0, &buf, -1);
		
	if(strcmp(buf, profile) == 0)
	{
	    gtk_combo_box_set_active_iter(
		GTK_COMBO_BOX(conn->priv->profile_combo),
		&iter);
	    g_free(buf);
	    return;
	}

	g_free(buf);
    }
    while(gtk_tree_model_iter_next(conn->priv->profile_model, &iter));
}

// Iconview Dialog
static void
mud_connections_show_icon_dialog(MudConnections *conn)
{
    GladeXML *glade;
    gint result;

    glade = glade_xml_new(GLADEDIR "/muds.glade", "iconselect", NULL);
    conn->priv->icon_dialog = glade_xml_get_widget(glade, "iconselect");
    conn->priv->icon_dialog_view = glade_xml_get_widget(glade, "view");
    conn->priv->icon_dialog_chooser =
	glade_xml_get_widget(glade, "chooser");
    g_object_unref(glade);

    conn->priv->icon_dialog_view_model =
	GTK_TREE_MODEL(gtk_list_store_new(MODEL_COLUMN_N,
					  G_TYPE_STRING,
					  GDK_TYPE_PIXBUF));

    gtk_icon_view_set_model(GTK_ICON_VIEW(conn->priv->icon_dialog_view),
			    conn->priv->icon_dialog_view_model);
    gtk_icon_view_set_text_column(
	GTK_ICON_VIEW(conn->priv->icon_dialog_view),
	MODEL_COLUMN_STRING);
    gtk_icon_view_set_pixbuf_column(
	GTK_ICON_VIEW(conn->priv->icon_dialog_view),
	MODEL_COLUMN_PIXBUF);

    if(conn->priv->icon_current != NULL)
	g_free(conn->priv->icon_current);
    conn->priv->icon_current = NULL;

    g_signal_connect(
	conn->priv->icon_dialog_chooser, "current-folder-changed",
		     G_CALLBACK(mud_connections_icon_fileset_cb),
		     conn);
    g_signal_connect(conn->priv->icon_dialog_view, "selection-changed",
		     G_CALLBACK(mud_connections_icon_select_cb),
		     conn);

    g_object_set(conn->priv->icon_dialog_view, "item-width", 64, NULL);
    gtk_file_chooser_set_current_folder(
	GTK_FILE_CHOOSER(conn->priv->icon_dialog_chooser),
	"/usr/share/icons");

    result = gtk_dialog_run(GTK_DIALOG(conn->priv->icon_dialog));
    gtk_widget_destroy(conn->priv->icon_dialog);

    if(result == GTK_RESPONSE_OK)
    {
	if(conn->priv->icon_current == NULL)
	    return;

	GdkPixbuf *icon = gdk_pixbuf_new_from_file_at_size(
	    conn->priv->icon_current, 48, 48, NULL);
	gtk_image_set_from_pixbuf(GTK_IMAGE(conn->priv->icon_image),
				  icon);
	g_object_unref(icon);
    }
}

void
mud_connections_icon_fileset_cb(GtkFileChooserButton *chooser,
				MudConnections *conn)
{
    const gchar *file;
    gchar *current_folder =
	gtk_file_chooser_get_current_folder(
	    GTK_FILE_CHOOSER(conn->priv->icon_dialog_chooser));
    GDir *dir = g_dir_open(current_folder ,0, NULL);
    GPatternSpec *xpm_pattern = g_pattern_spec_new("*.xpm");
    GPatternSpec *svg_pattern = g_pattern_spec_new("*.svg");
    GPatternSpec *bmp_pattern = g_pattern_spec_new("*.bmp");
    GPatternSpec *png_pattern = g_pattern_spec_new("*.png");

    gtk_list_store_clear(
	GTK_LIST_STORE(conn->priv->icon_dialog_view_model));

    while( (file = g_dir_read_name(dir) ) != NULL)
	if( g_pattern_match_string(xpm_pattern, file) ||
	    g_pattern_match_string(svg_pattern, file) ||
	    g_pattern_match_string(bmp_pattern, file) ||
	    g_pattern_match_string(png_pattern, file))
	{
	    gchar *full_path = g_strconcat(current_folder,
					   G_DIR_SEPARATOR_S,
					   file, NULL);
	    GdkPixbuf *icon = gdk_pixbuf_new_from_file_at_size(
		full_path, 48, 48, NULL);
	    GtkTreeIter iter;

            if(icon)
            {
                gtk_list_store_append(
                        GTK_LIST_STORE(conn->priv->icon_dialog_view_model),
                        &iter);
                gtk_list_store_set(
                        GTK_LIST_STORE(conn->priv->icon_dialog_view_model),
                        &iter,
                        MODEL_COLUMN_STRING, file,
                        MODEL_COLUMN_PIXBUF, icon,
                        -1);

                g_object_unref(icon);
            }

	    g_free(full_path);
	}

    g_free(current_folder);
    g_dir_close(dir);
    g_pattern_spec_free(xpm_pattern);
    g_pattern_spec_free(svg_pattern);
    g_pattern_spec_free(bmp_pattern);
    g_pattern_spec_free(png_pattern);
}

static void
mud_connections_icon_select_cb(GtkIconView *view, MudConnections *conn)
{
    GList *selected =
	gtk_icon_view_get_selected_items(
	    GTK_ICON_VIEW(conn->priv->icon_dialog_view));
    GtkTreeIter iter;
    gchar *buf;

    if(g_list_length(selected) == 0)
	return;

    gtk_tree_model_get_iter(conn->priv->icon_dialog_view_model,
			    &iter, (GtkTreePath *)selected->data);
    gtk_tree_model_get(conn->priv->icon_dialog_view_model,
		       &iter, 0, &buf, -1);

    if(conn->priv->icon_current != NULL)
	g_free(conn->priv->icon_current);

    conn->priv->icon_current = g_strconcat(
	gtk_file_chooser_get_current_folder(
	    GTK_FILE_CHOOSER(conn->priv->icon_dialog_chooser)),
	G_DIR_SEPARATOR_S,
	buf, NULL);

    g_free(buf);

    g_list_foreach(selected, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(selected);
}

