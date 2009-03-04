/* GNOME-Mud - A simple Mud Client
 * mud-telnet-msp.h
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

#ifndef MUD_TELNET_MSP_H
#define MUD_TELNET_MSP_H

#ifdef ENABLE_GST

#include <glib.h>
#include "mud-telnet.h"

typedef enum
{
    MSP_TYPE_SOUND,
    MSP_TYPE_MUSIC
} MudMSPTypes;

typedef enum
{
    MSP_STATE_TEXT,
    MSP_STATE_POSSIBLE_COMMAND,
    MSP_STATE_COMMAND,
    MSP_STATE_GET_ARGS,
    MSP_STATE_PARSE_ARGS
} MudMSPStates;

typedef struct MudMSPParser
{
    gboolean enabled;

    MudMSPStates state;

    gint lex_pos_start;
    gint lex_pos_end;

    GString *output;
    GString *arg_buffer;
} MudMSPParser;

typedef struct MudMSPCommand
{
    MudMSPTypes type;

    gchar *fName;

    gchar *V;
    gchar *L;
    gchar *P;
    gchar *C;
    gchar *T;
    gchar *U;

    gchar *mud_name;
    gchar *sfx_type;

    gint volume;
    gint priority;
    gint initial_repeat_count;
    gint current_repeat_count;
    gboolean loop;
    gboolean cont;

} MudMSPCommand;

typedef struct MudMSPDownloadItem
{
    gchar *url;
    gchar *file;
} MudMSPDownloadItem;

#include <gst/gst.h>
typedef struct MudMSPSound
{
    gboolean playing;
    gchar **files;
    gint files_len;

    GstElement *play;
    GstBus *bus;

    MudMSPCommand *current_command;
} MudMSPSound;

void mud_telnet_msp_init(MudTelnet *telnet);
void mud_telnet_msp_parser_clear(MudTelnet *telnet);
void mud_telnet_msp_download_item_free(MudMSPDownloadItem *item);
GString *mud_telnet_msp_parse(MudTelnet *telnet, GString *buf, gint *len);
void mud_telnet_msp_stop_playing(MudTelnet *telnet, MudMSPTypes type);

#endif

#endif // MUD_TELNET_MSP_H
