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

gint pre_process(char *buf, CONNECTION_DATA *connection)
{
	unsigned char *from, *to, option, subneg;
	int len;
  
	subneg = len = 0 ;

	from = (unsigned char *)buf;
	to   = (unsigned char *)buf;
  
	while (*from)
	{
		switch(*from)
		{
			/* got a telnet control char */
			case IAC: /* switch for telnet control */
				from++;
				switch(*from++)
				{
					/* IP kill connection */
					case IP: /* IP control */
						disconnect(NULL,NULL);
						return 0;
						break;
	
					/* prompt processing stuff */
					case GA: /* Go ahead (prompt) */
						*to++ = '\0';
						len++;
						break;
	
					case EOR: /* EOR (prompt) */
						*to++ = '\0';
						len++;
						break;
	
					case SB: /* SB (subnegotiation) */
						switch (*from++)
						{
							case TELOPT_TTYPE:
								if ( (*from++) == TELQUAL_SEND )
								{
									subneg = TELOPT_TTYPE ;
								}
								break ;

							default:
								break ;
						}
						break;

					case SE: /* SE (end subnegotiation) */
						switch (subneg)
						{
							case TELOPT_TTYPE:
								{
									int pos;
									unsigned char pkt[64] = { IAC, SB, TELOPT_TTYPE, TELQUAL_IS };
			
									/* if strlen(TERM) > 50 you have issues */
									strncpy(&pkt[4], (unsigned char *)prefs.TerminalType, 50);

									pos = (4 + strlen(prefs.TerminalType)) - 1;
									pkt[pos + 1] = IAC;
									pkt[pos + 2] = SE;

									write(connection->sockfd, &pkt, pos + 3);
								}
								break;

							default:
							break ;
						}

						subneg = 0 ;
						break ;

					/* WILL processing */
					case WILL: /* WILL control */
						switch(*from++)
						{
							case TELOPT_ECHO:
								connection_send_telnet_control(connection, 3, IAC, DO, TELOPT_ECHO);
								break;
	  
							case TELOPT_SGA: /* suppress GA */
								connection_send_telnet_control(connection, 3, IAC, DONT, TELOPT_SGA);
								break;
  
							case TELOPT_EOR:
								connection_send_telnet_control(connection, 3, IAC, DO, TELOPT_EOR);
								break;
						}
						break;
	
					/* WONT processing ... */
					case WONT: /* WONT control */
						switch(*from++)
						{
							case TELOPT_ECHO:
								connection_send_telnet_control(connection, 3, IAC, DONT, TELOPT_ECHO);
								break;
  
							case TELOPT_SGA:
								break;
	  
							case TELOPT_EOR:
								connection_send_telnet_control(connection, 3, IAC, DONT, TELOPT_EOR);
								break;
						}
						break;
	
					/* DO processing ... received request to do something ... */
					case DO:
						option = *from;

						switch(*from++)
						{
							case TELOPT_ECHO:
								if (connection->echo == FALSE)
								{
									connection->echo = TRUE;
									connection_send_telnet_control(connection, 3, IAC, WILL, TELOPT_ECHO);
								}
								break;
	  
							case TELOPT_SGA:
								connection_send_telnet_control(connection, 3, IAC, WONT, TELOPT_SGA);
								break;
	  
							case TELOPT_EOR:
								connection_send_telnet_control(connection, 3, IAC, WILL, TELOPT_EOR);
								break;
	  
							case TELOPT_TTYPE:
								connection_send_telnet_control(connection, 3, IAC, WILL, TELOPT_TTYPE);
								break ;

							default:
								connection_send_telnet_control(connection, 3, IAC, WONT, option) ;
								break;
						}
						break;
	
					/* DONT processing ... received request not to do something ... */
					case DONT:
						switch(*from++)
						{
							case TELOPT_ECHO:
								if (connection->echo == TRUE)
								{
									connection->echo = FALSE;
									connection_send_telnet_control(connection, 3, IAC, WONT, TELOPT_ECHO);
								}
								break;
	  
							case TELOPT_SGA:
								break;
	  
							case TELOPT_EOR:
								connection_send_telnet_control(connection, 3, IAC, WONT, TELOPT_EOR);
								break;
						}
						break;
	
				}
				break;
      
			case '\r':
				if (*(from+1)!='\n')
				{
					len++;
					*to++='\n';
				}
				from++;
				break;
				
			case '\n':
				if (*(from+1)=='\r') from++;
				len++;
				*to++='\n';
				from++;
				break;
				
			default:
				len++;
				*to++ = *from++;
				break;
		}
	}
	
	*to++=0;
  
	return len;
}
