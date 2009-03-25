/* GNOME-Mud - A simple Mud Client
 * mud-tray.c
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

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glib/gprintf.h>

#include "gnome-mud.h"
#include "mud-connection-view.h"
#include "mud-window.h"
#include "mud-tray.h"

struct _MudTrayPrivate
{
    GtkStatusIcon *icon;
    gboolean window_invisible;
};

/* Property Identifiers */
enum
{
    PROP_MUD_TRAY_0,
    PROP_MUD_TRAY_PARENT
};

/* Create the Type */
G_DEFINE_TYPE(MudTray, mud_tray, G_TYPE_OBJECT);

/* Class Functions */
static void mud_tray_init (MudTray *tray);
static void mud_tray_class_init (MudTrayClass *klass);
static void mud_tray_finalize (GObject *object);
static GObject *mud_tray_constructor (GType gtype,
                                      guint n_properties,
                                      GObjectConstructParam *properties);
static void mud_tray_set_property(GObject *object,
                                  guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec);
static void mud_tray_get_property(GObject *object,
                                  guint prop_id,
                                  GValue *value,
                                  GParamSpec *pspec);

/* Callback Functions */
static void mud_tray_window_toggle(GtkWidget *widget, MudTray *tray);
static void mud_tray_window_exit(GtkWidget *widget, MudTray *tray);
static void mud_tray_activate_cb(GtkStatusIcon *icon, MudTray *tray);
static void mud_tray_popup_menu_cb(GtkStatusIcon *icon,
                                   guint button,
                                   guint activate_time,
                                   MudTray *tray);


/* Private Methods */
static void mud_tray_create(MudTray *tray);
static void mud_tray_destroy(MudTray *tray);

/* MudTray class functions */
static void
mud_tray_class_init (MudTrayClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object constructor */
    object_class->constructor = mud_tray_constructor;

    /* Override base object's finalize */
    object_class->finalize = mud_tray_finalize;

    /* Override base object property methods */
    object_class->set_property = mud_tray_set_property;
    object_class->get_property = mud_tray_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(MudTrayPrivate));

    /* Install Properties */
    g_object_class_install_property(object_class,
            PROP_MUD_TRAY_PARENT,
            g_param_spec_object("parent-window",
                "parent gtk window",
                "the gtk window the tray is attached to",
                GTK_TYPE_WIDGET,
                G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));
}

static void
mud_tray_init (MudTray *tray)
{
    /* Get our private data */
    tray->priv = MUD_TRAY_GET_PRIVATE(tray);

    /* set public members to defaults */
    tray->parent_window = NULL;

    /* set private members to defaults */
    tray->priv->icon = NULL;
    tray->priv->window_invisible = FALSE;
}

static GObject *
mud_tray_constructor (GType gtype,
                      guint n_properties,
                      GObjectConstructParam *properties)
{
    MudTray *self;
    GObject *obj;
    MudTrayClass *klass;
    GObjectClass *parent_class;

    /* Chain up to parent constructor */
    klass = MUD_TRAY_CLASS( g_type_class_peek(MUD_TYPE_TRAY) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    self = MUD_TRAY(obj);

    if(!self->parent_window)
    {
        g_printf("ERROR: Tried to instantiate MudTray without passing parent GtkWindow\n");
        g_error("Tried to instantiate MudTray without passing parent GtkWindow\n");
    }

    mud_tray_create(self);

    return obj;
}

static void
mud_tray_finalize (GObject *object)
{
    MudTray *tray;
    GObjectClass *parent_class;

    tray = MUD_TRAY(object);

    mud_tray_destroy(tray);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

static void
mud_tray_set_property(GObject *object,
                      guint prop_id,
                      const GValue *value,
                      GParamSpec *pspec)
{
    MudTray *self;

    self = MUD_TRAY(object);

    switch(prop_id)
    {
        /* Parent is Construct Only */
        case PROP_MUD_TRAY_PARENT:
            self->parent_window = GTK_WIDGET(g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
mud_tray_get_property(GObject *object,
                      guint prop_id,
                      GValue *value,
                      GParamSpec *pspec)
{
    MudTray *self;

    self = MUD_TRAY(object);

    switch(prop_id)
    {
        case PROP_MUD_TRAY_PARENT:
            g_value_take_object(value, self->parent_window);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

/* Private Methods */
static void
mud_tray_create(MudTray *tray)
{
    if (tray->priv->icon) {
        /* if this is being called when a tray icon exists, it's because
           something messed up. try destroying it before we proceed */
        g_message("Trying to create icon but it already exists?\n");
        mud_tray_destroy(tray);
    }

    tray->priv->icon = gtk_status_icon_new(); /*(_("GNOME Mud"));*/
    g_signal_connect(tray->priv->icon, "activate",
            G_CALLBACK (mud_tray_activate_cb), tray);
    g_signal_connect(tray->priv->icon, "popup_menu",
            G_CALLBACK (mud_tray_popup_menu_cb), tray);

    mud_tray_update_icon(tray, offline_connecting);
}

static void
mud_tray_destroy(MudTray *tray)
{
    g_object_unref(G_OBJECT(tray->priv->icon));
    tray->priv->icon = NULL;
}

/* MudTray Callbacks */
static void
mud_tray_window_toggle(GtkWidget *widget, MudTray *tray)
{
    if (tray->priv->window_invisible == FALSE)
        gtk_widget_hide(tray->parent_window);
    else
        gtk_widget_show(tray->parent_window);
    tray->priv->window_invisible = !tray->priv->window_invisible;
}

static void
mud_tray_window_exit(GtkWidget *widget, MudTray *tray)
{
    mud_tray_destroy(tray);
}

static void
mud_tray_activate_cb(GtkStatusIcon *icon, MudTray *tray)
{
    mud_tray_window_toggle(NULL, tray);
}


static void
mud_tray_popup_menu_cb(GtkStatusIcon *icon, guint button,
                            guint activate_time, MudTray *tray)
{
    static GtkWidget *menu = NULL;
    GtkWidget *entry;

    if (menu) {
        gtk_widget_destroy(menu);
    }

    menu = gtk_menu_new();

    if (tray->priv->window_invisible == FALSE)
        entry = gtk_menu_item_new_with_mnemonic(_("_Hide window"));
    else
        entry = gtk_menu_item_new_with_mnemonic(_("_Show window"));
    g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(mud_tray_window_toggle), tray);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), entry);
    entry = gtk_separator_menu_item_new ();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), entry);
    entry = gtk_menu_item_new_with_mnemonic(_("_Quit"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), entry);
    g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(mud_tray_window_exit), tray);

    gtk_widget_show_all(menu);
    gtk_menu_popup(GTK_MENU(menu),
            NULL, NULL,
            gtk_status_icon_position_menu, icon,
            button, activate_time);
}

/* Public Methods */
void
mud_tray_update_icon(MudTray *tray, enum mud_tray_status icon)
{
    const gchar *icon_name = NULL;

    switch (icon) {
        case offline:
            //icon_name = GMPIXMAPSDIR "/connection-offline.png";
            icon_name = "gnome-mud";
            break;
        case offline_connecting:
        case online_connecting:
            icon_name = "gnome-mud";
            break;
        case online:
            //icon_name = GMPIXMAPSDIR "/connection-online.png";
            icon_name = "gnome-mud";
            break;
    }

    gtk_status_icon_set_from_icon_name(tray->priv->icon, icon_name);
}

