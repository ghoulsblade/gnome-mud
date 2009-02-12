/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1998-2006 Robin Ericsson <lobbin@localhost.nu>
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

	gchar *name;
	gchar *filename;
	gchar *dir;

	FILE *logfile;
};

GType mud_log_get_type (void);

static void mud_log_init (MudLog *log);
static void mud_log_class_init (MudLogClass *klass);
static void mud_log_finalize (GObject *object);

void mud_log_write(MudLog *log, gchar *data, gsize size);
void mud_log_remove(MudLog *log);

// MudLog class functions
GType
mud_log_get_type (void)
{
	static GType object_type = 0;

	g_type_init();

	if (!object_type)
	{
		static const GTypeInfo object_info =
		{
			sizeof (MudLogClass),
			NULL,
			NULL,
			(GClassInitFunc) mud_log_class_init,
			NULL,
			NULL,
			sizeof (MudLog),
			0,
			(GInstanceInitFunc) mud_log_init,
		};

		object_type = g_type_register_static(G_TYPE_OBJECT, "MudLog", &object_info, 0);
	}

	return object_type;
}

static void
mud_log_init (MudLog *log)
{
	log->priv = g_new0(MudLogPrivate, 1);

	log->priv->active = FALSE;
	log->priv->logfile = NULL;
	log->priv->name = NULL;
}

static void
mud_log_class_init (MudLogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = mud_log_finalize;
}

static void
mud_log_finalize (GObject *object)
{
	MudLog *MLog;
	GObjectClass *parent_class;

	MLog = MUD_LOG(object);

	g_free(MLog->priv);

	parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
	parent_class->finalize(object);
}

// MudLog Methods

void
mud_log_open(MudLog *log)
{
	gchar buf[1024];
	gchar nameBuf[1024];
	time_t t;

	g_snprintf(buf, 1024, "%s/.gnome-mud/logs/%s", g_get_home_dir(), log->priv->name);

	log->priv->dir = g_strdup(buf);

	if(!g_file_test(buf, G_FILE_TEST_IS_DIR))
		if(mkdir(buf, 0777 ) == -1)
			return;

	g_snprintf(nameBuf, 1024, "%s.log", log->priv->name);

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
mud_log_write(MudLog *log, gchar *data, gsize size)
{
	gchar *stripData;
	gint stripSize = 0;

	if(log->priv->logfile == NULL || data == NULL)
		return;

	stripData = strip_ansi((const gchar *)data);
	stripSize = strlen(stripData);

	if(!fwrite(stripData, 1, stripSize, log->priv->logfile))
		g_critical(_("Could not write data to log file!"));

	g_free(stripData);
}

void
mud_log_close(MudLog *log)
{
	gchar buf[255];
	time_t t;

	if(log->priv->logfile == NULL)
		return;

	time(&t);
	strftime(buf, 255,
			_("\n *** Log stops *** %d/%m/%Y %H:%M:%S\n"),
			localtime(&t));

	fprintf(log->priv->logfile, "%s", buf);
	fclose(log->priv->logfile);

	log->priv->active = FALSE;
}

void
mud_log_remove(MudLog *log)
{
	if(log->priv->active)
		mud_log_close(log);

	unlink(log->priv->filename);
}

gboolean
mud_log_islogging(MudLog *log)
{
	return log->priv->active;
}

// MudLog Utility Functions

void
mud_log_write_hook(MudLog *log, gchar *data, gint length)
{
	if(log->priv->active)
		mud_log_write(log, data, length);
}

// Instantiate MudLog
MudLog*
mud_log_new(gchar *mudName)
{
	MudLog *MLog;

	if( mudName == NULL)
		return NULL;

	MLog = g_object_new(MUD_TYPE_LOG, NULL);

	MLog->priv->name = mudName;

	return MLog;
}
