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

#ifndef MUD_TELNET_HANDLERS_H
#define MUD_TELNET_HANDLERS_H

#include "mud-telnet.h"

/* TTYPE */
void MudHandler_TType_Enable(MudTelnet *telnet, MudTelnetHandler *handler);
void MudHandler_TType_Disable(MudTelnet *telnet, MudTelnetHandler *handler);
void MudHandler_TType_HandleSubNeg(MudTelnet *telnet, guchar *buf,
    guint len, MudTelnetHandler *handler);

/* NAWS */
void MudHandler_NAWS_Enable(MudTelnet *telnet, MudTelnetHandler *handler);
void MudHandler_NAWS_Disable(MudTelnet *telnet, MudTelnetHandler *handler);
void MudHandler_NAWS_HandleSubNeg(MudTelnet *telnet, guchar *buf,
    guint len, MudTelnetHandler *handler);

/* ECHO */
void MudHandler_ECHO_Enable(MudTelnet *telnet, MudTelnetHandler *handler);
void MudHandler_ECHO_Disable(MudTelnet *telnet, MudTelnetHandler *handler);
void MudHandler_ECHO_HandleSubNeg(MudTelnet *telnet, guchar *buf,
    guint len, MudTelnetHandler *handler);

/* EOR */
void MudHandler_EOR_Enable(MudTelnet *telnet, MudTelnetHandler *handler);
void MudHandler_EOR_Disable(MudTelnet *telnet, MudTelnetHandler *handler);
void MudHandler_EOR_HandleSubNeg(MudTelnet *telnet, guchar *buf,
    guint len, MudTelnetHandler *handler);

/* CHARSET */
void MudHandler_CHARSET_Enable(MudTelnet *telnet, MudTelnetHandler *handler);
void MudHandler_CHARSET_Disable(MudTelnet *telnet, MudTelnetHandler *handler);
void MudHandler_CHARSET_HandleSubNeg(MudTelnet *telnet, guchar *buf,
    guint len, MudTelnetHandler *handler);

/* ZMP */
void MudHandler_ZMP_Enable(MudTelnet *telnet, MudTelnetHandler *handler);
void MudHandler_ZMP_Disable(MudTelnet *telnet, MudTelnetHandler *handler);
void MudHandler_ZMP_HandleSubNeg(MudTelnet *telnet, guchar *buf,
    guint len, MudTelnetHandler *handler);

#ifdef ENABLE_GST
/* MSP */
void MudHandler_MSP_Enable(MudTelnet *telnet, MudTelnetHandler *handler);
void MudHandler_MSP_Disable(MudTelnet *telnet, MudTelnetHandler *handler);
void MudHandler_MSP_HandleSubNeg(MudTelnet *telnet, guchar *buf,
    guint len, MudTelnetHandler *handler);
#endif

#endif // MUD_TELNET_HANDLERS_H
