/* GNOME-Mud - A simple Mud Client
 * mud-telnet-naws.c
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

#include "gnome-mud.h"
#include "mud-telnet.h"
#include "mud-telnet-handler-interface.h"
#include "mud-telnet-naws.h"

struct _MudTelnetNawsPrivate
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
    PROP_MUD_TELNET_NAWS_0,
    PROP_ENABLED,
    PROP_HANDLES_OPTION,
    PROP_TELNET
};

/* Class Functions */
static void mud_telnet_naws_init (MudTelnetNaws *self);
static void mud_telnet_naws_class_init (MudTelnetNawsClass *klass);
static void mud_telnet_naws_interface_init(MudTelnetHandlerInterface *iface);
static void mud_telnet_naws_finalize (GObject *object);
static GObject *mud_telnet_naws_constructor (GType gtype,
                                             guint n_properties,
                                             GObjectConstructParam *properties);
static void mud_telnet_naws_set_property(GObject *object,
                                         guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec);
static void mud_telnet_naws_get_property(GObject *object,
                                         guint prop_id,
                                         GValue *value,
                                         GParamSpec *pspec);

/*Interface Implementation */
void mud_telnet_naws_enable(MudTelnetHandler *self);
void mud_telnet_naws_disable(MudTelnetHandler *self);
void mud_telnet_naws_handle_sub_neg(MudTelnetHandler *self,
                                    guchar *buf,
                                    guint len);

/* Create the Type. We implement MudTelnetHandlerInterface */
G_DEFINE_TYPE_WITH_CODE(MudTelnetNaws, mud_telnet_naws, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE (MUD_TELNET_HANDLER_TYPE,
                                               mud_telnet_naws_interface_init));
/* MudTelnetNaws class functions */
static void
mud_telnet_naws_class_init (MudTelnetNawsClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object constructor */
    object_class->constructor = mud_telnet_naws_constructor;

    /* Override base object's finalize */
    object_class->finalize = mud_telnet_naws_finalize;

    /* Override base object property methods */
    object_class->set_property = mud_telnet_naws_set_property;
    object_class->get_property = mud_telnet_naws_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(MudTelnetNawsPrivate));

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
mud_telnet_naws_interface_init(MudTelnetHandlerInterface *iface)
{
    iface->Enable = mud_telnet_naws_enable;
    iface->Disable = mud_telnet_naws_disable;
    iface->HandleSubNeg = mud_telnet_naws_handle_sub_neg;
}

static void
mud_telnet_naws_init (MudTelnetNaws *self)
{
    /* Get our private data */
    self->priv = MUD_TELNET_NAWS_GET_PRIVATE(self);

    /* Set the defaults */
    self->priv->telnet = NULL;
    self->priv->option = TELOPT_NAWS;
    self->priv->enabled = FALSE;
}

static GObject *
mud_telnet_naws_constructor (GType gtype,
                              guint n_properties,
                              GObjectConstructParam *properties)
{
    MudTelnetNaws *self;
    GObject *obj;
    MudTelnetNawsClass *klass;
    GObjectClass *parent_class;

    /* Chain up to parent constructor */
    klass = MUD_TELNET_NAWS_CLASS( g_type_class_peek(MUD_TYPE_TELNET_NAWS) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    self = MUD_TELNET_NAWS(obj);

    if(!self->priv->telnet)
    {
        g_printf("ERROR: Tried to instantiate MudTelnetNaws without passing parent MudTelnet\n");
        g_error("Tried to instantiate MudTelnetNaws without passing parent MudTelnet");
    }

    return obj;
}

static void
mud_telnet_naws_finalize (GObject *object)
{
    MudTelnetNaws *self;
    GObjectClass *parent_class;

    self = MUD_TELNET_NAWS(object);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

static void
mud_telnet_naws_set_property(GObject *object,
                              guint prop_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
    MudTelnetNaws *self;

    self = MUD_TELNET_NAWS(object);

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
mud_telnet_naws_get_property(GObject *object,
                              guint prop_id,
                              GValue *value,
                              GParamSpec *pspec)
{
    MudTelnetNaws *self;

    self = MUD_TELNET_NAWS(object);

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
mud_telnet_naws_enable(MudTelnetHandler *handler)
{
    MudTelnetNaws *self;
    MudConnectionView *view;
    gint w, h;

    self = MUD_TELNET_NAWS(handler);

    g_return_if_fail(MUD_IS_TELNET_NAWS(self));

    self->priv->enabled = TRUE;

    mud_telnet_get_parent_size(self->priv->telnet, &w, &h);

    g_object_get(self->priv->telnet, "parent-view", &view, NULL);
    g_object_set(view,
                 "naws-enabled", TRUE,
                 NULL);

    mud_telnet_send_naws(self->priv->telnet, w, h);

    g_log("Telnet", G_LOG_LEVEL_INFO, "%s", "NAWS Enabled");
}

void
mud_telnet_naws_disable(MudTelnetHandler *handler)
{
    MudTelnetNaws *self;
    MudConnectionView *view;

    self = MUD_TELNET_NAWS(handler);

    g_return_if_fail(MUD_IS_TELNET_NAWS(self));

    self->priv->enabled = FALSE;

    g_object_get(self->priv->telnet, "parent-view", &view, NULL);
    g_object_set(view,
                 "naws-enabled", FALSE,
                 NULL);

    g_log("Telnet", G_LOG_LEVEL_INFO, "%s", "NAWS Disabled");
}

void
mud_telnet_naws_handle_sub_neg(MudTelnetHandler *handler,
                               guchar *buf,
                               guint len)
{
    MudTelnetNaws *self;

    self = MUD_TELNET_NAWS(handler);

    g_return_if_fail(MUD_IS_TELNET_NAWS(self));

    /* We never receive a NAWS subreq, we just send them */
    return;
}

/* Public Methods */
void
mud_telnet_naws_send(MudTelnetNaws *self, gint width, gint height)
{
    guchar w1, h1, w0, h0;
    
    g_return_if_fail(MUD_IS_TELNET_NAWS(self));

    w1 = (width >> 8) & 0xff;
    w0 = width & 0xff;

    h1 = (height >> 8) & 0xff;
    h0 = height & 0xff;

    mud_telnet_send_sub_req(self->priv->telnet, 5,
                            (guchar)self->priv->option,
                            w1, w0, h1, h0);
}

