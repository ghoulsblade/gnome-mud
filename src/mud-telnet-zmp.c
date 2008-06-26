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

/* Code originally from wxMUD. Converted to a GObject by Les Harris.
 * wxMUD - an open source cross-platform MUD client.
 * Copyright (C) 2003-2008 Mart Raudsepp and Gabriel Levin
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
#include "mud-telnet-zmp.h"

static void mud_zmp_register_core_functions(MudTelnet *telnet);
static void mud_zmp_send_command(MudTelnet *telnet, guint32 count, ...);
static void mud_zmp_destroy_key(gpointer k);
static void mud_zmp_destroy_command(gpointer c);

/* Core ZMP Functions */
static void ZMP_ident(MudTelnet *telnet, gint argc, gchar **argv);
static void ZMP_ping_and_time(MudTelnet *telnet, gint argc, gchar **argv);
static void ZMP_check(MudTelnet *telnet, gint argc, gchar **argv);

/* Public Interface */
void 
mud_zmp_init(MudTelnet *telnet)
{
	telnet->zmp_commands = g_hash_table_new_full(g_str_hash, g_str_equal,
		mud_zmp_destroy_key, mud_zmp_destroy_command);

	mud_zmp_register_core_functions(telnet);
}

void 
mud_zmp_finalize(MudTelnet *telnet)
{
	if(telnet->zmp_commands)
		g_hash_table_destroy(telnet->zmp_commands);
}

void
mud_zmp_register(MudTelnet *telnet, MudZMPCommand *command)
{		
	if(command && mud_zmp_has_command(telnet, command->name))
		return; // Function already registered.
	
	g_hash_table_insert(telnet->zmp_commands, g_strdup(command->name), command);
}

gboolean
mud_zmp_has_command(MudTelnet *telnet, gchar *name)
{
	return !(g_hash_table_lookup(telnet->zmp_commands, name) == NULL);
}

gboolean
mud_zmp_has_package(MudTelnet *telnet, gchar *package)
{
	GList *keys = g_hash_table_get_keys(telnet->zmp_commands);
	GList *iter;
	
	for(iter = g_list_first(keys); iter != NULL; iter = g_list_next(iter))
	{
		MudZMPCommand *command = 
			(MudZMPCommand *)g_hash_table_lookup(telnet->zmp_commands, 
			(gchar *)iter->data);
		
		if(strcmp(command->package, package) == 0)
		{
			g_list_free(keys);
			return TRUE;
		}
	}
	
	g_list_free(keys);
	
	return FALSE;
}

MudZMPFunction
mud_zmp_get_function(MudTelnet *telnet, gchar *name)
{
	MudZMPFunction ret = NULL;
	MudZMPCommand *val;
	
	if(!mud_zmp_has_command(telnet, name))
		return NULL;
	
	val = (MudZMPCommand *)g_hash_table_lookup(telnet->zmp_commands, name);
	
	if(val)
		ret = (MudZMPFunction)val->execute;
	
	return ret;
}

/* Private Methods */
static void
mud_zmp_send_command(MudTelnet *telnet, guint32 count, ...)
{
    guchar byte;
    guint32 i;
    guint32 j;
	guchar null = '\0';
	byte = (guchar)TEL_IAC;
	va_list va;
	va_start(va, count);
	gchar *arg;
	
	gnet_conn_write(telnet->conn, (gchar *)&byte, 1);
	byte = (guchar)TEL_SB;
	gnet_conn_write(telnet->conn, (gchar *)&byte, 1);
    byte = (guchar)TELOPT_ZMP;
    gnet_conn_write(telnet->conn, (gchar *)&byte, 1);
    
    for (i = 0; i < count; ++i)
	{
		arg = (gchar *)va_arg(va, gchar *);
		
		for(j = 0; j < strlen(arg); ++j)
		{
			byte = (guchar)arg[j];
			
			gnet_conn_write(telnet->conn, (gchar *)&byte, 1);
			
			if (byte == (guchar)TEL_IAC)
				gnet_conn_write(telnet->conn, (gchar *)&byte, 1);
		}
		
		gnet_conn_write(telnet->conn, (gchar *)&null, 1);
	}

	va_end(va);
	
	byte = (guchar)TEL_IAC;
	gnet_conn_write(telnet->conn, (gchar *)&byte, 1);
	byte = (guchar)TEL_SE;
	gnet_conn_write(telnet->conn, (gchar *)&byte, 1);
}

static void
mud_zmp_register_core_functions(MudTelnet *telnet)
{
	gint count = 4;
	gint i;
	
	telnet->commands[0].name = g_strdup("zmp.ident");
	telnet->commands[0].package = g_strdup("zmp.");
	telnet->commands[0].execute = ZMP_ident;
	
	telnet->commands[1].name = g_strdup("zmp.ping");
	telnet->commands[1].package = g_strdup("zmp.");
	telnet->commands[1].execute = ZMP_ping_and_time;
	
	telnet->commands[2].name = g_strdup("zmp.time");
	telnet->commands[2].package = g_strdup("zmp.");
	telnet->commands[2].execute = ZMP_ping_and_time;
	
	telnet->commands[3].name = g_strdup("zmp.check");
	telnet->commands[3].package = g_strdup("zmp.");
	telnet->commands[3].execute = ZMP_check;
	
	for(i = 0; i < count; ++i)
		mud_zmp_register(telnet, &telnet->commands[i]);
}

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
	}
}

/* Core ZMP Functions */
static void
ZMP_ident(MudTelnet *telnet, gint argc, gchar **argv)
{
	mud_zmp_send_command(telnet, 4, 
		"zmp.ident", "gnome-mud", "0.11", 
		"A mud client written for the GNOME environment.");
}

static void 
ZMP_ping_and_time(MudTelnet *telnet, gint argc, gchar **argv)
{
	time_t t;
	time(&t);
	gchar time_buffer[128];
	
	strftime(time_buffer, sizeof(time_buffer), 
		"%Y-%m-%d %H:%M:%S", gmtime(&t));

	mud_zmp_send_command(telnet, 2, "zmp.time", time_buffer);
}

static void
ZMP_check(MudTelnet *telnet, gint argc, gchar **argv)
{
	gchar *item = argv[1];
	
	if(item[strlen(item) - 1] == '.') // Check for package.
	{
		if(mud_zmp_has_package(telnet, item))
			mud_zmp_send_command(telnet, 2, "zmp.support", item);
		else
			mud_zmp_send_command(telnet, 2, "zmp.no-support", item);
	}
	else // otherwise command
	{
		if(mud_zmp_has_command(telnet, item))
			mud_zmp_send_command(telnet, 2, "zmp.support", item);
		else
			mud_zmp_send_command(telnet, 2, "zmp.no-support", item);
	}	
}

