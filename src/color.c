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
#include <glib.h>
#include <stdlib.h>
#include <time.h>

#include "amcl.h"

static char const rcsid[] =
    "$Id$";


#define C_MAX 16

GtkWidget *color_window, *color_widget;
GtkWidget *c_radio[C_MAX];
GtkBox    *c_box, *c_box2, *c_box3, *c_hbox, *c_hbox2;

GtkButton *c_ok, *c_apply, *c_cancel;
GtkAlignment *c_a1, *c_a2;

GdkColor  *c_structs[C_MAX] = {
    &color_black,
    &color_red,
    &color_green,
    &color_brown,
    &color_blue,
    &color_magenta,
    &color_cyan,
    &color_lightgrey,
    &color_grey,
    &color_lightred,
    &color_lightgreen,
    &color_yellow,
    &color_lightblue,
    &color_lightmagenta,
    &color_lightcyan,
    &color_white
};

const char *c_captions[C_MAX] = {
    "Black",
    "Dark Red",
    "Dark Green",
    "Brown",
    "Dark Blue",
    "Dark Magenta",
    "Dark Cyan",
    "Grey",
    "Dark Grey",
    "Red",
    "Green",
    "Yellow",
    "Blue",
    "Magenta",
    "Cyan",
    "White"
};

double *current_color = 0;

double colors[C_MAX][3] = {
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0 }
};

void copy_color_from_selector_to_array ()
{
    gtk_color_selection_get_color (GTK_COLOR_SELECTION (color_widget),
                                   current_color);
}

void copy_color_from_array_to_selector ()
{
    gtk_color_selection_set_color (GTK_COLOR_SELECTION (color_widget),
                                   current_color);
}

extern GdkColormap *cmap;

void update_gdk_colors()
{
    int i;

    for ( i = 0; i < 16; i++ )
    {
        c_structs[i]->red   = colors[i][0] * 65535;
        c_structs[i]->green = colors[i][1] * 65535;
        c_structs[i]->blue  = colors[i][2] * 65535;

        gdk_color_alloc (cmap, c_structs[i]);
    }
}

void color_ok_pressed ()
{
    gtk_widget_hide (GTK_WIDGET (color_window));
    gtk_widget_set_sensitive (menu_option_colors, TRUE);
    copy_color_from_selector_to_array ();
    update_gdk_colors();
    
}

void color_cancel_pressed ()
{
    gtk_widget_hide (GTK_WIDGET (color_window));
    gtk_widget_set_sensitive (menu_option_colors, TRUE);
}

void color_apply_pressed ()
{
    copy_color_from_selector_to_array ();
    update_gdk_colors ();
}

void color_radio_clicked (GtkWidget *what)
{
    int i;

    copy_color_from_selector_to_array ();

    for ( i = 0; i < 16; i++ )
    {
        if ( what == c_radio[i] )
        {
            current_color = colors[i];
            copy_color_from_array_to_selector ();
            return;
        }
    }
}

void create_color_box ()
{
    int i;
    current_color = colors[0];

    for ( i = 0; i < 16; i++ )
    {
        colors[i][0] = ((double) c_structs[i]->red)   / 65535;
        colors[i][1] = ((double) c_structs[i]->green) / 65535;
        colors[i][2] = ((double) c_structs[i]->blue)  / 65535;
    }

    color_window = GTK_WIDGET (gtk_window_new (GTK_WINDOW_TOPLEVEL));
    gtk_window_set_title (GTK_WINDOW (color_window), "Amcl Color Chooser");
    gtk_signal_connect_object (GTK_OBJECT (color_window), "destroy",
                               GTK_SIGNAL_FUNC (color_cancel_pressed),
                               NULL);
    
    color_widget = gtk_color_selection_new ();
    c_box        = GTK_BOX (gtk_vbox_new (FALSE, 5));
    c_box2       = GTK_BOX (gtk_vbox_new (FALSE, 5));
    c_box3       = GTK_BOX (gtk_vbox_new (FALSE, 5));
    c_hbox       = GTK_BOX (gtk_hbox_new (FALSE, 5));
    c_hbox2      = GTK_BOX (gtk_hbox_new (FALSE, 5));
    c_a1         = GTK_ALIGNMENT (gtk_alignment_new (0.5, 0.5, 0.0, 0.0));
    c_a2         = GTK_ALIGNMENT (gtk_alignment_new (0.5, 0.5, 0.0, 0.0));

    c_radio[0] = GTK_WIDGET (gtk_radio_button_new_with_label(NULL,
                                                             c_captions[0]));

    for ( i = 1; i < C_MAX; i++ )
        c_radio[i] = GTK_WIDGET (gtk_radio_button_new_with_label_from_widget (
                                 GTK_RADIO_BUTTON (c_radio[0]), c_captions[i]));

    gtk_container_border_width (GTK_CONTAINER (color_widget), 5);

    gtk_container_add (GTK_CONTAINER (color_window), GTK_WIDGET (c_hbox));
    gtk_container_add (GTK_CONTAINER (c_hbox), GTK_WIDGET (c_box));

    gtk_container_border_width (GTK_CONTAINER (c_box2), 5);
    gtk_container_border_width (GTK_CONTAINER (c_box3), 5);

    gtk_container_add (GTK_CONTAINER (c_hbox), GTK_WIDGET (c_box2));
    gtk_container_add (GTK_CONTAINER (c_hbox), GTK_WIDGET (c_box3));
    gtk_container_add (GTK_CONTAINER (c_box),  GTK_WIDGET (c_a1));
    gtk_container_add (GTK_CONTAINER (c_a1),   GTK_WIDGET (color_widget));
    gtk_container_add (GTK_CONTAINER (c_box),  GTK_WIDGET (c_a2));
    gtk_container_add (GTK_CONTAINER (c_a2),   GTK_WIDGET (c_hbox2));

    gtk_widget_show (GTK_WIDGET (c_a1));
    gtk_widget_show (GTK_WIDGET (c_a2));

    c_ok = GTK_BUTTON (gtk_button_new_with_label ("Ok"));
    c_cancel = GTK_BUTTON (gtk_button_new_with_label ("Cancel"));
    c_apply = GTK_BUTTON (gtk_button_new_with_label ("Apply"));

    gtk_signal_connect (GTK_OBJECT (c_ok), "clicked",
                        GTK_SIGNAL_FUNC (color_ok_pressed), 0);
    gtk_signal_connect (GTK_OBJECT (c_cancel), "clicked",
                        GTK_SIGNAL_FUNC (color_cancel_pressed), 0);
    gtk_signal_connect (GTK_OBJECT (c_apply), "clicked",
                        GTK_SIGNAL_FUNC (color_apply_pressed), 0);

    gtk_container_add (GTK_CONTAINER (c_hbox2), GTK_WIDGET (c_ok));
    gtk_container_add (GTK_CONTAINER (c_hbox2), GTK_WIDGET (c_cancel));
    gtk_container_add (GTK_CONTAINER (c_hbox2), GTK_WIDGET (c_apply));

    gtk_widget_show (GTK_WIDGET (c_ok));
    gtk_widget_show (GTK_WIDGET (c_cancel));
    gtk_widget_show (GTK_WIDGET (c_apply));

    for ( i = 0; i < 8; i++ )
        gtk_container_add (GTK_CONTAINER (c_box2), c_radio[i]);

    for ( i = 8; i < 16; i++ )
        gtk_container_add (GTK_CONTAINER (c_box3), c_radio[i]);

    for ( i = 0; i < 16; i++ )
    {
        gtk_signal_connect (GTK_OBJECT (c_radio[i]), "clicked",
                            GTK_SIGNAL_FUNC (color_radio_clicked), 0);
        gtk_widget_show (c_radio[i]);
    }

    gtk_widget_show (GTK_WIDGET (color_widget));
    gtk_widget_show (GTK_WIDGET (c_hbox2));
    gtk_widget_show (GTK_WIDGET (c_box));
    gtk_widget_show (GTK_WIDGET (c_box2));
    gtk_widget_show (GTK_WIDGET (c_box3));
    gtk_widget_show (GTK_WIDGET (c_hbox));

    copy_color_from_array_to_selector ();

    return;
}

void window_color (GtkWidget *a, gpointer d)
{
    gtk_widget_show (GTK_WIDGET (color_window));
    gtk_widget_set_sensitive (menu_option_colors, FALSE);
}

