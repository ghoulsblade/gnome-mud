/* AMCL - A simple Mud CLient
 * Copyright (C) 1999-2000 Robin Ericsson <lobbin@localhost.nu>
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

/*
 * This module/plug-in API is slighly based on the API in gEdit.
 */

#ifndef __MODULES_C__
#define __MODULES_C__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gtk/gtk.h>
#include <errno.h>
#include <libintl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#include "amcl.h"
#include "modules.h"

#define _(string) gettext(string)

static char const rcsid[] =
    "$Id$";

GList     *Plugin_list;
GList     *Plugin_data_list;
GtkWidget *plugin_name_entry;
GtkWidget *plugin_author_entry;
GtkWidget *plugin_version_entry;
GtkWidget *plugin_desc_entry;
GtkWidget *plugin_enable_check;
gint       plugin_selected_row;
FILE      *plugin_information;
gint       amount;

PLUGIN_OBJECT *plugin_get_plugin_object_by_handle (gint handle)
{
  PLUGIN_OBJECT *p;
  GList         *t;

  for (t = g_list_first(Plugin_list); t != NULL; t = t->next) {
      
    if (t->data != NULL) {
      p = (PLUGIN_OBJECT *) t->data;

      if ((int) p->handle == handle)
	return p;
    }
  }

  return NULL;
}

PLUGIN_OBJECT *plugin_get_plugin_object_by_name (gchar *name)
{
  PLUGIN_OBJECT *p;
  GList         *t;

  for (t = g_list_first(Plugin_list); t != NULL; t = t->next) {
    if (t->data != NULL) {
      p = (PLUGIN_OBJECT *) t->data;
      
      if (!strcmp (p->info->plugin_name, name))
	return p;
    }
  }
    
  return NULL;
}

void plugin_enable_check_cb (GtkWidget *widget, gpointer data)
{
  PLUGIN_OBJECT *p;
  gchar *text;
  
  gtk_clist_get_text ((GtkCList *) data, plugin_selected_row, 0, &text);

  p = plugin_get_plugin_object_by_name (text);

  if (p != NULL) {
    if (GTK_TOGGLE_BUTTON (widget)->active) {
      p->enabeled = TRUE;
    } else {
      p->enabeled = FALSE;
    }
  }
}

void plugin_clist_select_row_cb (GtkWidget *w, gint r, gint c, GdkEventButton *e, gpointer data)
{
  PLUGIN_OBJECT *p;
  gchar *text;

  plugin_selected_row = r;
  gtk_clist_get_text((GtkCList *) data, r, c, &text);

  p = plugin_get_plugin_object_by_name (text);

  if (p != NULL) {
    gtk_entry_set_text (GTK_ENTRY (plugin_name_entry),    p->info->plugin_name);
    gtk_entry_set_text (GTK_ENTRY (plugin_author_entry),  p->info->plugin_author);
    gtk_entry_set_text (GTK_ENTRY (plugin_version_entry), p->info->plugin_version);
    gtk_entry_set_text (GTK_ENTRY (plugin_desc_entry),    p->info->plugin_descr);
    if (p->enabeled)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(plugin_enable_check),TRUE);
  }
}

void plugin_clist_append (PLUGIN_OBJECT *p, GtkCList *clist)
{
  if ( p ) {
    gchar *text[2];

    text[0] = p->info->plugin_name;

    gtk_clist_append (GTK_CLIST (clist), text);
  }

  amount++;
}

void do_plugin_information(GtkWidget *widget, gpointer data)
{
  GtkWidget *window1;
  GtkWidget *hbox1;
  GtkWidget *clist1;
  GtkWidget *vbox1;
  GtkWidget *label1;
  GtkWidget *label6;
  GtkWidget *label3;
  GtkWidget *label7;
  GtkWidget *label4;
  GtkWidget *label8;
  GtkWidget *label5;
  GtkWidget *label9;

  window1 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (window1), "window1", window1);
  gtk_container_border_width (GTK_CONTAINER (window1), 7);
  gtk_window_set_title (GTK_WINDOW (window1), _("AMCL Plugin Information"));
  gtk_window_set_policy (GTK_WINDOW (window1), TRUE, TRUE, FALSE);
  gtk_widget_set_usize (window1, 430, 275);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_object_set_data (GTK_OBJECT (window1), "hbox1", hbox1);
  gtk_widget_show (hbox1);
  gtk_container_add (GTK_CONTAINER (window1), hbox1);

  clist1 = gtk_clist_new (1);
  gtk_object_set_data (GTK_OBJECT (window1), "clist1", clist1);
  gtk_widget_show (clist1);
  gtk_box_pack_start (GTK_BOX (hbox1), clist1, TRUE, TRUE, 0);
  gtk_widget_set_usize (clist1, 50, -2);
  gtk_signal_connect (GTK_OBJECT (clist1), "select_row", 
		      GTK_SIGNAL_FUNC(plugin_clist_select_row_cb),
		      (gpointer) clist1);
  
  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_object_set_data (GTK_OBJECT (window1), "vbox1", vbox1);
  gtk_widget_show (vbox1);
  gtk_box_pack_start (GTK_BOX (hbox1), vbox1, TRUE, TRUE, 0);
  gtk_widget_set_usize (vbox1, 150, -2);
  gtk_container_border_width (GTK_CONTAINER (vbox1), 7);

  label1 = gtk_label_new (_("Plugin Name:"));
  gtk_object_set_data (GTK_OBJECT (window1), "label1", label1);
  gtk_widget_show (label1);
  gtk_box_pack_start (GTK_BOX (vbox1), label1, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (label1), 0.0100002, 0.52);

  plugin_name_entry = gtk_entry_new ();
  gtk_object_set_data (GTK_OBJECT (window1), "plugin_name_entry", plugin_name_entry);
  gtk_widget_show (plugin_name_entry);
  gtk_box_pack_start (GTK_BOX (vbox1), plugin_name_entry, FALSE, TRUE, 0);
  gtk_entry_set_editable (GTK_ENTRY (plugin_name_entry), FALSE);

  label6 = gtk_label_new ("");
  gtk_object_set_data (GTK_OBJECT (window1), "label6", label6);
  gtk_widget_show (label6);
  gtk_box_pack_start (GTK_BOX (vbox1), label6, FALSE, TRUE, 0);

  label3 = gtk_label_new (_("Plugin Author:"));
  gtk_object_set_data (GTK_OBJECT (window1), "label3", label3);
  gtk_widget_show (label3);
  gtk_box_pack_start (GTK_BOX (vbox1), label3, FALSE, TRUE, 0);
  gtk_misc_set_alignment (GTK_MISC (label3), 1.93715e-07, 0.5);

  plugin_author_entry = gtk_entry_new ();
  gtk_object_set_data (GTK_OBJECT (window1), "plugin_author_entry", plugin_author_entry);
  gtk_widget_show (plugin_author_entry);
  gtk_box_pack_start (GTK_BOX (vbox1), plugin_author_entry, FALSE, TRUE, 0);
  gtk_entry_set_editable (GTK_ENTRY (plugin_author_entry), FALSE);

  label7 = gtk_label_new ("");
  gtk_object_set_data (GTK_OBJECT (window1), "label7", label7);
  gtk_widget_show (label7);
  gtk_box_pack_start (GTK_BOX (vbox1), label7, FALSE, TRUE, 0);

  label4 = gtk_label_new (_("Plugin Version:"));
  gtk_object_set_data (GTK_OBJECT (window1), "label4", label4);
  gtk_widget_show (label4);
  gtk_box_pack_start (GTK_BOX (vbox1), label4, FALSE, TRUE, 0);
  gtk_misc_set_alignment (GTK_MISC (label4), 1.93715e-07, 0.5);

  plugin_version_entry = gtk_entry_new ();
  gtk_object_set_data (GTK_OBJECT (window1), "plugin_version_entry", plugin_version_entry);
  gtk_widget_show (plugin_version_entry);
  gtk_box_pack_start (GTK_BOX (vbox1), plugin_version_entry, FALSE, TRUE, 0);
  gtk_entry_set_editable (GTK_ENTRY (plugin_version_entry), FALSE);

  label8 = gtk_label_new ("");
  gtk_object_set_data (GTK_OBJECT (window1), "label8", label8);
  gtk_widget_show (label8);
  gtk_box_pack_start (GTK_BOX (vbox1), label8, FALSE, TRUE, 0);

  label5 = gtk_label_new (_("Plugin Description:"));
  gtk_object_set_data (GTK_OBJECT (window1), "label5", label5);
  gtk_widget_show (label5);
  gtk_box_pack_start (GTK_BOX (vbox1), label5, FALSE, TRUE, 0);
  gtk_misc_set_alignment (GTK_MISC (label5), 1.93715e-07, 0.5);

  plugin_desc_entry = gtk_entry_new ();
  gtk_object_set_data (GTK_OBJECT (window1), "plugin_desc_entry", plugin_desc_entry);
  gtk_widget_show (plugin_desc_entry);
  gtk_box_pack_start (GTK_BOX (vbox1), plugin_desc_entry, FALSE, TRUE, 0);
  gtk_entry_set_editable (GTK_ENTRY (plugin_desc_entry), FALSE);

  label9 = gtk_label_new ("");
  gtk_object_set_data (GTK_OBJECT (window1), "label9", label9);
  gtk_widget_show (label9);
  gtk_box_pack_start (GTK_BOX (vbox1), label9, FALSE, TRUE, 0);

  plugin_enable_check = gtk_check_button_new_with_label (_(" Is Plugin Enabled?"));
  gtk_object_set_data (GTK_OBJECT (window1), "plugin_enable_check", plugin_enable_check);
  gtk_widget_show (plugin_enable_check);
  gtk_box_pack_start (GTK_BOX (vbox1), plugin_enable_check, FALSE, TRUE, 0);
  GTK_WIDGET_UNSET_FLAGS (plugin_enable_check, GTK_CAN_FOCUS);
  gtk_signal_connect(GTK_OBJECT(plugin_enable_check), "toggled",
		     GTK_SIGNAL_FUNC(plugin_enable_check_cb), (gpointer) clist1);

  g_list_foreach (Plugin_list, (GFunc) plugin_clist_append, clist1);
  gtk_clist_select_row (GTK_CLIST (clist1), 0, 0);

  gtk_widget_show(window1);
}

void save_plugins()
{
  PLUGIN_OBJECT *p;
  GList         *t;  
  FILE          *fp;
  
  if (!(fp = open_file ("plugins_info", "w"))) return;

  for (t = g_list_first(Plugin_list); t != NULL; t = t->next)
  {
     if (t->data != NULL)
     {
	p = (PLUGIN_OBJECT *) t->data;
	if (p->enabeled == TRUE)
	  fprintf(fp, p->name);
     }
  }
  if (fp) fclose (fp);
}

int init_modules(char *path)
{
  DIR            *directory;
  struct dirent  *direntity;
  gchar          *shortname;

  /* Why we open this file if we are not going to use??? */
  plugin_information = open_file("plugins_info","r");

  if ((directory = opendir(path)) == NULL) {
    g_message("Plugin error (%s): %s", path, strerror(errno));
    return FALSE;
  }
  
  while ((direntity = readdir(directory))) {
    PLUGIN_OBJECT *plugin;
    gchar *suffix;
    
    if (strrchr(direntity->d_name, '/'))
      shortname = (gchar *) strrchr(direntity->d_name, '/') + 1;
    else
      shortname = direntity->d_name;
    
    if (!strcmp(shortname, ".") || !strcmp(shortname, ".."))
      continue;
    
    suffix = (gchar *) strrchr(direntity->d_name, '.');
    if (!suffix || strcmp(suffix, ".plugin"))
      continue;
    
    plugin = plugin_query(direntity->d_name, path);
    if (!plugin)
      continue;
    
    plugin_register(plugin);
  }
  
  if (plugin_information)
    fclose(plugin_information);
  closedir(directory);
  
  return TRUE;
}

PLUGIN_OBJECT *plugin_query (gchar *plugin_name, gchar *plugin_path)
{
    PLUGIN_OBJECT *new_plugin = g_new0(PLUGIN_OBJECT, 1);
    gchar filename[60];

    new_plugin->name = g_strdup(plugin_name);
    sprintf (filename, "%s%s", plugin_path, plugin_name);
    if ((new_plugin->handle = dlopen (filename, RTLD_LAZY)) == NULL)
    {
        g_message (_("Error getting plugin handle (%s): %s."), plugin_name, dlerror());
        goto error;
    } else {
        if ((new_plugin->info = dlsym(new_plugin->handle,"amcl_plugin_info")) == NULL)
        {
            g_message (_("Error, %s not an AMCL module: %s."), plugin_name, dlerror());
            goto error;
        }
        new_plugin->filename = g_strdup (filename);
        return new_plugin;
    }

error:
    g_free (new_plugin->name);
    g_free (new_plugin->filename);
    g_free (new_plugin);

    return NULL;
}

void plugin_check_enable(PLUGIN_OBJECT *plugin)
{
  gchar line[255];
  
  g_message (_("Checking whether plugin should be enabled by default..."));
  
  while ( fgets (line, 80, plugin_information) != NULL) {
    if (!strcmp(line, plugin->name)) {
      plugin->enabeled = TRUE;
    }
  }

  rewind(plugin_information);
}

void plugin_register(PLUGIN_OBJECT *plugin)
{
    g_message (_("Registering plugin `%s'."), plugin->name);
    g_message (_("Plug-in internal name is `%s'."), plugin->info->plugin_name);

    plugin_check_enable(plugin);

    Plugin_list = g_list_append(Plugin_list, (gpointer) plugin);
    
    if (plugin->info->init_function)
    {
        g_message (_("Running init-function..."));
        plugin->info->init_function(NULL, (gint) plugin->handle);
    }
}
#endif
