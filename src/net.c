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

#include <gnome.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <errno.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

static char const rcsid[] =
    "$Id$";


/* Local functions */
static void read_from_connection (CONNECTION_DATA *, gint, GdkInputCondition);   
static void print_error (CONNECTION_DATA *connection, const gchar *error) ;
static void append_word_to_command (GString *string, gchar *word);

/* Global Variables */
extern bool		Keyflag;
extern SYSTEM_DATA	prefs;
extern CONNECTION_DATA	*main_connection;
extern CONNECTION_DATA	*connections[15];
extern GtkWidget	*main_notebook;
extern GtkWidget	*menu_main_disconnect;
extern GList		*EntryHistory;
extern GList		*EntryCurr;
gchar *host = "", *port = "";

/* Added by Bret Robideaux (fayd@alliences.org)
 * I needed a separate way to send triggered actions to game, without
 * messing up the players command line or adding to his history. */
static void check_aliases(GString *string, CONNECTION_DATA *cd, gchar *t, gint level)
{
	gchar **a = g_strsplit (t," ", -1), *r;

	if (level > 5)
	{
		g_message("Maximum nested alias reached.");
		append_word_to_command (string, t);
		return;
	}
	
	while(*a)
	{
		gchar *b = *a++;
		
		if ((r = check_alias (cd->profile->alias, b)))
		{
			check_aliases(string, cd, r, level+1);
		}
		else
		{
			append_word_to_command (string, b);
		}
	}
}

void action_send_to_connection (gchar *entry_text, CONNECTION_DATA *connection)
{
	GString *alias = g_string_new("");
	gchar   *a;
	
	check_aliases(alias, connection, entry_text, 0);

	a = g_strdup(alias->str);
	g_string_free(alias, TRUE);
	
   	connection_send (connection, check_vars (connection->profile->variables, a));
	connection_send (connection, "\n");

	g_free(a);
}

CONNECTION_DATA *make_connection(gchar *hoster, gchar *porter, gchar *profile)
{
  CONNECTION_DATA *connection;
  PROFILE_DATA	  *pd;
  GtkWidget       *label;
  GtkWidget       *box;

  if (main_connection->connected) {
    connection = g_malloc0( sizeof (CONNECTION_DATA));
    
    connection->window = gtk_text_new (NULL, NULL);
    GTK_WIDGET_UNSET_FLAGS(GTK_WIDGET(connection->window), GTK_CAN_FOCUS);
    gtk_widget_set_usize (connection->window, 500, 320);
    gtk_signal_connect (GTK_OBJECT (connection->window), "focus-in-event", GTK_SIGNAL_FUNC (grab_focus_cb), NULL);
    gtk_widget_show (connection->window);

    connection->vscrollbar = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(connection->vscrollbar), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_widget_show (connection->vscrollbar);
  } else {
    connection = main_connection;
  }

  g_free(connection->host); g_free(connection->port);
  connection->host = g_strdup(hoster);
  connection->port = g_strdup(porter);

  if ((pd = profiledata_find(profile)) == NULL)
  {
	  pd = profiledata_find("Default");
  }
  connection->profile = pd;

  if (main_connection != connection) {
    box = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new(connection->host);
    gtk_notebook_append_page (GTK_NOTEBOOK(main_notebook), box, label);
    gtk_widget_show(box);

    gtk_box_pack_start(GTK_BOX(box), connection->vscrollbar, TRUE, TRUE, 0);
   	gtk_container_add(GTK_CONTAINER(connection->vscrollbar), connection->window);
	
    gtk_widget_realize (connection->window);
    /*FIXME gdk_window_set_background (GTK_TEXT (connection->window)->text_area, &prefs.Background);*/
    gtk_notebook_next_page (GTK_NOTEBOOK (main_notebook));
    connection->notebook = gtk_notebook_get_current_page (GTK_NOTEBOOK (main_notebook));
    connections[connection->notebook] = connection;
#ifdef ENABLE_MCCP
	connection->mccp = mudcompress_new();
#endif /* ENABLE_MCCP */
	connection->foreground = &prefs.Foreground;
	connection->background = &prefs.Background;
  }
  
  open_connection (connection);

  return connection;
}

void disconnect (GtkWidget *widget, CONNECTION_DATA *connection)
{
    close (connection->sockfd);
#ifdef ENABLE_MCCP
    mudcompress_delete(connection->mccp);
    connection->mccp = mudcompress_new();
#endif /* ENABLE_MCCP */
    gdk_input_remove (connection->data_ready);

    textfield_add (connection, _("*** Connection closed.\r\n"), MESSAGE_SYSTEM);
    connection->connected = FALSE;
}

void open_connection (CONNECTION_DATA *connection)
{
    gchar buf[2048];
    struct addrinfo req, *ans,*tmpaddr;
    int ret;

    if ( (!connection->host) || (!strcmp (connection->host, "\0")) )
    {
        textfield_add (connection, _("*** Can't connect - you didn't specify a host.\r\n"), MESSAGE_ERR);
        return;
    }

    if ( (!connection->port) || (!strcmp(connection->port, "\0")) )
    {
		textfield_add (connection, _("*** No port specified - assuming port 23.\r\n"), MESSAGE_SYSTEM);

		if (connection->port[0] != '\0')
		{
			g_free(connection->port);
		}
		connection->port = g_strdup("23");

		if (port[0] != '\0')
		{
			g_free(port);
		}
        port = g_strdup("23");
    }

    g_snprintf (buf, 2047, _("*** Making connection to %s, port %s.\r\n"), connection->host, connection->port);
    textfield_add (connection, buf, MESSAGE_SYSTEM);

    /* strerror(3) */
    req.ai_flags = 0;
    req.ai_family = AF_UNSPEC;
    req.ai_socktype = SOCK_STREAM;
    req.ai_protocol =IPPROTO_TCP;

    ret = getaddrinfo(connection->host,connection->port,&req,&ans);
    if ( ret != 0) {
     	print_error(connection, gai_strerror(ret)) ;
      return;
    }

   tmpaddr = ans;


   while (tmpaddr != NULL) {
    char name[NI_MAXHOST],portname[NI_MAXSERV];
    getnameinfo(tmpaddr->ai_addr, tmpaddr->ai_addrlen,
        name,sizeof(name),portname,sizeof(portname),
        NI_NUMERICHOST | NI_NUMERICSERV);

    snprintf(buf,2047,_("*** Trying %s port %s...\r\n"),name,port);
    textfield_add (connection, buf, MESSAGE_SYSTEM);
  
    connection->sockfd = 
      socket(tmpaddr->ai_family,tmpaddr->ai_socktype,tmpaddr->ai_protocol);
    if (connection->sockfd < 0) {
      print_error(connection,strerror(errno));
    } else if ((ret=connect(connection->sockfd, tmpaddr->ai_addr,tmpaddr->ai_addrlen)) < 0) {
      print_error(connection,strerror(errno));
    } else {
	  break;
	}
    tmpaddr = tmpaddr->ai_next;
  } 
  freeaddrinfo(ans);
  textfield_add (connection, _("*** Connection established.\r\n"), MESSAGE_SYSTEM);

    connection->data_ready = gdk_input_add(connection->sockfd, GDK_INPUT_READ,
					   GTK_SIGNAL_FUNC(read_from_connection),
					   (gpointer) connection);
    connection->connected = TRUE;
}

static void read_from_connection (CONNECTION_DATA *connection, gint source, GdkInputCondition condition)
{
	gchar  buf[3000];
	gchar  *triggered_action;
	gint   numbytes;
	gint   len;
	GList *t;
	gchar *mccp_buffer = NULL;
	gchar *mccp_data;
#ifdef ENABLE_MCCP
	gchar *string;
	gint   mccp_i;
#endif
   
	if ( (numbytes = recv (connection->sockfd, buf, 2048, 0) ) == - 1 )
	{
		print_error (connection, strerror(errno)) ;
		disconnect (NULL, connection);
		return;
	}
  
	buf[numbytes] = '\0';
  
	if ( numbytes == 0 )
	{
		disconnect (NULL, connection);
		return;
	}
  
#ifdef ENABLE_MCCP
	mudcompress_receive(connection->mccp, buf, numbytes);
	while((mccp_i = mudcompress_pending(connection->mccp)) > 0)
	{
		mccp_buffer = g_malloc0(mccp_i);
		mudcompress_get(connection->mccp, mccp_buffer, mccp_i);
#else
	    mccp_buffer = g_strdup(buf);
#endif
    
		mccp_data = g_strdup(mccp_buffer);
#ifdef ENABLE_MCCP
		mccp_data[mccp_i] = '\0';
#endif

		for (t = g_list_first(Plugin_data_list); t != NULL; t = t->next)
		{
			PLUGIN_DATA *pd;
      
			if (t->data != NULL)
			{
				pd = (PLUGIN_DATA *) t->data;
	
				if (pd->plugin && pd->plugin->enabeled && (pd->dir == PLUGIN_DATA_IN))
				{
	  				(* pd->datafunc) (pd->plugin, connection, mccp_data, GPOINTER_TO_INT(pd->plugin->handle));
				}
      		}
    	}
    
    	//len = pre_process(mccp_data, connection);

#ifdef USE_PYTHON
    	mccp_data = python_process_input(connection, mccp_data);
#endif

	/* Is all this mucking about with len really necessary? I hope not, because
	 * I've replaced it with a simple g_strdup(). It broke when Python modified
	 * the mccp_buffer, because the length of the string would change between
	 * pre_process() above and the commented code below. g_strdup() doesn't mind
	 * this at all. I just hope I'm not missing anything crucial.
	 */
#if 0
	m = malloc(len + 2);
	memcpy(m, mccp_data, len + 1);

       /* this crashes gnome-mud */
	m = g_strdup(mccp_data);
#endif
	textfield_add (connection, mccp_data, MESSAGE_ANSI);

	/* Added by Bret Robideaux (fayd@alliances.org)
	 * OK, this seems like a good place to handle checking for action triggers
	 */
	if ((triggered_action = check_actions (connection->profile->triggers, mccp_data)))
		action_send_to_connection (triggered_action, connection);
   
	g_free(mccp_data);
	g_free(mccp_buffer);
#ifdef ENABLE_MCCP
	}
   
	string = (gchar *) mudcompress_response(connection->mccp);

	if (string != NULL)
	{
		send (connection->sockfd, string, strlen(string), 0);
 	}
	
	g_free(string);
#endif  
}

void connection_send_data (CONNECTION_DATA *connection, gchar *message, int echo)
{
    gint i;
    gchar *sent;
  
    if (connection->connected)
    {
		sent = g_strdup (message);

		for(i=0;sent[i]!=0;i++)
		{
	 		if(sent[i] == prefs.CommDev[0])
			{
	    		sent[i] = '\n';
	  		}
		}

		if (prefs.EchoText && echo)
			textfield_add (connection, sent, MESSAGE_SENT);

		send (connection->sockfd, sent, strlen (sent), 0);
		g_free(sent);
	}
}

void connection_send (CONNECTION_DATA *connection, gchar *message)
{
	gchar *sent = g_strdup(message);
	gint i;

	for(i=0;sent[i]!=0;i++)
	{
		if(sent[i] == prefs.CommDev[0])
		{
	    	sent[i] = '\n';
		}
    }
#ifdef USE_PYTHON
    sent = python_process_output(connection, sent);
#endif
    connection_send_data(connection, sent, 1);
    g_free(sent);
}

/* connection_send_telnet_control:
 * Vashti <vashti@dream.org.uk> 2 February 2002.
 *   Arguments: 0 - the connection to send the data to.
 *              1 - the number of bytes to send.
 *              2.. - a variable length list of bytes to send.
 */
void connection_send_telnet_control (CONNECTION_DATA *connection, int len, ...)
{
	int i;
	unsigned char pkt[256];
	va_list ap;

	va_start (ap, len);

	for (i = 0 ; i <= (len - 1); i++)
		pkt[i] = (unsigned char) va_arg (ap, int);

	write (connection->sockfd, pkt, i);

	va_end (ap);
} /* connection_send_telnet_control */

static void print_error (CONNECTION_DATA *cd, const gchar *error)
{
	gchar buf[256] ;

	g_snprintf(buf, 255, "*** %s.\r\n", error);
	textfield_add (cd, buf, MESSAGE_ERR);
} /* print_error */

/* append_word_to_command:
 *	Appends a word to a GString. Used by check_aliases.
 *	Arguments: 0 - the string to append to.
 *		   1 - the word to append.
 */

static void append_word_to_command (GString *string, gchar *word)
{
	if (!string->len)
	{
		g_string_append(string, word);
	}
	else
	{
		g_string_sprintfa(string, " %s", word);
	}
} /* append_word_to_command */

