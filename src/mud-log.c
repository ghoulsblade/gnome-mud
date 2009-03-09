/* GNOME-Mud - A simple Mud Client
 * mud-log.c
 * Copyright (C) 2006-2009 Les Harris <lharris@gnome.org>
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

#include <glib/gi18n.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "gnome-mud.h"
#include "mud-log.h"
#include "utils.h"

struct _MudLogPrivate
{
    gboolean active;

    gchar *filename;
    gchar *dir;

    FILE *logfile;
};

/* Property Identifiers */
enum
{
    PROP_MUD_LOG_0,
    PROP_MUD_NAME
};

/* Define the Type */
G_DEFINE_TYPE(MudLog, mud_log, G_TYPE_OBJECT);

/* Class Functions */
static void mud_log_init (MudLog *log);
static void mud_log_class_init (MudLogClass *klass);
static void mud_log_finalize (GObject *object);
static GObject *mud_log_constructor (GType gtype,
                                     guint n_properties,
                                     GObjectConstructParam *properties);
static void mud_log_set_property(GObject *object,
                                 guint prop_id,
                                 const GValue *value,
                                 GParamSpec *pspec);
static void mud_log_get_property(GObject *object,
                                 guint prop_id,
                                 GValue *value,
                                 GParamSpec *pspec);

/* Private Methods */
static void mud_log_write(MudLog *log, gchar *data, gsize size);
static void mud_log_remove(MudLog *log);

// MudLog class functions
static void
mud_log_class_init (MudLogClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object constructor */
    object_class->constructor = mud_log_constructor;

    /* Override base object's finalize */
    object_class->finalize = mud_log_finalize;

    /* Override base object property methods */
    object_class->set_property = mud_log_set_property;
    object_class->get_property = mud_log_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(MudLogPrivate));

    /* Install Properties */
    g_object_class_install_property(object_class,
            PROP_MUD_NAME,
            g_param_spec_string("mud-name",
                "mud name",
                "name of mud we are logging",
                "Unnamed",
                G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));
}

static void
mud_log_init (MudLog *log)
{
    log->priv = MUD_LOG_GET_PRIVATE(log);

    /* Set defaults for Public Members */
    log->mud_name = NULL;

    /* Set defaults for Private Members */
    log->priv->active = FALSE;
    log->priv->logfile = NULL;
}

static GObject *
mud_log_constructor (GType gtype,
                     guint n_properties,
                     GObjectConstructParam *properties)
{
    guint i;
    MudLog *self;
    GObject *obj;

    MudLogClass *klass;
    GObjectClass *parent_class;

    /* Chain up to parent constructor */
    klass = MUD_LOG_CLASS( g_type_class_peek(MUD_TYPE_LOG) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    self = MUD_LOG(obj);

    if(!self->mud_name)
    {
        g_printf("ERROR: Tried to instantiate MudLog without passing mud name.\n");
        g_error("Tried to instantiate MudLog without passing mud name.\n");
    }

    return obj;
}

static void
mud_log_finalize (GObject *object)
{
    MudLog *MLog;
    GObjectClass *parent_class;

    MLog = MUD_LOG(object);

    if(MLog->priv->active)
        mud_log_close(MLog);

    if(MLog->mud_name)
        g_free(MLog->mud_name);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

static void
mud_log_set_property(GObject *object,
                     guint prop_id,
                     const GValue *value,
                     GParamSpec *pspec)
{
    MudLog *self;
    
    gchar *new_mud_name;

    self = MUD_LOG(object);

    switch(prop_id)
    {
        /* Parent is Construct Only */
        case PROP_MUD_NAME:
            new_mud_name = g_value_dup_string(value);

            if(!self->mud_name)
                self->mud_name = g_strdup(new_mud_name);
            else if( strcmp(self->mud_name, new_mud_name) != 0)
            {
                g_free(self->mud_name);
                self->mud_name = g_strdup(new_mud_name);
            }

            g_free(new_mud_name);

            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
mud_log_get_property(GObject *object,
                     guint prop_id,
                     GValue *value,
                     GParamSpec *pspec)
{
    MudLog *self;

    self = MUD_LOG(object);

    switch(prop_id)
    {
        case PROP_MUD_NAME:
            g_value_set_string(value, self->mud_name);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

/* Public Methods */
void
mud_log_open(MudLog *log)
{
    gchar buf[1024];
    gchar nameBuf[1024];
    time_t t;

    g_return_if_fail(MUD_IS_LOG(log));

    g_snprintf(buf, 1024, "%s/.gnome-mud/logs/%s", g_get_home_dir(), log->mud_name);

    log->priv->dir = g_strdup(buf);

    if(!g_file_test(buf, G_FILE_TEST_IS_DIR))
        if(mkdir(buf, 0777 ) == -1)
            return;

    g_snprintf(nameBuf, 1024, "%s.log", log->mud_name);

    log->priv->filename = g_build_path( G_DIR_SEPARATOR_S, log->priv->dir, nameBuf, NULL);
    log->priv->logfile = fopen(log->priv->filename, "a");

    if (log->priv->logfile)
    {
        time(&t);
        strftime(buf, 1024,
                _("\n*** Log starts *** %d/%m/%Y %H:%M:%S\n"),
                localtime(&t));
        fprintf(log->priv->logfile, "%s", buf);
    }

    log->priv->active = TRUE;
}

void
mud_log_close(MudLog *log)
{
    gchar buf[255];
    time_t t;

    g_return_if_fail(MUD_IS_LOG(log));
    g_return_if_fail(log->priv->logfile != NULL);

    time(&t);
    strftime(buf, 255,
            _("\n *** Log stops *** %d/%m/%Y %H:%M:%S\n"),
            localtime(&t));

    fprintf(log->priv->logfile, "%s", buf);
    fclose(log->priv->logfile);

    log->priv->active = FALSE;
}

gboolean
mud_log_islogging(MudLog *log)
{
    if(!log)
        return FALSE;

    return log->priv->active;
}

void
mud_log_write_hook(MudLog *log, gchar *data, gint length)
{
    g_return_if_fail(MUD_IS_LOG(log));

    if(log->priv->active)
        mud_log_write(log, data, length);
}

/* Private Methods */
static void
mud_log_remove(MudLog *log)
{
    g_return_if_fail(MUD_IS_LOG(log));

    if(log->priv->active)
        mud_log_close(log);

    unlink(log->priv->filename);
}

static void
mud_log_write(MudLog *log, gchar *data, gsize size)
{
    gchar *stripData;
    gint stripSize = 0;
    gsize write_size;

    g_return_if_fail(MUD_IS_LOG(log));
    g_return_if_fail(log->priv->logfile != NULL);
    g_return_if_fail(data != NULL);

    stripData = utils_strip_ansi((const gchar *)data);
    stripSize = strlen(stripData);

    write_size = fwrite(stripData, 1, stripSize, log->priv->logfile);

    if(write_size != stripSize)
        g_critical(_("Could not write data to log file!"));

    g_free(stripData);
}

