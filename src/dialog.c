/* AMCL - A simple Mud CLient
 * Copyright (C) 1998-1999 Robin Ericsson <lobbin@localhost.nu>
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
#include <gtk/gtk.h>
#include <string.h>

#include "amcl.h"
#include "readme_doc.h"
#include "authors_doc.h"

static char const rcsid[] =
    "$Id$";

extern GdkFont  *font_fixed;

void doc_dialog (GtkWidget *widget, gint index)
{
    GtkWidget *doc_window;
    GtkWidget *doc_text;
    GtkWidget *doc_vbox;
    GtkWidget *doc_hbox;
    GtkWidget *doc_separator;
    GtkWidget *doc_button;
    char doc_title[80];

    strcpy (doc_title, " AMCL - ");

    doc_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_usize (GTK_WIDGET (doc_window), 600, 500);
    gtk_window_position (GTK_WINDOW (doc_window), GTK_WIN_POS_MOUSE);

    doc_vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_add (GTK_CONTAINER (doc_window), doc_vbox);
    gtk_container_border_width (GTK_CONTAINER (doc_vbox), 5);
    gtk_widget_show (doc_vbox);

    doc_text = gtk_text_new (NULL, NULL);
    
    {
        GtkWidget *table;
        GtkWidget *hscrollbar;
        GtkWidget *vscrollbar;
        static GdkColor text_bg =
        {0, 0xffec, 0xffec, 0xffec};
        GtkStyle *style;

        table = gtk_table_new (2, 2, FALSE);

        gtk_table_attach (GTK_TABLE (table), doc_text, 0, 1, 0, 1,
                          GTK_FILL | GTK_EXPAND,
                          GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 0);
        gtk_widget_show (doc_text);

        style = gtk_style_new ();
        style->base[GTK_STATE_NORMAL] = text_bg;
        gtk_widget_set_style (GTK_WIDGET (doc_text), style);

        hscrollbar = gtk_hscrollbar_new (GTK_TEXT (doc_text)->hadj);
        gtk_table_attach (GTK_TABLE (table), hscrollbar, 0, 1, 1, 2,
                          GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
        gtk_widget_show (hscrollbar);

        vscrollbar = gtk_vscrollbar_new (GTK_TEXT (doc_text)->vadj);
        gtk_table_attach (GTK_TABLE (table), vscrollbar, 1, 2, 0, 1,
                          GTK_FILL, GTK_EXPAND | GTK_FILL | GTK_SHRINK, 0, 0);
        gtk_widget_show (vscrollbar);

        gtk_box_pack_start (GTK_BOX (doc_vbox), table, TRUE, TRUE, 0);
        gtk_widget_show (table);
    }

    gtk_text_freeze (GTK_TEXT (doc_text));
    switch ((int)index)
    {
    case 1:
        gtk_text_insert (GTK_TEXT (doc_text), font_fixed, NULL, NULL, README_STRING, -1);
        strcat (doc_title, "README");
        break;

    case 2:
        gtk_text_insert (GTK_TEXT (doc_text), font_fixed, NULL, NULL, AUTHORS_STRING, -1);
        strcat (doc_title, "AUTHORS");
        break;

    }
    gtk_text_thaw (GTK_TEXT (doc_text));

    doc_separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (doc_vbox), doc_separator, FALSE, TRUE, 0);
    gtk_widget_show (doc_separator);

    doc_hbox = gtk_hbox_new (FALSE, 10);
    gtk_box_pack_start (GTK_BOX (doc_vbox), doc_hbox, FALSE, TRUE, 0);
    gtk_container_border_width (GTK_CONTAINER (doc_hbox), 5);
    gtk_widget_show (doc_hbox);

    doc_button = gtk_button_new_with_label (" Close ");
    gtk_signal_connect_object (GTK_OBJECT (doc_button), "clicked", gtk_widget_destroy,
                               GTK_OBJECT (doc_window));
    gtk_box_pack_start (GTK_BOX (doc_hbox), doc_button, TRUE, TRUE, 0);
    gtk_widget_show (doc_button);

    gtk_window_set_title (GTK_WINDOW (doc_window), doc_title);
    gtk_widget_show (doc_window);
}
