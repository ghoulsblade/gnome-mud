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
#include <gtk/gtk.h>
#include <vte/vte.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_TELNET_H
#include <telnet.h>
#endif
#ifdef HAVE_ARPA_TELNET_H
#include <arpa/telnet.h>
#endif

#include "gnome-mud.h"

static char const rcsid[] = 
    "$Id$";

/* Global variables */

extern SYSTEM_DATA prefs ;

/* Added by Benjamin Curtis, Code Swiped from TUsh by Simon Marsh
 * TERMINAL-TYPE support added by Vashti <vashti@dream.org.uk> February 2002 */

/* Rewritten 2003 by Abigail Brady to be safe */

static unsigned char *write_octet(unsigned char *p, unsigned char data)
{
  *p++ = data;
  if (data==IAC)
    *p++ = data;
  return p;
}

void connection_send_naws(CONNECTION_DATA *connection)
{
	unsigned char pkt[3 + 2 + 8] = { IAC, SB, TELOPT_NAWS };
 	unsigned long h = vte_terminal_get_row_count(VTE_TERMINAL(connection->window)),
    		      w = vte_terminal_get_column_count(VTE_TERMINAL(connection->window));
  
	unsigned char *p = pkt+3;

  	p = write_octet(p, (unsigned char)(w >> 8));
  	p = write_octet(p, (unsigned char)(w & 0xff));
  	p = write_octet(p, (unsigned char)(h >> 8));
  	p = write_octet(p, (unsigned char)(h & 0xff));

	*p++ = IAC;
	*p++ = SE;

  	write(connection->sockfd, pkt, p - pkt);
}

gint pre_process(char *buf, CONNECTION_DATA *connection)
{
	unsigned char *from, *to;
	int fromlen, i;

	from = to =(unsigned char *) buf;
	fromlen = strlen(buf);

	for(i = 0; i < fromlen; i++)
	{
	  	unsigned char data = from[i];

		switch(connection->telnet_state)
		{
				/* normal data mode */

			case 0:
				switch(data)
				{
					case IAC:
						connection->telnet_state = IAC;
						break;
					default:
						/* We discard the contents of IAC SB stuff at the moment, as there is nothing interesting to us */
						if(!connection->telnet_subneg)
							*to++ = data;
				}
				break;

				/* right after an IAC */
			case IAC:

				connection->telnet_state = 0;

				switch(data)
				{
				  	/* quote instance of IAC */
					case IAC:
						if(!connection->telnet_subneg)
							*to++ = data;
						break;

					/* IP kill connection */
					case IP: /* IP control */
						disconnect(NULL,NULL);
						return 0;
						break;

					case WILL:
					case WONT:
					case DO:
					case DONT:
					case SB:
						connection->telnet_state = data;
						break;

					case SE:
						switch(connection->telnet_subneg)
						{
							case TELOPT_TTYPE:
								{
									int pos;
									unsigned char pkt[64] = { IAC, SB, TELOPT_TTYPE, TELQUAL_IS };

									/* if strlen(TERM) > 50 you have issues */
									strncpy(&pkt[4], (unsigned char *) prefs.TerminalType, 50);

									pos = (4 + strlen(prefs.TerminalType)) - 1;
									pkt[pos + 1] = IAC;
									pkt[pos + 2] = SE;

									write(connection->sockfd, &pkt, pos + 3);
								}
								break;

							default:
								break;
						}
						connection->telnet_subneg = 0;
						break;

					default:
						break;
				}
				break;

			case WILL:
				connection->telnet_state = 0;
				switch(data)
				{
					case TELOPT_ECHO:
						connection_send_telnet_control(connection, 3, IAC, DO, TELOPT_ECHO);
						break;

						/* refusing SGA is a violation of RFC 1123 */
					case TELOPT_SGA:
					case TELOPT_EOR:
						connection_send_telnet_control(connection, 3, IAC, DONT, data);
						break;
				}
				break;

			case WONT:
				connection->telnet_state = 0;
				switch(data)
				{
					case TELOPT_ECHO:
					case TELOPT_EOR:
						connection_send_telnet_control(connection, 3, IAC, DONT, data);
						break;
				}
				break;

			case DO:
				connection->telnet_state = 0;
				switch(data)
				{

					case TELOPT_ECHO:
						if(connection->echo == FALSE)
						{
							connection->echo = TRUE;
							connection_send_telnet_control(connection, 3, IAC, WILL, TELOPT_ECHO);
						}
						break;

					case TELOPT_SGA:
						connection_send_telnet_control(connection, 3, IAC, WONT, TELOPT_SGA);
						break;

					case TELOPT_EOR:
					case TELOPT_TTYPE:
						connection_send_telnet_control(connection, 3, IAC, WILL, data);
						break;

					case TELOPT_NAWS:
						connection_send_telnet_control(connection, 3, IAC, WILL, data);
						connection_send_naws(connection);
						break;
				}
				break;

			case DONT:
				connection->telnet_state = 0;
				switch(data)
				{
					case TELOPT_EOR:
						connection_send_telnet_control(connection, 3, IAC, WONT, TELOPT_EOR);
						break;

					case TELOPT_ECHO:
						if(connection->echo)
						{
							connection->echo = FALSE;
							connection_send_telnet_control(connection, 3, IAC, WILL, TELOPT_ECHO);
						}
						break;
				}
				break;

			case SE:
				connection->telnet_state = 0;
				break;

			case SB:
				connection->telnet_subneg = data;
				connection->telnet_state = 0;
				break;
		}

	}

	*to = 0;
	return to - (unsigned char *) buf;
}
