/* AMCL - A simple Mud CLient
 * Copyright (C) 1998-2000 Robin Ericsson <lobbin@localhost.nu>
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

#include "config.h"

#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <libintl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gtk/gtk.h>

#include "amcl.h"

#define _(string) gettext(string)

static char const rcsid[] =
    "$Id$";

/* Local functions */
static void	check_callback (GtkWidget *, GtkWidget *);
static void	check_text_toggle (GtkWidget *, GtkWidget *);
static void	prefs_autosave_cb (GtkWidget *, GtkWidget *);
static void	prefs_close_window (void);
static void	prefs_divide_cb (GtkWidget *, GtkWidget *);
static void	prefs_font_selected (GtkWidget *, GtkFontSelectionDialog *);
static void	prefs_freeze_cb (GtkWidget *, GtkWidget *);
static void	prefs_select_font_callback (GtkWidget *, gpointer);

extern GdkFont  *font_normal;
extern GtkWidget *menu_option_prefs;

SYSTEM_DATA prefs;
GtkWidget   *prefs_window;
GtkWidget   *prefs_button_save;
GtkWidget   *entry_fontname;

FILE *open_file (gchar *filename, gchar *mode)
{
  struct stat file_stat;
  gchar buf[256];
  gchar dirname[256];
  FILE *fd;

  /* check for ~/.amcl dir ... */
  g_snprintf (dirname, 255, "%s/.amcl", g_get_home_dir());
  if ( stat (dirname, &file_stat) == 0) /* can we stat ~/.amcl? */
  {
    if ( !(S_ISDIR(file_stat.st_mode))) /* if it's not a directory */
    {
	g_snprintf (buf, 255, _("%s already exists and is not a directory!"), dirname);
	popup_window (buf);
	return NULL;
    }
  } 
  else  /* it must not exist */
    if ((mkdir (dirname, 0777)) == 0) /* this isn't dangerous, umask modifies it */
    {
    	/*popup_window (buf);*/
     } else {
      g_snprintf (buf, 255, _("%s does not exist and can NOT be created: %s"), dirname, strerror(errno));
      popup_window (buf);
      return NULL;
    }

  if (!filename || filename == "") return NULL;

  g_snprintf (buf, 255, "%s/%s", dirname, filename);
  if (!(fd = fopen (buf, mode)))
  {
	if (mode == "w")
	{
	    g_snprintf (dirname, 255, _("%s can NOT be opened in write mode."), buf);
	    popup_window (dirname);
	}
	else
		if (/*mode == "r" &&*/ stat(buf, &file_stat) == 0)
		{
		    g_snprintf (dirname, 255, _("%s exists and can NOT be opened in read mode."), buf);
		    popup_window (dirname);
		}
  }
  return fd;
}

void load_prefs ( void )
{
    FILE *fp;
    gchar line[255];
    gchar pref[25];
    gchar value[250];
    gint valint;

    prefs.EchoText = TRUE;
    prefs.KeepText = FALSE;
    prefs.AutoSave = FALSE;
    prefs.CommDev  = g_strdup (";");
    prefs.FontName = g_strdup ("fixed");
    
    if (!(fp = open_file ("preferences", "r"))) return;

    while ( fgets (line, 80, fp) != NULL )
    {
        if (sscanf (line, "%[^=]=%s", pref, value) != 2) continue;

	if (!strncasecmp(pref, "echotext", 8)) {
	  valint = atoi(value);

	  if (valint == 1)
	    prefs.EchoText = TRUE;
	  else
	    prefs.EchoText = FALSE;

	} else if (!strncasecmp(pref, "keeptext", 8)) {
	  valint = atoi(value);
	  
	  if (valint == 1)
	    prefs.KeepText = TRUE;
	  else
	    prefs.KeepText = FALSE;

	} else if (!strncasecmp(pref, "autosave", 8)) {
	  valint = atoi(value);

	  if (valint == 1) 
	    prefs.AutoSave = TRUE;
	  else
	    prefs.AutoSave = FALSE;

	} else if (!strncasecmp(pref, "freeze", 6)) {
	  valint = atoi(value);

	  if (valint == 1) 
	    prefs.Freeze = TRUE;
	  else
	    prefs.Freeze = FALSE;

	} else if (!strncasecmp(pref, "commdev", 7)) {
	  g_free(prefs.CommDev);
	  
	  prefs.CommDev = g_strdup(value);

	} else if (!strncasecmp(pref, "fontname", 8)) {
	  g_free(prefs.FontName);
	  
	  prefs.FontName = g_strdup(value);

	  font_normal = gdk_font_load(prefs.FontName);
	} else /* Unknown pref file entry! */ {
	  gchar buf[256];

	  g_snprintf(buf, 255, "Unknown preference file entry:  %s=%s.", pref, value);

	  popup_window(buf);
	}
    }

    if (fp) fclose (fp);
}

void save_prefs ( void )
{
    FILE *fp;

    if (!(fp = open_file ("preferences", "w"))) return;

    if ( prefs.EchoText ) {
      fprintf(fp, "EchoText=1\n");
    } else {
      fprintf(fp, "EchoText=0\n");
    }

    if ( prefs.KeepText ) {
      fprintf(fp, "KeepText=1\n");
    } else {
      fprintf(fp, "KeepText=0\n");
    }

    if ( prefs.AutoSave ) {
      fprintf(fp, "AutoSave=1\n");
    } else {
      fprintf(fp, "AutoSave=0\n");
    }

    if ( prefs.Freeze ) {
      fprintf(fp, "Freeze=1\n");
    } else {
      fprintf(fp, "Freeze=0\n");
    }

    fprintf(fp, "CommDev=%c\n", prefs.CommDev[0]);

    if ( strlen (prefs.FontName) > 0 )
        fprintf (fp, "FontName=%s\n", prefs.FontName);
    
    if (fp) fclose (fp);
}

static void check_text_toggle (GtkWidget *widget, GtkWidget *button)
{
    if ( GTK_TOGGLE_BUTTON (button)->active )
        prefs.KeepText = TRUE;
    else
        prefs.KeepText = FALSE;
}

static void prefs_autosave_cb (GtkWidget *widget, GtkWidget *check_autosave)
{
    if ( GTK_TOGGLE_BUTTON (check_autosave)->active )
        prefs.AutoSave = TRUE;
    else
        prefs.AutoSave = FALSE;
}

static void prefs_freeze_cb (GtkWidget *widget, GtkWidget *check_freeze)
{
    if ( GTK_TOGGLE_BUTTON (check_freeze)->active )
        prefs.Freeze = TRUE;
    else
        prefs.Freeze = FALSE;
}

static void prefs_divide_cb (GtkWidget *widget, GtkWidget *entry_divide)
{
  gchar *s;
  s = gtk_entry_get_text(GTK_ENTRY(entry_divide));
  if (s) prefs.CommDev[0] = s[0];  
}

static void check_callback (GtkWidget *widget, GtkWidget *check_button)
{
    if ( GTK_TOGGLE_BUTTON (check_button)->active )
        prefs.EchoText = TRUE;
    else
        prefs.EchoText = FALSE;
}

static void prefs_font_selected (GtkWidget *button, GtkFontSelectionDialog *fs)
{
    gchar *temp;

    temp = gtk_font_selection_get_font_name (GTK_FONT_SELECTION(fs->fontsel));

    if (temp != NULL) {
      gtk_entry_set_text (GTK_ENTRY (entry_fontname), temp);
      g_free (prefs.FontName);
      prefs.FontName = g_strdup (temp);

      font_normal = gdk_font_load (prefs.FontName);
    }

    g_free (temp);
}

static void prefs_select_font_callback (GtkWidget *button, gpointer data)
{
    GtkWidget *fontw;

    fontw = gtk_font_selection_dialog_new ("Font Selection...");
    gtk_font_selection_dialog_set_preview_text (GTK_FONT_SELECTION_DIALOG (fontw),
                                                "How about this font?");
    gtk_font_selection_dialog_set_font_name (GTK_FONT_SELECTION_DIALOG (fontw),"fixed");

    gtk_signal_connect (GTK_OBJECT (GTK_FONT_SELECTION_DIALOG (fontw)->ok_button),
                               "clicked", (GtkSignalFunc) prefs_font_selected,
                               GTK_OBJECT (fontw));
    gtk_signal_connect_object (GTK_OBJECT (GTK_FONT_SELECTION_DIALOG (fontw)->ok_button),
                               "clicked", (GtkSignalFunc) gtk_widget_destroy,
                               GTK_OBJECT (fontw));
    gtk_signal_connect_object (GTK_OBJECT (GTK_FONT_SELECTION_DIALOG (fontw)->cancel_button),
                               "clicked", (GtkSignalFunc) gtk_widget_destroy,
                               GTK_OBJECT (fontw));
    gtk_widget_show (fontw);
}

static void prefs_close_window (void)
{
    if ( prefs.AutoSave )
        save_prefs();

    gtk_widget_set_sensitive (menu_option_prefs, TRUE);
    gtk_widget_destroy (prefs_window);
}

void window_prefs (GtkWidget *widget, gpointer data)
{
    GtkWidget *check_text;
    GtkWidget *check_autosave;
    GtkWidget *check_button;
    GtkWidget *check_freeze;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *hbox_font;
    GtkWidget *hbox_divide;
    GtkWidget *entry_divide;
    GtkWidget *label;
    GtkWidget *button_close;
    GtkWidget *button_select_font;
    GtkWidget *separator;
    GtkTooltips *tooltip;

    gtk_widget_set_sensitive (menu_option_prefs, FALSE);
                              
    prefs_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (prefs_window), _("AMCL - Preferences"));
    gtk_signal_connect_object (GTK_OBJECT (prefs_window), "destroy",
                               GTK_SIGNAL_FUNC(prefs_close_window), NULL );

    vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (vbox), 5);
    gtk_container_add (GTK_CONTAINER (prefs_window), vbox);
    gtk_widget_show (vbox);

    tooltip = gtk_tooltips_new ();

    check_button = gtk_check_button_new_with_label (_("Echo the text sent?"));
    gtk_box_pack_start (GTK_BOX (vbox), check_button, FALSE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (check_button), "toggled",
                        GTK_SIGNAL_FUNC (check_callback), check_button);
    gtk_tooltips_set_tip (tooltip, check_button,
			_("With this toggled on, all the text you type and "
			  "enter will be echoed on the connection so you can "
			  "control what you are sending."),
			  NULL);
    GTK_WIDGET_UNSET_FLAGS (check_button, GTK_CAN_FOCUS);
    gtk_widget_show (check_button);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (check_button), prefs.EchoText);

    check_text = gtk_check_button_new_with_label (_("Keep the text entered?"));
    gtk_box_pack_start (GTK_BOX (vbox), check_text, FALSE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (check_text), "toggled",
                        GTK_SIGNAL_FUNC (check_text_toggle),
                        check_text);
    gtk_tooltips_set_tip (tooltip, check_text,
			_("With this toggled on, the text you have entered "
			  "and sent to the connection, will be left in the "
			  "entry box but seleceted. Turn this off to remove "
			  "the text after it has been sent."),
			  NULL);
    GTK_WIDGET_UNSET_FLAGS (check_text, GTK_CAN_FOCUS);
    gtk_widget_show (check_text);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (check_text), prefs.KeepText);

    check_autosave = gtk_check_button_new_with_label (_("AutoSave?"));
    gtk_box_pack_start (GTK_BOX (vbox), check_autosave, FALSE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (check_autosave), "toggled",
                        GTK_SIGNAL_FUNC (prefs_autosave_cb), check_autosave);
    gtk_tooltips_set_tip (tooltip, check_autosave,
			_("With this toggled on, Preferences, Connection "
			  "Wizard and Aliases will be saved automatically."),
			  NULL);
    GTK_WIDGET_UNSET_FLAGS (check_autosave, GTK_CAN_FOCUS);
    gtk_widget_show (check_autosave);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (check_autosave), prefs.AutoSave);
    
    check_freeze = gtk_check_button_new_with_label (_("Freeze/Thaw"));
    gtk_box_pack_start (GTK_BOX (vbox), check_freeze, FALSE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (check_freeze), "toggled",
                        GTK_SIGNAL_FUNC (prefs_freeze_cb), check_freeze);
    gtk_tooltips_set_tip (tooltip, check_freeze,
			_("Using this, text will draw faster but it will not "
			  "draw every character, only whole strings. This "
			  "will probably make displays a lot faster if you "
			  "are playing MUDs with a lot of different colours."),
			  NULL);
    GTK_WIDGET_UNSET_FLAGS (check_freeze, GTK_CAN_FOCUS);
    gtk_widget_show (check_freeze);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (check_freeze), prefs.Freeze);

    hbox_divide = gtk_hbox_new (TRUE, 0);
    gtk_container_add (GTK_CONTAINER (vbox), hbox_divide);
    gtk_widget_show (hbox_divide);
    
    label = gtk_label_new (_("   Command divide char"));
    gtk_box_pack_start (GTK_BOX (hbox_divide), label, TRUE, FALSE, 0);
    gtk_widget_show (label);
    
    entry_divide = gtk_entry_new_with_max_length (1);
    gtk_entry_set_text (GTK_ENTRY (entry_divide), prefs.CommDev);
    gtk_box_pack_start (GTK_BOX (hbox_divide), entry_divide, TRUE, FALSE, 0);
    gtk_widget_set_usize(GTK_WIDGET(entry_divide),30,23);
    gtk_widget_show (entry_divide);
    gtk_signal_connect (GTK_OBJECT (entry_divide), "changed",
                        GTK_SIGNAL_FUNC (prefs_divide_cb), entry_divide);
    
    hbox_font = gtk_hbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (vbox), hbox_font);
    gtk_widget_show (hbox_font);

    entry_fontname = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (entry_fontname), prefs.FontName);
    gtk_box_pack_start (GTK_BOX (hbox_font), entry_fontname, FALSE, TRUE, 10);
    gtk_tooltips_set_tip (tooltip, entry_fontname,
			_("This is the font that AMCL is using to print the "
			  "text from the MUD. If the font is unavailable "
			  "when you start AMCL, the default font \"fixed.\" "
			  "will be used."),
			  NULL);
    gtk_widget_show (entry_fontname);
    
    button_select_font = gtk_button_new_with_label (_("Select font"));

    gtk_signal_connect_object (GTK_OBJECT (button_select_font), "clicked",
                               GTK_SIGNAL_FUNC (prefs_select_font_callback),
                               NULL);
    gtk_tooltips_set_tip (tooltip, button_select_font,
			_("Use this button to open the font selector to "
			  "choose what font you will use."),
			  NULL);

    gtk_box_pack_start (GTK_BOX (hbox_font), button_select_font, FALSE, TRUE, 10);
    gtk_widget_show (button_select_font);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, TRUE, 5);
    gtk_widget_show (separator);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    prefs_button_save  = gtk_button_new_with_label (_("Save"));
    button_close = gtk_button_new_with_label (_("Close"));
    gtk_signal_connect_object (GTK_OBJECT (prefs_button_save), "clicked",
                               GTK_SIGNAL_FUNC (save_prefs),
                               NULL);
    gtk_signal_connect_object (GTK_OBJECT (button_close), "clicked",
                               GTK_SIGNAL_FUNC (prefs_close_window),
                               NULL);
    gtk_box_pack_start (GTK_BOX (hbox), prefs_button_save,  TRUE, TRUE, 15);
    gtk_box_pack_start (GTK_BOX (hbox), button_close, TRUE, TRUE, 15);
    gtk_widget_show (prefs_button_save);
    gtk_widget_show (button_close);
    
    gtk_widget_show (prefs_window);
}
