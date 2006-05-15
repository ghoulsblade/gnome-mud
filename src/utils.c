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

#include <glib.h>
#include <stdlib.h>
#include <string.h>

gchar *
remove_whitespace(gchar *string)
{
	gint i;
	GString *s = g_string_new(NULL);
	gchar *ret;
	
	for(i = 0; i < strlen(string); i++)
		if(!g_ascii_isspace(string[i]))
			g_string_append_c(s, string[i]);
			
	ret = g_strdup(s->str);
	
	g_string_free(s, TRUE);
	
	return ret;
}

gchar *
strip_ansi(const gchar *orig)
{
  gchar *buf;
  const gchar *c;
  gint currChar = 0;

  if (!orig) 
    return NULL;

  buf = g_malloc(strlen(orig) * sizeof(gchar));
  for (c = orig; *c;) 
  {
    switch (*c) 
    {
    	case '\x1B': // Esc Character
      		while (*c && *c++ != 'm') ;
      	break;

    	case '\02': // HTML Open bracket
      		while (*c && *c++ != '\03'); // HTML Close bracket
      	break;

    	default:
			buf[currChar++] = *c++;
    }
  }
  
  return buf;
}
