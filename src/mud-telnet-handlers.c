/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1998-2006 Robin Ericsson <lobbin@localhost.nu>
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

/* Code originally from wxMUD. Converted to a GObject by Les Harris.
 * wxMUD - an open source cross-platform MUD client.
 * Copyright (C) 2003-2008 Mart Raudsepp and Gabriel Levin
 */
 
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <gnet.h>

#include "gnome-mud.h"
#include "mud-telnet.h"
#include "mud-telnet-handlers.h"

/* TTYPE */

void 
MudHandler_TType_Enable(MudTelnet *telnet, MudTelnetHandler *handler)
{
    handler->enabled = TRUE;
}

void 
MudHandler_TType_Disable(MudTelnet *telnet, MudTelnetHandler *handler)
{
    handler->enabled = FALSE;
}

void 
MudHandler_TType_HandleSubNeg(MudTelnet *telnet, guchar *buf,
                              guint len, MudTelnetHandler *handler)
{
	if (len == 1 && buf[0] == TEL_TTYPE_SEND)
	{
	    mud_telnet_send_sub_req(telnet, 11, (guchar)TELOPT_TTYPE,
	                                       (guchar)TEL_TTYPE_IS,
	                                       'g','n','o','m','e','-','m','u','d');
	}
}

/* NAWS */
void 
MudHandler_NAWS_Enable(MudTelnet *telnet, MudTelnetHandler *handler)
{
    gint w, h;
    
    mud_telnet_get_parent_size(telnet, &w, &h);
    mud_telnet_set_parent_naws(telnet, TRUE);
    
    handler->enabled = TRUE;
    
    mud_telnet_send_naws(telnet, w, h);
}

void 
MudHandler_NAWS_Disable(MudTelnet *telnet, MudTelnetHandler *handler)
{
    handler->enabled = FALSE;
    mud_telnet_set_parent_naws(telnet, FALSE);
    
    g_message("Disabled NAWS.");
}

void 
MudHandler_NAWS_HandleSubNeg(MudTelnet *telnet, guchar *buf, 
    guint len, MudTelnetHandler *handler)
{
    return;   
}

/* ECHO */
void 
MudHandler_ECHO_Enable(MudTelnet *telnet, MudTelnetHandler *handler)
{
    mud_telnet_set_local_echo(telnet, FALSE);
    g_message("Enabled Serverside ECHO");
}

void 
MudHandler_ECHO_Disable(MudTelnet *telnet, MudTelnetHandler *handler)
{
    mud_telnet_set_local_echo(telnet, TRUE);
    g_message("Disabled Serverside ECHO.");
}

void 
MudHandler_ECHO_HandleSubNeg(MudTelnet *telnet, guchar *buf, 
    guint len, MudTelnetHandler *handler)
{
    return;   
}

/* EOR */
void 
MudHandler_EOR_Enable(MudTelnet *telnet, MudTelnetHandler *handler)
{
    telnet->eor_enabled = TRUE;
    g_message("Enabled EOR");
}

void 
MudHandler_EOR_Disable(MudTelnet *telnet, MudTelnetHandler *handler)
{
    telnet->eor_enabled = FALSE;
    g_message("Disabled EOR");
}

void 
MudHandler_EOR_HandleSubNeg(MudTelnet *telnet, guchar *buf, 
    guint len, MudTelnetHandler *handler)
{
    return;   
}
