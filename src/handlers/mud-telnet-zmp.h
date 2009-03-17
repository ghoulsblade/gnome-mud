/* GNOME-Mud - A simple Mud Client
 * mud-telnet-zmp.h
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

#ifndef MUD_TELNET_ZMP_H
#define MUD_TELNET_ZMP_H

G_BEGIN_DECLS

#include <glib.h>

#define MUD_TYPE_TELNET_ZMP              (mud_telnet_zmp_get_type ())
#define MUD_TELNET_ZMP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_TELNET_ZMP, MudTelnetZmp))
#define MUD_TELNET_ZMP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_TELNET_ZMP, MudTelnetZmpClass))
#define MUD_IS_TELNET_ZMP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_TELNET_ZMP))
#define MUD_IS_TELNET_ZMP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_TELNET_ZMP))
#define MUD_TELNET_ZMP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_TELNET_ZMP, MudTelnetZmpClass))
#define MUD_TELNET_ZMP_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MUD_TYPE_TELNET_ZMP, MudTelnetZmpPrivate))

typedef struct _MudTelnetZmp            MudTelnetZmp;
typedef struct _MudTelnetZmpClass       MudTelnetZmpClass;
typedef struct _MudTelnetZmpPrivate     MudTelnetZmpPrivate;

struct _MudTelnetZmpClass
{
    GObjectClass parent_class;
};

struct _MudTelnetZmp
{
    GObject parent_instance;

    /*< private >*/
    MudTelnetZmpPrivate *priv;
};

typedef void(*MudZMPFunction)(MudTelnetZmp *self, gint argc, gchar **argv);

typedef struct MudZMPCommand
{
    gchar *package;
    gchar *name;
    MudZMPFunction execute;
} MudZMPCommand;

GType mud_telnet_zmp_get_type (void);

MudZMPCommand* mud_zmp_new_command(const gchar *package,
                                   const gchar *name,
                                   MudZMPFunction execute);

void mud_zmp_register(MudTelnetZmp *self, MudZMPCommand *command);

gboolean mud_zmp_has_command(MudTelnetZmp *self, gchar *name);
gboolean mud_zmp_has_package(MudTelnetZmp *self, gchar *package);
void mud_zmp_send_command(MudTelnetZmp *self, guint32 count, ...);

#endif // MUD_TELNET_ZMP_H

