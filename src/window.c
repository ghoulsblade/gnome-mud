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
 */

#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gtk/gtktree.h>
#include <libintl.h>
#include <vte/vte.h>

#ifdef HAVE_TELNET_H
#include <telnet.h>
#endif
#ifdef HAVE_ARPA_TELNET_H
#include <arpa/telnet.h>
#endif

#include "gnome-mud.h"

#define _(string) gettext(string)

static char const rcsid[] =
	"$Id$";

/* Global Variables */
extern CONNECTION_DATA 	*connections[15];
extern GtkWidget       	*text_entry;
extern GtkWidget		*main_notebook;

void popup_window (const gchar *message)
{
    GtkWidget *window;
    GtkWidget *label;
    GtkWidget *button;
    GtkWidget *box;
    GtkWidget *separator;

    gchar       buf[2048];

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), _("GNOME-Mud Popup Message"));

    box = gtk_vbox_new (FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (box), 5);
    gtk_container_add (GTK_CONTAINER (window), box);

    g_snprintf ( buf, 2048, " %s ", message);
    label = gtk_label_new (buf);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 5);
    gtk_widget_show (label);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box), separator, TRUE, TRUE, 0);
    gtk_widget_show (separator);
    
    button = gtk_button_new_with_label (_("Ok"));
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
                        GTK_SIGNAL_FUNC (close_window), window);
    gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE, 5);
    gtk_widget_show (button);
    
    gtk_widget_show (box   );
    gtk_widget_show (window);
}

void grab_focus_cb (GtkWidget* widget, gpointer user_data)
{
    if (text_entry != NULL) gtk_widget_grab_focus(text_entry);
}

void switch_page_cb (GtkNotebook *widget, gpointer data, guint nb_int, gpointer data2)
{
	grab_focus_cb(NULL, NULL);

	/* fix the focus-problem */
	if (text_entry != NULL) gtk_widget_grab_focus(text_entry);
}

void textfield_add (CONNECTION_DATA *cd, gchar *message, gint colortype)
{
	GtkWidget *text_widget = cd->window;
	gint       i;

    if ( message[0] == '\0' )
    {
        return;
    }

	i = gtk_notebook_get_current_page (GTK_NOTEBOOK(main_notebook));
	if (connections[i]->logging
		&& colortype != MESSAGE_ERR
		&& colortype != MESSAGE_SYSTEM)
	{
		fputs(message, connections[i]->log);
	}

	switch (colortype)
    {
	    case MESSAGE_SENT:
			// FIXME correct graphic-codes here
			//gtk_text_insert (GTK_TEXT (text_widget), font_normal, &prefs.Colors[11], &prefs.Background, message, strlen (message));
			//break;
		
	    case MESSAGE_ERR:
			// FIXME correct graphic-codes here
    	    //gtk_text_insert (GTK_TEXT (text_widget), font_normal, &prefs.Colors[2], &prefs.Background, message, strlen (message));
			//break;
		
		case MESSAGE_SYSTEM:
			// FIXME correct graphic-codes here
			//
			//break;
		
		default:
			break;
    }

 	vte_terminal_feed(VTE_TERMINAL(text_widget), message, strlen(message));
}

