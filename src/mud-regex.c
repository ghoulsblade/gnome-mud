/* GNOME-Mud - A simple Mud Client
 * mud-regex.c
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <pcre.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <string.h>
#include <glib.h>
#include <stdio.h>

#include "mud-regex.h"

struct _MudRegexPrivate
{
    const gchar **substring_list;
    gint substring_count;
};

/* Create the Type */
G_DEFINE_TYPE(MudRegex, mud_regex, G_TYPE_OBJECT);

static void mud_regex_init (MudRegex *regex);
static void mud_regex_class_init (MudRegexClass *klass);
static void mud_regex_finalize (GObject *object);

/* Class Functions */
static void
mud_regex_class_init (MudRegexClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object's finalize */
    object_class->finalize = mud_regex_finalize;

    /* Add our private data */
    g_type_class_add_private(klass, sizeof(MudRegexPrivate));
}

static void
mud_regex_init (MudRegex *regex)
{
    regex->priv = MUD_REGEX_GET_PRIVATE(regex);

    regex->priv->substring_list = NULL;
    regex->priv->substring_count = 0;
}

static void
mud_regex_finalize (GObject *object)
{
    MudRegex *regex;
    GObjectClass *parent_class;

    regex = MUD_REGEX(object);

    if(regex->priv->substring_list)
         pcre_free_substring_list(regex->priv->substring_list);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

/* Public Methods */
gboolean
mud_regex_check(MudRegex *regex,
                const gchar *data,
		guint length,
		const gchar *rx,
		gint ovector[1020])
{
    pcre *re = NULL;
    const gchar *error = NULL;
    gint errorcode;
    gint erroroffset;
    gint rc;

    if(!MUD_IS_REGEX(regex))
        return FALSE;

    re = pcre_compile2(rx, 0, &errorcode, &error, &erroroffset, NULL);

    if(!re)
    {
	gint i;

	/*
	   This should never be called since we check the regex validity
	   at entry time.  But You Never Know(tm) so its here to catch
	   any runtime errors that cosmic rays, evil magic, errant gconf
	   editing, and Monday mornings might produce.
	 */

	g_warning("Error in Regex! - ErrCode: %d - %s", errorcode, error);
	printf("--> %s\n    ", rx);

	for(i = 0; i < erroroffset - 1; i++)
	    printf(" ");

	printf("^\n");

	return FALSE;

    }

    rc = pcre_exec(re, NULL, data, length, 0, 0, ovector, 1020);

    if(rc < 0)
	return FALSE;

    if(regex->priv->substring_list)
	pcre_free_substring_list(regex->priv->substring_list);

    pcre_get_substring_list(data, ovector, rc, &regex->priv->substring_list);
    regex->priv->substring_count = rc;

    return TRUE;
}

const gchar **
mud_regex_test(const gchar *data,
	       guint length,
	       const gchar *rx,
	       gint *rc,
	       const gchar **error,
	       gint *errorcode,
	       gint *erroroffset)
{
    pcre *re = NULL;
    gint ovector[1020];
    const gchar **sub_list;

    if(!data)
	return NULL;

    re = pcre_compile2(rx, 0, errorcode, error, erroroffset, NULL);

    if(!re)
	return NULL;

    *rc = pcre_exec(re, NULL, data, length, 0, 0, ovector, 1020);

    pcre_get_substring_list(data, ovector, *rc, &sub_list);

    return sub_list;
}

void
mud_regex_substring_clear(const gchar **substring_list)
{
    pcre_free_substring_list(substring_list);
}

const gchar **
mud_regex_get_substring_list(MudRegex *regex, gint *count)
{
    if(!MUD_IS_REGEX(regex))
        return FALSE;

    *count = regex->priv->substring_count;
    return regex->priv->substring_list;
}

