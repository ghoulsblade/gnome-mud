/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1998-2002 Robin Ericsson <lobbin@localhost.nu>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <gnome.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gnome-mud.h"

static char const rcsid[] =
    "$Id$";

extern GdkFont  *font_normal;
extern GList 	*EntryHistory;
extern GList	*EntryCurr;

SYSTEM_DATA prefs;
SYSTEM_DATA pre_prefs;

struct color_struct 
{
	gchar    *name;
	gchar    *def;
};

const struct color_struct c_structs[C_MAX] =
{
	{ "color_black",        "0,0,0"             },
	{ "color_red",          "32896,0,0"         },
	{ "color_green",        "0,32896,0"         },
	{ "color_brown",        "32896,32896,0"     },
	{ "color_blue",         "0,0,32896"         },
	{ "color_magenta",      "32896,0,32896"     },
	{ "color_cyan",         "0,32896,32896"     },
	{ "color_lightgrey",    "32896,32896,32896" },
	{ "color_grey",         "16448,16448,16448" },
	{ "color_lightred",     "65535,0,0"         },
	{ "color_lightgreen",   "0,65535,0"         },
	{ "color_yellow",       "65535,65535,0"     },
	{ "color_lightblue",    "0,0,65535"         },
	{ "color_lightmagenta", "65535,0,65535"     },
	{ "color_lightcyan",    "0,65535,65535"     },
	{ "color_white",        "65535,65535,65535" }
};

static void prefs_load_color(GdkColor *color, gchar *prefs, gchar *name, gchar *def)
{
	GdkColormap *cmap = gdk_colormap_get_system();

	gchar *str  = g_strconcat("/gnome-mud/", prefs, "/", name, "=", def, NULL);
	gchar *conf = gnome_config_get_string(str);
	gint   red, green, blue;

	if (sscanf(conf, "%d,%d,%d", &red, &green, &blue) == 3)
	{
		color->red   = red;
		color->green = green;
		color->blue  = blue;
		
		if (!gdk_colormap_alloc_color(cmap, color, TRUE, TRUE))
		{
			g_warning(_("Couldn't allocate color"));
		}
	}
	else
	{
		g_warning(_("Font %s Loading Error"), prefs);
	}

	g_free(str);
}

static void prefs_save_color(GdkColor *color, gchar *prefs, gchar *name)
{
	gchar key[1024];
	gchar value[20];

	g_snprintf(key, 1024, "/gnome-mud/%s/%s", prefs, name);
	g_snprintf(value, 20, "%d,%d,%d", color->red, color->green, color->blue);

	gnome_config_set_string(key, value);
}

void load_prefs ( void )
{
	struct stat file_stat;
	gchar dirname[256], buf[256];
	gint i;
	
	/*
	 * Check for ~/.gnome-mud
	 */
	g_snprintf (dirname, 255, "%s/.gnome-mud", g_get_home_dir());
	if ( stat (dirname, &file_stat) == 0) /* can we stat ~/.gnome-mud? */
	{
		if ( !(S_ISDIR(file_stat.st_mode))) /* if it's not a directory */
		{
			g_snprintf (buf, 255, _("%s already exists and is not a directory!"), dirname);
			popup_window (buf);
			return;
		}
	} 
	else /* it must not exist */
	{
		if ((mkdir (dirname, 0777)) != 0) /* this isn't dangerous, umask modifies it */
		{
			g_snprintf (buf, 255, _("%s does not exist and can NOT be created: %s"), dirname, strerror(errno));
			popup_window (buf);
			return;
		}
	}

	/*
	 * Get general parameters
	 */
	prefs.EchoText     = gnome_config_get_bool  ("/gnome-mud/Preferences/EchoText=true");
	prefs.KeepText     = gnome_config_get_bool  ("/gnome-mud/Preferences/KeepText=false");
	prefs.Freeze       = gnome_config_get_bool  ("/gnome-mud/Preferences/Freeze=false");
	prefs.DisableKeys  = gnome_config_get_bool  ("/gnome-mud/Preferences/DisableKeys=false");
	prefs.CommDev      = gnome_config_get_string("/gnome-mud/Preferences/CommDev=;");
	prefs.TerminalType = gnome_config_get_string("/gnome-mud/Preferences/TerminalType=ansi");
	prefs.FontName     = gnome_config_get_string("/gnome-mud/Preferences/FontName=fixed");
	prefs.MudListFile  = gnome_config_get_string("/gnome-mud/Preferences/MudListFile=");

	g_snprintf(buf, 255, "/gnome-mud/Preferences/LastLogDir=%s/", g_get_home_dir());
	prefs.LastLogDir  = gnome_config_get_string(buf);

	prefs.History     = gnome_config_get_int   ("/gnome-mud/Preferences/History=10");

	/*
	 * Fore-/Background Colors
	 */
	prefs_load_color(&prefs.Foreground,     "Preferences", "Foreground",     "65535,65535,65535");
	prefs_load_color(&prefs.BoldForeground, "Preferences", "BoldForeground", "65535,65535,65535");
	prefs_load_color(&prefs.Background,     "Preferences", "Background",     "0,0,0");

	/*
	 * Other Colors
	 */
	for (i = 0; i < C_MAX; i++)
	{
		prefs_load_color(&prefs.Colors[i], "Preferences", c_structs[i].name, c_structs[i].def);
	}
	
	/*
	 * Command history
	 */
	{
		gint  nr, i;
		gchar **cmd_history;
		gnome_config_get_vector("/gnome-mud/Data/CommandHistory", &nr, &cmd_history);

		for (i = 0; i < nr; i++)
		{
			EntryHistory = g_list_append(EntryHistory, (gpointer) cmd_history[i]);
		}

		EntryCurr = NULL;
	}
	
	/*
	 * Load font
	 */
	font_normal = gdk_font_load(prefs.FontName);
}

void save_prefs ( void )
{
	gint  i; 

	gnome_config_set_bool  ("/gnome-mud/Preferences/EchoText",    prefs.EchoText);
	gnome_config_set_bool  ("/gnome-mud/Preferences/KeepText",    prefs.KeepText);
	gnome_config_set_bool  ("/gnome-mud/Preferences/AutoSave",    prefs.AutoSave);
	gnome_config_set_bool  ("/gnome-mud/Preferences/Freeze",      prefs.Freeze);
	gnome_config_set_bool  ("/gnome-mud/Preferences/DisableKeys", prefs.DisableKeys);
	gnome_config_set_string("/gnome-mud/Preferences/CommDev",     prefs.CommDev);
	gnome_config_set_string("/gnome-mud/Preferences/TerminalType", prefs.TerminalType);
	gnome_config_set_string("/gnome-mud/Preferences/FontName",    prefs.FontName);
	gnome_config_set_string("/gnome-mud/Preferences/MudListFile", prefs.MudListFile);
	gnome_config_set_string("/gnome-mud/Preferences/LastLogDir",  prefs.LastLogDir);
	gnome_config_set_int   ("/gnome-mud/Preferences/History",     prefs.History);

	prefs_save_color(&prefs.Foreground,     "Preferences", "Foreground");
	prefs_save_color(&prefs.BoldForeground, "Preferences", "BoldForeground");
	prefs_save_color(&prefs.Background,     "Preferences", "Background");

	for (i = 0; i < C_MAX; i++)
	{
		prefs_save_color(&prefs.Colors[i], "Preferences", c_structs[i].name);
	}
	
	gnome_config_sync();
}

static void prefs_copy_color(GdkColor *a, GdkColor *b)
{
	a->red   = b->red;
	a->green = b->green;
	a->blue  = b->blue;
}

static void prefs_copy(SYSTEM_DATA *target, SYSTEM_DATA *prefs, gboolean alloc_col)
{
	gint i;
	
	target->EchoText     = prefs->EchoText;
	target->KeepText     = prefs->KeepText;
	target->AutoSave     = prefs->AutoSave;
	target->DisableKeys  = prefs->DisableKeys;
	target->Freeze       = prefs->Freeze;                 g_free(target->FontName);
	target->FontName     = g_strdup(prefs->FontName);     g_free(target->CommDev);
	target->CommDev      = g_strdup(prefs->CommDev);      g_free(target->MudListFile);
	target->MudListFile  = g_strdup(prefs->MudListFile);  g_free(target->TerminalType);
	target->TerminalType = g_strdup(prefs->TerminalType); g_free(target->LastLogDir);
	target->LastLogDir   = g_strdup(prefs->LastLogDir);
	target->History      = prefs->History;

	prefs_copy_color(&target->Foreground,     &prefs->Foreground);
	prefs_copy_color(&target->BoldForeground, &prefs->BoldForeground);
	prefs_copy_color(&target->Background,     &prefs->Background);

	for (i = 0; i < C_MAX; i++)
	{
		prefs_copy_color(&target->Colors[i], &prefs->Colors[i]);
	}
	
	if (alloc_col)
	{
		GdkColormap *cmap = gdk_colormap_get_system();

		gdk_color_alloc(cmap, &target->Foreground);
		gdk_color_alloc(cmap, &target->Background);

		for (i = 0; i < C_MAX; i++)
		{
			gdk_color_alloc(cmap, &target->Colors[i]);
		}
	}
}

static void prefs_checkbox_keep_cb (GtkWidget *widget, GnomePropertyBox *box)
{
    if ( GTK_TOGGLE_BUTTON (widget)->active )
        pre_prefs.KeepText = TRUE;
    else
        pre_prefs.KeepText = FALSE;

    gnome_property_box_changed(box);
}

static void prefs_checkbutton_freeze_cb (GtkWidget *widget, GnomePropertyBox *box)
{
  if ( GTK_TOGGLE_BUTTON (widget)->active )
    pre_prefs.Freeze = TRUE;
  else
    pre_prefs.Freeze = FALSE;
  
  gnome_property_box_changed(box);
}

static void prefs_checkbutton_disablekeys_cb(GtkWidget *widget, GnomePropertyBox *box)
{
	if (GTK_TOGGLE_BUTTON(widget)->active)
	{
		pre_prefs.DisableKeys = TRUE;
	}
	else
	{
		pre_prefs.DisableKeys = FALSE;
	}

	gnome_property_box_changed(box);
}

static void prefs_entry_terminal_cb(GtkWidget *widget, GnomePropertyBox *box)
{
	gchar *s;

	s = gtk_entry_get_text(GTK_ENTRY(widget));
	if (s)
	{
		pre_prefs.TerminalType = s;
		gnome_property_box_changed (box);
	}
}

static void prefs_entry_history_cb(GtkWidget *widget, GnomePropertyBox *box)
{
  gchar *s;

  s = gtk_entry_get_text(GTK_ENTRY(widget));
  if (s) {
    pre_prefs.History = atoi(s);
    gnome_property_box_changed(box);
  }
}

static void prefs_entry_mudlistfile_cb(GtkWidget *widget, GnomePropertyBox *box)
{
	gchar *s;

	s = gtk_entry_get_text(GTK_ENTRY(widget));
	if (s)
	{
		pre_prefs.MudListFile = g_strdup(s);
		gnome_property_box_changed(box);
	}
}

static void prefs_entry_divide_cb (GtkWidget *widget, GnomePropertyBox *box)
{
  gchar *s;
  s = gtk_entry_get_text(GTK_ENTRY(widget));
  if (s) {
    pre_prefs.CommDev = g_strdup(s);
    
    gnome_property_box_changed(box);
  }
}

static void prefs_checkbox_echo_cb(GtkWidget *widget, GnomePropertyBox *box)
{
  if ( GTK_TOGGLE_BUTTON (widget)->active )
    pre_prefs.EchoText = TRUE;
  else
    pre_prefs.EchoText = FALSE;
  
  gnome_property_box_changed(box);
}

static void prefs_select_font_cb(GnomeFontPicker *fontpicker, gchar *font, gpointer data)
{
	if (font != NULL)
	{
		g_free(pre_prefs.FontName);
		pre_prefs.FontName = g_strdup(font);
    
		gnome_property_box_changed((GnomePropertyBox *) data);
	}
}

static void prefs_select_color_cb(GnomeColorPicker *colorpicker, guint r, guint g, guint b, guint alpha, GdkColor *color)
{
	if (colorpicker != NULL)
	{
		GnomePropertyBox *box = gtk_object_get_data(GTK_OBJECT(colorpicker), "prefs_window");
		gnome_property_box_changed(box);
		
		color->red = r;
		color->blue = b;
		color->green = g;
	}
}

static void prefs_apply_cb(GnomePropertyBox *propertybox, gint page, gpointer data)
{
  if (page == -1) {
    prefs_copy(&prefs, &pre_prefs, TRUE);
    
    font_normal = gdk_font_load(prefs.FontName);

    save_prefs();
  }
}

static void prefs_set_color(GtkWidget *color_picker, gint color)
{
    GdkColor *this_color = &prefs.Colors[color];

	gnome_color_picker_set_i16(GNOME_COLOR_PICKER(color_picker), this_color->red, this_color->green, this_color->blue, 0);
}

GtkWidget *prefs_color_frame (GtkWidget *prefs_window)
{
	GtkWidget *table_colorfont;
	GtkWidget *label_palette;
	GtkWidget *label_background;
	GtkWidget *label_boldforeground;
	GtkWidget *label_foreground;
	GtkWidget *picker_foreground;
	GtkWidget *picker_boldforeground;
	GtkWidget *picker_background;
	GtkWidget *picker_font;
	GtkWidget *table2;
	GtkWidget *label_font;
	gint i, j, k;

	table_colorfont = gtk_table_new (5, 2, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table_colorfont), 8);
	gtk_table_set_row_spacings (GTK_TABLE (table_colorfont), 4);

	label_font = gtk_label_new (_("Font:"));
	gtk_widget_show (label_font);
	gtk_table_attach (GTK_TABLE (table_colorfont), label_font, 0, 1, 0, 1, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_font), 1, 0.5);
	gtk_misc_set_padding (GTK_MISC (label_font), 8, 0);

	picker_font = gnome_font_picker_new ();
	gtk_widget_show (picker_font);
	gtk_table_attach (GTK_TABLE (table_colorfont), picker_font, 1, 2, 0, 1, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gnome_font_picker_set_preview_text (GNOME_FONT_PICKER (picker_font), _("The quick brown fox jumps over the lazy dog"));
	gnome_font_picker_set_mode (GNOME_FONT_PICKER (picker_font), GNOME_FONT_PICKER_MODE_FONT_INFO);
	gnome_font_picker_fi_set_use_font_in_label (GNOME_FONT_PICKER (picker_font), TRUE, 0);
	if (!g_strcasecmp(prefs.FontName, "fixed"))
	{
		gnome_font_picker_set_font_name(GNOME_FONT_PICKER(picker_font), "-misc-fixed-medium-r-semicondensed-*-*-120-*-*-c-*-iso8859-1");
	}
	else
	{
		gnome_font_picker_set_font_name(GNOME_FONT_PICKER(picker_font), prefs.FontName);
	}
	
	label_palette = gtk_label_new (_("Color palette:"));
	gtk_widget_show (label_palette);
	gtk_table_attach (GTK_TABLE (table_colorfont), label_palette, 0, 1, 4, 5, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_palette), 1, 0.5);
	gtk_misc_set_padding (GTK_MISC (label_palette), 8, 0);

	label_background = gtk_label_new (_("Background color:"));
	gtk_widget_show (label_background);
	gtk_table_attach (GTK_TABLE (table_colorfont), label_background, 0, 1, 3, 4, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_background), 1, 0.5);
	gtk_misc_set_padding (GTK_MISC (label_background), 8, 0);

	label_foreground = gtk_label_new (_("Foreground color:"));
	gtk_widget_show (label_foreground);
	gtk_table_attach (GTK_TABLE (table_colorfont), label_foreground, 0, 1, 1, 2, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_foreground), 1, 0.5);
	gtk_misc_set_padding (GTK_MISC (label_foreground), 8, 0);

	picker_foreground = gnome_color_picker_new ();
	gtk_widget_show (picker_foreground);
	gtk_table_attach (GTK_TABLE (table_colorfont), picker_foreground, 1, 2, 1, 2, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gnome_color_picker_set_i16(GNOME_COLOR_PICKER(picker_foreground), prefs.Foreground.red, prefs.Foreground.green, prefs.Foreground.blue, 0);
  
	picker_background = gnome_color_picker_new ();
	gtk_widget_show (picker_background);
	gtk_table_attach (GTK_TABLE (table_colorfont), picker_background, 1, 2, 3, 4, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gnome_color_picker_set_i16(GNOME_COLOR_PICKER(picker_background), prefs.Background.red, prefs.Background.green, prefs.Background.blue, 0);
  
	/* Bold foreground label and picker */

	label_boldforeground = gtk_label_new (_("Bold foreground color:"));
	gtk_widget_show (label_boldforeground);
	gtk_table_attach (GTK_TABLE (table_colorfont), label_boldforeground, 0, 1, 2, 3, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label_boldforeground), 1, 0.5);
	gtk_misc_set_padding (GTK_MISC (label_boldforeground), 8, 0);

	picker_boldforeground = gnome_color_picker_new ();
	gtk_widget_show (picker_boldforeground);
	gtk_table_attach (GTK_TABLE (table_colorfont), picker_boldforeground, 1, 2, 2, 3, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER(picker_boldforeground), prefs.BoldForeground.red, prefs.BoldForeground.green, prefs.BoldForeground.blue, 0);

	table2 = gtk_table_new (2, 8, FALSE);
	gtk_widget_show (table2);
	gtk_table_attach (GTK_TABLE (table_colorfont), table2, 1, 2, 4, 5, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (GTK_FILL), 0, 0);

	for (i = 0, j = 0, k = 0; i < C_MAX; i++)
	{
		GtkWidget *picker = gnome_color_picker_new();
		
		prefs_set_color(picker, i);
		gtk_table_attach(GTK_TABLE(table2), picker, j, j + 1, k, k + 1, GTK_FILL, 0, 0, 0);
		gtk_widget_show(picker);

		if (j++ == 7)
		{
			j = 0;
			k = 1;
		}

		gtk_object_set_data(GTK_OBJECT(picker), "prefs_window", prefs_window);
		gtk_signal_connect(GTK_OBJECT(picker),  "color-set",    prefs_select_color_cb, (gpointer) &pre_prefs.Colors[i]);
	}
	
	/*
	 * Signals
	 */
	gtk_signal_connect(GTK_OBJECT(picker_font),       "font-set",   prefs_select_font_cb, (gpointer) prefs_window);

	gtk_object_set_data(GTK_OBJECT(picker_foreground), "prefs_window", prefs_window);
	gtk_object_set_data(GTK_OBJECT(picker_boldforeground), "prefs_window", prefs_window);
	gtk_object_set_data(GTK_OBJECT(picker_background), "prefs_window", prefs_window);
	
	gtk_signal_connect(GTK_OBJECT(picker_foreground), "color-set",  prefs_select_color_cb, (gpointer) &pre_prefs.Foreground);
	gtk_signal_connect(GTK_OBJECT(picker_boldforeground), "color-set", prefs_select_color_cb, (gpointer) &pre_prefs.BoldForeground);
	gtk_signal_connect(GTK_OBJECT(picker_background), "color-set",  prefs_select_color_cb, (gpointer) &pre_prefs.Background);
	
	return table_colorfont;
}


void window_prefs (GtkWidget *widget, gpointer data)
{
  static GtkWidget *prefs_window;
  
  GtkWidget *vbox2;
  GtkWidget *hbox1, *hbox_term;
  GtkWidget *checkbutton_echo, *checkbutton_keep, *checkbutton_freeze, *checkbutton;
  GtkWidget *label1, *label2, *label_term;
  GtkWidget *entry_divider, *entry_history, *entry_mudlistfile, *entry_term;
  GtkWidget *event_box_term;
  gchar      history[10];

  GtkTooltips *tooltip;

  if (prefs_window != NULL)
  {
    gdk_window_raise(prefs_window->window);
    gdk_window_show(prefs_window->window);
    return;
  }

  prefs_copy(&pre_prefs, &prefs, FALSE);
  
  prefs_window = gnome_property_box_new();
  gtk_window_set_title(GTK_WINDOW(prefs_window), _("GNOME-Mud Preferences"));
  gtk_window_set_policy(GTK_WINDOW(prefs_window), FALSE, FALSE, FALSE);
  gtk_signal_connect(GTK_OBJECT(prefs_window), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &prefs_window);
  gtk_signal_connect(GTK_OBJECT(prefs_window), "apply",   GTK_SIGNAL_FUNC(prefs_apply_cb), NULL);
  
  tooltip = gtk_tooltips_new();


  /*
  ** BEGIN PAGE TWO
  */
  vbox2 = gtk_vbox_new (TRUE, 0);
  gtk_widget_show (vbox2);

  checkbutton_echo = gtk_check_button_new_with_label (_("Echo the text sent?"));
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (checkbutton_echo), prefs.EchoText);
  gtk_widget_show (checkbutton_echo);
  gtk_box_pack_start (GTK_BOX (vbox2), checkbutton_echo, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltip, checkbutton_echo,
			_("With this toggled on, all the text you type and "
			  "enter will be echoed on the connection so you can "
			  "control what you are sending."),
			NULL);
  gtk_signal_connect(GTK_OBJECT(checkbutton_echo), "toggled", prefs_checkbox_echo_cb, (gpointer) prefs_window);

  checkbutton_keep = gtk_check_button_new_with_label (_("Keep the text entered?"));
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (checkbutton_keep), prefs.KeepText);
  gtk_widget_show (checkbutton_keep);
  gtk_box_pack_start (GTK_BOX (vbox2), checkbutton_keep, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltip, checkbutton_keep,
			_("With this toggled on, the text you have entered "
			  "and sent to the connection, will be left in the "
			  "entry box but selected. Turn this off to remove "
			  "the text after it has been sent."),
			NULL);
  gtk_signal_connect(GTK_OBJECT(checkbutton_keep), "toggled", prefs_checkbox_keep_cb, (gpointer) prefs_window);
 
  checkbutton_freeze = gtk_check_button_new_with_label (_("Freeze/Thaw?"));
  gtk_tooltips_set_tip (tooltip, checkbutton_freeze,
			_("Using this, text will draw faster but it will not "
			  "draw every character, only whole strings. This "
			  "will probably make displays a lot faster if you "
			  "are playing MUDs with a lot of different colours."),
			NULL);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (checkbutton_freeze), prefs.Freeze);
  gtk_box_pack_start (GTK_BOX (vbox2), checkbutton_freeze, FALSE, FALSE, 0);
  GTK_WIDGET_UNSET_FLAGS (checkbutton_freeze, GTK_CAN_FOCUS);
  gtk_widget_show(checkbutton_freeze);
  gtk_signal_connect(GTK_OBJECT(checkbutton_freeze), "toggled",
		     prefs_checkbutton_freeze_cb, (gpointer) prefs_window);

  checkbutton = gtk_check_button_new_with_label(_("Disable Systemkeys?"));
  gtk_tooltips_set_tip(tooltip, checkbutton,
		  _("GNOME-Mud ships with a few built-in keybinds. "
			"These keybinds can be overridden by own keybinds, "
			"or they can be disabled by toggling this checkbox"),
		  NULL);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(checkbutton), prefs.DisableKeys);
  gtk_box_pack_start(GTK_BOX(vbox2), checkbutton, FALSE, FALSE, 0);
  GTK_WIDGET_UNSET_FLAGS(checkbutton, GTK_CAN_FOCUS);
  gtk_widget_show(checkbutton);
  gtk_signal_connect(GTK_OBJECT(checkbutton), "toggled", prefs_checkbutton_disablekeys_cb, (gpointer) prefs_window);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox1, TRUE, TRUE, 0);

  label1 = gtk_label_new (_("Command divide char:"));
  gtk_widget_show (label1);
  gtk_box_pack_start (GTK_BOX (hbox1), label1, FALSE, FALSE, 10);

  entry_divider = gtk_entry_new_with_max_length (1);
  gtk_entry_set_text (GTK_ENTRY (entry_divider), prefs.CommDev);
  gtk_widget_show (entry_divider);
  gtk_box_pack_start (GTK_BOX (hbox1), entry_divider, FALSE, TRUE, 0);
  gtk_widget_set_usize (entry_divider, 15, -2);
  gtk_signal_connect(GTK_OBJECT(entry_divider), "changed", GTK_SIGNAL_FUNC(prefs_entry_divide_cb), (gpointer) prefs_window);

  label1 = gtk_label_new(_("Command history:"));
  gtk_widget_show(label1);
  gtk_box_pack_start(GTK_BOX(hbox1), label1, FALSE, FALSE, 10);

  entry_history = gtk_entry_new();
  g_snprintf(history, 10, "%d", prefs.History);
  gtk_entry_set_text(GTK_ENTRY(entry_history), history);
  gtk_widget_show(entry_history);
  gtk_box_pack_start(GTK_BOX(hbox1), entry_history, FALSE, TRUE, 0);
  gtk_widget_set_usize(entry_history, 35, -2);
  gtk_signal_connect(GTK_OBJECT(entry_history), "changed", GTK_SIGNAL_FUNC(prefs_entry_history_cb), (gpointer) prefs_window);

	/* Terminal type entry box and label */

	hbox_term = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox_term);

	event_box_term = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER(event_box_term), hbox_term);
	gtk_widget_show (event_box_term);
	gtk_box_pack_start (GTK_BOX(vbox2), event_box_term, TRUE, TRUE, 5);

	label_term = gtk_label_new (_("Terminal type:"));
	gtk_widget_show (label_term);
	gtk_box_pack_start (GTK_BOX(hbox_term), label_term, FALSE, FALSE, 10);

	entry_term = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY(entry_term), prefs.TerminalType);
	gtk_widget_show (entry_term);
	gtk_box_pack_start (GTK_BOX(hbox_term), entry_term, TRUE, TRUE, 5);

	gtk_tooltips_set_tip(tooltip, event_box_term,
		_("GNOME-Mud will attempt to transmit a terminal type "
		  "(like ANSI or VT100) if the MUD requests one. This "
		  "option sets the terminal type that will be sent."), NULL);

	gtk_signal_connect (GTK_OBJECT(entry_term), "changed", GTK_SIGNAL_FUNC(prefs_entry_terminal_cb), (gpointer) prefs_window);

	hbox1 = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox1);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox1, TRUE, TRUE, 5);
	
	label1 = gtk_label_new(_("MudList file:"));
	gtk_widget_show(label1);
	gtk_box_pack_start(GTK_BOX(hbox1), label1, FALSE, FALSE, 10);
 
	entry_mudlistfile = gnome_file_entry_new(NULL, _("Select a MudList File"));
	gtk_entry_set_text(GTK_ENTRY(gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(entry_mudlistfile))), prefs.MudListFile);
	gtk_widget_show(entry_mudlistfile);
	gtk_box_pack_start(GTK_BOX(hbox1), entry_mudlistfile, FALSE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT(gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(entry_mudlistfile))),
					   "changed",
					   GTK_SIGNAL_FUNC(prefs_entry_mudlistfile_cb), (gpointer) prefs_window);

  label2 = gtk_label_new (_("Functionality"));
  gtk_widget_show (label2); 

  gnome_property_box_append_page(GNOME_PROPERTY_BOX(prefs_window), vbox2, label2);
  /*
  ** END PAGE TWO
  */

  vbox2 = prefs_color_frame(prefs_window);
  gtk_widget_show(vbox2);
  
  label2 = gtk_label_new (_("Color and Fonts"));
  gtk_widget_show(label2);

  gnome_property_box_append_page(GNOME_PROPERTY_BOX(prefs_window), vbox2, label2);
  
  gtk_widget_show(prefs_window);
}
