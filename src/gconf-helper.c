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

#include <gdk/gdkcolor.h>
#include <glib/gi18n.h>
#include <gtk/gtkcolorsel.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include "gconf-helper.h"

void gm_gconf_load_preferences(MudProfile *profile)
{
	GConfClient *gconf_client;
	MudPrefs *prefs;
	GdkColor  color;
	GdkColor *colors;
	gint      n_colors;
	struct    stat file_stat;
	gchar     dirname[256], buf[256];
	gchar    *p = NULL;
	gchar     extra_path[512] = "", keyname[2048];

	gconf_client = gconf_client_get_default();
	prefs = profile->preferences;

	if (strcmp(profile->name, "Default"))
	{
		GError *error = NULL;

		/* Sanity check for whether profile has data or not */
		g_snprintf(keyname, 2048, "/apps/gnome-mud/profiles/%s/functionality/terminal_type", profile->name);
		p = gconf_client_get_string(gconf_client, keyname, &error);
		if (error || p == NULL)
		{
			g_message("Error getting data for profile %s, using default instead.", profile->name);
			mud_profile_copy_preferences(mud_profile_new("Default"), profile);
		}
		else
		{
			g_snprintf(extra_path, 512, "profiles/%s/", profile->name);
		}
	}


	/*
	 * Check for ~/.gnome-mud
	 */
	g_snprintf (dirname, 255, "%s/.gnome-mud", g_get_home_dir());
	if ( stat (dirname, &file_stat) == 0) /* can we stat ~/.gnome-mud? */
	{
		if ( !(S_ISDIR(file_stat.st_mode))) /* if it's not a directory */
		{
			g_snprintf (buf, 255, _("%s already exists and is not a directory!"), dirname);
			//popup_window (buf); FIXME
			return;
		}
	}
	else /* it must not exist */
	{
		if ((mkdir (dirname, 0777)) != 0) /* this isn't dangerous, umask modifies it */
		{
			g_snprintf (buf, 255, _("%s does not exist and can NOT be created: %s"), dirname, strerror(errno));
			//popup_window (buf); FIXME
			return;
		}
	}

#define	GCONF_GET_STRING(entry, subdir, variable)                                          \
	g_snprintf(keyname, 2048, "/apps/gnome-mud/%s" #subdir "/" #entry, extra_path); \
	p = gconf_client_get_string(gconf_client, keyname, NULL);\
	prefs->variable = g_strdup(p);

#define GCONF_GET_BOOLEAN(entry, subdir, variable)                                         \
	g_snprintf(keyname, 2048, "/apps/gnome-mud/%s" #subdir "/" #entry, extra_path); \
	prefs->variable = gconf_client_get_bool(gconf_client, keyname, NULL);

#define GCONF_GET_INT(entry, subdir, variable)                                             \
	g_snprintf(keyname, 2048, "/apps/gnome-mud/%s" #subdir "/" #entry, extra_path); \
	prefs->variable = gconf_client_get_int(gconf_client, keyname, NULL);

#define GCONF_GET_COLOR(entry, subdir, variable)                                           \
	g_snprintf(keyname, 2048, "/apps/gnome-mud/%s" #subdir "/" #entry, extra_path); \
	p = gconf_client_get_string(gconf_client, keyname, NULL);\
	if (p && gdk_color_parse(p, &color))                                                   \
	{                                                                                      \
		prefs->variable = color;                                                            \
	}

	GCONF_GET_STRING(font, 				ui,				FontName);
	GCONF_GET_COLOR(foreground_color,	ui,				Foreground);
	GCONF_GET_COLOR(background_color,	ui,				Background);
	GCONF_GET_INT(scrollback_lines,		ui,				Scrollback);
	GCONF_GET_STRING(tab_location,		ui,				TabLocation);
	GCONF_GET_STRING(commdev, 			functionality,	CommDev);
	GCONF_GET_BOOLEAN(echo,     		functionality,	EchoText);
	GCONF_GET_BOOLEAN(keeptext,			functionality,	KeepText);
	GCONF_GET_BOOLEAN(system_keys,		functionality,	DisableKeys);
	GCONF_GET_STRING(terminal_type,		functionality,	TerminalType);
	GCONF_GET_STRING(mudlist_file,		functionality,	MudListFile);
	GCONF_GET_BOOLEAN(scroll_on_output,	functionality,	ScrollOnOutput);
	GCONF_GET_INT(history_count,		functionality,	History);
	GCONF_GET_INT(flush_interval,		functionality,	FlushInterval);
	GCONF_GET_STRING(encoding,          functionality,  Encoding);
	GCONF_GET_STRING(proxy_version,     functionality,  ProxyVersion);
	GCONF_GET_BOOLEAN(use_proxy,        functionality,  UseProxy);
	GCONF_GET_BOOLEAN(remote_encoding,  functionality,  UseRemoteEncoding);
	GCONF_GET_STRING(proxy_hostname,    functionality,  ProxyHostname);

	/* palette */
	g_snprintf(keyname, 2048, "/apps/gnome-mud/%sui/palette", extra_path);
	p = gconf_client_get_string(gconf_client, keyname, NULL);

	if (p)
	{
		gtk_color_selection_palette_from_string(p, &colors, &n_colors);
		if (n_colors < C_MAX)
		{
		        g_printerr(ngettext("Palette had %d entry instead of %d\n",
					    "Palette had %d entries instead of %d\n",
					    n_colors),
				   n_colors, C_MAX);
		}
		memcpy(prefs->Colors, colors, C_MAX * sizeof(GdkColor));
		g_free(colors);
	}

	/* last log dir */
	g_snprintf(keyname, 2048, "/apps/gnome-mud/%sfunctionality/last_log_dir", extra_path);
	p = gconf_client_get_string(gconf_client, keyname, NULL);

	if (p == NULL || !g_ascii_strncasecmp(p, "", sizeof("")))
	{
		prefs->LastLogDir = g_strdup(g_get_home_dir());
	}
	else
	{
		prefs->LastLogDir = g_strdup(p);
	}

#ifdef ENABLE_MAPPER
	/* load automapper prefs : unusual_exits */
	/* FIXME, What? p = gconf_client_get_string(gconf_client, "/apps/gnome-mud/mapper/unusual_exits", NULL);
	automap_config = g_malloc0(sizeof(AutoMapConfig));
	if (p)
	{
		int i;
		gchar** unusual_exits;
		unusual_exits = g_malloc0(sizeof(char) * (strlen(p) + 2));
		unusual_exits = g_strsplit(p, ";", 100);

		for (i = 0; unusual_exits[i] != NULL; i++)
		{
			unusual_exits[i] = g_strstrip(unusual_exits[i]);
			if (unusual_exits[i][0] != '\0')
				automap_config->unusual_exits = g_list_append(automap_config->unusual_exits, g_strdup(unusual_exits[i]));
		}
		g_strfreev(unusual_exits);
	}
	else
	{
		automap_config->unusual_exits = NULL;
	}*/
#endif
	/*
	 * Command history
	 *
	{
		gint  nr, i;
		gchar **cmd_history;
		gnome_config_get_vector("/gnome-mud/Data/CommandHistory", &nr, &cmd_history);

		for (i = 0; i < nr; i++)
		{
			EntryHistory = g_list_append(EntryHistory, (gpointer) cmd_history[i]);
		}

		EntryCurr = NULL;
	}*/
#undef GCONF_GET_BOOLEAN
#undef GCONF_GET_COLOR
#undef GCONF_GET_INT
#undef GCONF_GET_STRING
}
