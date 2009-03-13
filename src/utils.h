/* GNOME-Mud - A simple Mud CLient
 * utils.h
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

#ifndef UTILS_H
#define UTILS_H

#include <gtk/gtk.h>

gchar *utils_remove_whitespace(const gchar *string);
gchar *utils_strip_ansi(const gchar *orig);
void utils_activate_url(GtkAboutDialog *about, const gchar *url, gpointer data);
void utils_error_message(GtkWidget *parent, const gchar *title, const gchar *fmt, ...);
void utils_str_replace (gchar *buf, const gchar *s, const gchar *repl);

#endif // UTILS_H

