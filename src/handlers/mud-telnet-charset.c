/* GNOME-Mud - A simple Mud Client
 * mud-telnet-charset.c
 * Copyright (C) 2005-2009 Les Harris <lharris@gnome.org>
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
#include "config.h"
#endif

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gnet.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib/gprintf.h>

#include "gnome-mud.h"
#include "mud-telnet.h"
#include "mud-telnet-handler-interface.h"
#include "mud-telnet-charset.h"

struct _MudTelnetCharsetPrivate
{
    /* Interface Properties */
    MudTelnet *telnet;
    gboolean enabled;
    gint option;

    /* Private Instance Members */
};

/* Property Identifiers */
enum
{
    PROP_MUD_TELNET_CHARSET_0,
    PROP_ENABLED,
    PROP_HANDLES_OPTION,
    PROP_TELNET
};

/* Class Functions */
static void mud_telnet_charset_init (MudTelnetCharset *self);
static void mud_telnet_charset_class_init (MudTelnetCharsetClass *klass);
static void mud_telnet_charset_interface_init(MudTelnetHandlerInterface *iface);
static void mud_telnet_charset_finalize (GObject *object);
static GObject *mud_telnet_charset_constructor (GType gtype,
                                             guint n_properties,
                                             GObjectConstructParam *properties);
static void mud_telnet_charset_set_property(GObject *object,
                                         guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec);
static void mud_telnet_charset_get_property(GObject *object,
                                         guint prop_id,
                                         GValue *value,
                                         GParamSpec *pspec);

/* Interface Implementation */
void mud_telnet_charset_enable(MudTelnetHandler *self);
void mud_telnet_charset_disable(MudTelnetHandler *self);
void mud_telnet_charset_handle_sub_neg(MudTelnetHandler *self,
                                    guchar *buf,
                                    guint len);

/* Private Methods */
static void mud_telnet_charset_send(MudTelnetCharset *self, gchar *encoding);

/* Create the Type. We implement MudTelnetHandlerInterface */
G_DEFINE_TYPE_WITH_CODE(MudTelnetCharset, mud_telnet_charset, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE (MUD_TELNET_HANDLER_TYPE,
                                               mud_telnet_charset_interface_init));
/* MudTelnetCharset class functions */
static void
mud_telnet_charset_class_init (MudTelnetCharsetClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object constructor */
    object_class->constructor = mud_telnet_charset_constructor;

    /* Override base object's finalize */
    object_class->finalize = mud_telnet_charset_finalize;

    /* Override base object property methods */
    object_class->set_property = mud_telnet_charset_set_property;
    object_class->get_property = mud_telnet_charset_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(MudTelnetCharsetPrivate));

    /* Override Implementation Properties */
    g_object_class_override_property(object_class,
                                     PROP_ENABLED,
                                     "enabled");

    g_object_class_override_property(object_class,
                                     PROP_HANDLES_OPTION,
                                     "handles-option");

    g_object_class_override_property(object_class,
                                     PROP_TELNET,
                                     "telnet");
}

static void
mud_telnet_charset_interface_init(MudTelnetHandlerInterface *iface)
{
    iface->Enable = mud_telnet_charset_enable;
    iface->Disable = mud_telnet_charset_disable;
    iface->HandleSubNeg = mud_telnet_charset_handle_sub_neg;
}

static void
mud_telnet_charset_init (MudTelnetCharset *self)
{
    /* Get our private data */
    self->priv = MUD_TELNET_CHARSET_GET_PRIVATE(self);

    /* Set the defaults */
    self->priv->telnet = NULL;
    self->priv->option = TELOPT_CHARSET;
    self->priv->enabled = FALSE;
}

static GObject *
mud_telnet_charset_constructor (GType gtype,
                              guint n_properties,
                              GObjectConstructParam *properties)
{
    MudTelnetCharset *self;
    GObject *obj;
    MudTelnetCharsetClass *klass;
    GObjectClass *parent_class;

    /* Chain up to parent constructor */
    klass = MUD_TELNET_CHARSET_CLASS( g_type_class_peek(MUD_TYPE_TELNET_CHARSET) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    self = MUD_TELNET_CHARSET(obj);

    if(!self->priv->telnet)
    {
        g_printf("ERROR: Tried to instantiate MudTelnetCharset without passing parent MudTelnet\n");
        g_error("Tried to instantiate MudTelnetCharset without passing parent MudTelnet");
    }

    return obj;
}

static void
mud_telnet_charset_finalize (GObject *object)
{
    MudTelnetCharset *self;
    GObjectClass *parent_class;

    self = MUD_TELNET_CHARSET(object);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

static void
mud_telnet_charset_set_property(GObject *object,
                              guint prop_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
    MudTelnetCharset *self;

    self = MUD_TELNET_CHARSET(object);

    switch(prop_id)
    {
        case PROP_TELNET:
            self->priv->telnet = MUD_TELNET(g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
mud_telnet_charset_get_property(GObject *object,
                              guint prop_id,
                              GValue *value,
                              GParamSpec *pspec)
{
    MudTelnetCharset *self;

    self = MUD_TELNET_CHARSET(object);

    switch(prop_id)
    {
        case PROP_ENABLED:
            g_value_set_boolean(value, self->priv->enabled);
            break;

        case PROP_HANDLES_OPTION:
            g_value_set_int(value, self->priv->option);
            break;

        case PROP_TELNET:
            g_value_take_object(value, self->priv->telnet);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

/* Interface Implementation */
void 
mud_telnet_charset_enable(MudTelnetHandler *handler)
{
    MudTelnetCharset *self;

    self = MUD_TELNET_CHARSET(handler);

    g_return_if_fail(MUD_IS_TELNET_CHARSET(self));

    self->priv->enabled = TRUE;

    g_log("Telnet", G_LOG_LEVEL_INFO, "%s", "CHARSET Enabled");
}

void
mud_telnet_charset_disable(MudTelnetHandler *handler)
{
    MudTelnetCharset *self;
    MudConnectionView *view;

    self = MUD_TELNET_CHARSET(handler);

    g_return_if_fail(MUD_IS_TELNET_CHARSET(self));

    self->priv->enabled = FALSE;

    g_object_get(self->priv->telnet, "parent-view", &view, NULL);

    g_object_set(view,
                 "remote-encode", FALSE,
                 "remote-encoding", NULL,
                 NULL);

    g_log("Telnet", G_LOG_LEVEL_INFO, "%s", "CHARSET Disabled");
}

void
mud_telnet_charset_handle_sub_neg(MudTelnetHandler *handler,
                               guchar *buf,
                               guint len)
{
    gint index = 0;
    guchar sep;
    guchar tbuf[9];
    gchar sep_buf[2];
    GString *encoding;
    gchar **encodings;
    MudTelnetCharset *self;

    self = MUD_TELNET_CHARSET(handler);

    g_return_if_fail(MUD_IS_TELNET_CHARSET(self));

    switch(buf[index])
    {
        case TEL_CHARSET_REQUEST:
            // Check for [TTABLE] and
            // reject if found.
            memcpy(tbuf, &buf[1], 8);
            tbuf[8] = '\0';

            if(strcmp((gchar *)tbuf, "[TTABLE]") == 0)
            {
                mud_telnet_send_sub_req(self->priv->telnet, 2,
                        (guchar)TELOPT_CHARSET,
                        (guchar)TEL_CHARSET_TTABLE_REJECTED);
                return;
            }

            sep = buf[++index];
            index++;

            encoding = g_string_new(NULL);

            while(buf[index] != (guchar)TEL_SE)
                encoding = g_string_append_c(encoding, buf[index++]);

            sep_buf[0] = (gchar)sep;
            sep_buf[1] = '\0';
            encodings = g_strsplit(encoding->str, sep_buf, -1);

            /* We are using VTE's locale fallback function
               to handle a charset we do not support so we
               just take the first returned and use it.

               This is potentially stupid. Fix me? */

            if(g_strv_length(encodings) != 0)
            {
                MudConnectionView *view;
                g_object_get(self->priv->telnet, "parent-view", &view, NULL);

                g_object_set(view,
                        "remote-encode", TRUE,
                        "remote-encoding", encodings[0],
                        NULL);


                mud_telnet_charset_send(self, encodings[0]);
            }

            g_string_free(encoding, TRUE);
            g_strfreev(encodings);

            break;
    }
}

/* Private Methods */
static void
mud_telnet_charset_send(MudTelnetCharset *self, gchar *encoding)
{
    guchar byte;
    guint32 i;
    GConn *conn;

    g_return_if_fail(MUD_IS_TELNET_CHARSET(self));

    if(!encoding)
	return;

    g_log("Telnet", G_LOG_LEVEL_DEBUG, "Sending Charset Accepted SubReq");

    g_object_get(self->priv->telnet, "connection", &conn, NULL);

    /* Writes IAC SB CHARSET ACCEPTED <charset> IAC SE to server */
    byte = (guchar)TEL_IAC;

    gnet_conn_write(conn, (gchar *)&byte, 1);
    byte = (guchar)TEL_SB;
    gnet_conn_write(conn, (gchar *)&byte, 1);
    byte = (guchar)TELOPT_CHARSET;
    gnet_conn_write(conn, (gchar *)&byte, 1);
    byte = (guchar)TEL_CHARSET_ACCEPT;
    gnet_conn_write(conn, (gchar *)&byte, 1);

    for (i = 0; i < strlen(encoding); ++i)
    {
	byte = (guchar)encoding[i];
	gnet_conn_write(conn, (gchar *)&byte, 1);

	if (byte == (guchar)TEL_IAC)
	    gnet_conn_write(conn, (gchar *)&byte, 1);
    }

    byte = (guchar)TEL_IAC;
    gnet_conn_write(conn, (gchar *)&byte, 1);
    byte = (guchar)TEL_SE;
    gnet_conn_write(conn, (gchar *)&byte, 1);
}

