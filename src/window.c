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


typedef enum { NORM, ESC, SQUARE, PARMS } STATE;

/* Global Variables */
extern GtkWidget       *menu_main_disconnect;
extern GtkWidget       *menu_main_close;
extern CONNECTION_DATA *connections[15];
extern GtkWidget       *main_notebook;
extern GtkWidget       *text_entry;
extern GdkFont         *font_normal;
extern SYSTEM_DATA      prefs;

static gint parms[10], nparms;
static gint nowcol = 0 ;
bool        BOLD   = FALSE;

static void cons_escm (CONNECTION_DATA *cd)
{
    int i, p;

    for ( i = 0; i < nparms; i++ )
    {
        switch ( p = parms[i])
        {
        case 0: /* none */
			cd->foreground = &prefs.Foreground;
			cd->background = &prefs.Background;
			nowcol = 0; /* default foreground colour */
            BOLD   = FALSE;
            break;

        case 1:/* bold */
            BOLD = TRUE;
            break;

        case 22: /* bold off */
            BOLD = FALSE;
            break;

        case 4: /* underscore */
        case 5: /* blink */
        case 7: /* inverse */
            break;

        case 30: case 31: case 32: case 33:
        case 34: case 35: case 36: case 37:
            nowcol = p;
            break;

        case 39:
            nowcol = 0;
            cd->foreground = &prefs.Foreground;
            break;
	
        case 40: case 41: case 42: case 43:
        case 44: case 45: case 46:
            cd->background = &prefs.Colors[ p - 40 ];
            break;

        case 47:
            cd->background = &prefs.Colors[15];
            break;

	case 49:
            cd->background = &prefs.Background;
            break;

        default:
            break;
        }
    }

	if ( nowcol >= 30 && nowcol <= 37 )
	{
		gint j;

		j = nowcol - ( BOLD ? 22 : 30 ) ;
	        cd->foreground = &prefs.Colors[j] ;
	}
	else if ( BOLD && !nowcol )
	{
		cd->foreground = &prefs.BoldForeground;
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
    gchar *start, *logtmp, c;
    gint  len;
	gint       i;
    static STATE state = NORM;

    if ( message[0] == '\0' )
    {
        return;
    }

    if ( prefs.Freeze )
    {
        gtk_text_freeze (GTK_TEXT (text_widget));
    }
    
	i = gtk_notebook_get_current_page (GTK_NOTEBOOK(main_notebook));
	if (connections[i]->logging
		&& colortype != MESSAGE_ANSI
		&& colortype != MESSAGE_ERR
		&& colortype != MESSAGE_SYSTEM)
	{
		fputs(message, connections[i]->log);
	}

	vte_terminal_feed(VTE_TERMINAL(text_widget), message, strlen(message));
	return;
	
    switch (colortype)
    {
    case MESSAGE_SENT:
        gtk_text_insert (GTK_TEXT (text_widget), font_normal, &prefs.Colors[11], &prefs.Background, message, strlen (message));
        break;
		
    case MESSAGE_ERR:
        gtk_text_insert (GTK_TEXT (text_widget), font_normal, &prefs.Colors[2], &prefs.Background, message, strlen (message));
        break;
		
    case MESSAGE_ANSI:
        if ( !strchr (message, '\033') )
        {
            gtk_text_insert (GTK_TEXT (text_widget), font_normal, cd->foreground, cd->background, message, -1);
            gtk_text_insert (GTK_TEXT (text_widget), NULL, NULL, NULL," ", 1 );
            gtk_text_backward_delete (GTK_TEXT (text_widget), 1);

	    if (connections[i]->logging)
	    {
		fputs(message, connections[i]->log);
	    }

            break;
        }
        
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
                                     cd->foreground,
                                     cd->background, start, len );

		    if (connections[i]->logging)
		    {
			logtmp = g_malloc (len + 1);
			strncpy(logtmp, start, len);
			logtmp[len] = '\0';
			fputs (logtmp, connections[i]->log);
			g_free (logtmp);
                    }
                }

                if ( !(c = *message))
                    break;

                if (c == '\xbf') {
                        state = SQUARE;
                        break;
                }

                if ( c != '\033' )
                {
                    gtk_text_insert (GTK_TEXT (text_widget), font_normal, cd->foreground, cd->background, &c, 1);
                    ++message;

		    if (connections[i]->logging)
		    {
			fputc(c, connections[i]->log);
                    }

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
                    cons_escm(cd);
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
    case MESSAGE_SYSTEM:
    default:
        gtk_text_insert (GTK_TEXT (text_widget), font_normal, &prefs.Foreground, &prefs.Background, message, strlen (message));
        break;
    }

    if ( prefs.Freeze )
    {
        gtk_text_thaw (GTK_TEXT (text_widget));
        gtk_text_insert (GTK_TEXT (text_widget), NULL, NULL, NULL, " ", 1 );
        gtk_text_backward_delete (GTK_TEXT (text_widget), 1);
    }
}

