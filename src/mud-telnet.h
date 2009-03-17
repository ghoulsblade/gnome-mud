/* GNOME-Mud - A simple Mud Client
 * mud-telnet.c
 * Code originally from wxMUD. Converted to a GObject by Les Harris.
 * wxMUD - an open source cross-platform MUD client.
 * Copyright (C) 2003-2008 Mart Raudsepp and Gabriel Levin
 * Copyright (C) 2008-2009 Les Harris <lharris@gnome.org>
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

#ifndef MUD_TELNET_H
#define MUD_TELNET_H

G_BEGIN_DECLS

#define MUD_TYPE_TELNET              (mud_telnet_get_type ())
#define MUD_TELNET(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_TELNET, MudTelnet))
#define MUD_TELNET_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_TELNET, MudTelnetClass))
#define MUD_IS_TELNET(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_TELNET))
#define MUD_IS_TELNET_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_TELNET))
#define MUD_TELNET_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_TELNET, MudTelnetClass))
#define MUD_TELNET_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MUD_TYPE_TELNET, MudTelnetPrivate))

#define TEL_SE				240 // End of subnegotiation parameters
#define TEL_NOP				241 // No operation
#define TEL_GA				249 // Go ahead
#define TEL_SB				250 // Indicates that what follows is subnegotiation of the indicated option
#define TEL_WILL			251 // I will use option
#define TEL_WONT			252 // I won't use option
#define TEL_DO				253 // Please, you use this option
#define TEL_DONT			254 // You are not to use this option
#define TEL_IAC				255 // Interpret as command escape sequence - prefix to all telnet commands
                                            // Two IAC's in a row means Data Byte 255

/* RFC 857 - Echo */
#define TELOPT_ECHO			1

/* RFC 1091 - Terminal Type */
#define TELOPT_TTYPE		        24
#	define TEL_TTYPE_IS		0   // Terminal type IS ...
#	define TEL_TTYPE_SEND	        1   // SEND me terminal type

/* RFC 885 - End of Record */
#define TELOPT_EOR			25
#   define TEL_EOR_BYTE                 239 // End of record byte.

/* RFC 1073 - Negotiate About Window Size */
#define TELOPT_NAWS			31

/* RFC 2066 - Charset */
#define TELOPT_CHARSET                  42
#   define TEL_CHARSET_REQUEST          1
#   define TEL_CHARSET_ACCEPT           2
#   define TEL_CHARSET_REJECT           3
#   define TEL_CHARSET_TTABLE_IS        4
#   define TEL_CHARSET_TTABLE_REJECTED  5
#   define TEL_CHARSET_TTABLE_ACK       6
#   define TEL_CHARSET_TTABLE_NAK       7

/* MCCP */
// We do not support COMPRESS1
#define TELOPT_MCCP                     85
#define TELOPT_MCCP2		        86

/* Mud Sound Protocol - http://www.zuggsoft.com/zmud/msp.htm */
#define TELOPT_MSP			90

/* Mud Extension Protocol - http://www.zuggsoft.com/zmud/mxp.htm */
#define TELOPT_MXP                      91

/* Zenith Mud Protocol - http://www.awemud.net/zmp/draft.php */
#define TELOPT_ZMP                      93

/* Mud Server Status Protocol - http://mudbytes.net/index.php?a=topic&t=1336&min=0&num=15*/
#define TELOPT_MSSP                     70
#define TEL_MSSP_VAR                    1
#define TEL_MSSP_VAL                    2

/* RFC 1572 - New Environ */
#define TELOPT_NEWENVIRON               39
#define     TEL_NEWENVIRON_IS           0
#define     TEL_NEWENVIRON_SEND         1
#define     TEL_NEWENVIRON_INFO         2
#define     TEL_NEWENVIRON_VAR          0
#define     TEL_NEWENVIRON_VALUE        1
#define     TEL_NEWENVIRON_ESC          2
#define     TEL_NEWENVIRON_USERVAR      3

#define TEL_SUBREQ_BUFFER_SIZE 16318 
#define TELOPT_STATE_QUEUE_EMPTY	FALSE
#define TELOPT_STATE_QUEUE_OPPOSITE	TRUE

typedef struct _MudTelnet            MudTelnet;
typedef struct _MudTelnetClass       MudTelnetClass;
typedef struct _MudTelnetPrivate     MudTelnetPrivate;

#include <gnet.h>
#include "mud-connection-view.h"
#include "mud-telnet-handler-interface.h"

enum TelnetState
{
    TEL_STATE_TEXT,
    TEL_STATE_IAC,
    TEL_STATE_WILL,
    TEL_STATE_WONT,
    TEL_STATE_DO,
    TEL_STATE_DONT,
    TEL_STATE_SB,
    TEL_STATE_SB_IAC
};

enum TelnetOptionState
{
    TELOPT_STATE_NO = 0,      // bits 00
    TELOPT_STATE_WANTNO = 1,  // bits 01
    TELOPT_STATE_WANTYES = 2, // bits 10
    TELOPT_STATE_YES = 3,     // bits 11
};

struct _MudTelnetClass
{
    GObjectClass parent_class;
};

struct _MudTelnet
{
    GObject parent_instance;

    /*< Private >*/
    MudTelnetPrivate *priv;

    /*< Public >*/
    gboolean ga_received;
    gboolean eor_received;

    MudConnectionView *parent_view;
};

GType mud_telnet_get_type (void);

GString *mud_telnet_process(MudTelnet *telnet, guchar * buf, guint32 count, gint *length);
void mud_telnet_send_sub_req(MudTelnet *telnet, guint32 count, ...);
void mud_telnet_send_raw(MudTelnet *telnet, guint32 count, ...);
MudTelnetHandler *mud_telnet_get_handler(MudTelnet *telnet, gint opt_no);

G_END_DECLS

#endif // MUD_TELNET_H

