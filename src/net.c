/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1998-2001 Robin Ericsson <lobbin@localhost.nu>
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

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

/* #include <db.h> */
#include <errno.h>
#include <libintl.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gtk/gtk.h>

/* Added by Michael Stevens */
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

#include "gnome-mud.h"
#include "modules.h"

#define _(string) gettext(string)

static char const rcsid[] =
    "$Id$";


/* Local functions */
static void open_connection (CONNECTION_DATA *);
static void action_send_to_connection (gchar *, CONNECTION_DATA *);
static void read_from_connection (CONNECTION_DATA *, gint,
                                  GdkInputCondition);   

/* Global Variables */
extern bool             Keyflag;
extern SYSTEM_DATA      prefs;
extern CONNECTION_DATA *main_connection;
extern CONNECTION_DATA *connections[15];
extern GtkWidget       *main_notebook;
extern GtkWidget       *menu_main_disconnect;
extern GdkColor         color_black;
extern GtkCList        *lists[3];
gchar *host = "", *port = "";

/* Added by Bret Robideaux (fayd@alliences.org)
 * I needed a separate way to send triggered actions to game, without
 * messing up the players command line or adding to his history. */
static void action_send_to_connection (gchar *entry_text, CONNECTION_DATA *connection)
{
    gchar **a = g_strsplit (entry_text," ", 2), *r;

    if (*a)
    {
        if ((r = check_alias (lists[0], *a)))
        {
            g_free(*a);
            *a = strdup(r);
        }
    }
    connection_send (connection, check_vars (lists[2], g_strjoinv (" ", a)));
    connection_send (connection, "\n");
    g_strfreev(a);
}

CONNECTION_DATA *make_connection(gchar *hoster, gchar *porter)
{
  CONNECTION_DATA *connection;
  GtkWidget       *label;
  GtkWidget       *box;

  if (main_connection->connected) {
    connection = g_malloc0( sizeof (CONNECTION_DATA));
    
    connection->window = gtk_text_new (NULL, NULL);
    GTK_WIDGET_UNSET_FLAGS(GTK_WIDGET(connection->window), GTK_CAN_FOCUS);
    gtk_widget_set_usize (connection->window, 500, 320);
    gtk_signal_connect (GTK_OBJECT (connection->window), "focus-in-event",
    							GTK_SIGNAL_FUNC (grab_focus_cb), NULL);
    gtk_widget_show (connection->window);

    connection->vscrollbar = gtk_vscrollbar_new(GTK_TEXT(connection->window)->vadj);
    gtk_widget_show (connection->vscrollbar);
  } else {
    connection = main_connection;
  }

  g_free(connection->host); g_free(connection->port);
  connection->host = g_strdup(hoster);
  connection->port = g_strdup(porter);

  if (main_connection != connection) {
    box = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new(connection->host);
    gtk_notebook_append_page (GTK_NOTEBOOK(main_notebook), box, label);
    gtk_widget_show(box);

    gtk_box_pack_start(GTK_BOX(box), connection->window, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), connection->vscrollbar, FALSE, FALSE, 0);
    
    gtk_widget_realize (connection->window);
    gdk_window_set_background (GTK_TEXT (connection->window)->text_area, &color_black); 
    gtk_notebook_next_page (GTK_NOTEBOOK (main_notebook));
    connection->notebook = gtk_notebook_get_current_page (GTK_NOTEBOOK (main_notebook));
    connections[connection->notebook] = connection;
    connection->mccp = mudcompress_new();
  }
  
  open_connection (connection);

  return connection;
}

void disconnect (GtkWidget *widget, CONNECTION_DATA *connection)
{
    close (connection->sockfd);
    mudcompress_delete(connection->mccp);
    connection->mccp = mudcompress_new();
    gdk_input_remove (connection->data_ready);
    textfield_add (connection->window, _("*** Connection closed.\n"), MESSAGE_NORMAL);
    connection->connected = FALSE;
}

static void open_connection (CONNECTION_DATA *connection)
{
    gchar buf[2048];
    struct hostent *he;
    struct sockaddr_in their_addr;

    if ( !(strcmp (connection->host, "\0")) )
    {
        sprintf (buf, _("*** Can't connect - you didn't specify a host\n"));
        textfield_add (connection->window, buf, MESSAGE_ERR);
        return;
    }

    if ( !(strcmp(connection->port, "\0")) )
    {
        sprintf (buf, _("*** No port specified - assuming port 23\n"));
        textfield_add (connection->window, buf, MESSAGE_NORMAL);
        port = "23\0";
    }

    sprintf (buf, _("*** Making connection to %s, port %s\n"), connection->host, connection->port);
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

    textfield_add (connection->window, _("*** Connection established.\n"), MESSAGE_NORMAL);

    connection->data_ready = gdk_input_add(connection->sockfd, GDK_INPUT_READ,
					   GTK_SIGNAL_FUNC(read_from_connection),
					   (gpointer) connection);
    connection->connected = TRUE;
}

static void read_from_connection (CONNECTION_DATA *connection, gint source, GdkInputCondition condition)
{
  gchar  buf[2048];
  gchar  *triggered_action;
  gint   numbytes;
  gchar *m;
  gint   i, len;
  GList *t;
#ifdef ENABLE_MCCP
  gchar *mccp_buffer = NULL, *string;
  gint   mccp_i;
#endif
    
  if ( (numbytes = recv (connection->sockfd, buf, 2048, 0) ) == - 1 ) {
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
  if ( numbytes == 0 ) {
    disconnect (NULL, connection);
    return;
  }
  
#ifdef ENABLE_MCCP
  mudcompress_receive(connection->mccp, buf, numbytes);
  while((mccp_i = mudcompress_pending(connection->mccp)) > 0) {
    mccp_buffer = g_malloc0(mccp_i);
    mudcompress_get(connection->mccp, mccp_buffer, mccp_i);
#else
#define mccp_buffer buf
#endif
    
    for (t = g_list_first(Plugin_data_list); t != NULL; t = t->next) {
      PLUGIN_DATA *pd;
      
      if (t->data != NULL) {
	pd = (PLUGIN_DATA *) t->data;
	
	if (pd->plugin && pd->plugin->enabeled && (pd->dir == PLUGIN_DATA_IN)) {
	  (* pd->datafunc) (pd->plugin, connection, mccp_buffer, (gint) pd->plugin->handle);
	}
      }
    }
    
    for (i = len = 0; mccp_buffer[i] !='\0'; mccp_buffer[len++] = mccp_buffer[i++])
      if (mccp_buffer[i] == '\r') i++;
    mccp_buffer[len] = mccp_buffer[i];
    
    /* Changes by Benjamin Curtis */
    len = (gint) pre_process(mccp_buffer, connection);
    m   = (gchar *) malloc(len + 2);
    memcpy(m, mccp_buffer, len + 1);
    
    textfield_add (connection->window, m, MESSAGE_ANSI);
    
    /* Added by Bret Robideaux (fayd@alliances.org)
     * OK, this seems like a good place to handle checking for action triggers
     */
    if ((triggered_action = check_actions (lists[1], mccp_buffer)))
        action_send_to_connection (triggered_action, connection);
    
    
#ifdef ENABLE_MCCP
    g_free(mccp_buffer);
  }
   string = (gchar *) mudcompress_response(connection->mccp);

  if (string != NULL) {
    send (connection->sockfd, string, strlen(string), 0);
  }
  g_free(string);
#endif  
}

void send_to_connection (GtkWidget *text_entry, gpointer data)
{
  CONNECTION_DATA *cd;
  extern GList *EntryHistory;
  extern GList *EntryCurr;
  gchar *entry_text;

  Keyflag = TRUE;
  cd = connections[gtk_notebook_get_current_page 
                   (GTK_NOTEBOOK (main_notebook))];
  entry_text = gtk_entry_get_text (GTK_ENTRY (text_entry));
  EntryCurr = g_list_last (EntryHistory);

  if (entry_text[0] == '\0') 
    {
      send (cd->sockfd, "\n", 1, 0);
      textfield_add(cd->window, "\n",MESSAGE_SENT);
      return;
    }
  
  if ( !EntryCurr || strcmp (EntryCurr->data, entry_text)) {
    EntryHistory = g_list_append (EntryHistory, g_strdup (entry_text));

    if ( g_list_length (EntryHistory) > prefs.History )
      EntryHistory = g_list_remove (EntryHistory, EntryHistory->data);

    EntryCurr = g_list_last (EntryHistory);
  }

  action_send_to_connection(g_strdup (entry_text), cd);
  
  gtk_combo_set_popdown_strings((GtkCombo *) data, EntryHistory);

  gtk_list_select_item(GTK_LIST(((GtkCombo *) data)->list), g_list_length(EntryHistory)-1);

  if ( prefs.KeepText ) {
    gtk_widget_realize(GTK_WIDGET(data));
    gtk_entry_select_region (GTK_ENTRY (text_entry), 0,
			     GTK_ENTRY (text_entry)->text_length);
  } else{
    gtk_entry_set_text (GTK_ENTRY (text_entry), "");
  }
}

void connection_send (CONNECTION_DATA *connection, gchar *message)
{
    gint i;
    gchar *sent;
  
    if (connection->connected)
    {
        sent = g_strdup (message);
        for(i=0;sent[i]!=0;i++) {
	  if(sent[i] == prefs.CommDev[0]) {
	    sent[i] = '\n';
	  }
	}

	if (prefs.EchoText)
            textfield_add (connection->window, sent, MESSAGE_SENT);

        send (connection->sockfd, sent, strlen (sent), 0);
        g_free(sent);
    }
}
