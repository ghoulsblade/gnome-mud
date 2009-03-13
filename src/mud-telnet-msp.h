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

G_BEGIN_DECLS

#include <glib.h>
#include <gst/gst.h>

#include "mud-telnet.h"

#define MUD_TYPE_TELNET_MSP              (mud_telnet_msp_get_type ())
#define MUD_TELNET_MSP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_TELNET_MSP, MudTelnetMsp))
#define MUD_TELNET_MSP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_TELNET_MSP, MudTelnetMspClass))
#define MUD_IS_TELNET_MSP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_TELNET_MSP))
#define MUD_IS_TELNET_MSP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_TELNET_MSP))
#define MUD_TELNET_MSP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_TELNET_MSP, MudTelnetMspClass))
#define MUD_TELNET_MSP_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MUD_TYPE_TELNET_MSP, MudTelnetMspPrivate))

typedef struct _MudTelnetMsp            MudTelnetMsp;
typedef struct _MudTelnetMspClass       MudTelnetMspClass;
typedef struct _MudTelnetMspPrivate     MudTelnetMspPrivate;

struct _MudTelnetMspClass
{
    GObjectClass parent_class;
};

struct _MudTelnetMsp
{
    GObject parent_instance;

    /*< private >*/
    MudTelnetMspPrivate *priv;
};

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

typedef enum
{
    ARG_STATE_FILE,
    ARG_STATE_V,
    ARG_STATE_L,
    ARG_STATE_C,
    ARG_STATE_T,
    ARG_STATE_U,
    ARG_STATE_P
} MudMSPArgStates;

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

typedef struct MudMSPSound
{
    gboolean playing;
    gchar **files;
    gint files_len;

    GstElement *play;
    GstBus *bus;

    MudMSPCommand *current_command;
} MudMSPSound;

GType mud_telnet_msp_get_type (void);

void mud_telnet_msp_download_item_free(MudMSPDownloadItem *item);
GString *mud_telnet_msp_parse(MudTelnetMsp *self, GString *buf, gint *len);
void mud_telnet_msp_stop_playing(MudTelnetMsp *self, MudMSPTypes type);
void mud_telnet_msp_parser_clear(MudTelnetMsp *self);

G_END_DECLS

#endif // ENABLE_MSP

#endif // MUD_TELNET_MSP_H

