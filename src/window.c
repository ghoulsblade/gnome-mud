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
extern SYSTEM_DATA		prefs;
extern CONNECTION_DATA 	*connections[15];
extern GtkWidget       	*text_entry;
extern GtkWidget		*main_notebook;

void popup_window (const gchar *message)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
								GTK_MESSAGE_INFO,
								GTK_BUTTONS_OK,
								message);
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);

	gtk_widget_show(dialog);
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
		struct timeval tv;
		
		fputs(message, connections[i]->log);

		gettimeofday(&tv, NULL);
		if ((connections[i]->last_log_flush + (unsigned) prefs.FlushInterval) < tv.tv_sec)
		{
			fflush(connections[i]->log);
			connections[i]->last_log_flush = tv.tv_sec;
		}
	}

	switch (colortype)
    {
	    case MESSAGE_SENT:
			terminal_feed(text_widget, "\e[1;33m");
			break;
		
	    case MESSAGE_ERR:
			terminal_feed(text_widget, "\e[1;31m");
			break;
		
		case MESSAGE_SYSTEM:
			terminal_feed(text_widget, "\e[1;32m");
			break;
		
		default:
			break;
    }

 	terminal_feed(text_widget, message);
	terminal_feed(text_widget, "\e[0m");
}

static void str_replace (char *buf, const char *s, const char *repl)
{
	char out_buf[4608];
	char *pc, *out;
	int  len = strlen (s);
	bool found = FALSE;

	for ( pc = buf, out = out_buf; *pc && (out-out_buf) < (4608-len-4);)
		if ( !strncasecmp(pc, s, len))
		{
			out += sprintf (out, repl);
			pc += len;
			found = TRUE;
		}
		else
			*out++ = *pc++;

	if ( found)
	{
		*out = '\0';
		strcpy (buf, out_buf);
	}
}

void terminal_feed(GtkWidget *widget, gchar *data)
{
	gint rlen = strlen(data);
	gchar buf[rlen*2];
	gchar *local;

	g_stpcpy(buf, data);
	str_replace(buf, "\r", "");
	str_replace(buf, "\n", "\n\r");

	local = g_locale_from_utf8(buf, -1, NULL, NULL, NULL);
	if (!local)
	{
		g_warning(_("Couldn't convert text input"));
		return;
	}
	
	vte_terminal_feed(VTE_TERMINAL(widget), local, strlen(local));
	g_free(local);
}

