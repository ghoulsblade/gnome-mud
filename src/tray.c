/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1998-2002 Robin Ericsson <lobbin@localhost.nu>
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

#include <gnome.h>

#include "gnome-mud.h"
#include "eggtrayicon.h"


extern GtkWidget *window;
EggTrayIcon * tray_icon = NULL;
GtkWidget *image = NULL;
gboolean window_invisible;


// callbacks
void window_toggle()
{
        if (window_invisible == FALSE)
                gtk_widget_hide_all(window);
        else
                gtk_widget_show_all(window);
        window_invisible = !window_invisible;
}

void window_exit()
{
        destroy(window);
}

gboolean tray_create_cb()
{
        tray_create();

        return FALSE; /* for when we're called by the glib idle handler */
}


void tray_destroyed_cb(GtkWidget *widget, void *data)
{
        g_object_unref(G_OBJECT(tray_icon));
        tray_icon = NULL;

        // for some reason, our tray icon was destroyed, re-create it!
        g_idle_add(tray_create_cb, &data);
}

void tray_clicked_cb(GtkWidget *button, GdkEventButton *event, void *data)
{
        if (event->type != GDK_BUTTON_PRESS)
                return;

        switch (event->button) {
                case 1:
                        // left-mouse button
                        window_toggle();
                        break;
                case 2:
                        // middle-mouse button
                        // suggestions anyone?
                        break;
                case 3:
                        tray_menu();
                        break;
        }
}



// functions
void tray_menu()
{
        static GtkWidget *menu = NULL;
        GtkWidget *entry;

        if (menu) {
                gtk_widget_destroy(menu);
        }

        menu = gtk_menu_new();

        if (window_invisible == FALSE)
                entry = gtk_menu_item_new_with_mnemonic(_("_Hide window"));
        else
                entry = gtk_menu_item_new_with_mnemonic(_("_Show window"));
        g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(window_toggle), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), entry);
	entry = gtk_separator_menu_item_new ();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), entry);
        entry = gtk_menu_item_new_with_mnemonic(_("_Quit"));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), entry);
        g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(window_exit), NULL);

        gtk_widget_show_all(menu);
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}

void tray_update_icon(enum tray_status icon)
{
        const gchar *icon_name = NULL;

        switch (icon) {
                case offline:
                        icon_name = PIXMAPSDIR "/gnome-gmush.png";
                        break;
                case offline_connecting:
                case online_connecting:
                        icon_name = PIXMAPSDIR "/gnome-gmush.png";
                        break;
                case online:
                        icon_name = PIXMAPSDIR "/gnome-gmush.png";
                        break;
        }

        gtk_image_set_from_file(GTK_IMAGE(image), icon_name);
}


void tray_destroy()
{
        g_signal_handlers_disconnect_by_func(G_OBJECT(tray_icon), G_CALLBACK(tray_destroyed_cb), NULL);
        gtk_widget_destroy(GTK_WIDGET(tray_icon));

        g_object_unref(G_OBJECT(tray_icon));
        tray_icon = NULL;
}


void tray_create()
{
        GtkWidget *box;

        if (tray_icon) {
                /* if this is being called when a tray icon exists, it's because
                   something messed up. try destroying it before we proceed */
                g_message("Trying to create icon but it already exists?\n");
                tray_destroy();
        }

        tray_icon = egg_tray_icon_new(_("GNOME Mud"));
        box = gtk_event_box_new();
        image = gtk_image_new();

        g_signal_connect(G_OBJECT(tray_icon), "destroy", G_CALLBACK(tray_destroyed_cb), NULL);
        g_signal_connect(G_OBJECT(box), "button-press-event", G_CALLBACK(tray_clicked_cb), NULL);

        gtk_container_add(GTK_CONTAINER(box), image);
        gtk_container_add(GTK_CONTAINER(tray_icon), box);

        tray_update_icon(offline);

        gtk_widget_show_all(GTK_WIDGET(tray_icon));

        /* ref the tray icon before we bandy it about the place */
        g_object_ref(G_OBJECT(tray_icon));
}
