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

#include <dlfcn.h>

/*
 * Typedefs
 */
typedef struct _plugin_object PLUGIN_OBJECT;
typedef struct _plugin_info   PLUGIN_INFO;

typedef void      (*plugin_initfunc) (PLUGIN_OBJECT *, gint);
typedef gboolean (*plugin_startfunc) (PLUGIN_OBJECT *, gint);
typedef void      (*plugin_menufunc) (GtkWidget *,     gint);

/*
 * Structures
 */
struct _plugin_info {
    gchar            *plugin_name;
    plugin_initfunc   init_function;
    plugin_startfunc  start_function;
};

struct _plugin_object {
    void  *handle;
    gchar *name;
    gchar *filename;
    void  *menu_item;
    PLUGIN_INFO *info;
};

/*
 * Functions
 */
PLUGIN_OBJECT *plugin_query    (gchar * plugin_name              );
void           plugin_register (PLUGIN_OBJECT *plugin            );
