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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef ENABLE_GST

#include <glib.h>
#include <gnet.h>
#include <string.h>
#include <gst/gst.h>
#include <ctype.h>

#include "gnome-mud.h"
#include "mud-telnet.h"
#include "mud-telnet-msp.h"

static void mud_telnet_msp_parser_reset(MudTelnet *telnet);
static void mud_telnet_msp_parser_args(MudTelnet *telnet);
static void mud_telnet_msp_command_free(MudMSPCommand *command);
static gboolean mud_telnet_msp_parser_is_param_char(gchar c);
static gboolean mud_telnet_msp_parser_switch_on_param_char(gint *state, gchar *buf, gint index, gint len);
static void mud_telnet_msp_process_command(MudTelnet *telnet, MudMSPCommand *command);
static void mud_telnet_msp_stop_playing(MudTelnet *telnet, MudMSPTypes type);
static void mud_telnet_msp_start_playing(MudTelnet *telnet, MudMSPTypes type);
static gboolean mud_telnet_msp_get_files(MudTelnet *telnet, MudMSPTypes type);
static gboolean mud_telnet_msp_sound_bus_call (GstBus *bus, GstMessage *msg, gpointer data);
static gboolean mud_telnet_msp_music_bus_call (GstBus *bus, GstMessage *msg, gpointer data);

GString *
mud_telnet_msp_parse(MudTelnet *telnet, GString *buf, gint *len)
{
	gint count;
	GString *ret = NULL;
	gchar *temp;

	mud_telnet_msp_parser_reset(telnet);

	if(telnet->prev_buffer)
	{
		g_string_prepend(buf, telnet->prev_buffer->str);
		g_string_free(telnet->prev_buffer, TRUE);
		telnet->prev_buffer = NULL;
	}

	while(telnet->msp_parser.lex_pos_start < *len)
	{
		switch(telnet->msp_parser.state)
		{
			case MSP_STATE_TEXT:
				if(buf->str[telnet->msp_parser.lex_pos_start] == '!')
					telnet->msp_parser.state = MSP_STATE_POSSIBLE_COMMAND;
				else
				{
					g_string_append_c(telnet->msp_parser.output,
						buf->str[telnet->msp_parser.lex_pos_start++]);
				}
			break;

			case MSP_STATE_POSSIBLE_COMMAND:
				if(telnet->msp_parser.lex_pos_start + 1 == *len)
					continue;
				else if(buf->str[telnet->msp_parser.lex_pos_start + 1] != '!')
				{
					g_string_append_c(telnet->msp_parser.output,
						buf->str[telnet->msp_parser.lex_pos_start++]);
					telnet->msp_parser.state = MSP_STATE_TEXT;
					continue;
				}

				telnet->msp_parser.state = MSP_STATE_COMMAND;
			break;

			case MSP_STATE_COMMAND:
				if(telnet->msp_parser.lex_pos_start + 8 >= *len)
				{
					telnet->prev_buffer = g_string_new(NULL);

					count = telnet->msp_parser.lex_pos_start;

					while(count != buf->len)
						g_string_append_c(telnet->prev_buffer, buf->str[count++]);

					telnet->msp_parser.lex_pos_start += count;
					continue;
				}

				if(buf->str[telnet->msp_parser.lex_pos_start + 2] == 'S' &&
				   buf->str[telnet->msp_parser.lex_pos_start + 3] == 'O' &&
				   buf->str[telnet->msp_parser.lex_pos_start + 4] == 'U' &&
				   buf->str[telnet->msp_parser.lex_pos_start + 5] == 'N' &&
				   buf->str[telnet->msp_parser.lex_pos_start + 6] == 'D')
				   	telnet->msp_type = MSP_TYPE_SOUND;
				else if(buf->str[telnet->msp_parser.lex_pos_start + 2] == 'M' &&
				   		buf->str[telnet->msp_parser.lex_pos_start + 3] == 'U' &&
				   		buf->str[telnet->msp_parser.lex_pos_start + 4] == 'S' &&
				   		buf->str[telnet->msp_parser.lex_pos_start + 5] == 'I' &&
				   		buf->str[telnet->msp_parser.lex_pos_start + 6] == 'C')
				   			telnet->msp_type = MSP_TYPE_MUSIC;
				else
				{
					/* Not an msp command, bail out. */
					g_string_append_c(telnet->msp_parser.output, buf->str[telnet->msp_parser.lex_pos_start++]);
					g_string_append_c(telnet->msp_parser.output, buf->str[telnet->msp_parser.lex_pos_start++]);

					telnet->msp_parser.state = MSP_STATE_TEXT;
					continue;
				}

				// Skip leading (
				telnet->msp_parser.lex_pos_start += 8;
				telnet->msp_parser.state = MSP_STATE_GET_ARGS;
				continue;
			break;

			case MSP_STATE_GET_ARGS:
				telnet->msp_parser.lex_pos_end = telnet->msp_parser.lex_pos_start;

				if(telnet->msp_parser.arg_buffer == NULL)
					telnet->msp_parser.arg_buffer = g_string_new(NULL);
				else
				{
					/* This stops some craziness where g_string_append_c
					   doesn't actually update the gstring. Glib bug? */
					temp = g_strdup(telnet->msp_parser.arg_buffer->str);
					g_string_free(telnet->msp_parser.arg_buffer, TRUE);
					telnet->msp_parser.arg_buffer = g_string_new(temp);
					g_free(temp);
				}

				while(telnet->msp_parser.lex_pos_end < *len && buf->str[telnet->msp_parser.lex_pos_end] != ')')
					g_string_append_c(telnet->msp_parser.arg_buffer, buf->str[telnet->msp_parser.lex_pos_end++]);

				if(telnet->msp_parser.lex_pos_end >= *len && buf->str[telnet->msp_parser.lex_pos_end - 1] != ')')
				{
					telnet->msp_parser.lex_pos_start = telnet->msp_parser.lex_pos_end;
					continue;
				}

				telnet->msp_parser.state = MSP_STATE_PARSE_ARGS;

			break;

			case MSP_STATE_PARSE_ARGS:
				mud_telnet_msp_parser_args(telnet);

				g_string_free(telnet->msp_parser.arg_buffer, TRUE);
				telnet->msp_parser.arg_buffer = NULL;
				telnet->msp_parser.lex_pos_start = telnet->msp_parser.lex_pos_end + 2;
				telnet->msp_parser.state = MSP_STATE_TEXT;
			break;
		}
	}

	if(telnet->msp_parser.state == MSP_STATE_TEXT)
	{
		ret = g_string_new(g_strdup(telnet->msp_parser.output->str));
		*len = telnet->msp_parser.output->len;
	}

	return ret;
}

void
mud_telnet_msp_init(MudTelnet *telnet)
{
	telnet->msp_parser.enabled = TRUE;
	telnet->msp_parser.state = MSP_STATE_TEXT;
	telnet->msp_parser.lex_pos_start = 0;
	telnet->msp_parser.lex_pos_end = 0;
	telnet->msp_parser.output = g_string_new(NULL);
	telnet->msp_parser.arg_buffer = NULL;
}

void
mud_telnet_msp_parser_clear(MudTelnet *telnet)
{
	if(telnet->msp_parser.output)
		g_string_free(telnet->msp_parser.output, TRUE);

	telnet->msp_parser.lex_pos_start = 0;
	telnet->msp_parser.lex_pos_end = 0;
	telnet->msp_parser.output = g_string_new(NULL);
}

void
mud_telnet_msp_download_item_free(MudMSPDownloadItem *item)
{
	if(!item)
		return;

	if(item->url)
		g_free(item->url);

	if(item->file)
		g_free(item->file);

	g_free(item);
}

static void
mud_telnet_msp_parser_reset(MudTelnet *telnet)
{
	telnet->msp_parser.lex_pos_start = 0;
	telnet->msp_parser.lex_pos_end = 0;
}

#define ARG_STATE_FILE 0
#define ARG_STATE_V 1
#define ARG_STATE_L 2
#define ARG_STATE_C 3
#define ARG_STATE_T 4
#define ARG_STATE_U 5
#define ARG_STATE_P 6

static void
mud_telnet_msp_parser_args(MudTelnet *telnet)
{
	gint state = ARG_STATE_FILE;
	gint i;
	GString *buffer = g_string_new(NULL);
	gchar *args = g_strdup(telnet->msp_parser.arg_buffer->str);
	gint len = strlen(args);
	MudMSPCommand *command = g_new0(MudMSPCommand, 1);

	command->type = telnet->msp_type;
	command->fName = NULL;
	command->V = NULL;
	command->L = NULL;
	command->C = NULL;
	command->T = NULL;
	command->U = NULL;
	command->P = NULL;

	command->mud_name = g_strdup(telnet->mud_name);
	command->sfx_type = NULL;

    /* Load defaults */
	command->volume = 100;
	command->priority = 50;
	command->initial_repeat_count = 1;
	command->current_repeat_count = 1;
	command->loop = FALSE;
	command->cont = (telnet->msp_type == MSP_TYPE_MUSIC);

	for(i = 0; i < len; ++i)
	{
		if(args[i] == ' ' || args[i] == '=' || args[i] == '"')
			continue;

		switch(state)
		{
			case ARG_STATE_FILE:
				if(mud_telnet_msp_parser_is_param_char(args[i]) &&
				   mud_telnet_msp_parser_switch_on_param_char(&state, args, i, len))
				   {
				   		command->fName = g_strdup(buffer->str);
				   		g_string_free(buffer, TRUE);
				   		buffer = g_string_new(NULL);
				   }
				   else
					  g_string_append_c(buffer, args[i]);
			break;

			case ARG_STATE_V:
				if(mud_telnet_msp_parser_is_param_char(args[i]) &&
				   mud_telnet_msp_parser_switch_on_param_char(&state, args, i, len))
				   {
				   		command->V = g_strdup(buffer->str);
				   		g_string_free(buffer, TRUE);
				   		buffer = g_string_new(NULL);
				   }
				   else
					  g_string_append_c(buffer, args[i]);
			break;

			case ARG_STATE_L:
				if(mud_telnet_msp_parser_is_param_char(args[i]) &&
				   mud_telnet_msp_parser_switch_on_param_char(&state, args, i, len))
				   {
				   		command->L = g_strdup(buffer->str);
				   		g_string_free(buffer, TRUE);
				   		buffer = g_string_new(NULL);
				   }
				   else
					  g_string_append_c(buffer, args[i]);
			break;

			case ARG_STATE_C:
				if(mud_telnet_msp_parser_is_param_char(args[i]) &&
				   mud_telnet_msp_parser_switch_on_param_char(&state, args, i, len))
				   {
				   		command->C = g_strdup(buffer->str);
				   		g_string_free(buffer, TRUE);
				   		buffer = g_string_new(NULL);
				   }
				   else
					  g_string_append_c(buffer, args[i]);
			break;

			case ARG_STATE_T:
				if(mud_telnet_msp_parser_is_param_char(args[i]) &&
				   mud_telnet_msp_parser_switch_on_param_char(&state, args, i, len))
				   {
				   		command->T = g_strdup(buffer->str);
				   		g_string_free(buffer, TRUE);
				   		buffer = g_string_new(NULL);
				   }
				   else
					  g_string_append_c(buffer, args[i]);
			break;

			case ARG_STATE_U:
				if(mud_telnet_msp_parser_is_param_char(args[i]) &&
				   mud_telnet_msp_parser_switch_on_param_char(&state, args, i, len))
				   {
				   		if(buffer->str[buffer->len - 1] != '/')
				   			g_string_append_c(buffer, '/');

				   		command->U = g_strdup(buffer->str);
				   		g_string_free(buffer, TRUE);
				   		buffer = g_string_new(NULL);
				   }
				   else
					  g_string_append_c(buffer, args[i]);
			break;

			case ARG_STATE_P:
				if(mud_telnet_msp_parser_is_param_char(args[i]) &&
				   mud_telnet_msp_parser_switch_on_param_char(&state, args, i, len))
				   {
				   		command->P = g_strdup(buffer->str);
				   		g_string_free(buffer, TRUE);
				   		buffer = g_string_new(NULL);
				   }
				   else
					  g_string_append_c(buffer, args[i]);
			break;
		}
	}

	switch(state)
	{
		case ARG_STATE_FILE:
			command->fName = g_strdup(buffer->str);
		break;

		case ARG_STATE_V:
			command->V = g_strdup(buffer->str);
		break;

		case ARG_STATE_L:
			command->L = g_strdup(buffer->str);
		break;

		case ARG_STATE_C:
			command->C = g_strdup(buffer->str);
		break;

		case ARG_STATE_T:
			command->T = g_strdup(buffer->str);
		break;

		case ARG_STATE_U:
	   		if(buffer->str[buffer->len - 1] != '/')
	   			g_string_append_c(buffer, '/');

			command->U = g_strdup(buffer->str);
		break;

		case ARG_STATE_P:
			command->P = g_strdup(buffer->str);
		break;
	}

	if(command->C)
		command->cont = atoi(command->C);

	if(command->T)
		command->sfx_type = g_strdup(command->T);

	if(command->V)
		command->volume = atoi(command->V);

	if(command->P)
		command->priority = atoi(command->P);

	if(command->L)
	{
		command->initial_repeat_count = atoi(command->L);

		if(command->initial_repeat_count == 0)
			command->initial_repeat_count = 1;

		command->current_repeat_count = command->initial_repeat_count;

		if(command->current_repeat_count == -1)
			command->loop = TRUE;
	}

	mud_telnet_msp_process_command(telnet, command);

	g_free(args);
	g_string_free(buffer, TRUE);
}

static gboolean
mud_telnet_msp_parser_is_param_char(gchar c)
{
	return (c == 'V' || c == 'L' || c == 'C' ||
			c == 'T' || c == 'U' || c == 'P');
}

static gboolean
mud_telnet_msp_parser_switch_on_param_char(gint *state, gchar *buf, gint index, gint len)
{
	if(index + 1 == len)
		return FALSE;

	if(buf[index + 1] != '=')
		return FALSE;

	switch(buf[index])
	{
			case 'V':
				*state = ARG_STATE_V;
				return TRUE;
			break;

			case 'L':
				*state = ARG_STATE_L;
				return TRUE;
			break;

			case 'C':
				*state = ARG_STATE_C;
				return TRUE;
			break;

			case 'T':
				*state = ARG_STATE_T;
				return TRUE;
			break;

			case 'U':
				*state = ARG_STATE_U;
				return TRUE;
			break;

			case 'P':
				*state = ARG_STATE_P;
				return TRUE;
			break;
	}

	return FALSE;
}

static void
mud_telnet_msp_command_free(MudMSPCommand *command)
{
	if(command == NULL)
		return;

	if(command->fName)
		g_free(command->fName);

	if(command->mud_name)
		g_free(command->mud_name);

	if(command->sfx_type)
		g_free(command->sfx_type);

	if(command->V)
		g_free(command->V);

	if(command->L)
		g_free(command->L);

	if(command->P)
		g_free(command->P);

	if(command->C)
		g_free(command->C);

	if(command->T)
		g_free(command->T);

	if(command->U)
		g_free(command->U);

	g_free(command);

}

static void
mud_telnet_msp_process_command(MudTelnet *telnet, MudMSPCommand *command)
{
	/*g_message("MSP Command Parse Results");
	g_print("Type: %s\n", (command->type == MSP_TYPE_SOUND) ? "Sound" : "Music" );
	g_print("Filename: %s\n", (command->fName != NULL) ? command->fName : "<null>");
	g_print("V: %s\n", (command->V != NULL) ? command->V : "<null>");
	g_print("L: %s\n", (command->L != NULL) ? command->L : "<null>");
	g_print("C: %s\n", (command->C != NULL) ? command->C : "<null>");
	g_print("T: %s\n", (command->T != NULL) ? command->T : "<null>");
	g_print("U: %s\n", (command->U != NULL) ? command->U : "<null>");
	g_print("P: %s\n", (command->P != NULL) ? command->P : "<null>");
	g_print("Sfx Type: %s Volume: %d  Priority: %d  Repeat %d times. %s %s\n", (command->sfx_type) ? command->sfx_type:"None", command->volume,
	command->priority, command->initial_repeat_count, (command->loop)? "Looping" : "Not Looping",
	(command->cont) ? "Continue" : "Stop");*/

	if(command->fName && strcmp(command->fName, "Off") == 0)
	{
		if(command->U)
		{
			if(telnet->base_url)
				g_free(telnet->base_url);


			telnet->base_url = g_strdup(command->U);
		}
		else
			mud_telnet_msp_stop_playing(telnet, command->type);

		mud_telnet_msp_command_free(command);

		return;
	}

	if(telnet->sound[command->type].current_command)
	{
		if(telnet->sound[command->type].playing)
		{
			if(command->priority > telnet->sound[command->type].current_command->priority)
			{
				mud_telnet_msp_stop_playing(telnet, command->type);
				telnet->sound[command->type].current_command = command;
				mud_telnet_msp_start_playing(telnet, command->type);
			}
			else
				mud_telnet_msp_command_free(command);
		}
		else
		{
			mud_telnet_msp_stop_playing(telnet, command->type);
			telnet->sound[command->type].current_command = command;
			mud_telnet_msp_start_playing(telnet, command->type);
		}
	}
	else
	{
		telnet->sound[command->type].current_command = command;
		mud_telnet_msp_start_playing(telnet, command->type);
	}
}

static void
mud_telnet_msp_stop_playing(MudTelnet *telnet, MudMSPTypes type)
{
	telnet->sound[type].playing = FALSE;

	if(GST_IS_ELEMENT(telnet->sound[type].play))
	{
		gst_element_set_state (telnet->sound[type].play, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (telnet->sound[type].play));
	}

	if(telnet->sound[type].files)
	{
			g_strfreev(telnet->sound[type].files);
			telnet->sound[type].files = NULL;
	}

	telnet->sound[type].files_len = 0;

	mud_telnet_msp_command_free(telnet->sound[type].current_command);
	telnet->sound[type].current_command = NULL;
}

static void
mud_telnet_msp_start_playing(MudTelnet *telnet, MudMSPTypes type)
{
	if(!telnet->sound[type].current_command)
		return;

	if(mud_telnet_msp_get_files(telnet, type))
	{
		gint num = 0;

		telnet->sound[type].playing = TRUE;

		if(telnet->sound[type].files_len != 0)
			num = rand() % telnet->sound[type].files_len;

		telnet->sound[type].play = gst_element_factory_make ("playbin", "play");
		g_object_set (G_OBJECT(telnet->sound[type].play),
			"uri", telnet->sound[type].files[num], NULL);
		g_object_set(G_OBJECT(telnet->sound[type].play),
			"volume", (double)telnet->sound[type].current_command->volume/100, NULL);

		telnet->sound[type].bus =
			gst_pipeline_get_bus (GST_PIPELINE (telnet->sound[type].play));

		if(type == MSP_TYPE_SOUND)
			gst_bus_add_watch (telnet->sound[type].bus, mud_telnet_msp_sound_bus_call, telnet);
		else
			gst_bus_add_watch (telnet->sound[type].bus, mud_telnet_msp_music_bus_call, telnet);

		gst_object_unref (telnet->sound[type].bus);

		gst_element_set_state (telnet->sound[type].play, GST_STATE_PLAYING);
	}
}

static gboolean
mud_telnet_msp_get_files(MudTelnet *telnet, MudMSPTypes type)
{
	gchar sound_dir[2048];
	const gchar *file;
	gchar **files;
	gchar **structure;
	GString *file_output;
	GString *url_output;
	GString *file_name;
	GString *subdir;
	GString *full_dir;
	GDir *dir;
	gint i, depth;
	GPatternSpec *regex;

	if(!telnet->sound[type].current_command)
		return FALSE;

	g_snprintf(sound_dir, 2048, "%s/.gnome-mud/audio/%s/",
		g_get_home_dir(), telnet->sound[type].current_command->mud_name);
	if(!g_file_test(sound_dir, G_FILE_TEST_IS_DIR))
		mkdir(sound_dir, 0777 );

	structure = g_strsplit(telnet->sound[type].current_command->fName, "/", 0);
	depth = g_strv_length(structure);

	subdir = g_string_new(NULL);

	for(i = 0; i < depth - 1; ++i)
	{
		g_string_append(subdir, structure[i]);
		g_string_append_c(subdir, '/');
	}

	file_name = g_string_new(structure[depth - 1]);

	g_strfreev(structure);

	full_dir = g_string_new(sound_dir);
	g_string_append(full_dir, subdir->str);

	if(telnet->sound[type].current_command->T)
		g_string_append(full_dir, telnet->sound[type].current_command->T);

	if(!g_file_test(full_dir->str, G_FILE_TEST_IS_DIR))
		g_mkdir_with_parents(full_dir->str, 0777);

	file_output = g_string_new(NULL);

	regex = g_pattern_spec_new(file_name->str);

	dir = g_dir_open(full_dir->str, 0, NULL);

	while((file = g_dir_read_name(dir)) != NULL)
	{
		if(g_pattern_match_string(regex, file))
		{
			g_string_append(file_output, "file://");
			g_string_append(file_output, full_dir->str);
			g_string_append_c(file_output, '/');
			g_string_append(file_output, file);
			g_string_append_c(file_output, '\n');
		}
	}

	g_dir_close(dir);

	// Try searching again in main directory since
	// some servers ignore the standard concering the
	// T parameter and don't put the sound in a T-named
	// subdir.
	if(file_output->len == 0 && telnet->sound[type].current_command->T)
	{
		g_string_free(full_dir, TRUE);
		full_dir = g_string_new(sound_dir);
		g_string_append(full_dir, subdir->str);

		dir = g_dir_open(full_dir->str, 0, NULL);

		while((file = g_dir_read_name(dir)) != NULL)
		{
			if(g_pattern_match_string(regex, file))
			{
				g_string_append(file_output, "file://");
				g_string_append(file_output, full_dir->str);
				g_string_append_c(file_output, '/');
				g_string_append(file_output, file);
				g_string_append_c(file_output, '\n');
			}
		}

		g_dir_close(dir);
	}

	g_pattern_spec_free(regex);

	if(file_output->len == 0) // no matches, file doesn't exist.
	{
		url_output = g_string_new(NULL);

		if(telnet->base_url || telnet->sound[type].current_command->U)
		{
			if(telnet->base_url)
				g_string_append(url_output, telnet->base_url);
			else
				g_string_append(url_output, telnet->sound[type].current_command->U);

			if(subdir->len != 0)
				g_string_append(url_output, subdir->str);

			if(telnet->sound[type].current_command->T)
			{
				g_string_append(url_output, telnet->sound[type].current_command->T);
				g_string_append_c(url_output, '/');
			}

			g_string_append(url_output, file_name->str);

			g_string_append(file_output, full_dir->str);
			if(telnet->sound[type].current_command->T)
				g_string_append_c(file_output, '/');
			g_string_append(file_output, file_name->str);

			telnet->sound[type].current_command->priority = 0;

			mud_connection_view_queue_download(telnet->parent, url_output->str, file_output->str);
		}

		g_string_free(url_output, TRUE);
		g_string_free(file_output, TRUE);
		g_string_free(full_dir, TRUE);
		g_string_free(subdir, TRUE);
		g_string_free(file_name, TRUE);

		return FALSE;
	}

	files = g_strsplit(file_output->str, "\n", 0);

	if(telnet->sound[type].files)
		g_strfreev(telnet->sound[type].files);

	telnet->sound[type].files = files;
	telnet->sound[type].files_len = g_strv_length(files) - 1;

	g_string_free(file_output, TRUE);
	g_string_free(full_dir, TRUE);
	g_string_free(subdir, TRUE);
	g_string_free(file_name, TRUE);

	return TRUE;
}

static gboolean
mud_telnet_msp_sound_bus_call (GstBus *bus, GstMessage *msg, gpointer data)
{
	MudTelnet *telnet = (MudTelnet *)data;

	switch (GST_MESSAGE_TYPE (msg))
	{
		case GST_MESSAGE_EOS:
			telnet->sound[MSP_TYPE_SOUND].playing = FALSE;

			telnet->sound[MSP_TYPE_SOUND].current_command->current_repeat_count--;

			gst_element_set_state (telnet->sound[MSP_TYPE_SOUND].play, GST_STATE_NULL);

			if(telnet->sound[MSP_TYPE_SOUND].current_command->loop ||
			   telnet->sound[MSP_TYPE_SOUND].current_command->current_repeat_count != 0)
			{
				gint num = 0;

				if(telnet->sound[MSP_TYPE_SOUND].files_len != 0)
					num = rand() % telnet->sound[MSP_TYPE_SOUND].files_len;

				g_object_set (G_OBJECT(telnet->sound[MSP_TYPE_SOUND].play),
					"uri", telnet->sound[MSP_TYPE_SOUND].files[num], NULL);
				g_object_set(G_OBJECT(telnet->sound[MSP_TYPE_SOUND].play),
					"volume", (double)telnet->sound[MSP_TYPE_SOUND].current_command->volume/100.0, NULL);

				gst_element_set_state (telnet->sound[MSP_TYPE_SOUND].play, GST_STATE_PLAYING);
			}
			else
				mud_telnet_msp_stop_playing(telnet, MSP_TYPE_SOUND);
		break;

		case GST_MESSAGE_ERROR:
		{
			gchar *debug;
			GError *err;

			gst_message_parse_error (msg, &err, &debug);
			g_free (debug);

			g_warning ("Error: %s", err->message);
			g_error_free (err);

			break;
		}

		default:
		break;
	}

	return TRUE;
}

static gboolean
mud_telnet_msp_music_bus_call (GstBus *bus, GstMessage *msg, gpointer data)
{
	MudTelnet *telnet = (MudTelnet *)data;

	switch (GST_MESSAGE_TYPE (msg))
	{
		case GST_MESSAGE_EOS:
			telnet->sound[MSP_TYPE_MUSIC].playing = FALSE;

			telnet->sound[MSP_TYPE_MUSIC].current_command->current_repeat_count--;

			gst_element_set_state (telnet->sound[MSP_TYPE_MUSIC].play, GST_STATE_NULL);

			if(telnet->sound[MSP_TYPE_MUSIC].current_command->loop ||
			   telnet->sound[MSP_TYPE_MUSIC].current_command->current_repeat_count != 0)
			{
				gint num = 0;

				if(telnet->sound[MSP_TYPE_MUSIC].files_len != 0)
					num = rand() % telnet->sound[MSP_TYPE_MUSIC].files_len;

				g_object_set (G_OBJECT(telnet->sound[MSP_TYPE_MUSIC].play),
					"uri", telnet->sound[MSP_TYPE_MUSIC].files[num], NULL);
				g_object_set(G_OBJECT(telnet->sound[MSP_TYPE_MUSIC].play),
					"volume", (double)telnet->sound[MSP_TYPE_MUSIC].current_command->volume/100.0, NULL);

				gst_element_set_state (telnet->sound[MSP_TYPE_MUSIC].play, GST_STATE_PLAYING);
			}
			else
				mud_telnet_msp_stop_playing(telnet, MSP_TYPE_MUSIC);

		break;

		case GST_MESSAGE_ERROR:
		{
			gchar *debug;
			GError *err;

			gst_message_parse_error (msg, &err, &debug);
			g_free (debug);

			g_warning ("Error: %s", err->message);
			g_error_free (err);

			break;
		}

		default:
		break;
	}

	return TRUE;
}
#endif
