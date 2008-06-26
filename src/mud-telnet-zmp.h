/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1998-2006 Robin Ericsson <lobbin@localhost.nu>
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

#include <glib.h>
#include "mud-telnet.h"

typedef void(*MudZMPFunction)(MudTelnet *telnet, gint argc, gchar **argv);

typedef struct MudZMPCommand
{
	gchar *package;
	gchar *name;
	MudZMPFunction execute;
} MudZMPCommand;

void mud_zmp_init(MudTelnet *telnet);
void mud_zmp_finalize(MudTelnet *telnet);
void mud_zmp_register(MudTelnet *telnet, MudZMPCommand *command);
gboolean mud_zmp_has_command(MudTelnet *telnet, gchar *name);
MudZMPFunction mud_zmp_get_function(MudTelnet *telnet, gchar *name);

#endif // MUD_TELNET_ZMP_H

