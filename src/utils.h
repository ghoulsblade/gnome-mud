#ifndef UTILS_H
#define UTILS_H

#include <gtk/gtkaboutdialog.h>

gchar * remove_whitespace(gchar *string);
gchar *strip_ansi(const gchar *orig);
void utils_activate_url(GtkAboutDialog *about, const gchar *url, gpointer data);
#endif // UTILS_H
