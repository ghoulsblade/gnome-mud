/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 2006 Robin Ericsson <lobbin@localhost.nu>
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
#include <glib/gprintf.h>

#include "mud-regex.h"    
            
struct _MudRegexPrivate
{
	const gchar **substring_list;
	gint substring_count;
};

GType mud_regex_get_type (void);
static void mud_regex_init (MudRegex *regex);
static void mud_regex_class_init (MudRegexClass *klass);
static void mud_regex_finalize (GObject *object);

// MudRegex class functions
GType
mud_regex_get_type (void)
{
	static GType object_type = 0;

	g_type_init();

	if (!object_type)
	{
		static const GTypeInfo object_info =
		{
			sizeof (MudRegexClass),
			NULL,
			NULL,
			(GClassInitFunc) mud_regex_class_init,
			NULL,
			NULL,
			sizeof (MudRegex),
			0,
			(GInstanceInitFunc) mud_regex_init,
		};

		object_type = g_type_register_static(G_TYPE_OBJECT, "MudRegex", &object_info, 0);
	}

	return object_type;
}

static void
mud_regex_init (MudRegex *regex)
{
	regex->priv = g_new0(MudRegexPrivate, 1);	
	
	regex->priv->substring_list = NULL;
	regex->priv->substring_count = 0;
}

static void
mud_regex_class_init (MudRegexClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = mud_regex_finalize;
}

static void
mud_regex_finalize (GObject *object)
{
	MudRegex *regex;
	GObjectClass *parent_class;

	regex = MUD_REGEX(object);
	
	g_free(regex->priv);

	parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
	parent_class->finalize(object);
}

// MudRegex Methods

gboolean 
mud_regex_check(const gchar *data, const gchar *rx, gint ovector[1020], MudRegex *regex)
{
	pcre *re = NULL;
	const gchar *error = NULL;
	gint errorcode;
	gint erroroffset;
	gint rc;
	
	re = pcre_compile2(rx, 0, &errorcode, &error, &erroroffset, NULL);
	
	if(!re)
	{
		gint i;
		
		/* This should never be called since we check the regex validity
		   at entry time.  But You Never Know(tm) so its here to catch
		   any runtime errors that cosmic rays, evil magic, errant gconf
		   editing, and Monday mornings might produce.					*/
		   
		g_warning("Error in Regex! - ErrCode: %d - %s", errorcode, error);
		g_printf("--> %s\n    ", rx);
		
		for(i = 0; i < erroroffset - 1; i++)
			g_printf(" ");
		
		g_printf("^\n");
		
		return FALSE;
		
	}

	rc = pcre_exec(re, NULL, data, strlen(data), 0, 0, ovector, 1020);

	if(rc < 0)
		return FALSE;
		
	if(regex->priv->substring_list)
		pcre_free_substring_list(regex->priv->substring_list);
		
	pcre_get_substring_list(data, ovector, rc, &regex->priv->substring_list);
	regex->priv->substring_count = rc;
	
	return TRUE;
}

const gchar **
mud_regex_test(const gchar *data, const gchar *rx, gint *rc, const gchar **error, gint *errorcode, gint *erroroffset)
{
	pcre *re = NULL;
	gint ovector[1020];
	const gchar **sub_list;
	
	re = pcre_compile2(rx, 0, errorcode, error, erroroffset, NULL);

	if(!re)
		return NULL;
	
	*rc = pcre_exec(re, NULL, data, strlen(data), 0, 0, ovector, 1020);
	
	pcre_get_substring_list(data, ovector, *rc, &sub_list);
	
	return sub_list;
}

void 
mud_regex_substring_clear(const gchar **substring_list)
{
	pcre_free_substring_list(substring_list);
}

const gchar **
mud_regex_get_substring_list(gint *count, MudRegex *regex)
{
	*count = regex->priv->substring_count;
	return regex->priv->substring_list;
}


// Instantiate MudRegex
MudRegex*
mud_regex_new(void)
{
	MudRegex *regex;
	
	regex = g_object_new(MUD_TYPE_REGEX, NULL);
	
	return regex;	
}
