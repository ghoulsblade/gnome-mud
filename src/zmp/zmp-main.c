/* GNOME-Mud - A simple Mud Client
 * zmp-main.c
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

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glib/gprintf.h>

#include "gnome-mud.h"
#include "mud-telnet.h"
#include "handlers/mud-telnet-handlers.h"

#include "zmp-main.h"

#include "zmp-package-interface.h"
#include "zmp-core.h"
#include "zmp-subwindow.h"

struct _ZmpMainPrivate
{
    MudTelnetZmp *parent;
    GList *packages;
};

/* Property Identifiers */
enum
{
    PROP_ZMP_MAIN_0,
    PROP_PARENT
};

/* Create the Type */
G_DEFINE_TYPE(ZmpMain, zmp_main, G_TYPE_OBJECT);

/* Class Functions */
static void zmp_main_init (ZmpMain *self);
static void zmp_main_class_init (ZmpMainClass *klass);
static void zmp_main_finalize (GObject *object);
static GObject *zmp_main_constructor (GType gtype,
                                      guint n_properties,
                                      GObjectConstructParam *properties);
static void zmp_main_set_property(GObject *object,
                                  guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec);
static void zmp_main_get_property(GObject *object,
                                  guint prop_id,
                                  GValue *value,
                                  GParamSpec *pspec);

/* Private Methods */
static void zmp_main_unref_package(gpointer value,
                                   gpointer user_data);

/* ZmpMain class functions */
static void
zmp_main_class_init (ZmpMainClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object constructor */
    object_class->constructor = zmp_main_constructor;

    /* Override base object's finalize */
    object_class->finalize = zmp_main_finalize;

    /* Override base object property methods */
    object_class->set_property = zmp_main_set_property;
    object_class->get_property = zmp_main_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(ZmpMainPrivate));

    /* Install Properties */
    g_object_class_install_property(object_class,
            PROP_PARENT,
            g_param_spec_object("parent-zmp",
                "Parent ZMP",
                "The Parent MudTelnetZMP Object",
                MUD_TYPE_TELNET_ZMP,
                G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));
}

static void
zmp_main_init (ZmpMain *self)
{
    /* Get our private data */
    self->priv = ZMP_MAIN_GET_PRIVATE(self);

    /* set defaults */
    self->priv->parent = NULL;
    self->priv->packages = NULL;
}

static GObject *
zmp_main_constructor (GType gtype,
                      guint n_properties,
                      GObjectConstructParam *properties)
{
    ZmpMain *self;
    GObject *obj;
    ZmpMainClass *klass;
    GObjectClass *parent_class;

    /* Chain up to parent constructor */
    klass = ZMP_MAIN_CLASS( g_type_class_peek(ZMP_TYPE_MAIN) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    self = ZMP_MAIN(obj);

    if(!self->priv->parent)
    {
        g_printf("ERROR: Tried to instantiate ZmpMain without passing parent MudTelnetZmp\n");
        g_error("Tried to instantiate ZmpMain without passing parent MudTelnetZmp");
    }

    /* zmp.core */
    self->priv->packages = g_list_append(self->priv->packages,
                                         g_object_new(ZMP_TYPE_CORE,
                                                      "parent", self->priv->parent,
                                                      NULL));

    /* subwindow */
    self->priv->packages = g_list_append(self->priv->packages,
                                         g_object_new(ZMP_TYPE_SUBWINDOW,
                                                      "parent", self->priv->parent,
                                                      NULL));

    return obj;
}

static void
zmp_main_finalize (GObject *object)
{
    ZmpMain *self;
    GObjectClass *parent_class;

    self = ZMP_MAIN(object);

    g_list_foreach(self->priv->packages, zmp_main_unref_package, NULL);
    g_list_free(self->priv->packages);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

static void
zmp_main_set_property(GObject *object,
                      guint prop_id,
                      const GValue *value,
                      GParamSpec *pspec)
{
    ZmpMain *self;

    self = ZMP_MAIN(object);

    switch(prop_id)
    {
        /* Parent is Construct Only */
        case PROP_PARENT:
            self->priv->parent = MUD_TELNET_ZMP(g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
zmp_main_get_property(GObject *object,
                      guint prop_id,
                      GValue *value,
                      GParamSpec *pspec)
{
    ZmpMain *self;

    self = ZMP_MAIN(object);

    switch(prop_id)
    {
        case PROP_PARENT:
            g_value_take_object(value, self->priv->parent);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

/* Private Methods */
static void
zmp_main_unref_package(gpointer value,
                       gpointer user_data)
{
    ZmpPackage *package = ZMP_PACKAGE(value);

    if(package)
        g_object_unref(package);
}

/* Public Methods */
void
zmp_main_register_commands(ZmpMain *self)
{
    GList *entry;

    g_return_if_fail(ZMP_IS_MAIN(self));

    entry = g_list_first(self->priv->packages);

    while(entry)
    {
        ZmpPackage *package = ZMP_PACKAGE(entry->data);

        zmp_package_register_commands(package, self->priv->parent);

        entry = g_list_next(entry);
    }
}

ZmpPackage *
zmp_main_get_package_by_name(ZmpMain *self, const gchar *package_query)
{
    GList *entry;

    if(!ZMP_IS_MAIN(self))
        return NULL;

    entry = g_list_first(self->priv->packages);

    while(entry)
    {
        gchar *name;
        ZmpPackage *package = ZMP_PACKAGE(entry->data);

        g_object_get(package, "package", &name, NULL);

        if(g_str_equal(package_query, name))
        {
            g_free(name);
            return package;
        }

        g_free(name);

        entry = g_list_next(entry);
    }

    return NULL;
}

