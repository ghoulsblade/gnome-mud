/* GNOME-Mud - A simple Mud Client
 * zmp-subwindow.c
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
#include "zmp-subwindow.h"

struct _ZmpSubwindowPrivate
{
    /* Interface Properties */
    gchar *package;

    /* Private Instance Members */
};

/* Property Identifiers */
enum
{
    PROP_ZMP_SUBWINDOW_0,
    PROP_PACKAGE,
};

/* Class Functions */
static void zmp_subwindow_init (ZmpSubwindow *self);
static void zmp_subwindow_class_init (ZmpSubwindowClass *klass);
static void zmp_subwindow_interface_init(ZmpPackageInterface *iface);
static void zmp_subwindow_finalize (GObject *object);
static GObject *zmp_subwindow_constructor (GType gtype,
                                           guint n_properties,
                                           GObjectConstructParam *properties);
static void zmp_subwindow_set_property(GObject *object,
                                       guint prop_id,
                                       const GValue *value,
                                       GParamSpec *pspec);
static void zmp_subwindow_get_property(GObject *object,
                                       guint prop_id,
                                       GValue *value,
                                       GParamSpec *pspec);

/* Interface Implementation */
static void zmp_subwindow_register_commands(MudTelnetZmp *zmp);

/* Subwindow Commands */
static void zmp_subwindow_open(MudTelnetZmp *self, gint argc, gchar **argv);
static void zmp_subwindow_close(MudTelnetZmp *self, gint argc, gchar **argv);
static void zmp_subwindow_size(MudTelnetZmp *self, gint argc, gchar **argv);
static void zmp_subwindow_set_input(MudTelnetZmp *self, gint argc, gchar **argv);
static void zmp_subwindow_select(MudTelnetZmp *self, gint argc, gchar **argv);

/* Create the Type. We implement ZmpPackageInterface */
G_DEFINE_TYPE_WITH_CODE(ZmpSubwindow, zmp_subwindow, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE (ZMP_PACKAGE_TYPE,
                                               zmp_subwindow_interface_init));
/* ZmpSubwindow class functions */
static void
zmp_subwindow_class_init (ZmpSubwindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object constructor */
    object_class->constructor = zmp_subwindow_constructor;

    /* Override base object's finalize */
    object_class->finalize = zmp_subwindow_finalize;

    /* Override base object property methods */
    object_class->set_property = zmp_subwindow_set_property;
    object_class->get_property = zmp_subwindow_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(ZmpSubwindowPrivate));

    /* Override Implementation Properties */
    g_object_class_override_property(object_class,
                                     PROP_PACKAGE,
                                     "package");
}

static void
zmp_subwindow_interface_init(ZmpPackageInterface *iface)
{
    iface->register_commands = zmp_subwindow_register_commands;
}

static void
zmp_subwindow_init (ZmpSubwindow *self)
{
    /* Get our private data */
    self->priv = ZMP_SUBWINDOW_GET_PRIVATE(self);

    /* Set the defaults */
    self->priv->package = NULL;
}

static GObject *
zmp_subwindow_constructor (GType gtype,
                           guint n_properties,
                           GObjectConstructParam *properties)
{
    ZmpSubwindow *self;
    GObject *obj;
    ZmpSubwindowClass *klass;
    GObjectClass *parent_class;

    /* Chain up to parent constructor */
    klass = ZMP_SUBWINDOW_CLASS( g_type_class_peek(ZMP_TYPE_SUBWINDOW) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    self = ZMP_SUBWINDOW(obj);

    self->priv->package = g_strdup("zmp.");

    return obj;
}

static void
zmp_subwindow_finalize (GObject *object)
{
    ZmpSubwindow *self;
    GObjectClass *parent_class;

    self = ZMP_SUBWINDOW(object);

    g_free(self->priv->package);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

static void
zmp_subwindow_set_property(GObject *object,
                           guint prop_id,
                           const GValue *value,
                           GParamSpec *pspec)
{
    ZmpSubwindow *self;

    self = ZMP_SUBWINDOW(object);

    switch(prop_id)
    {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
zmp_subwindow_get_property(GObject *object,
                           guint prop_id,
                           GValue *value,
                           GParamSpec *pspec)
{
    ZmpSubwindow *self;

    self = ZMP_SUBWINDOW(object);

    switch(prop_id)
    {
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
zmp_subwindow_register_commands(MudTelnetZmp *zmp)
{
    g_return_if_fail(MUD_IS_TELNET_ZMP(zmp));

    mud_zmp_register(zmp, mud_zmp_new_command("subwindow.",
                                              "subwindow.open",
                                              zmp_subwindow_open));
    mud_zmp_register(zmp, mud_zmp_new_command("subwindow.",
                                              "subwindow.close",
                                              zmp_subwindow_close));
    mud_zmp_register(zmp, mud_zmp_new_command("subwindow.",
                                              "subwindow.size",
                                              zmp_subwindow_size));
    mud_zmp_register(zmp, mud_zmp_new_command("subwindow.",
                                              "subwindow.set-input",
                                              zmp_subwindow_set_input));
    mud_zmp_register(zmp, mud_zmp_new_command("subwindow.",
                                              "subwindow.select",
                                              zmp_subwindow_select));
}

/* ZmpSubwindow Commands */
static void
zmp_subwindow_open(MudTelnetZmp *self, gint argc, gchar **argv)
{
}

static void
zmp_subwindow_close(MudTelnetZmp *self, gint argc, gchar **argv)
{
}

static void
zmp_subwindow_size(MudTelnetZmp *self, gint argc, gchar **argv)
{
}

static void
zmp_subwindow_set_input(MudTelnetZmp *self, gint argc, gchar **argv)
{
}

static void
zmp_subwindow_select(MudTelnetZmp *self, gint argc, gchar **argv)
{
}

