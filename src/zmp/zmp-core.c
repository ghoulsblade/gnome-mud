/* GNOME-Mud - A simple Mud Client
 * zmp-core.c
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
#include <string.h>

#include "gnome-mud.h"
#include "mud-telnet.h"
#include "handlers/mud-telnet-handlers.h"
#include "zmp-package-interface.h"
#include "zmp-core.h"

struct _ZmpCorePrivate
{
    /* Interface Properties */
    gchar *package;
    MudTelnetZmp *parent;

    /* Private Instance Members */
};

/* Property Identifiers */
enum
{
    PROP_ZMP_CORE_0,
    PROP_PACKAGE,
    PROP_PARENT
};

/* Class Functions */
static void zmp_core_init (ZmpCore *self);
static void zmp_core_class_init (ZmpCoreClass *klass);
static void zmp_core_interface_init(ZmpPackageInterface *iface);
static void zmp_core_finalize (GObject *object);
static GObject *zmp_core_constructor (GType gtype,
                                      guint n_properties,
                                      GObjectConstructParam *properties);
static void zmp_core_set_property(GObject *object,
                                  guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec);
static void zmp_core_get_property(GObject *object,
                                  guint prop_id,
                                  GValue *value,
                                  GParamSpec *pspec);

/*Interface Implementation */
static void zmp_core_register_commands(MudTelnetZmp *zmp);

/* ZmpCore zmp_core Commands */
static void zmp_core_ident(MudTelnetZmp *self, gint argc, gchar **argv);
static void zmp_core_ping_and_time(MudTelnetZmp *self, gint argc, gchar **argv);
static void zmp_core_check(MudTelnetZmp *self, gint argc, gchar **argv);

/* Create the Type. We implement ZmpPackageInterface */
G_DEFINE_TYPE_WITH_CODE(ZmpCore, zmp_core, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE (ZMP_PACKAGE_TYPE,
                                               zmp_core_interface_init));
/* ZmpCore class functions */
static void
zmp_core_class_init (ZmpCoreClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object constructor */
    object_class->constructor = zmp_core_constructor;

    /* Override base object's finalize */
    object_class->finalize = zmp_core_finalize;

    /* Override base object property methods */
    object_class->set_property = zmp_core_set_property;
    object_class->get_property = zmp_core_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(ZmpCorePrivate));

    /* Override Implementation Properties */
    g_object_class_override_property(object_class,
                                     PROP_PACKAGE,
                                     "package");

    g_object_class_override_property(object_class,
                                     PROP_PARENT,
                                     "parent");
}

static void
zmp_core_interface_init(ZmpPackageInterface *iface)
{
    iface->register_commands = zmp_core_register_commands;
}

static void
zmp_core_init (ZmpCore *self)
{
    /* Get our private data */
    self->priv = ZMP_CORE_GET_PRIVATE(self);

    /* Set the defaults */
    self->priv->package = NULL;
    self->priv->parent = NULL;
}

static GObject *
zmp_core_constructor (GType gtype,
                      guint n_properties,
                      GObjectConstructParam *properties)
{
    ZmpCore *self;
    GObject *obj;
    ZmpCoreClass *klass;
    GObjectClass *parent_class;

    /* Chain up to parent constructor */
    klass = ZMP_CORE_CLASS( g_type_class_peek(ZMP_TYPE_CORE) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    self = ZMP_CORE(obj);

    if(!self->priv->parent)
    {
        g_printf("ERROR: Tried to instantiate ZmpSubwindow without passing parent MudTelnetZMP\n");
        g_error("ERROR: Tried to instantiate ZmpSubwindow without passing parent MudTelnetZMP");
    }

    self->priv->package = g_strdup("zmp");

    return obj;
}

static void
zmp_core_finalize (GObject *object)
{
    ZmpCore *self;
    GObjectClass *parent_class;

    self = ZMP_CORE(object);

    g_free(self->priv->package);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

static void
zmp_core_set_property(GObject *object,
                      guint prop_id,
                      const GValue *value,
                      GParamSpec *pspec)
{
    ZmpCore *self;

    self = ZMP_CORE(object);

    switch(prop_id)
    {
        case PROP_PARENT:
            self->priv->parent = MUD_TELNET_ZMP(g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
zmp_core_get_property(GObject *object,
                      guint prop_id,
                      GValue *value,
                      GParamSpec *pspec)
{
    ZmpCore *self;

    self = ZMP_CORE(object);

    switch(prop_id)
    {
        case PROP_PARENT:
            g_value_take_object(value, self->priv->parent);
            break;

        case PROP_PACKAGE:
            g_value_set_string(value, self->priv->package);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

/* Interface Implementation */
static void
zmp_core_register_commands(MudTelnetZmp *zmp)
{
    g_return_if_fail(MUD_IS_TELNET_ZMP(zmp));

    mud_zmp_register(zmp, mud_zmp_new_command("zmp.",
                                              "zmp.ident",
                                              zmp_core_ident));
    mud_zmp_register(zmp, mud_zmp_new_command("zmp.",
                                              "zmp.ping",
                                              zmp_core_ping_and_time));
    mud_zmp_register(zmp, mud_zmp_new_command("zmp.",
                                              "zmp.time",
                                              zmp_core_ping_and_time));
    mud_zmp_register(zmp, mud_zmp_new_command("zmp.",
                                              "zmp.check",
                                              zmp_core_check));

    /* Client to Server Commands */
    mud_zmp_register(zmp, mud_zmp_new_command("zmp.",
                                              "zmp.input",
                                              NULL));
}

/* zmp.core Commands */
static void
zmp_core_ident(MudTelnetZmp *self, gint argc, gchar **argv)
{
    g_return_if_fail(MUD_IS_TELNET_ZMP(self));

    mud_zmp_send_command(self, 4,
            "zmp.ident", "gnome-mud", VERSION,
            "A mud client written for the GNOME environment.");
}

static void
zmp_core_ping_and_time(MudTelnetZmp *self, gint argc, gchar **argv)
{
    time_t t;
    gchar time_buffer[128];

    g_return_if_fail(MUD_IS_TELNET_ZMP(self));

    time(&t);

    strftime(time_buffer, sizeof(time_buffer),
            "%Y-%m-%d %H:%M:%S", gmtime(&t));

    mud_zmp_send_command(self, 2, "zmp.time", time_buffer);
}

static void
zmp_core_check(MudTelnetZmp *self, gint argc, gchar **argv)
{
    gchar *item;

    g_return_if_fail(MUD_IS_TELNET_ZMP(self));

    if(argc < 2)
        return; // Malformed zmp_core_Check

    item = argv[1];

    if(item[strlen(item) - 1] == '.') // Check for package.
    {
        if(mud_zmp_has_package(self, item))
            mud_zmp_send_command(self, 2, "zmp.support", item);
        else
            mud_zmp_send_command(self, 2, "zmp.no-support", item);
    }
    else // otherwise command
    {
        if(mud_zmp_has_command(self, item))
            mud_zmp_send_command(self, 2, "zmp.support", item);
        else
            mud_zmp_send_command(self, 2, "zmp.no-support", item);
    }
}

