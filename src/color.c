/* AMCL - A simple Mud CLient
 * Copyright (C) 1998-2000 Robin Ericsson <lobbin@localhost.nu>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <string.h>

#include "amcl.h"

static char const rcsid[] =
    "$Id$";

/* Local functions */
static void	color_apply_pressed (void);
static void	color_cancel_pressed (void);
static void	color_ok_pressed (void);
static void	color_radio_clicked (GtkWidget *);
static gushort	convert_color (guint);
static void	copy_color_from_selector_to_array (void);
static void	copy_color_from_array_to_selector (void);
static int	from_hex (const char *);
static void	grab_color (GdkColor *, const char *);
static void	load_colors_default (void);
static void	on_load_pressed (void);
static void	update_gdk_colors (void);

/* External functions */
void save_prefs	(GtkWidget *button, gpointer data);

#define C_MAX 16

GdkColormap *cmap;

GtkWidget *color_window, *color_widget;
GtkWidget *c_radio[C_MAX];

GdkColor  *c_structs[C_MAX] = {
    &color_black,
    &color_red,
    &color_green,
    &color_brown,
    &color_blue,
    &color_magenta,
    &color_cyan,
    &color_grey,
    &color_white,
    &color_lightred,
    &color_lightgreen,
    &color_yellow,
    &color_lightblue,
    &color_lightmagenta,
    &color_lightcyan,
    &color_lightgrey
};

const char *c_captions[C_MAX] = {
    "Black",
    "Dark Red",
    "Dark Green",
    "Brown",
    "Dark Blue",
    "Dark Magenta",
    "Dark Cyan",
    "White",
    "Dark Grey",
    "Red",
    "Green",
    "Yellow",
    "Blue",
    "Magenta",
    "Cyan",
    "Grey"
};

const char *default_color[C_MAX] = {
    "#000000",
    "#800000",
    "#008000",
    "#808000",
    "#000080",
    "#800080",
    "#008080",
    "#ffffff",
    "#404040",
    "#ff0000",
    "#00ff00",
    "#ffff00",
    "#0000ff",
    "#ff00ff",
    "#00ffff",
    "#808080"
};

char *text_color[C_MAX];

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

static gushort convert_color (guint c)
{
    if ( c == 0 )
        return 0;
    c *= 257;

    return (c > 0xffff) ? 0xffff : c;
}

static int from_hex (const char *what)
{
   return (what[0] - ((what[0] > '9')?('a'+10):'0'))*16 +
          (what[1] - ((what[1] > '9')?('a'+10):'0'));
}

static void grab_color (GdkColor *color, const char *col)
{
    color->red   = convert_color (from_hex (col+1));
    color->green = convert_color (from_hex (col+3));
    color->blue  = convert_color (from_hex (col+5));
}

void load_color_to_c_structs (void)
{
    gint i;
    cmap = gdk_colormap_get_system();
    for (i = 0; i < C_MAX ; i++) {
      grab_color ( c_structs[i], text_color[i]);
      if (!gdk_color_alloc (cmap, c_structs[i]))
	g_warning("Couldn't allocate color");
    }
}

static void load_colors_default (void)
{
    gint i;
    for (i = 0; i < C_MAX; i++)
        text_color[i] = (char *) strdup(default_color[i]);
    load_color_to_c_structs();

}

static void copy_color_from_selector_to_array (void)
{
    gtk_color_selection_get_color (GTK_COLOR_SELECTION (color_widget),
                                   current_color);
}

static void copy_color_from_array_to_selector (void)
{
    gtk_color_selection_set_color (GTK_COLOR_SELECTION (color_widget),
                                   current_color);
}

static void on_load_pressed (void)
{
    gint i;
    load_colors_default();
    current_color = colors[0];

    for ( i = 0; i < C_MAX; i++ )
    {
        colors[i][0] = ((double) c_structs[i]->red)   / 65535;
        colors[i][1] = ((double) c_structs[i]->green) / 65535;
        colors[i][2] = ((double) c_structs[i]->blue)  / 65535;
    }
    
    copy_color_from_array_to_selector();
}

void update_gdk_colors (void)
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

void color_apply_pressed (void)
{
    copy_color_from_selector_to_array ();
    update_gdk_colors ();
}
 
void color_cancel_pressed (void)
{
    gtk_widget_set_sensitive (menu_option_colors, TRUE);
    gtk_widget_destroy (color_window);
}
 
void color_ok_pressed (void)
{
    color_apply_pressed();
    if ( prefs.AutoSave ) 
      save_colors();
    color_cancel_pressed();
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

void window_color (GtkWidget *a, gpointer d)
{
  GtkBox    *c_box, *c_box2, *c_box3, *c_hbox, *c_hbox2;
  GtkButton *c_ok, *c_apply, *c_cancel, *c_save, *c_load;
  GtkAlignment *c_a1, *c_a2;
  int i;

  current_color = colors[0];
  
  for ( i = 0; i < 16; i++ ) {
    colors[i][0] = ((double) c_structs[i]->red)   / 65535;
    colors[i][1] = ((double) c_structs[i]->green) / 65535;
    colors[i][2] = ((double) c_structs[i]->blue)  / 65535;
  }
  
  gtk_widget_set_sensitive (menu_option_colors, FALSE);
  color_window = GTK_WIDGET (gtk_window_new (GTK_WINDOW_TOPLEVEL));
  gtk_window_set_title (GTK_WINDOW (color_window), "Amcl Color Chooser");
  gtk_signal_connect (GTK_OBJECT (color_window), "destroy",
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
  
  c_radio[0] = GTK_WIDGET (gtk_radio_button_new_with_label(NULL,c_captions[0]));
  
  for ( i = 1; i < C_MAX; i++ )
    c_radio[i] = GTK_WIDGET (gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (c_radio[0]), c_captions[i]));
  
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
  
  c_ok     = GTK_BUTTON (gtk_button_new_with_label (" Ok "));
  c_cancel = GTK_BUTTON (gtk_button_new_with_label (" Cancel "));
  c_apply  = GTK_BUTTON (gtk_button_new_with_label (" Apply "));
  c_save   = GTK_BUTTON (gtk_button_new_with_label (" Save "));
  c_load   = GTK_BUTTON (gtk_button_new_with_label (" Default "));    
  
  gtk_signal_connect (GTK_OBJECT (c_ok), "clicked",
		      GTK_SIGNAL_FUNC (color_ok_pressed), 0);
  gtk_signal_connect (GTK_OBJECT (c_cancel), "clicked",
		      GTK_SIGNAL_FUNC (color_cancel_pressed), 0);
  gtk_signal_connect (GTK_OBJECT (c_apply), "clicked",
		      GTK_SIGNAL_FUNC (color_apply_pressed), 0);
  gtk_signal_connect (GTK_OBJECT (c_save), "clicked",
		      GTK_SIGNAL_FUNC (save_colors), 0);
  gtk_signal_connect (GTK_OBJECT (c_load), "clicked",
		      GTK_SIGNAL_FUNC (on_load_pressed), 0);
  
  gtk_container_add (GTK_CONTAINER (c_hbox2), GTK_WIDGET (c_ok));
  gtk_container_add (GTK_CONTAINER (c_hbox2), GTK_WIDGET (c_cancel));
  gtk_container_add (GTK_CONTAINER (c_hbox2), GTK_WIDGET (c_apply));
  gtk_container_add (GTK_CONTAINER (c_hbox2), GTK_WIDGET (c_save));
  gtk_container_add (GTK_CONTAINER (c_hbox2), GTK_WIDGET (c_load));    
  
  gtk_widget_show (GTK_WIDGET (c_ok));
  gtk_widget_show (GTK_WIDGET (c_cancel));
  gtk_widget_show (GTK_WIDGET (c_apply));
  gtk_widget_show (GTK_WIDGET (c_save));
  gtk_widget_show (GTK_WIDGET (c_load));    
  
  for ( i = 0; i < 8; i++ )
    gtk_container_add (GTK_CONTAINER (c_box2), c_radio[i]);
  
  for ( i = 8; i < 16; i++ )
    gtk_container_add (GTK_CONTAINER (c_box3), c_radio[i]);
  
  for ( i = 0; i < 16; i++ ) {
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
  gtk_widget_show (GTK_WIDGET (color_window));
  
  copy_color_from_array_to_selector ();
  
  return;
}

void save_colors (void)
{
    FILE *fp;
    gint i;

    if (!(fp = open_file("colors", "w"))) return;

    for (i = 0 ; i < C_MAX; i++)
    {
	gint tmp;

		if ( strlen (c_captions[i]) > 7)
		    fprintf (fp, "%s\t#",c_captions[i]);
		else
		    fprintf (fp, "%s\t\t#", c_captions[i]);

                tmp = c_structs[i]->red / 256;
                if (tmp < 16)
                  fprintf(fp, "0%x",tmp);
                 else
                  fprintf(fp, "%x",tmp);
                tmp = c_structs[i]->green / 256;
                if (tmp < 16)
                  fprintf(fp, "0%x",tmp);
                 else
                  fprintf(fp, "%x",tmp);
                tmp = c_structs[i]->blue / 256;
                if (tmp < 16)
                  fprintf(fp, "0%x\n",tmp);
                else
                  fprintf(fp, "%x\n",tmp);
     }
     if (fp) fclose(fp);
}

void load_colors ( void )
{
    FILE *fp;
    gchar line[80];
  
    load_colors_default();
    
    if (!(fp = open_file("colors", "r"))) return;

    while ( fgets(line, 80, fp) != NULL)
    {
	gchar *fontvalue;
	gint  i;

	for (i = 0; line[i] != 0 && line[i] != '#'; i++);
	fontvalue = line+i;
	for (;i>0 && line[i] < 'a';i--);
	line[i+1] = 0;
	for ( i = 0; i < C_MAX ; i++)
	    if ( strcmp ( line, c_captions[i]) == 0)
		{
			free (text_color[i]);
			text_color[i] = strdup (fontvalue);
			break;
		}
    }

    if (fp) fclose (fp);

    if ( ( font_normal = gdk_font_load (prefs.FontName) ) == NULL )
    {
        g_warning ("Can't load font... %s Using default.\n", prefs.FontName);
        g_free ( prefs.FontName );
        prefs.FontName = g_strdup ("fixed");
	save_prefs (NULL, NULL);
    }

    load_color_to_c_structs();
}
