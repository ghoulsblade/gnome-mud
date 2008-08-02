#ifndef UTILS_H
#define UTILS_H

#include <gtk/gtk.h>

gchar * remove_whitespace(const gchar *string);
gchar *strip_ansi(const gchar *orig);
void utils_activate_url(GtkAboutDialog *about, const gchar *url, gpointer data);
void utils_error_message(GtkWidget *parent, const gchar *title, const gchar *fmt, ...);

#endif // UTILS_H
