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

/* Model Columns */
enum
{
    NAME_COLUMN,
    N_COLUMNS
};

/* Property Identifiers */
enum
{
    PROP_MUD_PROFILE_WINDOW_0,
    PROP_PARENT_WINDOW
};

/* Create the Type */
G_DEFINE_TYPE(MudProfileWindow, mud_profile_window, G_TYPE_OBJECT);

/* Class Functions */
static void mud_profile_window_init (MudProfileWindow *preferences);
static void mud_profile_window_class_init (MudProfileWindowClass *klass);
static void mud_profile_window_finalize (GObject *object);
static GObject *mud_profile_window_constructor (GType gtype,
                                                guint n_properties,
                                                GObjectConstructParam *properties);
static void mud_profile_window_set_property(GObject *object,
                                            guint prop_id,
                                            const GValue *value,
                                            GParamSpec *pspec);
static void mud_profile_window_get_property(GObject *object,
                                            guint prop_id,
                                            GValue *value,
                                            GParamSpec *pspec);

/* Callbacks */
static gint mud_profile_window_close_cb(GtkWidget *widget, MudProfileWindow *profwin);
static void mud_profile_window_add_cb(GtkWidget *widget, MudProfileWindow *profwin);
static void mud_profile_window_del_cb(GtkWidget *widget, MudProfileWindow *profwin);
static gboolean mud_profile_window_tree_select_cb(GtkTreeSelection *selection,
                     			   GtkTreeModel     *model,
                     			   GtkTreePath      *path,
                   			   gboolean        path_currently_selected,
                     			   gpointer          userdata);
/* Private Methods */
static void mud_profile_window_populate_treeview(MudProfileWindow *profwin);

/* Class Functions */
static void
mud_profile_window_class_init (MudProfileWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object constructor */
    object_class->constructor = mud_profile_window_constructor;

    /* Override base object's finalize */
    object_class->finalize = mud_profile_window_finalize;

    /* Override base object property methods */
    object_class->set_property = mud_profile_window_set_property;
    object_class->get_property = mud_profile_window_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(MudProfileWindowPrivate));

    /* Install Properties */
    g_object_class_install_property(object_class,
            PROP_PARENT_WINDOW,
            g_param_spec_object("parent-window",
                "parent gtk window",
                "the gtk window parent of this window",
                TYPE_MUD_WINDOW,
                G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));
}

static void
mud_profile_window_init (MudProfileWindow *self)
{
    /* Get our private data */
    self->priv = MUD_PROFILE_WINDOW_GET_PRIVATE(self);

    /* set public members to defaults */
    self->parent_window = NULL;
}

static GObject *
mud_profile_window_constructor (GType gtype,
                                guint n_properties,
                                GObjectConstructParam *properties)
{
    guint i;
    MudProfileWindow *profwin;
    GObject *obj;

    MudProfileWindowClass *klass;
    GObjectClass *parent_class;

    GladeXML *glade;
    GtkWindow *main_window;

    /* Chain up to parent constructor */
    klass = MUD_PROFILE_WINDOW_CLASS( g_type_class_peek(MUD_TYPE_PROFILE_WINDOW) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    profwin = MUD_PROFILE_WINDOW(obj);

    if(!profwin->parent_window)
    {
        g_printf("ERROR: Tried to instantiate MudProfileWindow without passing parent GtkWindow\n");
        g_error("Tried to instantiate MudProfileWindow without passing parent GtkWindow\n");
    }

    glade = glade_xml_new(GLADEDIR "/prefs.glade", "profiles_window", NULL);

    profwin->priv->window = glade_xml_get_widget(glade, "profiles_window");

    profwin->priv->btnAdd = glade_xml_get_widget(glade, "btnAdd");
    profwin->priv->btnDelete = glade_xml_get_widget(glade, "btnDelete");

    profwin->priv->treeview = glade_xml_get_widget(glade, "profilesView");
    profwin->priv->treestore = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING);

    g_object_set(profwin->priv->treeview,
                 "model", GTK_TREE_MODEL(profwin->priv->treestore),
                 "rules-hint", TRUE,
                 "headers-visible", FALSE,
                 NULL);
    
    profwin->priv->col = gtk_tree_view_column_new();

    gtk_tree_view_append_column(GTK_TREE_VIEW(profwin->priv->treeview),
                                profwin->priv->col);
    
    profwin->priv->renderer = gtk_cell_renderer_text_new();
    
    gtk_tree_view_column_pack_start(profwin->priv->col,
                                   profwin->priv->renderer,
                                   TRUE);

    gtk_tree_view_column_add_attribute(profwin->priv->col,
                                       profwin->priv->renderer,
                                       "text",
                                       NAME_COLUMN);

    gtk_tree_selection_set_select_function(
            gtk_tree_view_get_selection(GTK_TREE_VIEW(profwin->priv->treeview)),
            mud_profile_window_tree_select_cb,
            profwin, NULL);

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

    g_object_get(profwin->parent_window, "window", &main_window, NULL);

    gtk_window_set_transient_for(
            GTK_WINDOW(profwin->priv->window),
            main_window);

    gtk_widget_show_all(profwin->priv->window);

    g_object_unref(glade);

    return obj;
}

static void
mud_profile_window_finalize (GObject *object)
{
    MudProfileWindow *profwin;
    GObjectClass *parent_class;

    profwin = MUD_PROFILE_WINDOW(object);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

static void
mud_profile_window_set_property(GObject *object,
                                guint prop_id,
                                const GValue *value,
                                GParamSpec *pspec)
{
    MudProfileWindow *self;

    self = MUD_PROFILE_WINDOW(object);

    switch(prop_id)
    {
        /* Parent is Construct Only */
        case PROP_PARENT_WINDOW:
            self->parent_window = MUD_WINDOW(g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
mud_profile_window_get_property(GObject *object,
                                guint prop_id,
                                GValue *value,
                                GParamSpec *pspec)
{
    MudProfileWindow *self;

    self = MUD_PROFILE_WINDOW(object);

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

/* Callbacks */
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
        mud_window_populate_profiles_menu(profwin->parent_window);
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
        mud_window_populate_profiles_menu(profwin->parent_window);
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

/* Private Methods */
static void
mud_profile_window_populate_treeview(MudProfileWindow *profwin)
{
    const GList *profiles, *entry;
    GtkTreeStore* store;
    GtkTreeIter iter;

    g_return_if_fail(MUD_IS_PROFILE_WINDOW(profwin));

    store = GTK_TREE_STORE(profwin->priv->treestore);

    gtk_tree_store_clear(store);

    gtk_widget_set_sensitive(profwin->priv->btnDelete, FALSE);

    profiles = mud_profile_get_profiles();

    entry = profiles;
    while(entry)
    {
        MudProfile *profile = MUD_PROFILE(entry->data);

        gtk_tree_store_append(store, &iter, NULL);
        gtk_tree_store_set(store, &iter,
                           NAME_COLUMN, profile->name,
                           -1);

        entry = g_list_next(entry);
    }
}

