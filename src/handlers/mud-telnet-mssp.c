/* GNOME-Mud - A simple Mud Client
 * mud-telnet-mssp.c
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
#include "mud-telnet-mssp.h"

struct _MudTelnetMsspPrivate
{
    /* Interface Properties */
    MudTelnet *telnet;
    gboolean enabled;
    gint option;

    /* Private Instance Members */
    GHashTable *mssp_data;
};

/* Property Identifiers */
enum
{
    PROP_MUD_TELNET_MSSP_0,
    PROP_ENABLED,
    PROP_HANDLES_OPTION,
    PROP_TELNET,
    PROP_MSSP
};

enum
{
    PARSE_STATE_VAR,
    PARSE_STATE_VAR_STRING,
    PARSE_STATE_VAL,
    PARSE_STATE_VAL_STRING
};

/* Class Functions */
static void mud_telnet_mssp_init (MudTelnetMssp *self);
static void mud_telnet_mssp_class_init (MudTelnetMsspClass *klass);
static void mud_telnet_mssp_interface_init(MudTelnetHandlerInterface *iface);
static void mud_telnet_mssp_finalize (GObject *object);
static GObject *mud_telnet_mssp_constructor (GType gtype,
                                             guint n_properties,
                                             GObjectConstructParam *properties);
static void mud_telnet_mssp_set_property(GObject *object,
                                         guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec);
static void mud_telnet_mssp_get_property(GObject *object,
                                         guint prop_id,
                                         GValue *value,
                                         GParamSpec *pspec);

/*Interface Implementation */
void mud_telnet_mssp_enable(MudTelnetHandler *self);
void mud_telnet_mssp_disable(MudTelnetHandler *self);
void mud_telnet_mssp_handle_sub_neg(MudTelnetHandler *self,
                                    guchar *buf,
                                    guint len);

/* Private Methods */
static void mud_telnet_mssp_destroy_key(gpointer k);
static void mud_telnet_mssp_destroy_value(gpointer c);

/* Create the Type. We implement MudTelnetHandlerInterface */
G_DEFINE_TYPE_WITH_CODE(MudTelnetMssp, mud_telnet_mssp, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE (MUD_TELNET_HANDLER_TYPE,
                                               mud_telnet_mssp_interface_init));
/* MudTelnetMssp class functions */
static void
mud_telnet_mssp_class_init (MudTelnetMsspClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object constructor */
    object_class->constructor = mud_telnet_mssp_constructor;

    /* Override base object's finalize */
    object_class->finalize = mud_telnet_mssp_finalize;

    /* Override base object property methods */
    object_class->set_property = mud_telnet_mssp_set_property;
    object_class->get_property = mud_telnet_mssp_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(MudTelnetMsspPrivate));

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
    /* Custom Class Properties */
    g_object_class_install_property(object_class,
                            PROP_MSSP,
                            g_param_spec_pointer("mssp-data",
                                                 "MSSP Data",
                                                 "The MSSP data provided by the mud.",
                                                 G_PARAM_READABLE));
}

static void
mud_telnet_mssp_interface_init(MudTelnetHandlerInterface *iface)
{
    iface->Enable = mud_telnet_mssp_enable;
    iface->Disable = mud_telnet_mssp_disable;
    iface->HandleSubNeg = mud_telnet_mssp_handle_sub_neg;
}

static void
mud_telnet_mssp_init (MudTelnetMssp *self)
{
    /* Get our private data */
    self->priv = MUD_TELNET_MSSP_GET_PRIVATE(self);

    /* Set the defaults */
    self->priv->telnet = NULL;
    self->priv->option = TELOPT_MSSP;
    self->priv->enabled = FALSE;

    self->priv->mssp_data = NULL;
}

static GObject *
mud_telnet_mssp_constructor (GType gtype,
                             guint n_properties,
                             GObjectConstructParam *properties)
{
    MudTelnetMssp *self;
    GObject *obj;
    MudTelnetMsspClass *klass;
    GObjectClass *parent_class;

    /* Chain up to parent constructor */
    klass = MUD_TELNET_MSSP_CLASS( g_type_class_peek(MUD_TYPE_TELNET_MSSP) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    self = MUD_TELNET_MSSP(obj);

    if(!self->priv->telnet)
    {
        g_printf("ERROR: Tried to instantiate MudTelnetMssp without passing parent MudTelnet\n");
        g_error("Tried to instantiate MudTelnetMssp without passing parent MudTelnet");
    }

    self->priv->mssp_data = g_hash_table_new_full(g_str_hash,
                                                  g_str_equal,
                                                  mud_telnet_mssp_destroy_key,
                                                  mud_telnet_mssp_destroy_value);

    return obj;
}

static void
mud_telnet_mssp_finalize (GObject *object)
{
    MudTelnetMssp *self;
    GObjectClass *parent_class;

    self = MUD_TELNET_MSSP(object);

    if(self->priv->mssp_data)
        g_hash_table_destroy(self->priv->mssp_data);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

static void
mud_telnet_mssp_set_property(GObject *object,
                             guint prop_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
    MudTelnetMssp *self;

    self = MUD_TELNET_MSSP(object);

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
mud_telnet_mssp_get_property(GObject *object,
                             guint prop_id,
                             GValue *value,
                             GParamSpec *pspec)
{
    MudTelnetMssp *self;

    self = MUD_TELNET_MSSP(object);

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

        case PROP_MSSP:
            g_value_set_pointer(value, self->priv->mssp_data);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

/* Interface Implementation */
void 
mud_telnet_mssp_enable(MudTelnetHandler *handler)
{
    MudTelnetMssp *self;

    self = MUD_TELNET_MSSP(handler);

    g_return_if_fail(MUD_IS_TELNET_MSSP(self));

    self->priv->enabled = TRUE;

    g_log("Telnet", G_LOG_LEVEL_INFO, "%s", "MSSP Enabled");
}

void
mud_telnet_mssp_disable(MudTelnetHandler *handler)
{
    MudTelnetMssp *self;

    self = MUD_TELNET_MSSP(handler);

    g_return_if_fail(MUD_IS_TELNET_MSSP(self));

    self->priv->enabled = FALSE;

    g_log("Telnet", G_LOG_LEVEL_INFO, "%s", "MSSP Disabled");
}

void
mud_telnet_mssp_handle_sub_neg(MudTelnetHandler *handler,
                               guchar *buf,
                               guint len)
{
    MudTelnetMssp *self;
    guint i, state;
    GString *key, *value;

    key = value = NULL;

    self = MUD_TELNET_MSSP(handler);

    g_return_if_fail(MUD_IS_TELNET_MSSP(self));

    state = PARSE_STATE_VAR;

    for(i = 0; i < len - 1; ++i)
    {
        switch(state)
        {
            case PARSE_STATE_VAR:
                key = g_string_new(NULL);

                state = PARSE_STATE_VAR_STRING;
                break;

            case PARSE_STATE_VAR_STRING:
                if( buf[i] != TEL_MSSP_VAL )
                    key = g_string_append_c(key, buf[i]);
                else
                {
                    state = PARSE_STATE_VAL;
                    i--;
                } 
                break;

            case PARSE_STATE_VAL:
                value = g_string_new(NULL);

                state = PARSE_STATE_VAL_STRING;
                break;

            case PARSE_STATE_VAL_STRING:
                if( buf[i] != TEL_MSSP_VAR && buf[i] != TEL_MSSP_VAL )
                    value = g_string_append_c(value, buf[i]);
                else
                {
                    switch( buf[i] )
                    {
                        /* VAR's can have multiple VALs assigned to them. */
                        case TEL_MSSP_VAL:
                            g_hash_table_replace(self->priv->mssp_data,
                                    g_strdup(key->str),
                                    g_string_free(value, FALSE));

                            state = PARSE_STATE_VAL;
                            i--;
                            break;

                        case TEL_MSSP_VAR:
                            g_hash_table_replace(self->priv->mssp_data,
                                    g_string_free(key, FALSE),
                                    g_string_free(value, FALSE));

                            state = PARSE_STATE_VAR;
                            i--;
                            break;
                    }
                }

                if( i + 1 == len - 1) // Last value in subnegotiation.
                    g_hash_table_replace(self->priv->mssp_data,
                                         g_string_free(key, FALSE),
                                         g_string_free(value, FALSE));

                break;
        }
    }

    /* Log the results */
    {
        GList *key_list, *entry;

        g_log("Telnet", G_LOG_LEVEL_MESSAGE, "%s", "MSSP Data:");

        key_list = g_hash_table_get_keys(self->priv->mssp_data);

        entry = key_list;

        while(entry)
        {
            const gchar *keyv = entry->data;
            const gchar *valv = g_hash_table_lookup(self->priv->mssp_data, keyv);

            g_log("Telnet",
                  G_LOG_LEVEL_MESSAGE,
                  "\t%s = %s",
                  keyv, (valv) ? valv : "<null>");

            entry = g_list_next(entry);
        }

        g_list_free(key_list);
    }
}

/* Private Methods */
static void
mud_telnet_mssp_destroy_key(gpointer k)
{
    gchar *key = (gchar *)k;

    if(key)
        g_free(key);
}

static void
mud_telnet_mssp_destroy_value(gpointer v)
{
    gchar *value = (gchar *)v;

    if(value)
        g_free(value);
}

