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

#include <glib.h>
#include <glib/gi18n.h>
#include <zlib.h>

#include "gnome-mud.h"
#include "mud-connection-view.h"
#include "mud-telnet.h"

GString *
mud_mccp_decompress(MudTelnet *telnet, guchar *buffer, guint32 length)
{
    GString *ret = NULL;
    gint zstatus;
    gint i;

    if(!telnet->compress_out)
        return NULL;

    telnet->compress_out->next_in = buffer;
    telnet->compress_out->avail_in = length;

    ret = g_string_new(NULL);

    while(1)
    {
        if(telnet->compress_out->avail_in < 1)
            break;

        telnet->compress_out->avail_out = 4096;
        telnet->compress_out->next_out = telnet->compress_out_buf;

        zstatus = inflate(telnet->compress_out, Z_SYNC_FLUSH);

        if(zstatus == Z_OK)
        {
            for(i = 0; i < (4096 - telnet->compress_out->avail_out); ++i)
                    g_string_append_c(ret,
                        (gchar)telnet->compress_out_buf[i]);
            continue;
        }

        if(zstatus == Z_STREAM_END)
        {
            for(i = 0; i < (4096 - telnet->compress_out->avail_out); ++i)
                g_string_append_c(ret, (gchar)telnet->compress_out_buf[i]);

            if(telnet->compress_out->avail_in > 0)
                for(i = 0; i < telnet->compress_out->avail_in; ++i)
                    g_string_append_c(ret,
                            (gchar)telnet->compress_out->next_in[i]);

            inflateEnd(telnet->compress_out);

            g_free(telnet->compress_out);
            g_free(telnet->compress_out_buf);

            telnet->compress_out = NULL;
            telnet->compress_out_buf = NULL;

            telnet->mccp = FALSE;
            telnet->mccp_new = TRUE;

            break;
        }

        if(zstatus == Z_BUF_ERROR)
        {
            break;
        }

        if(zstatus == Z_DATA_ERROR)
        {
            mud_connection_view_add_text(telnet->parent,
                    _("\nMCCP data corrupted. Aborting connection.\n"),
                    Error);
            mud_connection_view_disconnect (telnet->parent);
            return NULL; 
        }
    }

    return ret;
}

