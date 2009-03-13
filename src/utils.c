/* GNOME-Mud - A simple Mud CLient
 * utils.c
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

#include <glib.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <gtk/gtk.h>

gchar *
utils_remove_whitespace(const gchar *string)
{
    guint i, len;
    GString *s;

    if(string == NULL)
        return NULL;

    s = g_string_new(NULL);
    len = strlen(string);

    for(i = 0; i < len; i++)
        if(!g_ascii_isspace(string[i]))
            s = g_string_append_c(s, string[i]);

    return g_string_free(s, FALSE);
}

// FIXME: This is terrible. We should replace this with something
// that can handle any sized string.
void
utils_str_replace (gchar *buf, const gchar *s, const gchar *repl)
{
    gchar out_buf[4608];
    gchar *pc, *out;
    gint  len = strlen (s);
    gboolean found = FALSE;

    for ( pc = buf, out = out_buf; *pc && (out-out_buf) < (4608-len-4);)
        if ( !strncasecmp(pc, s, len))
        {
            out += sprintf (out, "%s", repl);
            pc += len;
            found = TRUE;
        }
        else
            *out++ = *pc++;

    if ( found)
    {
        *out = '\0';
        strcpy (buf, out_buf);
    }
}

gchar *
utils_strip_ansi(const gchar *orig)
{
    GString *buf;
    const gchar *c;

    if (!orig)
        return NULL;

    buf = g_string_new(NULL);

    for (c = orig; *c;)
    {
        switch (*c)
        {
            case '\x1B': // Esc Character
                while (*c && *c++ != 'm');
                break;

            case '\02': // HTML Open bracket
                while (*c && *c++ != '\03'); // HTML Close bracket
                break;

            default:
                buf = g_string_append_c(buf, *c++);
        }
    }

    return g_string_free(buf, FALSE);
}

void
utils_activate_url(GtkAboutDialog *about, const gchar *url, gpointer data)
{
    // use gtk_show_uri when available.
}

void
utils_error_message(GtkWidget *parent, const gchar *title, const gchar *fmt, ...)
{
    GtkWidget *dialog, *label, *icon, *hbox;
    va_list args;
    gchar *message;

    dialog = gtk_dialog_new_with_buttons(title, GTK_WINDOW(parent),
            GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK,
            GTK_RESPONSE_NONE, NULL);

    if(fmt)
    {
        va_start(args, fmt);
        message = g_strdup_vprintf(fmt, args);
        va_end(args);

        label = gtk_label_new(message);
        gtk_label_set_selectable(GTK_LABEL(label), TRUE);
        g_free(message);
    }
    else
    {
        label = gtk_label_new("Unknown error.");
        gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    }

    icon = gtk_image_new_from_icon_name("gtk-dialog-error", GTK_ICON_SIZE_DIALOG);
    hbox = gtk_hbox_new(FALSE, 0);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

