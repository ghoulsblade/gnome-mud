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
#include <stdio.h>
#include <unistd.h>

#ifdef HAVE_TELNET_H
#include <telnet.h>
#endif
#ifdef HAVE_ARPA_TELNET_H
#include <arpa/telnet.h>
#endif

#include "amcl.h"

static char const rcsid[] = "$Id$";

/* Added by Benjamin Curtis
   Code Swiped from TUsh by Simon Marsh */
char *pre_process(char *buf, CONNECTION_DATA *connection)
{
  unsigned char *from, *to;
  char pling[3];
  int len=0;
  
  from = (unsigned char *)buf;
  to   = (unsigned char *)buf;
  
  while (*from) {
    switch(*from) {
      
      /* got a telnet control char */
    case IAC: /* switch for telnet control */
      from++;
      switch(*from++) {
	
	/* IP kill connection */
      case IP: /* IP control */
	disconnect(NULL,NULL);
	return 0;
	break;
	
	/* prompt processing stuff */
      case GA: /* Go ahead (prompt) */
	*to++=255;
	len++;
	break;
	
      case EOR: /* EOR  (prompt)  */
	*to++=255;
	len++;
	break;
	
	/* WILL processing */
      case WILL: /* WILL control */
	switch(*from++) {
	case TELOPT_ECHO:
	  if (connection->echo == TRUE) { 
	    connection->echo = FALSE; 
	    write(connection->sockfd,"\377\375\001",3);
	  }
	  break;
	  
	case TELOPT_SGA:
	  write(connection->sockfd,"\377\376\003",3);
	  break;
	  
	case TELOPT_EOR:
	  write(connection->sockfd,"\377\375\031",3);
	  break;
	}
	break;
	
	/* WONT processing ... */
      case WONT: /* WONT control */
	switch(*from++) {
	case TELOPT_ECHO:
	  if (connection->echo == FALSE) {
	    connection->echo = TRUE;
	    write(connection->sockfd,"\377\376\001",3);
	  }
	  break;
	  
	case TELOPT_SGA:
	  break;
	  
	case TELOPT_EOR:
	  write(connection->sockfd,"\377\376\031",3);
	  break;
	}
	break;
	
	/* DO processing ... received request to do something ... */
      case DO:
	switch(*from) {
	case TELOPT_ECHO:
	  if (connection->echo == FALSE) {
	    write(connection->sockfd,"\377\373\001",3);
	    from++;
	  }
	  break;
	  
	case TELOPT_SGA:
	  write(connection->sockfd,"\377\374\003",3);
	  from++;
	  break;
	  
	case TELOPT_EOR:
	  write(connection->sockfd,"\377\373\031",3);
	  break;
	  
	default:
	  pling[0]='\377';
	  pling[1]='\374';
	  pling[2]=*from++;
	  write(connection->sockfd,pling,3);
	  break;
	}
	break;
	
	/* DONT processing ... received request not to do something ... */
      case DONT:
	switch(*from++) {
	case TELOPT_ECHO:
	  if (connection->echo == TRUE) {
	    connection->echo = FALSE;
	    write(connection->sockfd,"\377\374\001",3);
	  }
	  break;
	  
	case TELOPT_SGA:
	  break;
	  
	case TELOPT_EOR:
	  write(connection->sockfd,"\377\374\031",3);
	}
	break;
	
      }
      break;
      
    case '\r':
      if (*(from+1)!='\n') {
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
  
  return (gchar *) len;
}
