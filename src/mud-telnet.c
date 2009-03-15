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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <gnet.h>
#include <stdarg.h>
#include <string.h> // memset

#include "gnome-mud.h"
#include "mud-telnet.h"

/* Handlers */
#include "mud-telnet-handler-interface.h"
#include "mud-telnet-ttype.h"
#include "mud-telnet-naws.h"
#include "mud-telnet-echo.h"
#include "mud-telnet-eor.h"
#include "mud-telnet-charset.h"
#include "mud-telnet-zmp.h"
#include "mud-telnet-mssp.h"
#include "mud-telnet-new-environ.h"

#ifdef ENABLE_GST
#include "mud-telnet-msp.h"
#endif

#ifdef ENABLE_MCCP
#include "mud-telnet-mccp.h"
#endif

struct _MudTelnetPrivate
{
    GConn *conn;
    gchar *mud_name;

    enum TelnetState tel_state;
    GHashTable *handlers;

    guchar telopt_states[256];

    guint32 subreq_pos;
    guchar subreq_buffer[TEL_SUBREQ_BUFFER_SIZE];

    GString *processed;
    GString *buffer;
    size_t pos;
};

/* Property Identifiers */
enum
{
    PROP_MUD_TELNET_0,
    PROP_PARENT_VIEW,
    PROP_GA_RECEIVED,
    PROP_EOR_RECEIVED,
    PROP_CONNECTION
};

/* Create the Type */
G_DEFINE_TYPE(MudTelnet, mud_telnet, G_TYPE_OBJECT);

/* Class Functions */
static void mud_telnet_init (MudTelnet *pt);
static void mud_telnet_class_init (MudTelnetClass *klass);
static void mud_telnet_finalize (GObject *object);
static GObject *mud_telnet_constructor (GType gtype,
                                        guint n_properties,
                                        GObjectConstructParam *properties);
static void mud_telnet_set_property(GObject *object,
                                    guint prop_id,
                                    const GValue *value,
                                    GParamSpec *pspec);
static void mud_telnet_get_property(GObject *object,
                                    guint prop_id,
                                    GValue *value,
                                    GParamSpec *pspec);

/* Private Methods */
static gchar *mud_telnet_get_telopt_string(guchar c);
static gchar *mud_telnet_get_telnet_string(guchar c);

static void mud_telnet_register_handlers(MudTelnet *telnet);
static void mud_telnet_send_iac(MudTelnet *telnet, guchar ch1, guchar ch2);

static gint   mud_telnet_get_telopt_queue(guchar *storage, const guint bitshift);
static guchar mud_telnet_get_telopt_state(guchar *storage, const guint bitshift);
static void   mud_telnet_set_telopt_state(guchar *storage,
					  const enum TelnetOptionState state,
                                          const guint bitshift);

static void mud_telnet_set_telopt_queue    (guchar *storage,
					    gint bit_on,
                                            const guint bitshift);

static gint mud_telnet_handle_positive_nego(MudTelnet *telnet,
                                            const guchar opt_no,
                                            const guchar affirmative,
                                            const guchar negative,
                                            gint him);

static gint mud_telnet_handle_negative_nego(MudTelnet *telnet,
                                            const guchar opt_no,
                                            const guchar affirmative,
                                            const guchar negative,
                                            gint him);

static void mud_telnet_on_handle_subnego   (MudTelnet *telnet);

static void mud_telnet_on_enable_opt       (MudTelnet *telnet,
				            const guchar opt_no);

static void mud_telnet_on_disable_opt      (MudTelnet *telnet,
				            const guchar opt_no);


static gboolean mud_telnet_isenabled       (MudTelnet *telnet,
                                            gint option_number);


static void mud_telnet_unref_handler       (gpointer data);


/* Class Functions */
static void
mud_telnet_class_init (MudTelnetClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object constructor */
    object_class->constructor = mud_telnet_constructor;

    /* Override base object's finalize */
    object_class->finalize = mud_telnet_finalize;

    /* Override base object property methods */
    object_class->set_property = mud_telnet_set_property;
    object_class->get_property = mud_telnet_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(MudTelnetPrivate));

    /* Install Properties */
    g_object_class_install_property(object_class,
            PROP_PARENT_VIEW,
            g_param_spec_object("parent-view",
                "parent view",
                "the parent mudconnectionview",
                MUD_TYPE_CONNECTION_VIEW,
                G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property(object_class,
            PROP_GA_RECEIVED,
            g_param_spec_boolean("ga-received",
                "ga received",
                "set if GA has been received",
                FALSE,
                G_PARAM_READABLE));

    g_object_class_install_property(object_class,
            PROP_EOR_RECEIVED,
            g_param_spec_boolean("eor-received",
                "eor received",
                "set if EOR has been received",
                FALSE,
                G_PARAM_READABLE));

    g_object_class_install_property(object_class,
            PROP_CONNECTION,
            g_param_spec_pointer("connection",
                "Connection",
                "The Connection Object to the MUD",
                G_PARAM_READABLE));
}

static void
mud_telnet_init (MudTelnet *telnet)
{
    /* Get our private data */
    telnet->priv = MUD_TELNET_GET_PRIVATE(telnet);

    /* Set some defaults */

    telnet->parent_view = NULL;
    telnet->ga_received = FALSE;
    telnet->eor_received = FALSE;

    telnet->priv->pos = 0;
    telnet->priv->buffer = NULL;
    telnet->priv->subreq_pos = 0;
    telnet->priv->tel_state = TEL_STATE_TEXT;
    telnet->priv->processed = g_string_new(NULL);
}

static GObject *
mud_telnet_constructor (GType gtype,
                        guint n_properties,
                        GObjectConstructParam *properties)
{
    MudTelnet *self;
    GObject *obj;
    MudTelnetClass *klass;
    GObjectClass *parent_class;

    /* Chain up to parent constructor */
    klass = MUD_TELNET_CLASS( g_type_class_peek(MUD_TYPE_TELNET) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    self = MUD_TELNET(obj);

    if(!self->parent_view)
    {
        g_printf("ERROR: Tried to instantiate MudTelnet without passing parent view\n");
        g_error("Tried to instantiate MudTelnet without passing parent view");
    }

    self->priv->handlers = g_hash_table_new_full(g_direct_hash,
                                                 g_direct_equal,
                                                 NULL,
                                                 mud_telnet_unref_handler);

    memset(self->priv->telopt_states, 0, sizeof(self->priv->telopt_states));

    mud_telnet_register_handlers(self);

    g_object_get(self->parent_view,
                 "connection", &self->priv->conn,
                 "mud-name", &self->priv->mud_name,
                 NULL);

    return obj;
}

static void
mud_telnet_finalize (GObject *object)
{
    MudTelnet *telnet;
    GObjectClass *parent_class;

    telnet = MUD_TELNET(object);

    if(telnet->priv->processed)
        g_string_free(telnet->priv->processed, TRUE);

    if(telnet->priv->buffer)
        g_string_free(telnet->priv->buffer, TRUE);

    if(telnet->priv->mud_name)
        g_free(telnet->priv->mud_name);

    g_hash_table_destroy(telnet->priv->handlers);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

static void
mud_telnet_set_property(GObject *object,
                        guint prop_id,
                        const GValue *value,
                        GParamSpec *pspec)
{
    MudTelnet *self;

    self = MUD_TELNET(object);

    switch(prop_id)
    {
        /* Parent is Construct Only */
        case PROP_PARENT_VIEW:
            self->parent_view = MUD_CONNECTION_VIEW(g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
mud_telnet_get_property(GObject *object,
                        guint prop_id,
                        GValue *value,
                        GParamSpec *pspec)
{
    MudTelnet *self;

    self = MUD_TELNET(object);

    switch(prop_id)
    {
        case PROP_PARENT_VIEW:
            g_value_take_object(value, self->parent_view);
            break;

        case PROP_GA_RECEIVED:
            g_value_set_boolean(value, self->ga_received);
            break;

        case PROP_EOR_RECEIVED:
            g_value_set_boolean(value, self->eor_received);
            break;

        case PROP_CONNECTION:
            g_value_set_pointer(value, self->priv->conn);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

/*** Public Methods ***/
void
mud_telnet_get_parent_size(MudTelnet *telnet, gint *w, gint *h)
{
    g_return_if_fail(MUD_IS_TELNET(telnet));

    mud_connection_view_get_term_size(telnet->parent_view, w, h);
}

void
mud_telnet_send_naws(MudTelnet *telnet, gint width, gint height)
{
    MudTelnetHandler *handler;
    
    g_return_if_fail(MUD_IS_TELNET(telnet));

    handler = MUD_TELNET_HANDLER(
                g_hash_table_lookup(telnet->priv->handlers,
                                    GINT_TO_POINTER(TELOPT_NAWS)));
    if(!handler)
        return;

    mud_telnet_naws_send(MUD_TELNET_NAWS(handler), width, height);
}

MudTelnetHandler *
mud_telnet_get_handler(MudTelnet *telnet, gint opt_no)
{
    MudTelnetHandler *handler;

    if(!MUD_IS_TELNET(telnet))
        return NULL;

    handler = MUD_TELNET_HANDLER(
            g_hash_table_lookup(telnet->priv->handlers,
                                GINT_TO_POINTER(opt_no)));
    return handler;
}

GString *
mud_telnet_process(MudTelnet *telnet, guchar * buf, guint32 c, gint *len)
{
    guint32 i;
    guint32 count;
    gchar *opt;

#ifdef ENABLE_MCCP
    gboolean mccp_active, mccp_new;
    MudTelnetMccp *mccp_handler;
#endif

    g_assert(MUD_IS_TELNET(telnet));

    if(telnet->priv->buffer != NULL)
    {
        g_string_free(telnet->priv->buffer, TRUE);
        telnet->priv->buffer = NULL;
    }

    telnet->priv->buffer = g_string_new(NULL);

#ifdef ENABLE_MCCP
    mccp_active = FALSE;
    mccp_new = TRUE;
    mccp_handler = MUD_TELNET_MCCP(mud_telnet_get_handler(telnet, TELOPT_MCCP2));

    if(mccp_handler != NULL)
        g_object_get(mccp_handler,
                "mccp-active", &mccp_active,
                "mccp-new",    &mccp_new,
                NULL);

    if(mccp_active)
    {
        GString *ret = NULL;

        // decompress the buffer.
        ret = mud_mccp_decompress(mccp_handler, buf, c);

        g_object_set(mccp_handler, "mccp-new", FALSE, NULL);

        if(ret == NULL)
            return ret;
        else
        {
            telnet->priv->buffer = g_string_append_len(telnet->priv->buffer, ret->str, ret->len);
            g_string_free(ret, TRUE);
        }
    }
    else
#endif
        telnet->priv->buffer = g_string_append_len(telnet->priv->buffer, (gchar *)buf, c);

    count = telnet->priv->buffer->len;

    for (i = 0; i < count; ++i)
    {
        switch (telnet->priv->tel_state)
        {
            case TEL_STATE_TEXT:
#ifdef ENABLE_MCCP
                /* The following is only done when compressing is first
                   enabled in order to decompress any part of the buffer
                   that remains after the subnegotation takes place */

                if(mccp_handler)
                    g_object_get(mccp_handler,
                            "mccp-active", &mccp_active,
                            "mccp-new", &mccp_new,
                            NULL);

                if(mccp_active && mccp_new)
                {
                    GString *ret = NULL;
                    g_object_set(mccp_handler, "mccp-new", FALSE, NULL);

                    // decompress the rest of the buffer.
                    ret = mud_mccp_decompress(mccp_handler, &buf[i], count - i);

                    if(telnet->priv->buffer)
                    {
                        g_string_free(telnet->priv->buffer, TRUE);
                        telnet->priv->buffer = NULL;
                    }

                    if(ret == NULL)
                    {
                        GString *ret_string =
                            g_string_new_len(telnet->priv->processed->str, telnet->priv->pos);

                        *len = telnet->priv->pos;
                        telnet->priv->pos= 0;

                        if(telnet->priv->processed)
                        {
                            g_string_free(telnet->priv->processed, TRUE);
                            telnet->priv->processed = g_string_new(NULL);
                        }

                        return ret_string;
                    }

                    telnet->priv->buffer = g_string_new(ret->str);

                    if(telnet->priv->buffer->len == 0)
                    {
                        GString *ret_string =
                            g_string_new_len(telnet->priv->processed->str, telnet->priv->pos);
                        *len = telnet->priv->pos;

                        telnet->priv->pos= 0;

                        if(telnet->priv->processed)
                        {
                            g_string_free(telnet->priv->processed, TRUE);
                            telnet->priv->processed = g_string_new(NULL);
                        }

                        if(telnet->priv->buffer)
                        {
                            g_string_free(telnet->priv->buffer, TRUE);
                            telnet->priv->buffer = NULL;
                        }
                        return ret_string;
                    }

                    i = 0;
                    count = telnet->priv->buffer->len;
                }
#endif

                if ((guchar)telnet->priv->buffer->str[i] == (guchar)TEL_IAC)
                    telnet->priv->tel_state = TEL_STATE_IAC;
                else
                {
                    telnet->priv->processed = 
                        g_string_append_c(telnet->priv->processed, telnet->priv->buffer->str[i]);
                    telnet->priv->pos++;
                }
                break;

            case TEL_STATE_IAC:
                switch ((guchar)telnet->priv->buffer->str[i])
                {
                    case (guchar)TEL_IAC:
                        telnet->priv->pos++;
                        telnet->priv->processed = 
                            g_string_append_c(telnet->priv->processed, telnet->priv->buffer->str[i]);
                        telnet->priv->tel_state = TEL_STATE_TEXT;
                        break;

                    case (guchar)TEL_DO:
                        telnet->priv->tel_state = TEL_STATE_DO;
                        break;

                    case (guchar)TEL_DONT:
                        telnet->priv->tel_state = TEL_STATE_DONT;
                        break;

                    case (guchar)TEL_WILL:
                        telnet->priv->tel_state = TEL_STATE_WILL;
                        break;

                    case (guchar)TEL_NOP:
                        telnet->priv->tel_state = TEL_STATE_TEXT;
                        break;

                    case (guchar)TEL_GA:
                        // TODO: Hook up to triggers.
                        telnet->priv->tel_state = TEL_STATE_TEXT;
                        break;

                    case (guchar)TEL_EOR_BYTE:
                        // TODO: Hook up to triggers.
                        telnet->priv->tel_state = TEL_STATE_TEXT;
                        break;

                    case (guchar)TEL_WONT:
                        telnet->priv->tel_state = TEL_STATE_WONT;
                        break;

                    case (guchar)TEL_SB:
                        telnet->priv->tel_state = TEL_STATE_SB;
                        telnet->priv->subreq_pos = 0;
                        break;

                    default:
                        g_warning("Illegal IAC command %d received", telnet->priv->buffer->str[i]);
                        telnet->priv->tel_state = TEL_STATE_TEXT;
                        break;
                }
                break;

            case TEL_STATE_DO:
                opt = mud_telnet_get_telopt_string((guchar)telnet->priv->buffer->str[i]);
                g_log("Telnet", G_LOG_LEVEL_MESSAGE, "Recieved: DO %s", opt);
                g_free(opt);

                mud_telnet_handle_positive_nego(
                        telnet,
                        (guchar)telnet->priv->buffer->str[i],
                        (guchar)TEL_WILL,
                        (guchar)TEL_WONT, FALSE);

                telnet->priv->tel_state = TEL_STATE_TEXT;
                break;

            case TEL_STATE_WILL:
                opt = mud_telnet_get_telopt_string((guchar)telnet->priv->buffer->str[i]);
                g_log("Telnet", G_LOG_LEVEL_MESSAGE, "Recieved: WILL %s", opt);
                g_free(opt);

                mud_telnet_handle_positive_nego(
                        telnet,
                        (guchar)telnet->priv->buffer->str[i],
                        (guchar)TEL_DO,
                        (guchar)TEL_DONT, TRUE);

                telnet->priv->tel_state = TEL_STATE_TEXT;
                break;

            case TEL_STATE_DONT:
                opt = mud_telnet_get_telopt_string((guchar)telnet->priv->buffer->str[i]);
                g_log("Telnet", G_LOG_LEVEL_MESSAGE, "Recieved: DONT %s", opt);
                g_free(opt);

                mud_telnet_handle_negative_nego(
                        telnet,
                        (guchar)telnet->priv->buffer->str[i],
                        (guchar)TEL_WILL,
                        (guchar)TEL_WONT, FALSE);

                telnet->priv->tel_state = TEL_STATE_TEXT;
                break;

            case TEL_STATE_WONT:
                opt = mud_telnet_get_telopt_string((guchar)telnet->priv->buffer->str[i]);
                g_log("Telnet", G_LOG_LEVEL_MESSAGE, "Recieved: WONT %s", opt);
                g_free(opt);

                mud_telnet_handle_negative_nego(
                        telnet,
                        (guchar)telnet->priv->buffer->str[i],
                        (guchar)TEL_DO,
                        (guchar)TEL_DONT, TRUE);

                telnet->priv->tel_state = TEL_STATE_TEXT;
                break;

            case TEL_STATE_SB:
                if ((guchar)telnet->priv->buffer->str[i] == (guchar)TEL_IAC)
                    telnet->priv->tel_state = TEL_STATE_SB_IAC;
                else
                {
                    // FIXME: Handle overflow
                    if (telnet->priv->subreq_pos >= TEL_SUBREQ_BUFFER_SIZE)
                    {
                        g_warning("Subrequest buffer full. Oddities in output will happen. Sorry.");
                        telnet->priv->subreq_pos = 0;
                        telnet->priv->tel_state = TEL_STATE_TEXT;
                    }
                    else
                        telnet->priv->subreq_buffer[telnet->priv->subreq_pos++] =
                            (guchar)telnet->priv->buffer->str[i];
                }
                break;

            case TEL_STATE_SB_IAC:
                if ((guchar)telnet->priv->buffer->str[i] == (guchar)TEL_IAC)
                {
                    if (telnet->priv->subreq_pos >= TEL_SUBREQ_BUFFER_SIZE)
                    {
                        g_warning("Subrequest buffer full. Oddities in output will happen. Sorry.");
                        telnet->priv->subreq_pos = 0;
                        telnet->priv->tel_state = TEL_STATE_TEXT;
                    }
                    else
                        telnet->priv->subreq_buffer[telnet->priv->subreq_pos++] =
                            (guchar)telnet->priv->buffer->str[i];

                    telnet->priv->tel_state = TEL_STATE_SB;
                }
                else if ((guchar)telnet->priv->buffer->str[i] == (guchar)TEL_SE)
                {
                    telnet->priv->subreq_buffer[telnet->priv->subreq_pos++] =
                        (guchar)telnet->priv->buffer->str[i];

                    mud_telnet_on_handle_subnego(telnet);

                    telnet->priv->subreq_pos = 0;
                    telnet->priv->tel_state = TEL_STATE_TEXT;
                } else {
                    g_warning("Erronous byte %d after an IAC inside a subrequest",
                            telnet->priv->buffer->str[i]);

                    telnet->priv->subreq_pos = 0;
                    telnet->priv->tel_state = TEL_STATE_TEXT;
                }
                break;
        }
    }

    if(telnet->priv->tel_state == TEL_STATE_TEXT)
    {
        GString *ret =
            g_string_new_len(telnet->priv->processed->str, telnet->priv->pos);
        *len = telnet->priv->pos;

        telnet->priv->pos= 0;

        if(telnet->priv->processed)
        {
            g_string_free(telnet->priv->processed, TRUE);
            telnet->priv->processed = g_string_new(NULL);
        }

        return ret;
    }

    return NULL;
}

void
mud_telnet_send_sub_req(MudTelnet *telnet, guint32 count, ...)
{
    guchar byte;
    guint32 i;
    va_list va;

    g_return_if_fail(MUD_IS_TELNET(telnet));

    va_start(va, count);

    g_log("Telnet", G_LOG_LEVEL_DEBUG, "Sending Subreq...");
    
    byte = (guchar)TEL_IAC;

    gnet_conn_write(telnet->priv->conn, (gchar *)&byte, 1);
    byte = (guchar)TEL_SB;
    gnet_conn_write(telnet->priv->conn, (gchar *)&byte, 1);

    for (i = 0; i < count; ++i)
    {
	byte = (guchar)va_arg(va, gint);
	gnet_conn_write(telnet->priv->conn, (gchar *)&byte, 1);

	if (byte == (guchar)TEL_IAC)
	    gnet_conn_write(telnet->priv->conn, (gchar *)&byte, 1);
    }

    va_end(va);

    byte = (guchar)TEL_IAC;
    gnet_conn_write(telnet->priv->conn, (gchar *)&byte, 1);
    byte = (guchar)TEL_SE;
    gnet_conn_write(telnet->priv->conn, (gchar *)&byte, 1);
}

void
mud_telnet_send_raw(MudTelnet *telnet, guint32 count, ...)
{
    guchar byte;
    guint32 i;
    va_list va;

    g_return_if_fail(MUD_IS_TELNET(telnet));

    va_start(va, count);

    for (i = 0; i < count; ++i)
    {
	byte = (guchar)va_arg(va, gint);
	gnet_conn_write(telnet->priv->conn, (gchar *)&byte, 1);

	if (byte == (guchar)TEL_IAC)
	    gnet_conn_write(telnet->priv->conn, (gchar *)&byte, 1);
    }

    va_end(va);
}

/*** Private Methods ***/
static void
mud_telnet_register_handlers(MudTelnet *telnet)
{
    g_return_if_fail(MUD_IS_TELNET(telnet));

    /* TTYPE */
    g_hash_table_replace(telnet->priv->handlers,
                         GINT_TO_POINTER(TELOPT_TTYPE),
                         g_object_new(MUD_TYPE_TELNET_TTYPE,
                                      "telnet", telnet,
                                      NULL));
    /* NAWS */
    g_hash_table_replace(telnet->priv->handlers,
                         GINT_TO_POINTER(TELOPT_NAWS),
                         g_object_new(MUD_TYPE_TELNET_NAWS,
                                      "telnet", telnet,
                                      NULL));
    /* ECHO */
    g_hash_table_replace(telnet->priv->handlers,
                         GINT_TO_POINTER(TELOPT_ECHO),
                         g_object_new(MUD_TYPE_TELNET_ECHO,
                                      "telnet", telnet,
                                      NULL));
    /* EOR */
    g_hash_table_replace(telnet->priv->handlers,
                         GINT_TO_POINTER(TELOPT_EOR),
                         g_object_new(MUD_TYPE_TELNET_EOR,
                                      "telnet", telnet,
                                      NULL));
    /* CHARSET */
    g_hash_table_replace(telnet->priv->handlers,
                         GINT_TO_POINTER(TELOPT_CHARSET),
                         g_object_new(MUD_TYPE_TELNET_CHARSET,
                                      "telnet", telnet,
                                      NULL));
    /* ZMP */
    g_hash_table_replace(telnet->priv->handlers,
                         GINT_TO_POINTER(TELOPT_ZMP),
                         g_object_new(MUD_TYPE_TELNET_ZMP,
                                      "telnet", telnet,
                                      NULL));
    /* MSSP */
    g_hash_table_replace(telnet->priv->handlers,
                         GINT_TO_POINTER(TELOPT_MSSP),
                         g_object_new(MUD_TYPE_TELNET_MSSP,
                                      "telnet", telnet,
                                      NULL));
    /* NEW-ENVIRON */
    g_hash_table_replace(telnet->priv->handlers,
                         GINT_TO_POINTER(TELOPT_NEWENVIRON),
                         g_object_new(MUD_TYPE_TELNET_NEWENVIRON,
                                      "telnet", telnet,
                                      NULL));
#ifdef ENABLE_GST
    /* MSP */
    g_hash_table_replace(telnet->priv->handlers,
                         GINT_TO_POINTER(TELOPT_MSP),
                         g_object_new(MUD_TYPE_TELNET_MSP,
                                      "telnet", telnet,
                                      NULL));
#endif

#ifdef ENABLE_MCCP
    /* MCCP2 */
    g_hash_table_replace(telnet->priv->handlers,
                         GINT_TO_POINTER(TELOPT_MCCP2),
                         g_object_new(MUD_TYPE_TELNET_MCCP,
                                      "telnet", telnet,
                                      NULL));
#endif

}

static gchar *
mud_telnet_get_telnet_string(guchar ch)
{
    GString *string = g_string_new(NULL);

    switch (ch)
    {
        case TEL_WILL:
            string = g_string_append(string, "WILL");
            break;
        case TEL_WONT:
            string = g_string_append(string, "WONT");
            break;
        case TEL_DO:
            string = g_string_append(string, "DO");
            break;
        case TEL_DONT:
            string = g_string_append(string, "DONT");
            break;
        case TEL_IAC:
            string = g_string_append(string, "IAC");
            break;
        default:
            string = g_string_append_c(string,ch);
            break;
    }

    return g_string_free(string, FALSE);
}

static gchar*
mud_telnet_get_telopt_string(guchar ch)
{
    GString *string = g_string_new(NULL);

    switch (ch)
    {
        case TELOPT_ECHO:
            string = g_string_append(string, "ECHO");
            break;

        case TELOPT_TTYPE:
            string = g_string_append(string, "TTYPE");
            break;

        case TELOPT_EOR:
            string = g_string_append(string, "END-OF-RECORD");
            break;

        case TELOPT_NAWS:
            string = g_string_append(string, "NAWS");
            break;

        case TELOPT_CHARSET:
            string = g_string_append(string, "CHARSET");
            break;

        case TELOPT_MCCP:
            string = g_string_append(string, "COMPRESS");
            break;

        case TELOPT_MCCP2:
            string = g_string_append(string, "COMPRESS2");
            break;

        case TELOPT_MSP:
            string = g_string_append(string, "MSP");
            break;

        case TELOPT_MXP:
            string = g_string_append(string, "MXP");
            break;

        case TELOPT_ZMP:
            string = g_string_append(string, "ZMP");
            break;

        default:
            g_string_printf(string, "Dec: %d Hex: %x Ascii: %c", ch, ch, ch);
            break;
    }

    return g_string_free(string, FALSE);
}

static void
mud_telnet_send_iac(MudTelnet *telnet, guchar ch1, guchar ch2)
{
    guchar buf[3];
    buf[0] = (guchar)TEL_IAC;
    buf[1] = ch1;
    buf[2] = ch2;

    gnet_conn_write(telnet->priv->conn, (gchar *)buf, 3);
}

static void
mud_telnet_unref_handler(gpointer data)
{
    MudTelnetHandler *handler = MUD_TELNET_HANDLER(data);

    g_object_unref(handler);
}

static void
mud_telnet_on_handle_subnego(MudTelnet *telnet)
{
    MudTelnetHandler *handler;
    gboolean enabled;

    g_return_if_fail(MUD_IS_TELNET(telnet));

    if (telnet->priv->subreq_pos < 1)
	return;

    handler = 
        MUD_TELNET_HANDLER(
                g_hash_table_lookup(telnet->priv->handlers,
                                    GINT_TO_POINTER(
                                        (gint)telnet->priv->subreq_buffer[0])));

    if(!handler)
    {
        g_warning("Invalid Telnet Option passed: %d", telnet->priv->subreq_buffer[0]);
        return;
    }

    g_object_get(handler, "enabled", &enabled, NULL);

    if(enabled)
        mud_telnet_handler_handle_sub_neg(handler,
                telnet->priv->subreq_buffer + 1,
                telnet->priv->subreq_pos - 1);
}

static void
mud_telnet_on_enable_opt(MudTelnet *telnet, const guchar opt_no)
{
    MudTelnetHandler *handler;

    g_return_if_fail(MUD_IS_TELNET(telnet));

    handler = MUD_TELNET_HANDLER(
            g_hash_table_lookup(telnet->priv->handlers,
                                GINT_TO_POINTER((gint)opt_no)));

    if(!handler)
    {
        g_warning("Invalid Telnet Option passed: %d", opt_no);
        return;
    }

    mud_telnet_handler_enable(handler);
}


static void
mud_telnet_on_disable_opt(MudTelnet *telnet, const guchar opt_no)
{
    MudTelnetHandler *handler;

    g_return_if_fail(MUD_IS_TELNET(telnet));

    handler = MUD_TELNET_HANDLER(
            g_hash_table_lookup(telnet->priv->handlers,
                                GINT_TO_POINTER((gint)opt_no)));

    if(!handler)
    {
        g_warning("Invalid Telnet Option passed: %d", opt_no);
        return;
    }

    mud_telnet_handler_disable(handler);
}

static gboolean
mud_telnet_isenabled(MudTelnet *telnet, gint option_number)
{
    MudTelnetHandler *handler;
    gboolean enabled;

    if(!MUD_IS_TELNET(telnet))
        return FALSE;

    handler = MUD_TELNET_HANDLER(
            g_hash_table_lookup(telnet->priv->handlers,
                                GINT_TO_POINTER(option_number)));
    if(!handler)
        return FALSE;

    g_object_get(handler, "enabled", &enabled, NULL);

    return !enabled;
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
mud_telnet_set_telopt_state(guchar *storage,
                            const enum TelnetOptionState state,
			    const guint bitshift)
{
    *storage = (*storage & ~(0x03u << bitshift)) | (state << bitshift);
}

static void
mud_telnet_set_telopt_queue(guchar *storage, gint bit_on, const guint bitshift)
{
    *storage = bit_on ? (*storage | (0x04u << bitshift)) : (*storage & ~(0x04u << bitshift));
}

static gint
mud_telnet_handle_positive_nego(MudTelnet *telnet,
                                const guchar opt_no,
				const guchar affirmative,
				const guchar negative,
				gint him)
{
    const guint bitshift = him ? 4 : 0;
    guchar *opt;
    gchar *tel_string, *opt_string;

    if(!MUD_IS_TELNET(telnet))
        return FALSE;

    opt = &(telnet->priv->telopt_states[opt_no]);

    switch (mud_telnet_get_telopt_state(opt, bitshift))
    {
        case TELOPT_STATE_NO:
            // if we agree that server should enable telopt, set
            // his state to yes and send do; otherwise send dont
            // fixme-us/him
            // fixme: what to do in the opposite "him" gint value case?
            if (mud_telnet_isenabled(telnet, opt_no))
            {
                tel_string = mud_telnet_get_telnet_string(affirmative);
                opt_string = mud_telnet_get_telopt_string(opt_no);
                g_log("Telnet", G_LOG_LEVEL_DEBUG, "sent: %s %s",
                        tel_string, opt_string );
                g_free(tel_string);
                g_free(opt_string);

                mud_telnet_set_telopt_state(opt, TELOPT_STATE_YES, bitshift);
                mud_telnet_send_iac(telnet, affirmative, opt_no);
                mud_telnet_on_enable_opt(telnet, opt_no);
                return TRUE;
            } else {
                tel_string = mud_telnet_get_telnet_string(negative);
                opt_string = mud_telnet_get_telopt_string(opt_no);
                g_log("Telnet", G_LOG_LEVEL_DEBUG, "sent: %s %s",
                        tel_string, opt_string );
                g_free(tel_string);
                g_free(opt_string);

                mud_telnet_send_iac(telnet, negative, opt_no);
                return FALSE;
            }
            break;

        case TELOPT_STATE_YES:
            // ignore, he already supposedly has it enabled. includes the case where
            // dont was answered by will with himq = opposite to prevent loop.
            return FALSE;
            break;

        case TELOPT_STATE_WANTNO:
            if (mud_telnet_get_telopt_queue(opt, bitshift) == TELOPT_STATE_QUEUE_EMPTY)
            {
                mud_telnet_set_telopt_state(opt, TELOPT_STATE_NO, bitshift);
                g_warning("telnet negotiation: dont answered by will; ill-behaved server. ignoring iac will %d. him = no\n", opt_no);
                return FALSE;
            } else { // the opposite is queued
                mud_telnet_set_telopt_state(opt, TELOPT_STATE_YES, bitshift);
                mud_telnet_set_telopt_queue(opt, TELOPT_STATE_QUEUE_EMPTY, bitshift);
                g_warning("telnet negotiation: dont answered by will; ill-behaved server. ignoring iac will %d. him = yes, himq = empty\n", opt_no);
                return FALSE;
            }
            break;

        case TELOPT_STATE_WANTYES:
            if (mud_telnet_get_telopt_queue(opt, bitshift) == TELOPT_STATE_QUEUE_EMPTY)
            {
                tel_string = mud_telnet_get_telnet_string(affirmative);
                opt_string = mud_telnet_get_telopt_string(opt_no);
                g_log("Telnet", G_LOG_LEVEL_DEBUG, "sent: %s %s",
                        tel_string, opt_string );
                g_free(tel_string);
                g_free(opt_string);

                mud_telnet_set_telopt_state(opt, TELOPT_STATE_YES, bitshift);
                mud_telnet_send_iac(telnet, affirmative, opt_no);
                mud_telnet_on_enable_opt(telnet, opt_no);
                return TRUE;
            } else { // the opposite is queued
                tel_string = mud_telnet_get_telnet_string(negative);
                opt_string = mud_telnet_get_telopt_string(opt_no);
                g_log("Telnet", G_LOG_LEVEL_DEBUG, "sent: %s %s",
                        tel_string, opt_string );
                g_free(tel_string);
                g_free(opt_string);

                mud_telnet_set_telopt_state(opt, TELOPT_STATE_WANTNO, bitshift);
                mud_telnet_set_telopt_queue(opt, TELOPT_STATE_QUEUE_EMPTY, bitshift);
                mud_telnet_send_iac(telnet, negative, opt_no);
                return FALSE;
            }
            break;

        default:
            g_warning("something went really wrong\n");
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
    guchar *opt;
    gchar *opt_string, *tel_string;

    if(!MUD_IS_TELNET(telnet))
        return FALSE;

    opt = &(telnet->priv->telopt_states[opt_no]);

    switch (mud_telnet_get_telopt_state(opt, bitshift))
    {
        case TELOPT_STATE_NO:
            // ignore, he already supposedly has it disabled
            return FALSE;

        case TELOPT_STATE_YES:
            tel_string = mud_telnet_get_telnet_string(negative);
            opt_string = mud_telnet_get_telopt_string(opt_no);
            g_log("Telnet", G_LOG_LEVEL_DEBUG, "sent: %s %s",
                    tel_string, opt_string );
            g_free(tel_string);
            g_free(opt_string);

            mud_telnet_set_telopt_state(opt, TELOPT_STATE_NO, bitshift);
            mud_telnet_send_iac(telnet, negative, opt_no);
            mud_telnet_on_disable_opt(telnet, opt_no);
            return TRUE;
            break;

        case TELOPT_STATE_WANTNO:
            if (mud_telnet_get_telopt_queue(opt, bitshift) == TELOPT_STATE_QUEUE_EMPTY)
            {
                mud_telnet_set_telopt_state(opt, TELOPT_STATE_NO, bitshift);
                return FALSE;
            } else {
                tel_string = mud_telnet_get_telnet_string(affirmative);
                opt_string = mud_telnet_get_telopt_string(opt_no);
                g_log("Telnet", G_LOG_LEVEL_DEBUG, "sent: %s %s",
                        tel_string, opt_string );
                g_free(tel_string);
                g_free(opt_string);

                mud_telnet_set_telopt_state(opt, TELOPT_STATE_WANTYES, bitshift);
                mud_telnet_set_telopt_queue(opt, TELOPT_STATE_QUEUE_EMPTY, bitshift);
                mud_telnet_send_iac(telnet, affirmative, opt_no);
                mud_telnet_on_enable_opt(telnet, opt_no); // fixme: is this correct?
                return TRUE;
            }
            break;

        case TELOPT_STATE_WANTYES:
            if (mud_telnet_get_telopt_queue(opt, bitshift) == TELOPT_STATE_QUEUE_EMPTY)
            {
                mud_telnet_set_telopt_state(opt, TELOPT_STATE_NO, bitshift);
                return FALSE;
            } else { // the opposite is queued
                mud_telnet_set_telopt_state(opt, TELOPT_STATE_NO, bitshift);
                mud_telnet_set_telopt_queue(opt, TELOPT_STATE_QUEUE_EMPTY, bitshift);
                return FALSE;
            }
            break;

        default:
            g_warning("telnet negotiation: something went really wrong\n");
            return FALSE;
    }
}

