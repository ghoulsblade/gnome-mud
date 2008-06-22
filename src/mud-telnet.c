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
#include <stdarg.h>
#include <string.h> // memset

#include "gnome-mud.h"
#include "mud-telnet.h"

#define TELNET_TRACE_MASK _("telnet")

struct _MudTelnetPrivate
{

};

GType mud_telnet_get_type (void);
static void mud_telnet_init (MudTelnet *pt);
static void mud_telnet_class_init (MudTelnetClass *klass);
static void mud_telnet_finalize (GObject *object);

static void mud_telnet_send_iac(MudTelnet *telnet, guchar ch1, guchar ch2);
static void mud_telnet_on_handle_subnego(MudTelnet *telnet);
static void mud_telnet_on_enable_opt(MudTelnet *telnet, 
    const guchar opt_no, gint him);
static void mud_telnet_on_disable_opt(MudTelnet *telnet, 
    const guchar opt_no, gint him);
static guchar mud_telnet_get_telopt_state(guchar *storage, const guint bitshift);
static gint mud_telnet_get_telopt_queue(guchar *storage, const guint bitshift);
static void  mud_telnet_set_telopt_state(guchar *storage, 
    const enum TelnetOptionState state, const guint bitshift);
static void mud_telnet_set_telopt_queue(guchar *storage, 
    gint bit_on, const guint bitshift);
static gint mud_telnet_handle_positive_nego(MudTelnet *telnet,
                                const guchar opt_no,
								const guchar affirmative,
								const guchar negative,
								gint him);
static gint 
mud_telnet_handle_negative_nego(MudTelnet *telnet,
                                const guchar opt_no,
								const guchar affirmative,
								const guchar negative,
								gint him);

// MudTelnet class functions
GType
mud_telnet_get_type (void)
{
	static GType object_type = 0;

	g_type_init();

	if (!object_type)
	{
		static const GTypeInfo object_info =
		{
			sizeof (MudTelnetClass),
			NULL,
			NULL,
			(GClassInitFunc) mud_telnet_class_init,
			NULL,
			NULL,
			sizeof (MudTelnet),
			0,
			(GInstanceInitFunc) mud_telnet_init,
		};

		object_type = g_type_register_static(G_TYPE_OBJECT, "MudTelnet", &object_info, 0);
	}

	return object_type;
}

static void
mud_telnet_init (MudTelnet *telnet)
{
	telnet->priv = g_new0(MudTelnetPrivate, 1);
	
}

static void
mud_telnet_class_init (MudTelnetClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = mud_telnet_finalize;
}

static void
mud_telnet_finalize (GObject *object)
{
	MudTelnet *telnet;
	GObjectClass *parent_class;

    telnet = MUD_TELNET(object);
	
	g_free(telnet->priv);

	parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
	parent_class->finalize(object);
}

/*** Public Methods ***/

// Instantiate MudTelnet
MudTelnet*
mud_telnet_new(MudConnectionView *parent, GConn *connection)
{
	MudTelnet *telnet;
	
	telnet = g_object_new(MUD_TYPE_TELNET, NULL);
	
	telnet->parent = parent;
	telnet->conn = connection;
	// FIXME
	
	memset(telnet->telopt_states, 0, sizeof(telnet->telopt_states));
/*	memset(m_handlers, 0, sizeof(m_handlers));
	*/
	
	return telnet;	
}

gint
mud_telnet_register_handler(MudTelnet *telnet, guint8 option_number, const gchar *classname)
{

    // FIXME
    /*
	TeloptHandlersMap::const_iterator iter = telopt_handler_names.find(option_number);
	if (iter == telopt_handler_names.end())
	{
		// FIXME: Leaks?
		telopt_handler_names[option_number] = wxString(classname);
		//wxPrintf(wxT("class '%s' registered as handling option %u\n"), classname, option_number);
		return TRUE;
	}
	else
		// Already registered.
		// FIXME: Should we consider re-registering and handling properly the already existing
		// FIXME: handler instances in m_handlers[option_number]?
		return FALSE;
	*/
	
	return FALSE;
}

gint
mud_telnet_isenabled(MudTelnet *telnet, guint8 option_number, gint him)
{
    // FIXME!!
	/*TeloptHandlersMap::const_iterator iter = telopt_handler_names.find(option_number);

	if (iter != telopt_handler_names.end() && (*iter).second)
	{
		if (!m_handlers[option_number])
			m_handlers[option_number] = (TelnetHandler*)wxCreateDynamicObject((*iter).second);

		return m_handlers[option_number]->IsEnabled();
	}
	else
		return FALSE;*/
		
	return FALSE;
}

MudTelnetBuffer 
mud_telnet_process(MudTelnet *telnet, guchar * buf, guint32 count)
{
	guchar processed[32768];
	size_t pos = 0;
	guint32 i;
	MudTelnetBuffer ret;
	g_assert(telnet != NULL);

	for (i = 0;i < count;++i)
	{
		switch (telnet->tel_state)
		{
			case TEL_STATE_TEXT:
			    g_message("Text state\n");
				if (buf[i] == (guchar)TEL_IAC)
					telnet->tel_state = TEL_STATE_IAC;
				else
					processed[pos++] = buf[i];
				break;

			case TEL_STATE_IAC:
			    g_message("IAC\n");
				switch (buf[i])
				{
					case (guchar)TEL_IAC:
					    g_message("\tIAC\n");
						processed[pos++] = buf[i];
						telnet->tel_state = TEL_STATE_TEXT;
						break;

					case (guchar)TEL_DO:
					    g_message("\tDO\n");
						telnet->tel_state = TEL_STATE_DO;
						break;

					case (guchar)TEL_DONT:
					    g_message("\tDONT\n");
						telnet->tel_state = TEL_STATE_DONT;
						break;

					case (guchar)TEL_WILL:
					    g_message("\tWILL\n");
						telnet->tel_state = TEL_STATE_WILL;
						break;

					case (guchar)TEL_WONT:
					    g_message("\tWONT\n");
						telnet->tel_state = TEL_STATE_WONT;
						break;

					case (guchar)TEL_SB:
						g_message("Receiving subrequest\n");
						telnet->tel_state = TEL_STATE_SB;
						telnet->subreq_pos = 0;
						break;

					default:
						g_warning("Illegal IAC command %d received\n", buf[i]);
						telnet->tel_state = TEL_STATE_TEXT;
						break;
				}
				break;

			case TEL_STATE_DO:
			    g_message("STATE_DO\n");
				mud_telnet_handle_positive_nego(telnet, buf[i], 
				    (guchar)TEL_WILL, (guchar)TEL_WONT, FALSE);
				telnet->tel_state = TEL_STATE_TEXT;

			case TEL_STATE_WILL:
			    g_message("STATE_WILL\n");
				mud_telnet_handle_positive_nego(telnet, buf[i], 
				    (guchar)TEL_DO, (guchar)TEL_DONT, TRUE);
				telnet->tel_state = TEL_STATE_TEXT;
				break;

			case TEL_STATE_DONT:
			    g_message("STATE_DONT\n");
				mud_telnet_handle_negative_nego(telnet, 
				    buf[i], (guchar)TEL_WILL, (guchar)TEL_WONT, FALSE);
				telnet->tel_state = TEL_STATE_TEXT;
				break;

			case TEL_STATE_WONT:
			    g_message("STATE_WONT\n");
				mud_telnet_handle_negative_nego(telnet, 
				    buf[i], (guchar)TEL_DO, (guchar)TEL_DONT, TRUE);
				telnet->tel_state = TEL_STATE_TEXT;
				break;

			case TEL_STATE_SB:
			    g_message("STATE_SB\n");
				if (buf[i] == (guchar)TEL_IAC)
					telnet->tel_state = TEL_STATE_SB_IAC;
				else
				{
					if (telnet->subreq_pos == 0)
						g_message("Subrequest for option %d\n", buf[i]);

					// FIXME: Handle overflow
					if (telnet->subreq_pos >= TEL_SUBREQ_BUFFER_SIZE)
					{
						g_warning("Subrequest buffer full. Oddities in output will happen. Sorry.\n");
						telnet->subreq_pos = 0;
						telnet->tel_state = TEL_STATE_TEXT;
					}
					else
						telnet->subreq_buffer[telnet->subreq_pos++] = buf[i];
				}
				break;

			case TEL_STATE_SB_IAC:
			    g_message("STATE_SB_IAC\n");
				if (buf[i] == (guchar)TEL_IAC)
				{
					if (telnet->subreq_pos >= TEL_SUBREQ_BUFFER_SIZE)
					{
						g_warning("Subrequest buffer full. Oddities in output will happen. Sorry.\n");
						telnet->subreq_pos = 0;
						telnet->tel_state = TEL_STATE_TEXT;
					}
					else
						telnet->subreq_buffer[telnet->subreq_pos++] = buf[i];

					telnet->tel_state = TEL_STATE_SB;
				}
				else if (buf[i] == (guchar)TEL_SE)
				{
				    g_message("STATE_TEL_SE\n");
					g_message("Subrequest for option %d succesfully received with length %d\n", 
					    telnet->subreq_buffer[0], telnet->subreq_pos-1);
					g_message("Subreq buffer content: %d %d %d %d\n", 
					    telnet->subreq_buffer[0], telnet->subreq_buffer[1], 
					    telnet->subreq_buffer[2], telnet->subreq_buffer[3]);
					    
					mud_telnet_on_handle_subnego(telnet);
					telnet->tel_state = TEL_STATE_TEXT;
				} else {
					g_warning("Erronous byte %d after an IAC inside a subrequest\n", buf[i]);
					telnet->subreq_pos = 0;
					telnet->tel_state = TEL_STATE_TEXT;
				}
				break;
		}
	}
		
	ret.buffer = processed;
	ret.len = pos;
	
	return ret;
}

gchar*
mud_telnet_get_telnet_string(guchar ch)
{
    GString *string = g_string_new(NULL);
    gchar *ret = string->str;
    
	switch (ch)
	{
		case TEL_WILL:
			g_string_append(string, "WILL");
		case TEL_WONT:
			g_string_append(string, "WONT");
		case TEL_DO:
			g_string_append(string, "DO");
		case TEL_DONT:
			g_string_append(string, "DONT");
		case TEL_IAC:
			g_string_append(string, "IAC");
		default:
			g_string_append_c(string,ch);
	}
	
	g_string_free(string, FALSE);
	
	return ret;
}

gchar* 
mud_telnet_get_telopt_string(guchar ch)
{
    GString *string = g_string_new(NULL);
    gchar *ret = string->str;
    
	switch (ch)
	{
		case TELOPT_ECHO:
			g_string_append(string, "ECHO");
		case TELOPT_TTYPE:
			g_string_append(string, "TTYPE");
		case TELOPT_EOR:
			g_string_append(string, "END-OF-RECORD");
		case TELOPT_NAWS:
			g_string_append(string, "NAWS");
		case TELOPT_CHARSET:
			g_string_append(string, "CHARSET");
		case TELOPT_MCCP:
			g_string_append(string, "MCCP");
		case TELOPT_MCCP2:
			g_string_append(string, "MCCPv2");
		case TELOPT_CLIENT:
			g_string_append(string, "CLIENT");
		case TELOPT_CLIENTVER:
			g_string_append(string, "CLIENTVER");
		case TELOPT_MSP:
			g_string_append(string, "MSP");
		case TELOPT_MXP:
			g_string_append(string, "MXP");
		case TELOPT_ZMP:
			g_string_append(string, "ZMP");
		default:
			g_string_append_c(string, ch);
	}
	
	g_string_free(string, FALSE);
	
	return ret;
}

void 
mud_telnet_send_sub_req(MudTelnet *telnet, guint32 count, ...)
{
	guchar byte;
	guint32 i;
	va_list va;
	va_start(va, count);

	byte = (guchar)TEL_IAC;
	
	gnet_conn_write(telnet->conn, (gchar *)&byte, 1);
	byte = (guchar)TEL_SB;
	gnet_conn_write(telnet->conn, (gchar *)&byte, 1);

	for (i = 0; i < count; ++i)
	{
		byte = (guchar)va_arg(va, gint);
		gnet_conn_write(telnet->conn, (gchar *)&byte, 1);
		
		if (byte == (guchar)TEL_IAC)
			gnet_conn_write(telnet->conn, (gchar *)&byte, 1);
	}

	va_end(va);

	byte = (guchar)TEL_IAC;
	gnet_conn_write(telnet->conn, (gchar *)&byte, 1);
	byte = (guchar)TEL_SE;
	gnet_conn_write(telnet->conn, (gchar *)&byte, 1);
}

/*** Private Methods ***/
static void 
mud_telnet_send_iac(MudTelnet *telnet, guchar ch1, guchar ch2)
{
	guchar buf[3];
	buf[0] = (guchar)TEL_IAC;
	buf[1] = ch1;
	buf[2] = ch2;
	
	// Replace with gnet equivalent
	gnet_conn_write(telnet->conn, (gchar *)buf, 3);
}

static void
mud_telnet_on_handle_subnego(MudTelnet *telnet)
{
	if (telnet->subreq_pos < 1) 
	    return;

	/*if (mud_telnet_is_enabled(telnet, subreq_buffer[0], FALSE))
		m_handlers[subreq_buffer[0]]->HandleSubNego(this, subreq_buffer + 1, subreq_pos - 1);
	*/
}

static void
mud_telnet_on_enable_opt(MudTelnet *telnet, const guchar opt_no, gint him)
{
	if (telnet->subreq_pos < 1) 
	    return;

	/*if (mud_telnet_is_enabled(telnet, subreq_buffer[0], FALSE))
		m_handlers[subreq_buffer[0]]->HandleSubNego(this, subreq_buffer + 1, subreq_pos - 1);
	*/
}

static void
mud_telnet_on_disable_opt(MudTelnet *telnet, const guchar opt_no, gint him)
{
	if (telnet->subreq_pos < 1) 
	    return;

	/*if (mud_telnet_is_enabled(telnet, subreq_buffer[0], FALSE))
		m_handlers[subreq_buffer[0]]->HandleSubNego(this, subreq_buffer + 1, subreq_pos - 1);
	*/
}

static guchar
mud_telnet_get_telopt_state(guint8 *storage, const guint bitshift)
{
	return (*storage >> bitshift) & 0x03u;
}

static gint
mud_telnet_get_telopt_queue(guchar *storage, const guint bitshift)
{
	return !!((*storage >> bitshift) & 0x04u);
}

static void 
mud_telnet_set_telopt_state(guchar *storage, const enum TelnetOptionState state,
						    const guint bitshift)
{
	*storage = (*storage & ~(0x03u << bitshift)) | (state << bitshift);
}

static void
mud_telnet_set_telopt_queue(guchar *storage, gint bit_on, const guint bitshift)
{
	*storage = bit_on ? (*storage | (0x04u << bitshift)) : (*storage & ~(0x04u << bitshift));
}

// Otherwise handlers called on state changes could see the wrong options
// (I think theoretically they should not care at all, but in practice...)
static gint
mud_telnet_handle_positive_nego(MudTelnet *telnet,
                                const guchar opt_no,
								const guchar affirmative,
								const guchar negative,
								gint him)
{
	const guint bitshift = him ? 4 : 0;
	guchar * opt = &(telnet->telopt_states[opt_no]);

	switch (mud_telnet_get_telopt_state(opt, bitshift))
	{
		case TELOPT_STATE_NO:
			// If we agree that server should enable telopt, set
			// his state to YES and send DO; otherwise send DONT
			// FIXME-US/HIM
			// FIXME: What to do in the opposite "him" gint value case?
			if (mud_telnet_isenabled(telnet, opt_no, him))
			{
				mud_telnet_set_telopt_state(opt, TELOPT_STATE_YES, bitshift);
				mud_telnet_send_iac(telnet, affirmative, opt_no);
				mud_telnet_on_enable_opt(telnet, opt_no, him);
				return TRUE;
			} else {
				// printf("Server, you DONT do %d\n", opt_no);
				mud_telnet_send_iac(telnet, negative, opt_no);
				return FALSE;
			}

		case TELOPT_STATE_YES:
			// Ignore, he already supposedly has it enabled. Includes the case where
			// DONT was answered by WILL with himq = OPPOSITE to prevent loop.
			return FALSE;

		case TELOPT_STATE_WANTNO:
			if (mud_telnet_get_telopt_queue(opt, bitshift) == TELOPT_STATE_QUEUE_EMPTY)
			{
				mud_telnet_set_telopt_state(opt, TELOPT_STATE_NO, bitshift);
				g_message("TELNET NEGOTIATION: DONT answered by WILL; ill-behaved server. Ignoring IAC WILL %d. him = NO\n", opt_no);
				return FALSE;
			} else { // The opposite is queued
				mud_telnet_set_telopt_state(opt, TELOPT_STATE_YES, bitshift);
				mud_telnet_set_telopt_queue(opt, TELOPT_STATE_QUEUE_EMPTY, bitshift);
				g_message("TELNET NEGOTIATION: DONT answered by WILL; ill-behaved server. Ignoring IAC WILL %d. him = YES, himq = EMPTY\n", opt_no);
				return FALSE;
			}
			break;

		case TELOPT_STATE_WANTYES:
			if (mud_telnet_get_telopt_queue(opt, bitshift) == TELOPT_STATE_QUEUE_EMPTY)
			{
				mud_telnet_set_telopt_state(opt, TELOPT_STATE_YES, bitshift);
				mud_telnet_on_enable_opt(telnet, opt_no, him);
				return TRUE;
			} else { // The opposite is queued
				mud_telnet_set_telopt_state(opt, TELOPT_STATE_WANTNO, bitshift);
				mud_telnet_set_telopt_queue(opt, TELOPT_STATE_QUEUE_EMPTY, bitshift);
				//printf("Server, you should make up your mind. Fine DONT do %d then.\n", opt_no);
				mud_telnet_send_iac(telnet, negative, opt_no);
				return FALSE;
			}
		default:
			g_warning("Something went really wrong\n");
			return FALSE;
	}
}

static gint 
mud_telnet_handle_negative_nego(MudTelnet *telnet,
                                const guchar opt_no,
								const guchar affirmative,
								const guchar negative,
								gint him)
{
	const guint bitshift = him ? 4 : 0;
	guchar * opt = &(telnet->telopt_states[opt_no]);

	switch (mud_telnet_get_telopt_state(opt, bitshift))
	{
		case TELOPT_STATE_NO:
			// Ignore, he already supposedly has it disabled
			return FALSE;

		case TELOPT_STATE_YES:
			mud_telnet_set_telopt_state(opt, TELOPT_STATE_NO, bitshift);
			//printf("Ok, server, I ACK that you DONT do %d\n", opt_no);
			mud_telnet_send_iac(telnet, negative, opt_no);
			mud_telnet_on_disable_opt(telnet, opt_no, him);
			return TRUE;

		case TELOPT_STATE_WANTNO:
			if (mud_telnet_get_telopt_queue(opt, bitshift) == TELOPT_STATE_QUEUE_EMPTY)
			{
				mud_telnet_set_telopt_state(opt, TELOPT_STATE_NO, bitshift);
				return FALSE;
			} else { // but the opposite is queued...
				mud_telnet_set_telopt_state(opt, TELOPT_STATE_WANTYES, bitshift);
				mud_telnet_set_telopt_queue(opt, TELOPT_STATE_QUEUE_EMPTY, bitshift);
				//printf("Ok, server, you are making me angry. DO %d", opt_no);
				mud_telnet_send_iac(telnet, affirmative, opt_no);
				mud_telnet_on_enable_opt(telnet, opt_no, him); // FIXME: Is this correct?
				return TRUE;
			}

		case TELOPT_STATE_WANTYES:
			if (mud_telnet_get_telopt_queue(opt, bitshift) == TELOPT_STATE_QUEUE_EMPTY)
			{
				mud_telnet_set_telopt_state(opt, TELOPT_STATE_NO, bitshift);
				return FALSE;
			} else { // The opposite is queued
				mud_telnet_set_telopt_state(opt, TELOPT_STATE_NO, bitshift);
				mud_telnet_set_telopt_queue(opt, TELOPT_STATE_QUEUE_EMPTY, bitshift);
				return FALSE;
			}
		default:
			g_warning("TELNET NEGOTIATION: Something went really wrong\n");
			return FALSE;
	}
}

