/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 2006 Robin Ericsson <lobbin@localhost.nu>
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
 * Copyright (C) 2003-2008 Mart Raudsepp
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

#define TEL_SE				240	// End of subnegotiation parameters
#define TEL_NOP				241	// No operation
#define TEL_GA				249	// Go ahead
#define TEL_SB				250	// Indicates that what follows is subnegotiation of the indicated option
#define TEL_WILL			251	// I will use option
#define TEL_WONT			252	// I won't use option
#define TEL_DO				253	// Please, you use this option
#define TEL_DONT			254	// You are not to use this option
#define TEL_IAC				255	// Interpret as command escape sequence - prefix to all telnet commands
// Two IAC's in a row means Data Byte 255

#define TELOPT_ECHO			  1	// Echo					- RFC  857
#define TELOPT_TTYPE		 24	// Terminal type		- RFC 1091
#	define TEL_TTYPE_IS		  0	// Terminal type IS ...
#	define TEL_TTYPE_SEND	  1	// SEND me terminal type
#define TELOPT_EOR			 25	// End of record		- RFC  885
#   define TEL_EOR_BYTE     239 // End of record byte.
#define TELOPT_NAWS			 31	// Window size			- RFC 1073
#define TELOPT_CHARSET		 42	// Charset				- RFC 2066
#   define TEL_CHARSET_REQUEST              1
#   define TEL_CHARSET_ACCEPT               2
#   define TEL_CHARSET_REJECT               3
#   define TEL_CHARSET_TTABLE_IS            4
#   define TEL_CHARSET_TTABLE_REJECTED      5
#   define TEL_CHARSET_TTABLE_ACK           6
#   define TEL_CHARSET_TTABLE_NAK           7
#define TELOPT_MCCP			 85	// MCCP is not support by GnomeMud.
#define TELOPT_MCCP2		 86	// MCCP2
#define TELOPT_CLIENT		 88	// Client name - from Clandestine MUD protocol
#define TELOPT_CLIENTVER	 89	// Client version - from Clandestine MUD protocol
#define TELOPT_MSP			 90	// MSP - http://www.zuggsoft.com/zmud/msp.htm
#define TELOPT_MXP			 91	// MXP - http://www.zuggsoft.com/zmud/mxp.htm
#define TELOPT_ZMP			 93	// ZMP - http://www.awemud.net/zmp/draft.php

// FIXME: What size should we use?
#define TEL_SUBREQ_BUFFER_SIZE 256
#define TEL_HANDLERS_SIZE 256
#define TELOPT_STATE_QUEUE_EMPTY	FALSE
#define TELOPT_STATE_QUEUE_OPPOSITE	TRUE

typedef struct _MudTelnet            MudTelnet;
typedef struct _MudTelnetClass       MudTelnetClass;
typedef struct _MudTelnetPrivate     MudTelnetPrivate;
typedef struct _MudTelnetBuffer      MudTelnetBuffer;
typedef struct _MudTelnetHandler     MudTelnetHandler;

typedef void(*MudTelnetOnEnableFunc)(MudTelnet *telnet, MudTelnetHandler *handler);
typedef void(*MudTelnetOnDisableFunc)(MudTelnet *telnet, MudTelnetHandler *handler);
typedef void(*MudTelnetOnHandleSubNegFunc)(MudTelnet *telnet,
        guchar *buf, guint len, MudTelnetHandler *handler);

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

enum TelnetHandlerType
{
    HANDLER_NONE,
    HANDLER_TTYPE,
    HANDLER_NAWS,
    HANDLER_ECHO,
    HANDLER_EOR,
    HANDLER_CHARSET,
    HANDLER_ZMP,
    HANDLER_MSP,
    HANDLER_MCCP2
};

struct _MudTelnetClass
{
    GObjectClass parent_class;
};

struct _MudTelnetHandler
{
    enum TelnetHandlerType type;
    guchar option_number;

    gint enabled;

    MudTelnet *instance;

    MudTelnetOnEnableFunc enable;
    MudTelnetOnDisableFunc disable;
    MudTelnetOnHandleSubNegFunc handle_sub_neg;
};

#include <gnet.h>
#include "mud-connection-view.h"
#include "mud-telnet-zmp.h"

#ifdef ENABLE_GST
#include "mud-telnet-msp.h"
#endif

#ifdef ENABLE_MCCP
#include <zlib.h>
typedef struct z_stream_s z_stream;
#endif

struct _MudTelnet
{
    GObject parent_instance;

    MudTelnetPrivate *priv;

    enum TelnetState tel_state;
    guchar subreq_buffer[TEL_SUBREQ_BUFFER_SIZE];
    guint32 subreq_pos;

    guchar telopt_states[256];
    gint eor_enabled;
    gint ttype_iteration;

    GConn *conn;
    MudConnectionView *parent;

    MudTelnetHandler handlers[TEL_HANDLERS_SIZE];

    GHashTable *zmp_commands;
    MudZMPCommand commands[2048];

#ifdef ENABLE_GST
    MudMSPParser msp_parser;
    MudMSPTypes msp_type;
    MudMSPSound sound[2];
    gchar *base_url;
#endif

#ifdef ENABLE_MCCP
    z_stream *compress_out;
    guchar *compress_out_buf;
    gboolean mccp;
    gboolean mccp_new;
    GString *prev_buffer;
#endif

    GString *processed;
    GString *buffer;
    size_t pos;

    gchar *mud_name;
};

GType mud_telnet_get_type (void) G_GNUC_CONST;

MudTelnet *mud_telnet_new(MudConnectionView *parent, GConn *connection, gchar *mud_name);

void mud_telnet_register_handlers(MudTelnet *telnet);
gint mud_telnet_isenabled(MudTelnet *telnet, guint8 option_number, gint him);
GString *mud_telnet_process(MudTelnet *telnet, guchar * buf, guint32 count, gint *length);
void mud_telnet_send_sub_req(MudTelnet *telnet, guint32 count, ...);
void mud_telnet_get_parent_size(MudTelnet *telnet, gint *w, gint *h);
void mud_telnet_send_raw(MudTelnet *telnet, guint32 count, ...);
void mud_telnet_set_parent_naws(MudTelnet *telnet, gint enabled);
void mud_telnet_set_parent_remote_encode(MudTelnet *telnet, gint enabled, gchar *encoding);
void mud_telnet_send_naws(MudTelnet *telnet, gint w, gint h);
void mud_telnet_set_local_echo(MudTelnet *telnet, gint enabled);
void mud_telnet_send_charset_req(MudTelnet *telnet, gchar *encoding);

G_END_DECLS

#endif // MUD_TELNET_H
