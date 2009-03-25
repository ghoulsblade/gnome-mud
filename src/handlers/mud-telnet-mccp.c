/* GNOME-Mud - A simple Mud Client
 * mud-telnet-mccp.c
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

#ifdef ENABLE_MCCP

#include <glib.h>
#include <glib/gi18n.h>
#include <zlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib/gprintf.h>

#include "gnome-mud.h"
#include "mud-connection-view.h"
#include "mud-telnet.h"
#include "mud-telnet-handler-interface.h"
#include "mud-telnet-mccp.h"

struct _MudTelnetMccpPrivate
{
    /* Interface Properties */
    MudTelnet *telnet;
    gboolean enabled;
    gint option;

    /* Private Instance Members */
    z_stream *compress_out;
    guchar *compress_out_buf;
    gboolean mccp_new;
    gboolean mccp;
};

/* Property Identifiers */
enum
{
    PROP_MUD_TELNET_MCCP_0,
    PROP_ENABLED,
    PROP_HANDLES_OPTION,
    PROP_TELNET,
    PROP_MCCP_NEW,
    PROP_MCCP
};

/* Class Functions */
static void mud_telnet_mccp_init (MudTelnetMccp *self);
static void mud_telnet_mccp_class_init (MudTelnetMccpClass *klass);
static void mud_telnet_mccp_interface_init(MudTelnetHandlerInterface *iface);
static void mud_telnet_mccp_finalize (GObject *object);
static GObject *mud_telnet_mccp_constructor (GType gtype,
                                             guint n_properties,
                                             GObjectConstructParam *properties);
static void mud_telnet_mccp_set_property(GObject *object,
                                         guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec);
static void mud_telnet_mccp_get_property(GObject *object,
                                         guint prop_id,
                                         GValue *value,
                                         GParamSpec *pspec);

/*Interface Implementation */
void mud_telnet_mccp_enable(MudTelnetHandler *self);
void mud_telnet_mccp_disable(MudTelnetHandler *self);
void mud_telnet_mccp_handle_sub_neg(MudTelnetHandler *self,
                                    guchar *buf,
                                    guint len);

/* Create the Type. We implement MudTelnetHandlerInterface */
G_DEFINE_TYPE_WITH_CODE(MudTelnetMccp, mud_telnet_mccp, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE (MUD_TELNET_HANDLER_TYPE,
                                               mud_telnet_mccp_interface_init));
/* MudTelnetMccp class functions */
static void
mud_telnet_mccp_class_init (MudTelnetMccpClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object constructor */
    object_class->constructor = mud_telnet_mccp_constructor;

    /* Override base object's finalize */
    object_class->finalize = mud_telnet_mccp_finalize;

    /* Override base object property methods */
    object_class->set_property = mud_telnet_mccp_set_property;
    object_class->get_property = mud_telnet_mccp_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(MudTelnetMccpPrivate));

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

    g_object_class_install_property(object_class,
                            PROP_MCCP_NEW,
                            g_param_spec_boolean("mccp-new",
                                "MCCP New",
                                "Is MCCP newly enabled.",
                                FALSE,
                                G_PARAM_READWRITE));

    g_object_class_install_property(object_class,
                            PROP_MCCP,
                            g_param_spec_boolean("mccp-active",
                                "MCCP Active",
                                "Is MCCP Active.",
                                FALSE,
                                G_PARAM_READABLE));
}

static void
mud_telnet_mccp_interface_init(MudTelnetHandlerInterface *iface)
{
    iface->Enable = mud_telnet_mccp_enable;
    iface->Disable = mud_telnet_mccp_disable;
    iface->HandleSubNeg = mud_telnet_mccp_handle_sub_neg;
}

static void
mud_telnet_mccp_init (MudTelnetMccp *self)
{
    /* Get our private data */
    self->priv = MUD_TELNET_MCCP_GET_PRIVATE(self);

    /* Set the defaults */
    self->priv->telnet = NULL;
    self->priv->option = TELOPT_MCCP2;
    self->priv->enabled = FALSE;

    self->priv->mccp_new = TRUE;
    self->priv->mccp = FALSE;
}

static GObject *
mud_telnet_mccp_constructor (GType gtype,
                              guint n_properties,
                              GObjectConstructParam *properties)
{
    MudTelnetMccp *self;
    GObject *obj;
    MudTelnetMccpClass *klass;
    GObjectClass *parent_class;

    /* Chain up to parent constructor */
    klass = MUD_TELNET_MCCP_CLASS( g_type_class_peek(MUD_TYPE_TELNET_MCCP) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    self = MUD_TELNET_MCCP(obj);

    if(!self->priv->telnet)
    {
        g_printf("ERROR: Tried to instantiate MudTelnetMccp without passing parent MudTelnet\n");
        g_error("Tried to instantiate MudTelnetMccp without passing parent MudTelnet");
    }

    return obj;
}

static void
mud_telnet_mccp_finalize (GObject *object)
{
    MudTelnetMccp *self;
    GObjectClass *parent_class;

    self = MUD_TELNET_MCCP(object);

    if(self->priv->compress_out != NULL)
    {
        inflateEnd(self->priv->compress_out);

        g_free(self->priv->compress_out);
        g_free(self->priv->compress_out_buf);
    }

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

static void
mud_telnet_mccp_set_property(GObject *object,
                              guint prop_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
    MudTelnetMccp *self;

    self = MUD_TELNET_MCCP(object);

    switch(prop_id)
    {
        case PROP_TELNET:
            self->priv->telnet = MUD_TELNET(g_value_get_object(value));
            break;

        case PROP_MCCP_NEW:
            self->priv->mccp_new = g_value_get_boolean(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
mud_telnet_mccp_get_property(GObject *object,
                              guint prop_id,
                              GValue *value,
                              GParamSpec *pspec)
{
    MudTelnetMccp *self;

    self = MUD_TELNET_MCCP(object);

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

        case PROP_MCCP_NEW:
            g_value_set_boolean(value, self->priv->mccp_new);
            break;

        case PROP_MCCP:
            g_value_set_boolean(value, self->priv->mccp);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

/* Interface Implementation */
void 
mud_telnet_mccp_enable(MudTelnetHandler *handler)
{
    MudTelnetMccp *self;

    self = MUD_TELNET_MCCP(handler);

    g_return_if_fail(MUD_IS_TELNET_MCCP(self));

    self->priv->enabled = TRUE;
    self->priv->mccp = FALSE;

    g_log("Telnet", G_LOG_LEVEL_INFO, "%s", "MCCP Requested");
}

void
mud_telnet_mccp_disable(MudTelnetHandler *handler)
{
    MudTelnetMccp *self;

    self = MUD_TELNET_MCCP(handler);

    g_return_if_fail(MUD_IS_TELNET_MCCP(self));

    self->priv->enabled = FALSE;
    self->priv->mccp = FALSE;

    if (self->priv->compress_out != NULL)
    {
        inflateEnd(self->priv->compress_out);

        g_free(self->priv->compress_out);
        g_free(self->priv->compress_out_buf);
        
        self->priv->compress_out = NULL;
    }

    g_log("Telnet", G_LOG_LEVEL_INFO, "%s", "MCCP Disabled");
}

void
mud_telnet_mccp_handle_sub_neg(MudTelnetHandler *handler,
                               guchar *buf,
                               guint len)
{
    MudTelnetMccp *self;

    self = MUD_TELNET_MCCP(handler);

    g_return_if_fail(MUD_IS_TELNET_MCCP(self));

    self->priv->mccp_new = TRUE;
    self->priv->mccp = TRUE;

    self->priv->compress_out = g_new0(z_stream, 1);
    self->priv->compress_out_buf = g_new0(guchar, 4096);

    self->priv->compress_out->next_out = self->priv->compress_out_buf;
    self->priv->compress_out->avail_out = 4096;

    if(inflateInit(self->priv->compress_out) != Z_OK)
    {
        MudConnectionView *view;
        g_object_get(self->priv->telnet, "parent-view", &view, NULL);

        g_printf("failed to init compression\n");
        g_critical("Failed to initialize compression.");

        g_free(self->priv->compress_out);
        g_free(self->priv->compress_out_buf);

        self->priv->compress_out = NULL;
        self->priv->compress_out_buf = NULL;

        mud_connection_view_disconnect(view);
    }

    g_log("Telnet", G_LOG_LEVEL_INFO, "MCCP Enabled.");
}

/* Public Methods */
GString *
mud_mccp_decompress(MudTelnetMccp *self, guchar *buffer, guint32 length)
{
    GString *ret = NULL;
    gint zstatus;
    MudConnectionView *view;

    if(!MUD_IS_TELNET_MCCP(self))
        return NULL;

    if(!self->priv->compress_out)
        return NULL;

    g_object_get(self->priv->telnet, "parent-view", &view, NULL);

    self->priv->compress_out->next_in = buffer;
    self->priv->compress_out->avail_in = length;

    ret = g_string_new(NULL);

    while(1)
    {
        if(self->priv->compress_out->avail_in < 1)
            break;

        self->priv->compress_out->avail_out = 4096;
        self->priv->compress_out->next_out = self->priv->compress_out_buf;

        /* Inflating */
        zstatus = inflate(self->priv->compress_out, Z_SYNC_FLUSH);

        if(zstatus == Z_OK)
        {
            ret = g_string_append_len(ret, 
                    (gchar *)self->priv->compress_out_buf, 
                    (4096 - self->priv->compress_out->avail_out));

            continue;
        }

        if(zstatus == Z_STREAM_END)
        {
            ret = g_string_append_len(ret, 
                    (gchar *)self->priv->compress_out_buf, 
                    (4096 - self->priv->compress_out->avail_out));

            if(self->priv->compress_out->avail_in > 0)
                ret = g_string_append_len(ret, 
                        (gchar *)self->priv->compress_out->next_in, 
                        self->priv->compress_out->avail_in);

            inflateEnd(self->priv->compress_out);

            g_free(self->priv->compress_out);
            g_free(self->priv->compress_out_buf);

            self->priv->compress_out = NULL;
            self->priv->compress_out_buf = NULL;

            self->priv->mccp = FALSE;
            self->priv->mccp_new = TRUE;

            break;
        }

        if(zstatus == Z_BUF_ERROR)
        {
            break;
        }

        if(zstatus == Z_DATA_ERROR)
        {
            mud_connection_view_add_text(view,
                    _("\nMCCP data corrupted. Aborting connection.\n"),
                    Error);
            mud_connection_view_disconnect (view);

            g_printf("Z DATA ERROR!\n");

            return NULL; 
        }
    }

    return ret;
}

#endif // ENABLE_MCCP

