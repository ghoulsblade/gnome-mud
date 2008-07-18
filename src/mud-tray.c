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
 *
 *
 * Heavily inspired on the gaim docklet plugin
 */

#include <glib/gi18n.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkwidget.h>

#include "gnome-mud.h"
#include "mud-connection-view.h"
#include "mud-window.h"
#include "mud-tray.h"

struct _MudTrayPrivate
{
	MudWindow *mainWindow;

	GtkWidget *window;
	GtkStatusIcon *icon;
	gboolean window_invisible;
};

GType mud_tray_get_type (void);
static void mud_tray_init (MudTray *tray);
static void mud_tray_class_init (MudTrayClass *klass);
static void mud_tray_finalize (GObject *object);

void mud_tray_window_toggle(GtkWidget *widget, MudTray *tray);
void mud_tray_window_exit(GtkWidget *widget, MudTray *tray);
gboolean mud_tray_create_cb(gpointer data);

void mud_tray_create(MudTray *tray);
void mud_tray_destroy(MudTray *tray);


// MudTray class functions
GType
mud_tray_get_type (void)
{
	static GType object_type = 0;

	g_type_init();

	if (!object_type)
	{
		static const GTypeInfo object_info =
		{
			sizeof (MudTrayClass),
			NULL,
			NULL,
			(GClassInitFunc) mud_tray_class_init,
			NULL,
			NULL,
			sizeof (MudTray),
			0,
			(GInstanceInitFunc) mud_tray_init,
		};

		object_type = g_type_register_static(G_TYPE_OBJECT, "MudTray", &object_info, 0);
	}

	return object_type;
}

static void
mud_tray_init (MudTray *tray)
{
	tray->priv = g_new0(MudTrayPrivate, 1);

	tray->priv->window = NULL;
	tray->priv->icon = NULL;
	tray->priv->window_invisible = FALSE;

        mud_tray_create(tray);
}

static void
mud_tray_class_init (MudTrayClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = mud_tray_finalize;
}

static void
mud_tray_finalize (GObject *object)
{
	MudTray *tray;
	GObjectClass *parent_class;

	tray = MUD_TRAY(object);

	mud_tray_destroy(tray);
	g_free(tray->priv);

	parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
	parent_class->finalize(object);
}

// MudTray Callbacks
void mud_tray_window_toggle(GtkWidget *widget, MudTray *tray)
{
        if (tray->priv->window_invisible == FALSE)
                gtk_widget_hide(tray->priv->window);
        else
                gtk_widget_show(tray->priv->window);
        tray->priv->window_invisible = !tray->priv->window_invisible;
}

void mud_tray_window_exit(GtkWidget *widget, MudTray *tray)
{
        mud_tray_destroy(tray);
}

gboolean mud_tray_create_cb(gpointer data)
{
	MudTray *tray = MUD_TRAY(data);

        mud_tray_create(tray);

        return FALSE; /* for when we're called by the glib idle handler */
}

void mud_tray_activate_cb(GtkStatusIcon *icon, MudTray *tray)
{
	mud_tray_window_toggle(NULL, tray);
}


void mud_tray_popup_menu_cb(GtkStatusIcon *icon, guint button,
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
        gtk_signal_connect(GTK_OBJECT(entry), "activate", G_CALLBACK(mud_tray_window_toggle), tray);
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

void mud_tray_update_icon(MudTray *tray, enum mud_tray_status icon)
{
        const gchar *icon_name = NULL;

        switch (icon) {
                case offline:
                        icon_name = GMPIXMAPSDIR "/connection-offline.png";
                        break;
                case offline_connecting:
                case online_connecting:
                        icon_name = GMPIXMAPSDIR "/gnome-mud.svg";
                        break;
                case online:
                        icon_name = GMPIXMAPSDIR "/connection-online.png";
                        break;
        }

        gtk_status_icon_set_from_file(tray->priv->icon, icon_name);
}

void
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

void
mud_tray_destroy(MudTray *tray)
{
        g_object_unref(G_OBJECT(tray->priv->icon));
        tray->priv->icon = NULL;

        g_object_unref(tray->priv->mainWindow);
}

// Instantiate MudTray
MudTray*
mud_tray_new(MudWindow *mainWindow, GtkWidget *window)
{
	MudTray *tray;

	tray = g_object_new(MUD_TYPE_TRAY, NULL);

	tray->priv->window = window;
	tray->priv->mainWindow = mainWindow;

	return tray;
}
