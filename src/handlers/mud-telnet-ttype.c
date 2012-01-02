/* GNOME-Mud - A simple Mud Client
 * mud-telnet-ttype.c
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
#include <glib/gprintf.h>

#include "gnome-mud.h"
#include "mud-telnet.h"
#include "mud-telnet-handler-interface.h"
#include "mud-telnet-ttype.h"

struct _MudTelnetTTypePrivate
{
    /* Interface Properties */
    MudTelnet *telnet;
    gboolean enabled;
    gint option;

    /* Private Instance Members */
    gint ttype_iteration;
};

/* Property Identifiers */
enum
{
    PROP_MUD_TELNET_TTYPE_0,
    PROP_ENABLED,
    PROP_HANDLES_OPTION,
    PROP_TELNET
};

/* Class Functions */
static void mud_telnet_ttype_init (MudTelnetTType *self);
static void mud_telnet_ttype_class_init (MudTelnetTTypeClass *klass);
static void mud_telnet_ttype_interface_init(MudTelnetHandlerInterface *iface);
static void mud_telnet_ttype_finalize (GObject *object);
static GObject *mud_telnet_ttype_constructor (GType gtype,
                                              guint n_properties,
                                              GObjectConstructParam *properties);
static void mud_telnet_ttype_set_property(GObject *object,
                                          guint prop_id,
                                          const GValue *value,
                                          GParamSpec *pspec);
static void mud_telnet_ttype_get_property(GObject *object,
                                          guint prop_id,
                                          GValue *value,
                                          GParamSpec *pspec);

/*Interface Implementation */
void mud_telnet_ttype_enable(MudTelnetHandler *self);
void mud_telnet_ttype_disable(MudTelnetHandler *self);
void mud_telnet_ttype_handle_sub_neg(MudTelnetHandler *self,
                                     guchar *buf,
                                     guint len);

/* Create the Type. We implement MudTelnetHandlerInterface */
G_DEFINE_TYPE_WITH_CODE(MudTelnetTType, mud_telnet_ttype, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE (MUD_TELNET_HANDLER_TYPE,
                                               mud_telnet_ttype_interface_init));
/* MudTelnetTType class functions */
static void
mud_telnet_ttype_class_init (MudTelnetTTypeClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object constructor */
    object_class->constructor = mud_telnet_ttype_constructor;

    /* Override base object's finalize */
    object_class->finalize = mud_telnet_ttype_finalize;

    /* Override base object property methods */
    object_class->set_property = mud_telnet_ttype_set_property;
    object_class->get_property = mud_telnet_ttype_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(MudTelnetTTypePrivate));

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
mud_telnet_ttype_interface_init(MudTelnetHandlerInterface *iface)
{
    iface->Enable = mud_telnet_ttype_enable;
    iface->Disable = mud_telnet_ttype_disable;
    iface->HandleSubNeg = mud_telnet_ttype_handle_sub_neg;
}

static void
mud_telnet_ttype_init (MudTelnetTType *self)
{
    /* Get our private data */
    self->priv = MUD_TELNET_TTYPE_GET_PRIVATE(self);

    /* Set the defaults */
    self->priv->telnet = NULL;
    self->priv->option = TELOPT_TTYPE;
    self->priv->enabled = FALSE;
}

static GObject *
mud_telnet_ttype_constructor (GType gtype,
                              guint n_properties,
                              GObjectConstructParam *properties)
{
    MudTelnetTType *self;
    GObject *obj;
    MudTelnetTTypeClass *klass;
    GObjectClass *parent_class;

    /* Chain up to parent constructor */
    klass = MUD_TELNET_TTYPE_CLASS( g_type_class_peek(MUD_TYPE_TELNET_TTYPE) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    self = MUD_TELNET_TTYPE(obj);

    if(!self->priv->telnet)
    {
        g_printf("ERROR: Tried to instantiate MudTelnetTType without passing parent MudTelnet\n");
        g_error("Tried to instantiate MudTelnetTType without passing parent MudTelnet");
    }

    self->priv->ttype_iteration = 0;

    return obj;
}

static void
mud_telnet_ttype_finalize (GObject *object)
{
    GObjectClass *parent_class;

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

static void
mud_telnet_ttype_set_property(GObject *object,
                              guint prop_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
    MudTelnetTType *self;

    self = MUD_TELNET_TTYPE(object);

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
mud_telnet_ttype_get_property(GObject *object,
                              guint prop_id,
                              GValue *value,
                              GParamSpec *pspec)
{
    MudTelnetTType *self;

    self = MUD_TELNET_TTYPE(object);

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
mud_telnet_ttype_enable(MudTelnetHandler *handler)
{
    MudTelnetTType *self;

    self = MUD_TELNET_TTYPE(handler);

    g_return_if_fail(MUD_IS_TELNET_TTYPE(self));

    self->priv->enabled = TRUE;
    self->priv->ttype_iteration = 0;

    g_log("Telnet", G_LOG_LEVEL_INFO, "%s", "Terminal Type Enabled");
}

void
mud_telnet_ttype_disable(MudTelnetHandler *handler)
{
    MudTelnetTType *self;

    self = MUD_TELNET_TTYPE(handler);

    g_return_if_fail(MUD_IS_TELNET_TTYPE(self));

    self->priv->enabled = FALSE;

    g_log("Telnet", G_LOG_LEVEL_INFO, "%s", "Terminal Type Disabled");
}

void
mud_telnet_ttype_handle_sub_neg(MudTelnetHandler *handler,
                                guchar *buf,
                                guint len)
{
    MudTelnetTType *self;

    self = MUD_TELNET_TTYPE(handler);

    g_return_if_fail(MUD_IS_TELNET_TTYPE(self));

    g_log("Telnet", G_LOG_LEVEL_INFO, "%s", "Sending Terminal Type");

    if(self->priv->enabled)
    {
        switch(self->priv->ttype_iteration)
        {
            case 0:
                mud_telnet_send_sub_req(self->priv->telnet, 11,
                        (guchar)self->priv->option,
                        (guchar)TEL_TTYPE_IS,
                        'g','n','o','m','e','-','m','u','d');
                self->priv->ttype_iteration++;
                break;

            case 1:
                mud_telnet_send_sub_req(self->priv->telnet, 7,
                        (guchar)TELOPT_TTYPE,
                        (guchar)TEL_TTYPE_IS,
                        'x','t','e','r','m');
                self->priv->ttype_iteration++;
                break;

            case 2:
                mud_telnet_send_sub_req(self->priv->telnet, 6,
                        (guchar)TELOPT_TTYPE,
                        (guchar)TEL_TTYPE_IS,
                        'a','n','s','i');
                self->priv->ttype_iteration++;
                break;

            case 3:
                mud_telnet_send_sub_req(self->priv->telnet, 9,
                        (guchar)TELOPT_TTYPE,
                        (guchar)TEL_TTYPE_IS,
                        'U','N','K','N','O','W','N');
                self->priv->ttype_iteration++;
                break;

            /* RFC 1091 says we need to repeat the last type */
            case 4:
                mud_telnet_send_sub_req(self->priv->telnet, 9,
                        (guchar)TELOPT_TTYPE,
                        (gchar)TEL_TTYPE_IS,
                        'U','N','K','N','O','W','N');
                self->priv->ttype_iteration = 0;
                break;

        }
    }
}

