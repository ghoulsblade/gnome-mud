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

#include "amcl.h"
#include "modules.h"

static char const rcsid[] =
    "$Id$";


/*
 * Global Variables
 */
extern bool Keyflag;
gchar *host, *port;
extern GList *alias_list2;

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
  
  for ( tmp = alias_list2; tmp != NULL; tmp = tmp->next ) {
    if ( tmp->data ) {
      alias = (ALIAS_DATA *) tmp->data;
      
      if ( alias->alias && !strcmp (word, alias->alias) ) {
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
static void action_send_to_connection (gchar *entry_text, CONNECTION_DATA *connection)
{
    gchar *temp_entry;
    gchar *word;
    gchar *foo;
    gchar *sent=0;
    gint   i = 0;

    temp_entry = g_malloc0 (strlen (entry_text) + 2);
    word       = g_malloc0 (strlen (entry_text) + 2);
    foo        = g_malloc0 (strlen (entry_text) + 2);
    strcat (temp_entry, entry_text);
    strcat (temp_entry, "\n");

    sent = alias_check (temp_entry, word, foo);

    if ( !sent )
        sent = temp_entry;

    for(;sent[i]!=0;i++)
      if(sent[i] == prefs.CommDev[0]) sent[i] = '\n';
    
    if (connection->connected) {
      /* error checking here */
      send (connection->sockfd, sent, strlen (sent), 0);
    }

    if ( sent != temp_entry )
        g_free (sent);

    g_free (temp_entry);
    g_free (word);
    g_free (foo);
}


CONNECTION_DATA *make_connection( gchar *hoster, gchar *porter)
{
  CONNECTION_DATA *connection;
  GtkWidget       *label;

  if (main_connection->connected) {
    connection = g_malloc0( sizeof (CONNECTION_DATA));
    connection->window = gtk_text_new (NULL, NULL);
    gtk_widget_show (connection->window);
  } else {
    connection = main_connection;
  }

  g_free(connection->host); g_free(connection->port);
  connection->host = g_strdup(hoster);
  connection->port = g_strdup(porter);

  if (main_connection != connection) {
    label = gtk_label_new(connection->host);
    gtk_notebook_append_page (GTK_NOTEBOOK (main_notebook), connection->window, label);
    gtk_widget_realize (connection->window);
    gdk_window_set_background (GTK_TEXT (connection->window)->text_area, &color_black); 
    gtk_notebook_next_page (GTK_NOTEBOOK (main_notebook));
    connection->notebook = gtk_notebook_get_current_page (GTK_NOTEBOOK (main_notebook));
    connections[connection->notebook] = connection;
  }
  
  open_connection (connection);

  return connection;
}

void disconnect (GtkWidget *widget, CONNECTION_DATA *connection)
{
    close (connection->sockfd);
    gdk_input_remove (connection->data_ready);
    textfield_add (connection->window, "*** Connection closed.\n", MESSAGE_NORMAL);
    connection->connected = FALSE;
}

void open_connection (CONNECTION_DATA *connection)
{
    gchar buf[2048];
    struct hostent *he;
    struct sockaddr_in their_addr;

    if ( !(strcmp (connection->host, "\0")) )
    {
        sprintf (buf, "*** Can't connect - you didn't specify a host\n");
        textfield_add (connection->window, buf, MESSAGE_ERR);
        return;
    }

    if ( !(strcmp(connection->port, "\0")) )
    {
        sprintf (buf, "*** No port specified - assuming port 23\n");
        textfield_add (connection->window, buf, MESSAGE_NORMAL);
        port = "23\0";
    }

    sprintf (buf, "*** Making connection to %s, port %s\n", connection->host, connection->port);
    textfield_add (connection->window, buf, MESSAGE_NORMAL);


    /* strerror(3) */
    if ( ( he = gethostbyname (connection->host) ) == NULL )
    {
        return;
    }

    if ( ( connection->sockfd = socket (AF_INET, SOCK_STREAM, 0)) == -1 )
    {
        textfield_add (connection->window, strerror(errno), MESSAGE_ERR);
        return;
    }

    their_addr.sin_family = AF_INET;
    their_addr.sin_port   = htons( atoi (connection->port));
    their_addr.sin_addr   = *((struct in_addr *)he->h_addr);
    bzero (&(their_addr.sin_zero), 8);

    if (connect (connection->sockfd, (struct sockaddr *)&their_addr,
                 sizeof (struct sockaddr)) == -1 )
    {
        textfield_add (connection->window, strerror(errno), MESSAGE_ERR);
        return;
    }

    textfield_add (connection->window, "*** Connection established.\n", MESSAGE_NORMAL);

    connection->data_ready = gdk_input_add(connection->sockfd, GDK_INPUT_READ,
					   GTK_SIGNAL_FUNC(read_from_connection),
					   (gpointer) connection);
    connection->connected = TRUE;

    gtk_widget_set_sensitive (menu_main_disconnect, TRUE);
}

void read_from_connection (CONNECTION_DATA *connection, gint source, GdkInputCondition condition)
{
    gchar  buf[4096];
    gchar  triggered_action[85];
    gint   numbytes;
    gchar *m;
    gint   len;
    GList *t;
    
    if ( (numbytes = recv (connection->sockfd, buf, 2048, 0) ) == - 1 )
    {
        textfield_add (connection->window, strerror( errno), MESSAGE_ERR);
        disconnect (NULL, connection);
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
        disconnect (NULL, connection);
        return;
    }

    for (t = g_list_first(Plugin_data_list); t != NULL; t = t->next) {
      PLUGIN_DATA *pd;
      
      if (t->data != NULL) {
	pd = (PLUGIN_DATA *) t->data;

	if (pd->plugin && pd->plugin->enabeled && (pd->dir == PLUGIN_DATA_IN)) {
	  (* pd->datafunc) (pd->plugin, connection, buf, (gint) pd->plugin->handle);
	}
      }
    }
    
    str_replace (buf, "\r", "");

    /* Changes by Benjamin Curtis */
    len = pre_process(buf, connection);
    m   = (gchar *) malloc(len + 2);
    memcpy(m, buf, len+1);

    textfield_add (connection->window, m, MESSAGE_ANSI);

    /* Added by Bret Robideaux (fayd@alliances.org)
     * OK, this seems like a good place to handle checking for action triggers
     */
    if ( check_actions (buf, triggered_action) )
    {
        action_send_to_connection (triggered_action, connection);
    }
}

void send_to_connection (GtkWidget *widget, gpointer data)
{
  CONNECTION_DATA *cd;
  gint number;

  extern GList *EntryHistory;
  extern GList *EntryCurr;
  GList *tmp;
  ALIAS_DATA *alias;
  gchar *entry_text;
  gchar *temp_entry;
  gchar *word;
  gchar *foo;
  gchar *sent=0;
  gint   i=0;

  Keyflag = TRUE;
  number = gtk_notebook_get_current_page (GTK_NOTEBOOK (main_notebook));
  cd = connections[number];

  entry_text = gtk_entry_get_text (GTK_ENTRY (text_entry));
  EntryCurr = g_list_last (EntryHistory);

  if (entry_text[0] == '\0') 
    {
      send (cd->sockfd, "\n", 1, 0);
      textfield_add(cd->window, "\n",MESSAGE_SENT);
      return;
    }
  
  if ( !EntryCurr || strcmp (EntryCurr->data, entry_text))
    {
      EntryHistory = g_list_append (EntryHistory, g_strdup (entry_text));
      
      
      if ( g_list_length (EntryHistory) > 10 )
	{
	  gchar *temp;
	  temp = (gchar *) EntryHistory->data;
	  
	  EntryHistory = g_list_remove (EntryHistory, EntryHistory->data);
	  g_free (temp);
	}
    }
  EntryCurr = g_list_last (EntryHistory);
  
  temp_entry = g_malloc0 (strlen (entry_text) + 2);
  word       = g_malloc0 (strlen (entry_text) + 2);
  foo        = g_malloc0 (strlen (entry_text) + 2);
  strcat (temp_entry, entry_text);
  strcat (temp_entry, "\n");
  sscanf (temp_entry, "%s %[^\n]", word, foo);
  
  for ( tmp = g_list_first (alias_list2); tmp != NULL; tmp = tmp->next ) {
    if ( tmp->data != NULL ) {
      alias = (ALIAS_DATA *) tmp->data;
      
      if ( alias->alias && !strcmp (word, alias->alias) ) {
	sent = g_malloc0(strlen(alias->replace)+strlen(foo)+3);
	sprintf (sent, "%s %s\n", alias->replace, foo);
	break;
      }
    }
  }

  if( !sent )
    sent = temp_entry;

  for(;sent[i]!=0;i++)
    if(sent[i] == prefs.CommDev[0]) sent[i] = '\n';
  
  if (cd->connected) {
    /* error checking here */
    send (cd->sockfd, sent, strlen (sent), 0);
  }

  if (prefs.EchoText) {
    textfield_add (cd->window, sent, MESSAGE_SENT);
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

void connection_send (CONNECTION_DATA *connection, gchar *message)
{
  gint i=0;
  
  gchar *sent = g_strdup (message);
  
  for(;sent[i]!=0;i++)
    if(sent[i] == prefs.CommDev[0]) sent[i] = '\n';
  
  if (connection->echo && prefs.EchoText) { 
    textfield_add (connection->window, message, MESSAGE_SENT);
  }
  
  send (connection->sockfd, message, strlen (message), 0);
  free(sent);
}
