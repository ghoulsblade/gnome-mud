/* GNOME-Mud - A simple Mud Client
 * mud-telnet-echo.c
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
#include "mud-telnet-echo.h"

struct _MudTelnetEchoPrivate
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
    PROP_MUD_TELNET_ECHO_0,
    PROP_ENABLED,
    PROP_HANDLES_OPTION,
    PROP_TELNET
};

/* Class Functions */
static void mud_telnet_echo_init (MudTelnetEcho *self);
static void mud_telnet_echo_class_init (MudTelnetEchoClass *klass);
static void mud_telnet_echo_interface_init(MudTelnetHandlerInterface *iface);
static void mud_telnet_echo_finalize (GObject *object);
static GObject *mud_telnet_echo_constructor (GType gtype,
                                             guint n_properties,
                                             GObjectConstructParam *properties);
static void mud_telnet_echo_set_property(GObject *object,
                                         guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec);
static void mud_telnet_echo_get_property(GObject *object,
                                         guint prop_id,
                                         GValue *value,
                                         GParamSpec *pspec);

/*Interface Implementation */
void mud_telnet_echo_enable(MudTelnetHandler *self);
void mud_telnet_echo_disable(MudTelnetHandler *self);
void mud_telnet_echo_handle_sub_neg(MudTelnetHandler *self,
                                    guchar *buf,
                                    guint len);

/* Create the Type. We implement MudTelnetHandlerInterface */
G_DEFINE_TYPE_WITH_CODE(MudTelnetEcho, mud_telnet_echo, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE (MUD_TELNET_HANDLER_TYPE,
                                               mud_telnet_echo_interface_init));
/* MudTelnetEcho class functions */
static void
mud_telnet_echo_class_init (MudTelnetEchoClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object constructor */
    object_class->constructor = mud_telnet_echo_constructor;

    /* Override base object's finalize */
    object_class->finalize = mud_telnet_echo_finalize;

    /* Override base object property methods */
    object_class->set_property = mud_telnet_echo_set_property;
    object_class->get_property = mud_telnet_echo_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(MudTelnetEchoPrivate));

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
mud_telnet_echo_interface_init(MudTelnetHandlerInterface *iface)
{
    iface->Enable = mud_telnet_echo_enable;
    iface->Disable = mud_telnet_echo_disable;
    iface->HandleSubNeg = mud_telnet_echo_handle_sub_neg;
}

static void
mud_telnet_echo_init (MudTelnetEcho *self)
{
    /* Get our private data */
    self->priv = MUD_TELNET_ECHO_GET_PRIVATE(self);

    /* Set the defaults */
    self->priv->telnet = NULL;
    self->priv->option = TELOPT_ECHO;
    self->priv->enabled = FALSE;
}

static GObject *
mud_telnet_echo_constructor (GType gtype,
                              guint n_properties,
                              GObjectConstructParam *properties)
{
    MudTelnetEcho *self;
    GObject *obj;
    MudTelnetEchoClass *klass;
    GObjectClass *parent_class;

    /* Chain up to parent constructor */
    klass = MUD_TELNET_ECHO_CLASS( g_type_class_peek(MUD_TYPE_TELNET_ECHO) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    self = MUD_TELNET_ECHO(obj);

    if(!self->priv->telnet)
    {
        g_printf("ERROR: Tried to instantiate MudTelnetEcho without passing parent MudTelnet\n");
        g_error("Tried to instantiate MudTelnetEcho without passing parent MudTelnet");
    }

    return obj;
}

static void
mud_telnet_echo_finalize (GObject *object)
{
    MudTelnetEcho *self;
    GObjectClass *parent_class;

    self = MUD_TELNET_ECHO(object);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

static void
mud_telnet_echo_set_property(GObject *object,
                              guint prop_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
    MudTelnetEcho *self;

    self = MUD_TELNET_ECHO(object);

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
mud_telnet_echo_get_property(GObject *object,
                              guint prop_id,
                              GValue *value,
                              GParamSpec *pspec)
{
    MudTelnetEcho *self;

    self = MUD_TELNET_ECHO(object);

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
mud_telnet_echo_enable(MudTelnetHandler *handler)
{
    MudTelnetEcho *self;
    MudConnectionView *view;

    self = MUD_TELNET_ECHO(handler);

    g_return_if_fail(MUD_IS_TELNET_ECHO(self));

    self->priv->enabled = TRUE;

    g_object_get(self->priv->telnet, "parent-view", &view, NULL);
    g_object_set(view,
                 "local-echo", FALSE,
                 NULL);

    g_log("Telnet", G_LOG_LEVEL_INFO, "%s", "ECHO Enabled");
}

void
mud_telnet_echo_disable(MudTelnetHandler *handler)
{
    MudTelnetEcho *self;
    MudConnectionView *view;

    self = MUD_TELNET_ECHO(handler);

    g_return_if_fail(MUD_IS_TELNET_ECHO(self));

    self->priv->enabled = FALSE;

    g_object_get(self->priv->telnet, "parent-view", &view, NULL);
    g_object_set(view,
                 "local-echo", TRUE,
                 NULL);

    g_log("Telnet", G_LOG_LEVEL_INFO, "%s", "ECHO Disabled");
}

void
mud_telnet_echo_handle_sub_neg(MudTelnetHandler *handler,
                               guchar *buf,
                               guint len)
{
    MudTelnetEcho *self;

    self = MUD_TELNET_ECHO(handler);

    g_return_if_fail(MUD_IS_TELNET_ECHO(self));

    /* There is no ECHO subreq.*/
    return;
}

