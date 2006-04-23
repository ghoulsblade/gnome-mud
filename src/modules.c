/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1999-2006 Robin Ericsson <lobbin@localhost.nu>
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
** This module/plug-in API is slighly based on the API in gEdit. 
*/

#ifndef __MODULES_C__
#define __MODULES_C__

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib/gi18n.h>
#include <libgnome/gnome-config.h>
#include <libgnomeui/gnome-dialog.h>

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

#include "gnome-mud.h"
#include "mud-connection-view.h"
#include "mud-window.h"
#include "modules.h"

GList     *Plugin_list;
GList     *Plugin_data_list;
int       plugin_selected_row;
gint       amount;
MudWindow *gGMudWindow;

PLUGIN_OBJECT *plugin_get_plugin_object_by_handle (gint handle)
{
  PLUGIN_OBJECT *p;
  GList         *t;

  for (t = g_list_first(Plugin_list); t != NULL; t = t->next) {
      
    if (t->data != NULL) {
      p = (PLUGIN_OBJECT *) t->data;

      if (GPOINTER_TO_INT(p->handle) == handle)
	return p;
    }
  }

  return NULL;
}

PLUGIN_OBJECT static *plugin_get_plugin_object_by_name (gchar *name)
{
	PLUGIN_OBJECT *p;
	GList         *t;

	for (t = g_list_first(Plugin_list); t != NULL; t = t->next)
	{
		if (t->data != NULL)
		{
			p = (PLUGIN_OBJECT *) t->data;
      
			if (!strcmp (p->info->plugin_name, name))
			{
				return p;
			}
		}
	}
    
	return NULL;
}

static void plugin_enable_check_cb (GtkWidget *widget, gpointer data)
{
  PLUGIN_OBJECT *p;
  gchar *text;
  
  gtk_clist_get_text ((GtkCList *) data, plugin_selected_row, 0, &text);

  p = plugin_get_plugin_object_by_name (text);

  if (p != NULL) {
    gchar path[50];

    if (GTK_TOGGLE_BUTTON (widget)->active) {
      p->enabeled = TRUE;
    } else {
      p->enabeled = FALSE;
    }

    g_snprintf(path, 50, "/gnome-mud/Plugins/%s", p->name);
    gnome_config_set_bool(path, p->enabeled);
  }
}

static void plugin_clist_select_row_cb (GtkWidget *clist, gint r, gint c, GdkEventButton *e, gpointer data)
{
	PLUGIN_OBJECT *p;
	gchar *text;

	plugin_selected_row = r;
	gtk_clist_get_text(GTK_CLIST(clist), r, c, &text);

	p = plugin_get_plugin_object_by_name (text);

	if (p != NULL)
	{
		GtkTextBuffer *buffer;
		GtkWidget *plugin_desc_text = gtk_object_get_data(GTK_OBJECT(clist), "plugin_desc_text");
	  
		gtk_entry_set_text (GTK_ENTRY (gtk_object_get_data(GTK_OBJECT(clist), "plugin_name_entry")),    p->info->plugin_name);
		gtk_entry_set_text (GTK_ENTRY (gtk_object_get_data(GTK_OBJECT(clist), "plugin_author_entry")),  p->info->plugin_author);
		gtk_entry_set_text (GTK_ENTRY (gtk_object_get_data(GTK_OBJECT(clist), "plugin_version_entry")), p->info->plugin_version);

		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(plugin_desc_text));
		gtk_text_buffer_set_text(buffer, p->info->plugin_descr, -1);
    
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_object_get_data(GTK_OBJECT(clist), "plugin_enable_check")), p->enabeled);
	}
}

static void plugin_clist_append (PLUGIN_OBJECT *p, GtkCList *clist)
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
  static GtkWidget *dialog1;
  GtkWidget *dialog_vbox1;
  GtkWidget *table1;
  GtkWidget *label1;
  GtkWidget *label2;
  GtkWidget *label3;
  GtkWidget *label4;
  GtkWidget *scrolledwindow1;
  GtkWidget *clist1;
  GtkWidget *label5;
  GtkWidget *scrolledwindow2;
  GtkWidget *dialog_action_area1;
  GtkWidget *button1;
  GtkWidget *plugin_name_entry;
  GtkWidget *plugin_author_entry;
  GtkWidget *plugin_version_entry;
  GtkWidget *plugin_desc_text;
  GtkWidget *plugin_enable_check;
  
  if (dialog1 != NULL) {
    gtk_window_present (GTK_WINDOW (dialog1));
    return;
  }

  dialog1 = gnome_dialog_new (NULL, NULL);
  gtk_object_set_data (GTK_OBJECT (dialog1), "dialog1", dialog1);
  gtk_widget_set_usize (dialog1, 430, -2);
 
  gtk_window_set_title(GTK_WINDOW(dialog1), _("Plugin Information"));
  gtk_window_set_policy (GTK_WINDOW (dialog1), FALSE, FALSE, FALSE);

  dialog_vbox1 = GNOME_DIALOG (dialog1)->vbox;
  gtk_object_set_data (GTK_OBJECT (dialog1), "dialog_vbox1", dialog_vbox1);
  gtk_widget_show (dialog_vbox1);

  table1 = gtk_table_new (9, 2, FALSE);
  gtk_widget_ref (table1);
  gtk_object_set_data_full (GTK_OBJECT (dialog1), "table1", table1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), table1, TRUE, TRUE, 0);
  gtk_table_set_row_spacings (GTK_TABLE (table1), 3);
  gtk_table_set_col_spacings (GTK_TABLE (table1), 7);

  label1 = gtk_label_new (_("Plugin Name:"));
  gtk_widget_ref (label1);
  gtk_object_set_data_full (GTK_OBJECT (dialog1), "label1", label1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label1);
  gtk_table_attach (GTK_TABLE (table1), label1, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);

  plugin_name_entry = gtk_entry_new ();
  gtk_widget_ref (plugin_name_entry);
  gtk_object_set_data_full (GTK_OBJECT (dialog1), "plugin_name_entry", plugin_name_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin_name_entry);
  gtk_table_attach (GTK_TABLE (table1), plugin_name_entry, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_entry_set_editable (GTK_ENTRY (plugin_name_entry), FALSE);

  label2 = gtk_label_new (_("Plugin Author:"));
  gtk_widget_ref (label2);
  gtk_object_set_data_full (GTK_OBJECT (dialog1), "label2", label2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label2);
  gtk_table_attach (GTK_TABLE (table1), label2, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);

  label3 = gtk_label_new (_("Plugin Version:"));
  gtk_widget_ref (label3);
  gtk_object_set_data_full (GTK_OBJECT (dialog1), "label3", label3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label3);
  gtk_table_attach (GTK_TABLE (table1), label3, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label3), 0, 0.5);

  label4 = gtk_label_new (_("Plugin Description:"));
  gtk_widget_ref (label4);
  gtk_object_set_data_full (GTK_OBJECT (dialog1), "label4", label4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label4);
  gtk_table_attach (GTK_TABLE (table1), label4, 1, 2, 6, 7,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label4), 0, 0.5);

  plugin_enable_check = gtk_check_button_new_with_label (_("Enable plugin"));
  gtk_widget_ref (plugin_enable_check);
  gtk_object_set_data_full (GTK_OBJECT (dialog1), "plugin_enable_check", plugin_enable_check,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin_enable_check);
  gtk_table_attach (GTK_TABLE (table1), plugin_enable_check, 1, 2, 8, 9,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  plugin_author_entry = gtk_entry_new ();
  gtk_widget_ref (plugin_author_entry);
  gtk_object_set_data_full (GTK_OBJECT (dialog1), "plugin_author_entry", plugin_author_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin_author_entry);
  gtk_table_attach (GTK_TABLE (table1), plugin_author_entry, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_entry_set_editable (GTK_ENTRY (plugin_author_entry), FALSE);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow1);
  gtk_object_set_data_full (GTK_OBJECT (dialog1), "scrolledwindow1", scrolledwindow1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow1);
  gtk_table_attach (GTK_TABLE (table1), scrolledwindow1, 0, 1, 0, 9,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);

  clist1 = gtk_clist_new (1);
  gtk_widget_ref (clist1);
  gtk_object_set_data_full (GTK_OBJECT (dialog1), "clist1", clist1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (clist1);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), clist1);
  gtk_widget_set_usize (clist1, 150, -2);
  GTK_WIDGET_UNSET_FLAGS (clist1, GTK_CAN_FOCUS);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 0, 80);
  gtk_clist_column_titles_hide (GTK_CLIST (clist1));

  label5 = gtk_label_new ("");
  gtk_widget_ref (label5);
  gtk_object_set_data_full (GTK_OBJECT (dialog1), "label5", label5,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label5);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 0, label5);

  plugin_version_entry = gtk_entry_new ();
  gtk_widget_ref (plugin_version_entry);
  gtk_object_set_data_full (GTK_OBJECT (dialog1), "plugin_version_entry", plugin_version_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (plugin_version_entry);
  gtk_table_attach (GTK_TABLE (table1), plugin_version_entry, 1, 2, 5, 6,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_entry_set_editable (GTK_ENTRY (plugin_version_entry), FALSE);

  scrolledwindow2 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow2);
  gtk_object_set_data_full (GTK_OBJECT (dialog1), "scrolledwindow2", scrolledwindow2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow2);
  gtk_table_attach (GTK_TABLE (table1), scrolledwindow2, 1, 2, 7, 8,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwindow2), GTK_SHADOW_IN);

  plugin_desc_text = gtk_text_view_new();
  gtk_widget_show(plugin_desc_text);
  gtk_container_add(GTK_CONTAINER(scrolledwindow2), plugin_desc_text);

  dialog_action_area1 = GNOME_DIALOG (dialog1)->action_area;
  gtk_object_set_data (GTK_OBJECT (dialog1), "dialog_action_area1", dialog_action_area1);
  gtk_widget_show (dialog_action_area1);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area1), 8);

  gnome_dialog_append_button (GNOME_DIALOG (dialog1), GNOME_STOCK_BUTTON_CLOSE);
  button1 = GTK_WIDGET (g_list_last (GNOME_DIALOG (dialog1)->buttons)->data);
  gtk_widget_ref (button1);
  gtk_object_set_data_full (GTK_OBJECT (dialog1), "button1", button1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button1);
  GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (plugin_enable_check), "toggled",
                      GTK_SIGNAL_FUNC (plugin_enable_check_cb),
                      clist1);
  gtk_signal_connect (GTK_OBJECT (clist1), "select_row",
                      GTK_SIGNAL_FUNC (plugin_clist_select_row_cb),
                      NULL);
  gtk_signal_connect_object(GTK_OBJECT(button1), "clicked",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(dialog1));
  gtk_signal_connect (GTK_OBJECT(dialog1), "destroy",
		      GTK_SIGNAL_FUNC(gtk_widget_destroyed), &dialog1);

  gtk_object_set_data(GTK_OBJECT(clist1), "plugin_name_entry",    plugin_name_entry);
  gtk_object_set_data(GTK_OBJECT(clist1), "plugin_author_entry",  plugin_author_entry);
  gtk_object_set_data(GTK_OBJECT(clist1), "plugin_version_entry", plugin_version_entry);
  gtk_object_set_data(GTK_OBJECT(clist1), "plugin_desc_text",     plugin_desc_text);
  gtk_object_set_data(GTK_OBJECT(clist1), "plugin_enable_check",  plugin_enable_check);  

  g_list_foreach (Plugin_list, (GFunc) plugin_clist_append, clist1);
  gtk_clist_select_row (GTK_CLIST (clist1), 0, 0);
 
  gtk_widget_show(dialog1);
}

int init_modules(char *path)
{
  DIR            *directory;
  struct dirent  *direntity;
  gchar          *shortname;

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
        if ((new_plugin->info = dlsym(new_plugin->handle,"gnomemud_plugin_info")) == NULL)
        {
            g_message (_("Error, %s not an GNOME-Mud module: %s."), plugin_name, dlerror());
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

static void plugin_check_enable(PLUGIN_OBJECT *plugin)
{
  gchar path[50];

  g_snprintf(path, 50, "/gnome-mud/Plugins/%s=false", plugin->name);

  plugin->enabeled = gnome_config_get_bool(path);
}

void plugin_register(PLUGIN_OBJECT *plugin)
{
    g_message (_("Registering plugin `%s' under the name `%s'."), plugin->name, plugin->info->plugin_name);

    plugin_check_enable(plugin);

    Plugin_list = g_list_append(Plugin_list, (gpointer) plugin);
    
    if (plugin->info->init_function) {
      plugin->info->init_function(NULL, GPOINTER_TO_INT(plugin->handle));
    }
}
void popup_message(const gchar *data)
{
	GtkWidget *dialog;
	
	dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, data);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

void 	       
init_modules_win(MudWindow *win)
{
	gGMudWindow = win;
}

#endif
