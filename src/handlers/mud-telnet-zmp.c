/* GNOME-Mud - A simple Mud Client
 * mud-telnet-zmp.c
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
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include "gnome-mud.h"
#include "mud-telnet.h"
#include "mud-telnet-handler-interface.h"
#include "mud-telnet-zmp.h"

#include "zmp/zmp-main.h"

struct _MudTelnetZmpPrivate
{
    /* Interface Properties */
    MudTelnet *telnet;
    gboolean enabled;
    gint option;

    /* Private Instance Members */
    GHashTable *commands;
    ZmpMain *main_zmp;
};

/* Property Identifiers */
enum
{
    PROP_MUD_TELNET_ZMP_0,
    PROP_ENABLED,
    PROP_HANDLES_OPTION,
    PROP_TELNET
};

/* Class Functions */
static void mud_telnet_zmp_init (MudTelnetZmp *self);
static void mud_telnet_zmp_class_init (MudTelnetZmpClass *klass);
static void mud_telnet_zmp_interface_init(MudTelnetHandlerInterface *iface);
static void mud_telnet_zmp_finalize (GObject *object);
static GObject *mud_telnet_zmp_constructor (GType gtype,
                                             guint n_properties,
                                             GObjectConstructParam *properties);
static void mud_telnet_zmp_set_property(GObject *object,
                                         guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec);
static void mud_telnet_zmp_get_property(GObject *object,
                                         guint prop_id,
                                         GValue *value,
                                         GParamSpec *pspec);

/*Interface Implementation */
void mud_telnet_zmp_enable(MudTelnetHandler *self);
void mud_telnet_zmp_disable(MudTelnetHandler *self);
void mud_telnet_zmp_handle_sub_neg(MudTelnetHandler *self,
                                    guchar *buf,
                                    guint len);

/* Create the Type. We implement MudTelnetHandlerInterface */
G_DEFINE_TYPE_WITH_CODE(MudTelnetZmp, mud_telnet_zmp, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE (MUD_TELNET_HANDLER_TYPE,
                                               mud_telnet_zmp_interface_init));

/* Private Methods */
static void mud_zmp_destroy_key(gpointer k);
static void mud_zmp_destroy_command(gpointer c);
static MudZMPFunction mud_zmp_get_function(MudTelnetZmp *self, gchar *name);


/* MudTelnetZmp class functions */
static void
mud_telnet_zmp_class_init (MudTelnetZmpClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object constructor */
    object_class->constructor = mud_telnet_zmp_constructor;

    /* Override base object's finalize */
    object_class->finalize = mud_telnet_zmp_finalize;

    /* Override base object property methods */
    object_class->set_property = mud_telnet_zmp_set_property;
    object_class->get_property = mud_telnet_zmp_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(MudTelnetZmpPrivate));

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
mud_telnet_zmp_interface_init(MudTelnetHandlerInterface *iface)
{
    iface->Enable = mud_telnet_zmp_enable;
    iface->Disable = mud_telnet_zmp_disable;
    iface->HandleSubNeg = mud_telnet_zmp_handle_sub_neg;
}

static void
mud_telnet_zmp_init (MudTelnetZmp *self)
{
    /* Get our private data */
    self->priv = MUD_TELNET_ZMP_GET_PRIVATE(self);

    /* Set the defaults */
    self->priv->telnet = NULL;
    self->priv->option = TELOPT_ZMP;
    self->priv->enabled = FALSE;
}

static GObject *
mud_telnet_zmp_constructor (GType gtype,
                            guint n_properties,
                            GObjectConstructParam *properties)
{
    MudTelnetZmp *self;
    GObject *obj;
    MudTelnetZmpClass *klass;
    GObjectClass *parent_class;

    /* Chain up to parent constructor */
    klass = MUD_TELNET_ZMP_CLASS( g_type_class_peek(MUD_TYPE_TELNET_ZMP) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    self = MUD_TELNET_ZMP(obj);

    if(!self->priv->telnet)
    {
        g_printf("ERROR: Tried to instantiate MudTelnetZmp without passing parent MudTelnet\n");
        g_error("Tried to instantiate MudTelnetZmp without passing parent MudTelnet");
    }

    self->priv->commands = g_hash_table_new_full(g_str_hash,
                                                 g_str_equal,
                                                 mud_zmp_destroy_key,
                                                 mud_zmp_destroy_command);

    self->priv->main_zmp = g_object_new(ZMP_TYPE_MAIN,
                                        "parent-zmp", self,
                                        NULL);

    zmp_main_register_commands(self->priv->main_zmp);

    return obj;
}

static void
mud_telnet_zmp_finalize (GObject *object)
{
    MudTelnetZmp *self;
    GObjectClass *parent_class;

    self = MUD_TELNET_ZMP(object);

    if(self->priv->commands)
        g_hash_table_destroy(self->priv->commands);

    g_object_unref(self->priv->main_zmp);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

static void
mud_telnet_zmp_set_property(GObject *object,
                              guint prop_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
    MudTelnetZmp *self;

    self = MUD_TELNET_ZMP(object);

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
mud_telnet_zmp_get_property(GObject *object,
                              guint prop_id,
                              GValue *value,
                              GParamSpec *pspec)
{
    MudTelnetZmp *self;

    self = MUD_TELNET_ZMP(object);

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
mud_telnet_zmp_enable(MudTelnetHandler *handler)
{
    MudTelnetZmp *self;

    self = MUD_TELNET_ZMP(handler);

    g_return_if_fail(MUD_IS_TELNET_ZMP(self));

    self->priv->enabled = TRUE;

    g_log("Telnet", G_LOG_LEVEL_INFO, "%s", "ZMP Enabled");
}

void
mud_telnet_zmp_disable(MudTelnetHandler *handler)
{
    MudTelnetZmp *self;

    self = MUD_TELNET_ZMP(handler);

    g_return_if_fail(MUD_IS_TELNET_ZMP(self));

    /* Cannot disable ZMP once enabled per specification */
    return;

}

void
mud_telnet_zmp_handle_sub_neg(MudTelnetHandler *handler,
                              guchar *buf,
                              guint len)
{
    guint i;
    gint argc;
    gchar **argv;
    GString *args;
    MudZMPFunction zmp_handler = NULL;
    MudTelnetZmp *self = MUD_TELNET_ZMP(handler);

    g_return_if_fail(MUD_IS_TELNET_ZMP(self));

    /* Count Strings */
    for(i = 0; i < len; ++i)
        if(buf[i] == '\0')
            ++argc;

    argv = g_new0(gchar *, argc);
    argc = 0;
    args = g_string_new(NULL);

    for(i = 0; i < len; ++i)
    {
        if(buf[i] == '\0')
        {
            argv[argc++] = g_string_free(args, FALSE);
            args = g_string_new(NULL);
            continue;
        }

        args = g_string_append_c(args, buf[i]);
    }

    g_string_free(args, TRUE);

    g_log("Telnet", G_LOG_LEVEL_MESSAGE, "Received: ZMP Command: %s", argv[0]);
    for(i = 1; i < argc; ++i)
        g_log("Telnet", G_LOG_LEVEL_MESSAGE, "\t%s", argv[i]);

    if(mud_zmp_has_command(self, argv[0]))
    {
        zmp_handler = mud_zmp_get_function(self, argv[0]);

        if(zmp_handler)
            zmp_handler(self, argc, argv);
        else
            g_warning("NULL ZMP functioned returned.");
    }
    else
        g_warning("Server sent unsupported ZMP command. Bad server, bad.");

    for(i = 0; i < argc; ++i)
        g_free(argv[i]);

    g_free(argv);
}

/* Private Methods */
static void
mud_zmp_destroy_key(gpointer k)
{
    gchar *key = (gchar *)k;

    if(key)
        g_free(key);
}

static void
mud_zmp_destroy_command(gpointer c)
{
    MudZMPCommand *command = (MudZMPCommand *)c;

    if(command)
    {
        if(command->name)
            g_free(command->name);

        if(command->package)
            g_free(command->package);

        g_free(command);
    }
}

static MudZMPFunction
mud_zmp_get_function(MudTelnetZmp *self, gchar *name)
{
    MudZMPFunction ret = NULL;
    MudZMPCommand *val;

    g_return_if_fail(MUD_IS_TELNET_ZMP(self));

    if(!mud_zmp_has_command(self, name))
        return NULL;

    val = (MudZMPCommand *)g_hash_table_lookup(self->priv->commands, name);

    if(val)
        ret = (MudZMPFunction)val->execute;

    return ret;
}

/* Public Methods */
void
mud_zmp_send_command(MudTelnetZmp *self, guint32 count, ...)
{
    guchar byte;
    guint32 i;
    guint32 j;
    guchar null = '\0';
    va_list va;
    gchar *arg;
    GConn *conn;

    g_return_if_fail(MUD_IS_TELNET_ZMP(self));

    g_object_get(self->priv->telnet, "connection", &conn, NULL);

    va_start(va, count);

    g_log("Telnet", G_LOG_LEVEL_DEBUG, "Sending ZMP Command:");

    byte = (guchar)TEL_IAC;
    gnet_conn_write(conn, (gchar *)&byte, 1);
    byte = (guchar)TEL_SB;
    gnet_conn_write(conn, (gchar *)&byte, 1);
    byte = (guchar)TELOPT_ZMP;
    gnet_conn_write(conn, (gchar *)&byte, 1);

    for (i = 0; i < count; ++i)
    {
        arg = (gchar *)va_arg(va, gchar *);

        g_log("Telnet", G_LOG_LEVEL_DEBUG, "\t%s", arg);

        for(j = 0; j < strlen(arg); ++j)
        {
            byte = (guchar)arg[j];

            gnet_conn_write(conn, (gchar *)&byte, 1);

            if (byte == (guchar)TEL_IAC)
                gnet_conn_write(conn, (gchar *)&byte, 1);
        }

        gnet_conn_write(conn, (gchar *)&null, 1);
    }

    va_end(va);

    byte = (guchar)TEL_IAC;
    gnet_conn_write(conn, (gchar *)&byte, 1);
    byte = (guchar)TEL_SE;
    gnet_conn_write(conn, (gchar *)&byte, 1);
}

gboolean
mud_zmp_has_command(MudTelnetZmp *self, gchar *name)
{
    if(!MUD_IS_TELNET_ZMP(self))
        return FALSE;

    return !(g_hash_table_lookup(self->priv->commands, name) == NULL);
}

gboolean
mud_zmp_has_package(MudTelnetZmp *self, gchar *package)
{
    GList *keys;
    GList *iter;

    g_return_if_fail(MUD_IS_TELNET_ZMP(self));

    keys = g_hash_table_get_keys(self->priv->commands);

    for(iter = g_list_first(keys); iter != NULL; iter = g_list_next(iter))
    {
        MudZMPCommand *command =
            (MudZMPCommand *)g_hash_table_lookup(self->priv->commands,
                    (gchar *)iter->data);

        if(g_str_equal(command->package, package))
        {
            g_list_free(keys);
            return TRUE;
        }
    }

    g_list_free(keys);

    return FALSE;
}

void
mud_zmp_register(MudTelnetZmp *self, MudZMPCommand *command)
{
    g_return_if_fail(MUD_IS_TELNET_ZMP(self));

    if(!command)
        return;

    if(mud_zmp_has_command(self, command->name))
        return; // Function already registered.

    g_hash_table_replace(self->priv->commands,
                         g_strdup(command->name),
                         command);
}

MudZMPCommand *
mud_zmp_new_command(const gchar *package,
                    const gchar *name,
                    MudZMPFunction execute)
{
    MudZMPCommand *new_command = g_new0(MudZMPCommand, 1);

    if(!new_command)
        return NULL;

    new_command->name = g_strdup(name);
    new_command->package = g_strdup(package);
    new_command->execute = execute;

    return new_command;
}

