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
 * Heavily inspired on the gaim docklet plugin (give credit where credit is due I say).
 */

#include <gtk/gtkmain.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkimage.h>
#include <libgnome/gnome-i18n.h>

#include "gnome-mud.h"
#include "eggtrayicon.h"
#include "mud-connection-view.h"
#include "mud-window.h"
#include "mud-tray.h"

static char const rcsid[] = "$Id: ";

struct _MudTrayPrivate
{
	MudWindow *mainWindow;
	
	GtkWidget *window;
	EggTrayIcon *tray_icon;
	GtkWidget *image;
	gboolean window_invisible;
};
	
GType mud_tray_get_type (void);
static void mud_tray_init (MudTray *tray);
static void mud_tray_class_init (MudTrayClass *klass);
static void mud_tray_finalize (GObject *object);

void mud_tray_window_toggle(GtkWidget *widget, MudTray *tray);
void mud_tray_window_exit(GtkWidget *widget, MudTray *tray);
gboolean mud_tray_create_cb(gpointer data);
void mud_tray_destroyed_cb(GtkWidget *widget, void *data);
void mud_tray_clicked_cb(GtkWidget *button, GdkEventButton *event, void *data);

void mud_tray_menu(MudTray *tray);
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
	tray->priv->tray_icon = NULL;
	tray->priv->image = NULL;
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
                gtk_widget_hide_all(tray->priv->window);
        else
                gtk_widget_show_all(tray->priv->window);
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


void mud_tray_destroyed_cb(GtkWidget *widget, void *data)
{
	MudTray *tray = MUD_TRAY(data);
	
        g_object_unref(G_OBJECT(tray->priv->tray_icon));
        tray->priv->tray_icon = NULL;

        // for some reason, our tray icon was destroyed, re-create it!
        g_idle_add(mud_tray_create_cb, &data);
}

void mud_tray_clicked_cb(GtkWidget *button, GdkEventButton *event, void *data)
{
	MudTray *tray = MUD_TRAY(data);
	
        if (event->type != GDK_BUTTON_PRESS)
                return;

        switch (event->button) {
                case 1:
                        // left-mouse button
                        mud_tray_window_toggle(NULL, tray);
                        break;
                case 2:
                        // middle-mouse button
                        // suggestions anyone?
                        break;
                case 3:
                        mud_tray_menu(tray);
                        break;
        }
}



// functions
void mud_tray_menu(MudTray *tray)
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
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
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
                        icon_name = GMPIXMAPSDIR "/gnome-mud-tray-icon.png";
                        break;
                case online:
                        icon_name = GMPIXMAPSDIR "/connection-online.png";
                        break;
        }

        gtk_image_set_from_file(GTK_IMAGE(tray->priv->image), icon_name);
}

void 
mud_tray_create(MudTray *tray)
{
	GtkWidget *box;

        if (tray->priv->tray_icon) {
                /* if this is being called when a tray icon exists, it's because
                   something messed up. try destroying it before we proceed */
                g_message("Trying to create icon but it already exists?\n");
                mud_tray_destroy(tray);
        }
        
	tray->priv->tray_icon = egg_tray_icon_new(_("GNOME Mud"));
        box = gtk_event_box_new();
        tray->priv->image = gtk_image_new();

        g_signal_connect(G_OBJECT(tray->priv->tray_icon), "destroy", G_CALLBACK(mud_tray_destroyed_cb), tray);
        g_signal_connect(G_OBJECT(box), "button-press-event", G_CALLBACK(mud_tray_clicked_cb), tray);

        gtk_container_add(GTK_CONTAINER(box), tray->priv->image);
        gtk_container_add(GTK_CONTAINER(tray->priv->tray_icon), box);

        mud_tray_update_icon(tray, offline_connecting);

        gtk_widget_show_all(GTK_WIDGET(tray->priv->tray_icon));

        /* ref the tray icon before we bandy it about the place */
        g_object_ref(G_OBJECT(tray->priv->tray_icon));
}

void
mud_tray_destroy(MudTray *tray)
{
        g_signal_handlers_disconnect_by_func(G_OBJECT(tray->priv->tray_icon), G_CALLBACK(mud_tray_destroyed_cb), tray);
        gtk_widget_destroy(GTK_WIDGET(tray->priv->tray_icon));

        g_object_unref(G_OBJECT(tray->priv->tray_icon));
        tray->priv->tray_icon = NULL;
        
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
