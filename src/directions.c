/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1999-2001 Robin Ericsson <lobbin@localhost.nu>
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
#include <gtk/gtk.h>
#include <glade/glade-xml.h>

#include <gdk/gdkkeysyms.h>

#include "gnome-mud.h"
#include "directions.h"

extern SYSTEM_DATA  prefs;
extern GConfClient	*gconf_client;

struct DIR_WIDGETS {
	GtkWidget *window;

	GtkWidget *ok_button;
	GtkWidget *cancel_button;

	GtkWidget *nentry;
	GtkWidget *neentry;
	GtkWidget *eentry;
	GtkWidget *seentry;
	GtkWidget *sentry;
	GtkWidget *swentry;
	GtkWidget *wentry;
	GtkWidget *nwentry;
	GtkWidget *upentry;
	GtkWidget *downentry;
	GtkWidget *lookentry;

	PROFILE_DATA *pd;
};


/* directions.c */

static void directions_gconf_sync(PROFILE_DATA *pd)
{
	gchar *pname = gconf_escape_key(pd->name, -1);
	gchar *key = g_strdup_printf("/apps/gnome-mud/profiles/%s/directions", pname);

	gconf_client_set_list(gconf_client, key, GCONF_VALUE_STRING, pd->directions, NULL);

	g_free(key);
	g_free(pname);
}

#define SAVE_ENTRY(a)	s = g_strdup(gtk_entry_get_text(GTK_ENTRY(dw->a))); \
						pd->directions = g_slist_append(pd->directions, s);

void directions_window_ok_cb(GtkButton *button, gpointer user_data) {
	struct DIR_WIDGETS *dw = (struct DIR_WIDGETS *) user_data;
	PROFILE_DATA *pd = dw->pd;
	gchar *s;
	
	/* Free old list */
	g_slist_free(pd->directions);
	pd->directions = NULL;

	/* Store entries */
	SAVE_ENTRY(nentry);
	SAVE_ENTRY(nwentry);
	SAVE_ENTRY(eentry);
	SAVE_ENTRY(seentry);
	SAVE_ENTRY(sentry);
	SAVE_ENTRY(swentry);
	SAVE_ENTRY(wentry);
	SAVE_ENTRY(nwentry);
	SAVE_ENTRY(upentry);
	SAVE_ENTRY(downentry);
	SAVE_ENTRY(lookentry);

	directions_gconf_sync(pd);

	gtk_widget_destroy(dw->window);
}

#undef SAVE_ENTRY

void directions_window_cancel_cb(GtkButton *button, gpointer user_data) {
	struct DIR_WIDGETS *dw = (struct DIR_WIDGETS*) user_data;
	
	gtk_widget_destroy(dw->window);
	dw->window = NULL;
}

#define LOAD_ENTRY(a, b)	d_widgets.a = glade_xml_get_widget(window, b);
#define SET_ENTRY(a, b)		gtk_entry_set_text(GTK_ENTRY(d_widgets.a), \
								g_slist_nth_data(pd->directions, b));

void window_directions(PROFILE_DATA *pd) 
{
	GladeXML *window;
	gchar *path;
	static struct DIR_WIDGETS d_widgets;

	d_widgets.pd = pd;

	if (d_widgets.window == NULL) {
		path = g_strconcat(PKGDATADIR, "/directions.glade", NULL);
		window = glade_xml_new(path, NULL, NULL);
		g_free(path);

		/* Window */
		d_widgets.window = glade_xml_get_widget(window, "directionswindow");
		
		/* Buttons */
		d_widgets.ok_button = glade_xml_get_widget(window, "ok_button");
		d_widgets.cancel_button = glade_xml_get_widget(window, "cancel_button");

		/* Connect signals */
		g_signal_connect(G_OBJECT(d_widgets.ok_button), "clicked",
						 G_CALLBACK(directions_window_ok_cb), &d_widgets);
		g_signal_connect(G_OBJECT(d_widgets.cancel_button), "clicked",
						 G_CALLBACK(directions_window_cancel_cb), &d_widgets);
		g_signal_connect(G_OBJECT(d_widgets.window), "destroy",
						 G_CALLBACK(directions_window_cancel_cb), &d_widgets);

		/* Get all entries */
		LOAD_ENTRY(nentry,    "nentry");
		LOAD_ENTRY(neentry,   "neentry");
		LOAD_ENTRY(eentry,    "eentry");
		LOAD_ENTRY(seentry,   "seentry");
		LOAD_ENTRY(sentry,    "sentry");
		LOAD_ENTRY(swentry,   "swentry");
		LOAD_ENTRY(wentry,    "wentry");
		LOAD_ENTRY(nwentry,   "nwentry");
		LOAD_ENTRY(upentry,   "upentry");
		LOAD_ENTRY(downentry, "downentry");
		LOAD_ENTRY(lookentry, "lookentry");
	} else {
		gtk_window_present(GTK_WINDOW(d_widgets.window));
	}

	/* Set data */
	SET_ENTRY(nentry,    DIRECTION_NORTH);
	SET_ENTRY(neentry,   DIRECTION_NORTHEAST);
	SET_ENTRY(eentry,    DIRECTION_EAST);
	SET_ENTRY(seentry,   DIRECTION_SOUTHEAST);
	SET_ENTRY(sentry,    DIRECTION_SOUTH);
	SET_ENTRY(swentry,   DIRECTION_SOUTHWEST);
	SET_ENTRY(wentry,    DIRECTION_WEST);
	SET_ENTRY(nwentry,   DIRECTION_NORTHWEST);
	SET_ENTRY(upentry,   DIRECTION_UP);
	SET_ENTRY(downentry, DIRECTION_DOWN);
	SET_ENTRY(lookentry, DIRECTION_LOOK);
}

#undef SET_ENTRY
#undef LOAD_ENTRY
