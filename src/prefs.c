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
#include <libintl.h>
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

SYSTEM_DATA prefs;
SYSTEM_DATA pre_prefs;

void load_prefs ( void )
{
	struct stat file_stat;
	gchar dirname[256], buf[256];

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
	prefs.EchoText = gnome_config_get_bool  ("/gnome-mud/Preferences/EchoText=true");
	prefs.KeepText = gnome_config_get_bool  ("/gnome-mud/Preferences/KeepText=false");
	prefs.Freeze   = gnome_config_get_bool  ("/gnome-mud/Preferences/Freeze=false");
	prefs.CommDev  = gnome_config_get_string("/gnome-mud/Preferences/CommDev=;");
	prefs.FontName = gnome_config_get_string("/gnome-mud/Preferences/FontName=fixed");
	prefs.History  = gnome_config_get_int   ("/gnome-mud/Preferences/History=10");

	/*
	 * Command history
	 */
	{
		gint  nr, i;
		gchar **cmd_history;
		gnome_config_get_vector("/gnome-mud/Data/CommandHistory", &nr, &cmd_history);

		EntryHistory = g_list_append(EntryHistory, "");

		for (i = 0; i < nr; i++)
		{
			EntryHistory = g_list_append(EntryHistory, (gpointer) cmd_history[i]);
		}
	}
	
	/*
	 * Load font
	 */
	font_normal = gdk_font_load(prefs.FontName);
}

void save_prefs ( void )
{
	gnome_config_set_bool  ("/gnome-mud/Preferences/EchoText", prefs.EchoText);
	gnome_config_set_bool  ("/gnome-mud/Preferences/KeepText", prefs.KeepText);
	gnome_config_set_bool  ("/gnome-mud/Preferences/AutoSave", prefs.AutoSave);
	gnome_config_set_bool  ("/gnome-mud/Preferences/Freeze",   prefs.Freeze);
	gnome_config_set_string("/gnome-mud/Preferences/CommDev",  prefs.CommDev);
	gnome_config_set_string("/gnome-mud/Preferences/FontName", prefs.FontName);
	gnome_config_set_int   ("/gnome-mud/Preferences/History",  prefs.History);
	
	gnome_config_sync();
}

static void copy_preferences(SYSTEM_DATA *target, SYSTEM_DATA *prefs)
{
	target->EchoText = prefs->EchoText;
	target->KeepText = prefs->KeepText;
	target->AutoSave = prefs->AutoSave;
	target->Freeze   = prefs->Freeze;             g_free(target->FontName);
	target->FontName = g_strdup(prefs->FontName); g_free(target->CommDev);
	target->CommDev  = g_strdup(prefs->CommDev);
	target->History  = prefs->History;
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

static void prefs_entry_history_cb(GtkWidget *widget, GnomePropertyBox *box)
{
  gchar *s;

  s = gtk_entry_get_text(GTK_ENTRY(widget));
  if (s) {
    pre_prefs.History = atoi(s);
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
  if (font != NULL) {
    g_free(pre_prefs.FontName);
    pre_prefs.FontName = g_strdup(font);
    
    gnome_property_box_changed((GnomePropertyBox *) data);
  }
}

static void prefs_apply_cb(GnomePropertyBox *propertybox, gint page, gpointer data)
{
  if (page == -1) {
    copy_preferences(&prefs, &pre_prefs);
    
    font_normal = gdk_font_load(prefs.FontName);

    save_prefs();
  }
}

void window_prefs (GtkWidget *widget, gpointer data)
{
  static GtkWidget *prefs_window;
  
  GtkWidget *vbox1, *vbox2;
  GtkWidget *hbox1;
  GtkWidget *checkbutton_echo, *checkbutton_keep, *checkbutton_freeze;
  GtkWidget *label1, *label2;
  GtkWidget *fontpicker;
  GtkWidget *entry_divider, *entry_history;
  gchar      history[10];

  GtkTooltips *tooltip;

  if (prefs_window != NULL) {
    gdk_window_raise(prefs_window->window);
    gdk_window_show(prefs_window->window);
    return;
  }

  copy_preferences(&pre_prefs, &prefs);
  
  prefs_window = gnome_property_box_new();
  gtk_window_set_policy(GTK_WINDOW(prefs_window), FALSE, FALSE, FALSE);
  gtk_signal_connect(GTK_OBJECT(prefs_window), "destroy",
		     GTK_SIGNAL_FUNC(gtk_widget_destroyed), &prefs_window);
  gtk_signal_connect(GTK_OBJECT(prefs_window), "apply",
		     GTK_SIGNAL_FUNC(prefs_apply_cb), NULL);

  tooltip = gtk_tooltips_new();

  /*
  ** BEGIN PAGE ONE
  */
  vbox1 = gtk_vbox_new (TRUE, 0);
  gtk_widget_show (vbox1);
  
  checkbutton_echo = gtk_check_button_new_with_label (_("Echo the text sent?"));
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (checkbutton_echo), prefs.EchoText);
  gtk_widget_show (checkbutton_echo);
  gtk_box_pack_start (GTK_BOX (vbox1), checkbutton_echo, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltip, checkbutton_echo,
			_("With this toggled on, all the text you type and "
			  "enter will be echoed on the connection so you can "
			  "control what you are sending."),
			NULL);
  gtk_signal_connect(GTK_OBJECT(checkbutton_echo), "toggled",
		     prefs_checkbox_echo_cb, (gpointer) prefs_window);

  checkbutton_keep = gtk_check_button_new_with_label (_("Keep the text entered?"));
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (checkbutton_keep), prefs.KeepText);
  gtk_widget_show (checkbutton_keep);
  gtk_box_pack_start (GTK_BOX (vbox1), checkbutton_keep, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltip, checkbutton_keep,
			_("With this toggled on, the text you have entered "
			  "and sent to the connection, will be left in the "
			  "entry box but selected. Turn this off to remove "
			  "the text after it has been sent."),
			NULL);
  gtk_signal_connect(GTK_OBJECT(checkbutton_keep), "toggled",
		     prefs_checkbox_keep_cb, (gpointer) prefs_window);
  
  fontpicker = gnome_font_picker_new();
  gtk_widget_show (fontpicker);

  if (!g_strcasecmp(prefs.FontName, "fixed"))
    gnome_font_picker_set_font_name(GNOME_FONT_PICKER(fontpicker), "-misc-fixed-medium-r-semicondensed-*-*-120-*-*-c-*-iso8859-1");
  else
    gnome_font_picker_set_font_name(GNOME_FONT_PICKER(fontpicker), prefs.FontName);

  gnome_font_picker_set_mode(GNOME_FONT_PICKER(fontpicker), GNOME_FONT_PICKER_MODE_FONT_INFO);
  gnome_font_picker_fi_set_show_size(GNOME_FONT_PICKER (fontpicker), FALSE);
  gnome_font_picker_fi_set_use_font_in_label(GNOME_FONT_PICKER (fontpicker), TRUE, 12);  
  gtk_box_pack_start (GTK_BOX (vbox1), fontpicker, FALSE, FALSE, 0);  
  gtk_tooltips_set_tip (tooltip, fontpicker,
			_("Use this button to open the font selector to "
			  "choose what font you will use."),
			NULL);
  gtk_signal_connect (GTK_OBJECT(fontpicker), "font-set", prefs_select_font_cb, (gpointer) prefs_window);

  label1 = gtk_label_new (_("Appearance"));
  gtk_widget_show (label1); 
  
  gnome_property_box_append_page(GNOME_PROPERTY_BOX(prefs_window),
				 vbox1, label1);
  /*
  ** END PAGE ONE
  */


  /*
  ** BEGIN PAGE TWO
  */
  vbox2 = gtk_vbox_new (TRUE, 0);
  gtk_widget_show (vbox2);

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
  gtk_signal_connect(GTK_OBJECT(entry_divider), "changed",
		     GTK_SIGNAL_FUNC(prefs_entry_divide_cb), (gpointer) prefs_window);

  label1 = gtk_label_new(_("Command history:"));
  gtk_widget_show(label1);
  gtk_box_pack_start(GTK_BOX(hbox1), label1, FALSE, FALSE, 10);

  entry_history = gtk_entry_new();
  g_snprintf(history, 10, "%d", prefs.History);
  gtk_entry_set_text(GTK_ENTRY(entry_history), history);
  gtk_widget_show(entry_history);
  gtk_box_pack_start(GTK_BOX(hbox1), entry_history, FALSE, TRUE, 0);
  gtk_widget_set_usize(entry_history, 35, -2);
  gtk_signal_connect(GTK_OBJECT(entry_history), "changed",
		     GTK_SIGNAL_FUNC(prefs_entry_history_cb), (gpointer) prefs_window);

  label2 = gtk_label_new (_("Functionality"));
  gtk_widget_show (label2); 

  gnome_property_box_append_page(GNOME_PROPERTY_BOX(prefs_window), vbox2, label2);
  /*
  ** END PAGE TWO
  */

  gtk_widget_show(prefs_window);
}
