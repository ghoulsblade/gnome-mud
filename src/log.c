/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1998-2002 Robin Ericsson <lobbin@localhost.nu>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gconf/gconf-client.h>
#include <gnome.h>
#include <vte/vte.h>
#include <stdio.h>

#include "gnome-mud.h"

static char const rcsid[] = "$Id: ";

/* function prototypes */
static  void do_log_filename_window (gchar *title) ;
static	void file_selection_cb_open(GtkWidget *button, GtkWidget *fs) ;
static	void file_selection_cancel (GtkWidget *fs) ;
static	void file_selection_delete
(GtkWidget *fs, GdkEventAny *e, gpointer data) ;

/* global variables */
extern CONNECTION_DATA *main_connection;
extern CONNECTION_DATA *connections[15];
extern SYSTEM_DATA      prefs;
extern GtkWidget       *main_notebook ;
extern GConfClient     *gconf_client;

void window_menu_file_start_logging_cb (GtkWidget *widget, gpointer data)
{
  CONNECTION_DATA *cd ;
  gchar            buf[256] ;
  gint             i ;

  i = gtk_notebook_get_current_page (GTK_NOTEBOOK(main_notebook)) ;
  cd = connections[i] ;
  g_return_if_fail (cd) ;

  if (cd->logging) {
    g_snprintf(buf, 255, _("*** Already logging to %s. Close that log first.\n"),
      cd->log_filename) ;
    textfield_add(cd, buf, MESSAGE_ERR) ;
    return ;
  }

  do_log_filename_window(_("Open log")) ;
} /* window_menu_file_start_logging_cb */

void window_menu_file_stop_logging_cb (GtkWidget *widget, gpointer data)
{
  CONNECTION_DATA *cd ;
  gint             i ;

  i = gtk_notebook_get_current_page (GTK_NOTEBOOK(main_notebook)) ;
  cd = connections[i] ;
  g_return_if_fail (cd) ;

  if (!cd->logging) {
    textfield_add(cd, _("*** No log to be closed is open in this window.\n"),
      MESSAGE_ERR) ;
    return ;
  }

  stop_logging_connection(cd) ;
} /* window_menu_file_stop_logging_cb */

void file_selection_cb_open
(GtkWidget *button, GtkWidget *fs)
{
  CONNECTION_DATA *cd ;
  gchar            buf[256] ;
  gint             i ;
  time_t           t ;

  i = gtk_notebook_get_current_page (GTK_NOTEBOOK(main_notebook)) ;
  cd = connections[i] ;

  cd->logging = TRUE ;
  cd->log_filename =
    g_strdup(gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs))) ;

  gconf_client_set_string(gconf_client, "/apps/gnome-mud/functionality/last_log_dir", cd->log_filename, NULL);
  
  cd->log = fopen(cd->log_filename, "a") ;
  if (!cd->log) {
    g_snprintf(buf, 255, _("*** Couldn't open %s.\n"), cd->log_filename) ;
    textfield_add(cd, buf, MESSAGE_ERR) ;

    stop_logging_connection(cd) ;
  }
  else
  {
    t = time(&t) ;
    strftime(buf, 255, "Started logging %A %d %B %Y %H:%M:%S.\n\n",
      localtime(&t)) ;
    fputs(buf, cd->log);

    g_snprintf(buf, 255, _("*** Logging to %s.\n"), cd->log_filename) ;
    textfield_add(cd, buf, MESSAGE_SYSTEM) ;
  }

  file_selection_cancel(fs) ;
} /* file_selection_cb_open */

void file_selection_cancel
(GtkWidget *fs)
{
  gtk_widget_destroy(fs) ;
} /* file_selection_cancel */

void file_selection_delete
(GtkWidget *fs, GdkEventAny *e, gpointer data) {
  file_selection_cancel(fs) ;
  return ;
} /* file_selection_delete */

void do_log_filename_window
(gchar *title)
{
  GtkWidget *fs ;

  g_return_if_fail (title) ;

  fs = gtk_file_selection_new(_(title)) ;
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(fs), prefs.LastLogDir) ;

  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->ok_button), "clicked",
    GTK_SIGNAL_FUNC(file_selection_cb_open), fs) ;
  gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(fs)->cancel_button),
    "clicked", GTK_SIGNAL_FUNC(file_selection_cancel), GTK_OBJECT(fs)) ;

  gtk_signal_connect(GTK_OBJECT(fs), "delete_event",
    GTK_SIGNAL_FUNC(file_selection_delete), NULL) ;

  gtk_widget_show(fs) ;
} /* do_log_filename_window */


/* stop_logging_connection:
 *   Takes a connection as its argument, and stops it logging.
 *   Closes files and frees memory.
 */
void stop_logging_connection(CONNECTION_DATA *cn) {
  gchar  buf[256] ;
  time_t t ;

  g_return_if_fail (cn) ;

  cn->logging = FALSE ;

  if (cn->log)
  {
    t = time(&t) ;
    strftime(buf, 255, "\nStopped logging %A %d %B %Y %H:%M:%S\n\n",
      localtime(&t)) ;
    fputs(buf, cn->log);

    fclose(cn->log) ;
    g_snprintf(buf, 255, _("*** Stopped logging to %s.\n"), cn->log_filename) ;
    textfield_add(cn, buf, MESSAGE_SYSTEM) ;
    cn->log = NULL ;
  }

  g_free(cn->log_filename) ;
  cn->log_filename = NULL ;
} /* stop_logging_connection */

static void window_menu_file_save_log_file_ok_cb
(GtkWidget *widget, GtkFileSelection *file_selector)
{
	FILE *fp;
	CONNECTION_DATA *cd;
	gchar *textdata;

	gint number = gtk_notebook_get_current_page(GTK_NOTEBOOK(main_notebook));
	const gchar *filename = gtk_file_selection_get_filename( GTK_FILE_SELECTION(file_selector) );

	gconf_client_set_string(gconf_client, "/apps/gnome-mud/functionality/last_log_dir", filename, NULL);
	
	cd = connections[number];

	if ((fp = fopen(filename, "w")) == NULL)
	{
		textfield_add(cd, _("*** Could not open file for writing.\n"), MESSAGE_ERR);
		return;
	}

	textdata = vte_terminal_get_text (VTE_TERMINAL (cd->window), NULL, NULL, NULL);
	fputs(textdata, fp);
	g_free(textdata);

	fclose(fp);
}

void window_menu_file_save_buffer_cb (GtkWidget *widget, gpointer data)
{
	GtkWidget *file_selector = gtk_file_selection_new(
		_("Please select a log file..."));

        gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selector),
		prefs.LastLogDir);

        gtk_signal_connect(
		GTK_OBJECT(GTK_FILE_SELECTION(file_selector)->ok_button),
		"clicked",
		GTK_SIGNAL_FUNC(window_menu_file_save_log_file_ok_cb),
		file_selector);
        gtk_signal_connect_object(
		GTK_OBJECT(GTK_FILE_SELECTION(file_selector)->ok_button),
		"clicked",
		GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer) file_selector);
        gtk_signal_connect_object(
		GTK_OBJECT(GTK_FILE_SELECTION(file_selector)->cancel_button),
		 "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy),
		(gpointer) file_selector);
        gtk_widget_show(file_selector);
}

