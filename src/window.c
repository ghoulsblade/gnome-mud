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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <libintl.h>

#include "amcl.h"

#ifdef HAVE_TELNET_H
#include <telnet.h>
#endif
#ifdef HAVE_ARPA_TELNET_H
#include <arpa/telnet.h>
#endif

#define _(string) gettext(string)

static char const rcsid[] =
	"$Id$";


/* Local functions */
static void	cons_escm (void);
	
typedef enum { NORM, ESC, SQUARE, PARMS } STATE;



/* Global Variables */
extern GdkColor         color_white;
extern GdkColor         color_black;
extern GdkColor         color_blue; 
extern GdkColor         color_green; 
extern GdkColor         color_red;  
extern GdkColor         color_brown;
extern GdkColor         color_magenta;  
extern GdkColor         color_lightred;
extern GdkColor         color_yellow;  
extern GdkColor         color_lightgreen;
extern GdkColor         color_cyan;
extern GdkColor         color_lightcyan;
extern GdkColor         color_lightblue;
extern GdkColor         color_lightmagenta;
extern GdkColor         color_grey;
extern GdkColor         color_lightgrey;
extern GtkWidget       *menu_main_disconnect;
extern GtkWidget       *menu_main_close;
extern CONNECTION_DATA *connections[15];
extern GtkWidget       *text_entry;
extern GdkFont         *font_normal;
extern SYSTEM_DATA      prefs;

GdkColor  *foreground;
GdkColor  *background;
static gint parms[10], nparms;
bool      BOLD = FALSE;

static void cons_escm (void)
{
    int i, p;

    for ( i = 0; i < nparms; i++ )
    {
        switch ( p = parms[i])
        {
        case 0: /* none */
            foreground = &color_white;
            background = &color_black;
            BOLD       = FALSE;
            break;
        case 1:/* bold */
            BOLD = TRUE;
            break;
        case 4: /* underscore */
        case 5: /* blink */
        case 7: /* inverse */
            break;

        case 30:
            if ( BOLD )
                foreground = &color_grey;
            else
                foreground = &color_black;
            break;

        case 31:
            if ( BOLD )
                foreground = &color_lightred;
            else
                foreground = &color_red;
            break;

        case 32:
            if ( BOLD )
                foreground = &color_lightgreen;
            else
                foreground = &color_green;
            break;

        case 33:
            if ( BOLD )
                foreground = &color_yellow;
            else
                foreground = &color_brown;
            break;

        case 34:
            if ( BOLD )
                foreground = &color_lightblue;
            else
                foreground = &color_blue;
            break;

        case 35:
            if ( BOLD )
                foreground = &color_lightmagenta;
            else
                foreground = &color_magenta;
            break;

        case 36:
            if ( BOLD )
                foreground = &color_lightcyan;
            else
                foreground = &color_cyan;
            break;

        case 37:
            if ( BOLD )
                foreground = &color_white;
            else
                foreground = &color_lightgrey;
            break;

        case 40:
            background = &color_black;
            break;

        case 41:
            background = &color_red;
            break;

        case 42:
            background = &color_green;
            break;

        case 43:
            background = &color_yellow;
            break;

        case 44:
            background = &color_blue;
            break;

        case 45:
            background = &color_magenta;
            break;

        case 46:
            background = &color_cyan;
            break;

        case 47:
            background = &color_white;
            break;

        default:
            break;
        }
    }
}

void popup_window (const gchar *message)
{
    GtkWidget *window;
    GtkWidget *label;
    GtkWidget *button;
    GtkWidget *box;
    GtkWidget *separator;

    gchar       buf[2048];

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), _("AMCL Popup Message"));

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
  if (connections[nb_int] && menu_main_disconnect)
      gtk_widget_set_sensitive (menu_main_disconnect, connections[nb_int]->connected);
  gtk_widget_set_sensitive (menu_main_close, !(nb_int == 0 && menu_main_close));
  grab_focus_cb(NULL, NULL);
  /* fix the focus-problem */
  if (text_entry != NULL) gtk_widget_grab_focus(text_entry);
}

void textfield_add (GtkWidget *text_widget, gchar *message, gint colortype)
{
    gchar *start, c;
    gint  len;
    static STATE state = NORM;

    if ( message[0] == '\0' )
    {
        return;
    }

    if ( prefs.Freeze )
    {
        gtk_text_freeze (GTK_TEXT (text_widget));
    }
    
    switch (colortype)
    {
    case MESSAGE_SENT:
        gtk_text_insert (GTK_TEXT (text_widget), font_normal, &color_yellow,
                         NULL, message, strlen (message));
        break;
    case MESSAGE_ERR:
        gtk_text_insert (GTK_TEXT (text_widget), font_normal, &color_green,
                         NULL, _("AMCL Internal Error: "), 21);
        gtk_text_insert (GTK_TEXT (text_widget), font_normal, &color_green,
                         NULL, message, strlen (message));
        break;
    case MESSAGE_ANSI:
        if ( !strchr (message, '\033') )
        {
            gtk_text_insert (GTK_TEXT (text_widget), font_normal,
                             foreground, background, message, -1);
            gtk_text_insert (GTK_TEXT (text_widget), NULL, NULL, NULL," ", 1 );
            gtk_text_backward_delete (GTK_TEXT (text_widget), 1);

            break;
        }
        
        /*while ( *message != '\0' || message == NULL )
        {
            if ( *message == '\033' || *message =='\e')
            {
                while ( !done )
                {
                    switch ( *message )
                    {
                    case '[':
                        found_1 = TRUE;
                        break;
                        
                    case ';':
                        found_2 = TRUE;
                        break;
                        
                    case 'm':
                        found_3 = TRUE;
                        break;
                    }
                    if ( found_1 && !found_2 && !found_3)
                    {
                        message++;

                        if ( *message == 'm' )
                        {
                            *//* This is color reset *//*
                            foreground = &color_white;
                            found_2 = TRUE;
                            found_3 = TRUE;
                        }
                        else
                        {
                            found_2 = TRUE;
                            *ptr = *message;
                            bold = atoi(ptr);

                            message++; message++;
                            ptr[0] = *message;
                            message++;

                            if ( *message != 'm' )
                            {
                                ptr[1] = *message;
                            }
                            color = atoi(ptr);

                            if ( bold == 0 )
                                *//* wonder if these colors are right *//*
                                switch (color)
                                {
                                case 30: foreground = &color_black;   break;
                                case 31: foreground = &color_red;     break;
                                case 32: foreground = &color_green;   break;
                                case 33: foreground = &color_brown;   break;
                                case 34: foreground = &color_blue;    break;
                                case 35: foreground = &color_purple;  break;
                                case 36: foreground = &color_cyan;    break;
                                case 37: foreground = &color_white;   break;
                                }
                            else if ( bold == 1 )
                            {
                                switch (color)
                                {
                                case 30: foreground = &color_grey;       break;
                                case 31: foreground = &color_orange;     break;
                                case 32: foreground = &color_lightgreen; break;
                                case 33: foreground = &color_yellow;     break;
                                case 34: foreground = &color_lightblue;  break;
                                case 35: foreground = &color_pink;       break;
                                case 36: foreground = &color_lightcyan;  break;
                                case 37: foreground = &color_white;      break;
                                }
                            }

                        }
                    }
                    message++;

                    if ((found_1 == TRUE) && (found_2 == TRUE) && (found_3 == TRUE))
                        done = TRUE;
                }

                found_1 = FALSE;
                found_2 = FALSE;
                found_3 = FALSE;
                done    = FALSE;
                ptr[0]  = ptr[1] = '\0';
            }
            else
            {
                if ( *message != '\r' )
                {
                    gtk_text_insert (GTK_TEXT (text_field), display_font,
                                     foreground, NULL, message, 1);
                }
                message++;
            }
            }*/

        while ( (c = *message) )
        {
            switch ( state )
            {
            case NORM:
                if ( c >= ' ' )
                {
                    start = message; len = 0;
                    while ( *message && *message >= ' ')
                    {
                        message++;
                        len++;
                    }
                    gtk_text_insert (GTK_TEXT (text_widget), font_normal,
                                     foreground,
                                     background, start, len );
                }

                if ( !(c = *message))
                    break;

                if ( c != '\033' )
                {
                    gtk_text_insert (GTK_TEXT (text_widget), font_normal,
                                     foreground,
                                     background, &c, 1);
                    ++message;
                    break;
                }

                state = ESC;
                /*c = *++message;*/
                message++;
                break;

            case ESC:
                if ( c != '[' )
                {
                    state = NORM;
                    break;
                }

                state = SQUARE;
                /*c = *++message;*/
                message++;
                break;

            case SQUARE:
                nparms = 0;

                while ( c && (isdigit (c) || c == ';'))
                {
                    if ( isdigit (c))
                    {
                        if ( nparms < 10 )
                        {
                            parms[nparms] = atoi(message);
                            nparms++;
                        }
                        while ( isdigit(c))
                            c = *++message;

                        if ( c == ';' )
                            c = *++message;
                    }
                    else if ( c == ';' )
                    {
                        if ( nparms < 10 )
                        {
                            parms[nparms] = 0;
                            nparms++;
                        }
                        c = *++message;
                    }
                }
                if ( nparms == 0 )
                    parms[nparms++] = 0;

                state = PARMS;

            case PARMS:
                switch (c)
                {
                case 'A':
                    ++message;
                    break;
                case 'B':
                    ++message;
                    break;
                case 'C':
                    ++message;
                    break;
                case 'D':
                    ++message;
                    break;
                case 'H':
                    ++message;
                    break;
                case 'J':
                    ++message;
                    break;
                case 'K':
                    ++message;
                    break;
                case 'm':
                    cons_escm();
                    ++message;
                    break;
                default:
                    break;
                }
                state = NORM;
            }
        }

        break;
        
    case MESSAGE_NORMAL:
    case MESSAGE_NONE:
    default:
        gtk_text_insert (GTK_TEXT (text_widget), font_normal, &color_white,
                         NULL, message, strlen (message));
        break;
    }

    if ( prefs.Freeze )
    {
        gtk_text_thaw (GTK_TEXT (text_widget));
        gtk_text_insert (GTK_TEXT (text_widget), NULL, NULL, NULL, " ", 1 );
        gtk_text_backward_delete (GTK_TEXT (text_widget), 1);
    }
}
