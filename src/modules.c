/* AMCL - A simple Mud CLient
 * Copyright (C) 1999 Robin Ericsson <lobbin@localhost.nu>
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

#include "config.h"
#include <gtk/gtk.h>
#include <dirent.h>
#include <errno.h>

#include "amcl.h"
#include "modules.h"

static char const rcsid[] =
    "$Id$";

int init_modules(char *path)
{
    DIR            *directory;
    struct dirent  *direntity;
    gint            dirn;
    gchar          *shortname;

    if ((directory = opendir(path)) == NULL)
    {
        g_warning("Plugin error: %s\n", strerror(errno));
        return FALSE;
    }

    while ((direntity = readdir(directory)))
    {
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

        g_message("Loading plugin description from `%s'.", direntity->d_name);

        plugin = plugin_query(direntity->d_name);
        if (!plugin)
            continue;

        plugin_register(plugin);
    }
    
    closedir(directory);

    return TRUE;
}

PLUGIN_OBJECT *plugin_query (gchar *plugin_name)
{
    PLUGIN_OBJECT *new_plugin = g_new0(PLUGIN_OBJECT, 1);
    gchar filename[60];

    new_plugin->name = g_strdup (strrchr(plugin_name, '/') ?
                                 strrchr(plugin_name, '/') + 1 :
                                 plugin_name);

    sprintf (filename, "./plugins/%s", plugin_name);
    if ((new_plugin->handle = dlopen (filename, RTLD_NOW)) == NULL)
    {
        g_message ("Error getting plugin handle (%s): %s.", plugin_name, dlerror());
        goto error;
    } else {
        if ((new_plugin->info = dlsym(new_plugin->handle,"amcl_plugin_info")) == NULL)
        {
            g_message ("Error, not an AMCL module: %s.", plugin_name, dlerror());
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

void plugin_register(PLUGIN_OBJECT *plugin)
{
    g_message ("Registering plugin `%s'.", plugin->name);
    g_message ("Plug-in internal name is `%s'.", plugin->info->plugin_name);
    if (plugin->info->init_function)
    {
        g_message ("Running init-function...");
        plugin->info->init_function(NULL, 0);
    }

    /*plugin->menu_item = (void *) gtk_menu_item_new_with_label (plugin->info->plugin_name);
    gtk_menu_append (GTK_MENU (menu_plugin_menu), (GtkWidget *) plugin->menu_item);
    gtk_widget_show ((GtkWidget *) plugin->menu_item);
    if (plugin->info->menu_function)
        gtk_signal_connect_object (GTK_OBJECT ((GtkWidget *) plugin->menu_item),
                                   "activate", GTK_SIGNAL_FUNC (plugin->info->menu_function),
                                   NULL);*/
}
#endif
