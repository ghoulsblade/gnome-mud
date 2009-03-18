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
#include <stdlib.h>

#include "gnome-mud.h"
#include "mud-telnet.h"
#include "mud-connection-view.h"
#include "handlers/mud-telnet-handlers.h"
#include "zmp-main.h"
#include "zmp-package-interface.h"
#include "zmp-subwindow.h"

struct _ZmpSubwindowPrivate
{
    /* Interface Properties */
    gchar *package;
    MudTelnetZmp *parent;

    /* Private Instance Members */
};

/* Property Identifiers */
enum
{
    PROP_ZMP_SUBWINDOW_0,
    PROP_PACKAGE,
    PROP_PARENT
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
static void zmp_subwindow_select(MudTelnetZmp *self, gint argc, gchar **argv);
static void zmp_subwindow_do_input(MudSubwindow *sub,
                                   const gchar *input,
                                   ZmpSubwindow *self);
static void zmp_subwindow_size(MudSubwindow *sub,
                               guint width,
                               guint height,
                               ZmpSubwindow *self);

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

    g_object_class_override_property(object_class,
                                     PROP_PARENT,
                                     "parent");
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
    self->priv->parent = NULL;
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

    if(!self->priv->parent)
    {
        g_printf("ERROR: Tried to instantiate ZmpSubwindow without passing parent MudTelnetZMP\n");
        g_error("ERROR: Tried to instantiate ZmpSubwindow without passing parent MudTelnetZMP");
    }

    self->priv->package = g_strdup("subwindow");

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
        case PROP_PARENT:
            self->priv->parent = MUD_TELNET_ZMP(g_value_get_object(value));
            break;

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
                                              "subwindow.select",
                                              zmp_subwindow_select));

    /* If the server sends us these, its a broken server.
     * We send these commands to the server. */
    mud_zmp_register(zmp, mud_zmp_new_command("subwindow.",
                                              "subwindow.size",
                                              NULL));
    mud_zmp_register(zmp, mud_zmp_new_command("subwindow.",
                                              "subwindow.set-input",
                                              NULL));
}

/* ZmpSubwindow Commands */
static void
zmp_subwindow_open(MudTelnetZmp *self,
                   gint argc,
                   gchar **argv)
{
    MudConnectionView *view;
    MudTelnet *telnet;
    MudSubwindow *sub;
    ZmpSubwindow *pkg;
    ZmpMain *zmp_main;
    gboolean visible;

    if(argc != 5)
        return;

    g_object_get(self,
                 "telnet", &telnet,
                 "zmp-main", &zmp_main,
                 NULL);
    g_object_get(telnet, "parent-view", &view, NULL);
    
    pkg = ZMP_SUBWINDOW(zmp_main_get_package_by_name(zmp_main, "subwindow"));

    if(mud_connection_view_has_subwindow(view, argv[1]))
    {
        sub = mud_connection_view_get_subwindow(view, argv[1]);

        g_object_get(sub, "visible", &visible, NULL);

        if(!visible)
            mud_connection_view_show_subwindow(view, argv[1]);
        
        g_object_set(sub, "title", argv[2], NULL);
        mud_subwindow_set_title(sub, argv[2]);

        g_object_set(sub,
                     "old-width", (guint)atol(argv[3]),
                     "old-height", (guint)atol(argv[4]),
                     NULL);
        
        mud_subwindow_set_size(sub,
                               (guint)atol(argv[3]),
                               (guint)atol(argv[4]));
    }
    else
    {
        sub = mud_connection_view_create_subwindow(view,
                                                   argv[2],
                                                   argv[1],
                                                   (guint)atol(argv[3]),
                                                   (guint)atol(argv[4]));

        g_signal_connect(sub,
                         "resized",
                         G_CALLBACK(zmp_subwindow_size),
                         pkg);

        g_signal_connect(sub,
                         "input-received",
                         G_CALLBACK(zmp_subwindow_do_input),
                         pkg);
    }

    g_object_set(sub,
                 "old-width", (guint)atol(argv[3]),
                 "old-height", (guint)atol(argv[4]),
                 NULL);

    mud_zmp_send_command(self, 4,
                         "subwindow.size",
                         argv[1],
                         argv[3],
                         argv[4]);
}

static void
zmp_subwindow_close(MudTelnetZmp *self,
                    gint argc,
                    gchar **argv)
{
    MudConnectionView *view;
    MudTelnet *telnet;

    if(argc != 2)
        return;

    g_object_get(self, "telnet", &telnet, NULL);
    g_object_get(telnet, "parent-view", &view, NULL);

    mud_connection_view_remove_subwindow(view, argv[1]);
}

static void
zmp_subwindow_select(MudTelnetZmp *self,
                     gint argc,
                     gchar **argv)
{
    MudConnectionView *view;
    MudTelnet *telnet;

    if(argc != 2)
        return;

    g_object_get(self, "telnet", &telnet, NULL);
    g_object_get(telnet, "parent-view", &view, NULL);

    mud_connection_view_set_output(view, argv[1]);
}

static void
zmp_subwindow_do_input(MudSubwindow *sub,
                       const gchar *input,
                       ZmpSubwindow *self)
{
    MudConnectionView *view;
    gchar *identifier;

    g_object_get(sub,
                 "identifier", &identifier,
                 "parent-view", &view,
                 NULL);

    mud_zmp_send_command(self->priv->parent, 2,
                         "subwindow.set-input",
                         identifier);

    mud_connection_view_send(view, input);

    mud_zmp_send_command(self->priv->parent, 2,
                         "subwindow.set-input",
                         "main");

    g_free(identifier);
}

static void
zmp_subwindow_size(MudSubwindow *sub,
                   guint width,
                   guint height,
                   ZmpSubwindow *self)
{
    gchar *identifier;
    gchar *w, *h;
    guint old_w, old_h;

    g_object_get(sub,
                 "old-width", &old_w,
                 "old-height", &old_h,
                 NULL);

    if(width  != old_w ||
       height != old_h)
    {
        g_object_get(sub, "identifier", &identifier, NULL);
        g_object_set(sub,
                     "old-width", width,
                     "old-height", height,
                     NULL);

        w = g_strdup_printf("%d", width);
        h = g_strdup_printf("%d", height);

        mud_zmp_send_command(self->priv->parent, 4,
                "subwindow.size",
                identifier,
                w,
                h);

        g_free(w);
        g_free(h);
        g_free(identifier);
    }
}

