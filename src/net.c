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
#include <db.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

/*
 * Added by Michael Stevens
 */
#ifndef INHIBIT_STRING_HEADER
# if defined (HAVE_STRING_H) || defined (STDC_HEADERS) || defined (_LIBC)
#  include <string.h>
#  ifndef bcmp
#   define bcmp(s1, s2, n) memcmp ((s1), (s2), (n))
#  endif
#  ifndef bcopy
#   define bcopy(s, d, n)  memcpy ((d), (s), (n))
#  endif
#  ifndef bzero
#   define bzero(s, n)     memset ((s), 0, (n))
#  endif
# else
#  include <strings.h>
# endif
#endif

#ifdef HAVE_TELNET_H
#include <telnet.h>
#endif
#ifdef HAVE_ARPA_TELNET_H
#include <arpa/telnet.h>
#endif

#include "amcl.h"

static char const rcsid[] =
    "$Id$";


/*
 * Global Variables
 */
gint  sockfd;
bool  connected;
bool  echo;
gint  input_monitor;
gchar *host, *port;
extern GList *alias_list2;

const  gchar	echo_off_str	[] = { IAC, WILL, TELOPT_ECHO, '\0' };
const  gchar	echo_on_str	[] = { IAC, WONT, TELOPT_ECHO, '\0' };
const  gchar 	go_ahead_str	[] = { IAC, GA, '\0' };

/* mudFTP, www.abandoned.org/drylock/ */
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

/* Added by Bret Robideaux (fayd@alliences.org)
 * I needed this functionality broken out, so that triggered actions
 * could send an alias have it expanded properly
 */
static gchar *alias_check (gchar *buf, gchar *word, gchar *foo)
{
    GList      *tmp;
    ALIAS_DATA *alias;
    gchar *sent = 0;

    sscanf (buf, "%s %[^\n]", word, foo);

    for ( tmp = alias_list2; tmp != NULL; tmp = tmp->next )
    {
        if ( tmp->data )
        {
            alias = (ALIAS_DATA *) tmp->data;

            if ( alias->alias && !strcmp (word, alias->alias) )
            {
                sent = g_malloc0 (strlen (alias->replace) + strlen (foo) + 3);
                sprintf (sent, "%s %s\n", alias->replace, foo);
                break;
            }
        }
    }

    return sent;

}

/* Added by Bret Robideaux (fayd@alliences.org)
 * I needed a separate way to send triggered actions to game, without
 * messing up the players command line or adding to his history.
 */
static void action_send_to_connection (gchar *entry_text)
{
    gchar *temp_entry;
    gchar *word;
    gchar *foo;
    gchar *sent=0;

    temp_entry = g_malloc0 (strlen (entry_text) + 2);
    word       = g_malloc0 (strlen (entry_text) + 2);
    foo        = g_malloc0 (strlen (entry_text) + 2);
    strcat (temp_entry, entry_text);
    strcat (temp_entry, "\n");

    sent = alias_check (temp_entry, word, foo);

    if ( !sent )
        sent = temp_entry;

    if ( connected )
    {
        /* error checking here */
        send (sockfd, sent, strlen (sent), 0);
    }

    if ( sent != temp_entry )
        g_free (sent);

    g_free (temp_entry);
    g_free (word);
    g_free (foo);
}


void make_connection (GtkWidget *widget, gpointer data)
{
    g_free (host);
    g_free (port);

    host = g_strdup (gtk_entry_get_text (GTK_ENTRY( entry_host)));
    port = g_strdup (gtk_entry_get_text (GTK_ENTRY( entry_port)));

    open_connection (host, port);
}

void disconnect (GtkWidget *widget, gpointer data)
{
    close (sockfd);
    gdk_input_remove (input_monitor);
    /* FIXME */
    textfield_add (main_connection->window, "*** Connection closed.\n", MESSAGE_NORMAL);
    connected = FALSE;
    
    gtk_widget_set_sensitive (menu_main_connect, TRUE);
    gtk_widget_set_sensitive (menu_main_disconnect, FALSE);
    gtk_window_set_title (GTK_WINDOW (window), "AMCL "VERSION"");
}

void open_connection (const gchar *host, const gchar *port)
{
    gchar buf[2048];
    struct hostent *he;
    struct sockaddr_in their_addr;

    if ( !(strcmp (host, "\0")) )
    {
        sprintf (buf, "*** Can't connect - you didn't specify a host\n");
        /* FIXME */
        textfield_add (main_connection->window, buf, MESSAGE_ERR);
        return;
    }

    if ( !(strcmp(port, "\0")) )
    {
        sprintf (buf, "*** No port specified - assuming port 23\n");
        /* FIXME */
        textfield_add (main_connection->window, buf, MESSAGE_NORMAL);
        port = "23\0";
    }

    sprintf (buf, "*** Making connection to %s, port %s\n", host, port);
    /* FIXME */
    textfield_add (main_connection->window, buf, MESSAGE_NORMAL);


    /* strerror(3) */
    if ( ( he = gethostbyname (host) ) == NULL )
    {
        return;
    }

    if ( ( sockfd = socket (AF_INET, SOCK_STREAM, 0)) == -1 )
    {
        /* FIXME */
        textfield_add (main_connection->window, strerror(errno), MESSAGE_ERR);
        return;
    }

    their_addr.sin_family = AF_INET;
    their_addr.sin_port   = htons( atoi (port));
    their_addr.sin_addr   = *((struct in_addr *)he->h_addr);
    bzero (&(their_addr.sin_zero), 8);

    if (connect (sockfd, (struct sockaddr *)&their_addr,
                 sizeof (struct sockaddr)) == -1 )
    {
        /* FIXME */
        textfield_add (main_connection->window, strerror(errno), MESSAGE_ERR);
        return;
    }

    /* FIXME */
    textfield_add (main_connection->window, "*** Connection established.\n", MESSAGE_NORMAL);

    input_monitor = gdk_input_add (sockfd, GDK_INPUT_READ,
                                   read_from_connection,
                                   NULL );
    connected = TRUE;
    echo = TRUE;

    gtk_widget_set_sensitive (menu_main_connect, FALSE);
    gtk_widget_set_sensitive (menu_main_disconnect, TRUE);
}

void read_from_connection (gpointer data, gint source, GdkInputCondition condition)
{
    gchar buf[4096];
    gchar triggered_action[85];
    gint  numbytes;
    
    if ( (numbytes = recv (sockfd, buf, 2048, 0) ) == - 1 )
    {
        /* FIXME */
        textfield_add (main_connection->window, strerror( errno), MESSAGE_ERR);
        disconnect ( NULL, NULL);
        return;
    }

    buf[numbytes] = '\0';

    /*
     * Sometimes we get here even though there isn't any data to read
     * from the socket..
     *
     * found by Michael Stevens
     */
    if ( numbytes == 0 )
    {
        disconnect (NULL, NULL);
        return;
    }

    if ( strstr (buf, echo_off_str ) )
    {
        echo = FALSE;
        str_replace (buf, echo_off_str, "");
    }

    if ( strstr (buf, echo_on_str ) )
    {
        echo = TRUE;
        str_replace (buf, echo_on_str, "");
    }

    str_replace (buf, "\r", "");

    /* FIXME */
    textfield_add (main_connection->window, buf, MESSAGE_ANSI);

    /* Added by Bret Robideaux (fayd@alliances.org)
     * OK, this seems like a good place to handle checking for action triggers
     */
    if ( check_actions (buf, triggered_action) )
    {
        action_send_to_connection (triggered_action);
    }
}

void send_to_connection (GtkWidget *widget, gpointer data)
{
    extern GList *EntryHistory;
    GList *tmp;
    ALIAS_DATA *alias;
    gchar *entry_text;
    gchar *temp_entry;
    gchar *word;
    gchar *foo;
    gchar *sent=0;

    entry_text = gtk_entry_get_text (GTK_ENTRY (text_entry));

    EntryHistory = g_list_append (EntryHistory, g_strdup (entry_text));

    if ( g_list_length (EntryHistory) > 10 )
    {
        gchar *temp;

        temp = (gchar *) EntryHistory->data;

        EntryHistory = g_list_remove (EntryHistory, EntryHistory->data);
        g_free (temp);
    }

    temp_entry = g_malloc0 (strlen (entry_text) + 2);
    word       = g_malloc0 (strlen (entry_text) + 2);
    foo        = g_malloc0 (strlen (entry_text) + 2);
    strcat (temp_entry, entry_text);
    strcat (temp_entry, "\n");
    sscanf (temp_entry, "%s %[^\n]", word, foo);

    for ( tmp = g_list_first (alias_list2); tmp != NULL; tmp = tmp->next )
    {
        if ( tmp->data != NULL )
        {
            alias = (ALIAS_DATA *) tmp->data;

            if ( alias->alias && !strcmp (word, alias->alias) )
            {
                sent = g_malloc0(strlen(alias->replace)+strlen(foo)+3);
                sprintf (sent, "%s %s\n", alias->replace, foo);
                break;
            }
        }
    }
    /*for ( alias = alias_list; alias != NULL; alias = alias->next )
    {
        if ( alias->alias && !strcmp (word, alias->alias) )
        {
            sent = g_malloc0(strlen(alias->replace)+strlen(foo)+3);
            sprintf (sent, "%s %s\n", alias->replace, foo);
            break;
        }
    }*/

    if( !sent )
      sent = temp_entry;

    if ( connected )
    {
        /* error checking here */
        send (sockfd, sent, strlen (sent), 0);
    }

    if ( echo && prefs.EchoText)
    {
        /* FIXME */
        textfield_add (main_connection->window, sent, MESSAGE_SENT);
    }

    if ( prefs.KeepText )
        gtk_entry_select_region (GTK_ENTRY (text_entry), 0,
                                 GTK_ENTRY (text_entry)->text_length);
    else
        gtk_entry_set_text (GTK_ENTRY (text_entry), "");

    if( sent != temp_entry )
      g_free( sent );
    g_free (temp_entry);
    g_free (word);
    g_free (foo);
}

void connection_send (gchar *message)
{
    send (sockfd, message, strlen (message), 0);
}
