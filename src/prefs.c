/* AMCL - A simple Mud CLient
 * Copyright (C) 1998-1999 Robin Ericsson <lobbin@localhost.nu>
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

#include <gtk/gtk.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>

#include "amcl.h"

static char const rcsid[] =
    "$Id$";


SYSTEM_DATA prefs;
GtkWidget   *prefs_window;
GtkWidget   *prefs_button_save;
GtkWidget   *entry_fontname;

int check_amcl_dir (gchar *dirname)
{
    struct stat file_stat;
    int return_val = 0;
    gchar buf[256];

    if ( stat (dirname, &file_stat) == 0) /* can we stat ~/.amcl? */
    {
        if ( !(S_ISDIR(file_stat.st_mode))) /* if it's not a directory */
        {
            g_snprintf (buf, 256, "%s already exists and is not a directory!", dirname);
            popup_window (buf);
            return_val = -1;
        }
    }
    else /* it must not exist */
    {
        g_snprintf (buf, 256, "%s does not exist, Creating it as a directory.", dirname);
        popup_window (buf);

        if ((mkdir (dirname, 0777)) == 0) /* this isn't dangerous, umask modifies it */
        {
            g_snprintf (buf, 256, "%s created.", dirname);
            popup_window (buf);
        }
        else
        {
            g_snprintf (buf, 256, "%s NOT created: %s", dirname, strerror (errno));
            popup_window (buf);
            return_val = errno;
        }
    }

    return (return_val);
}

void load_prefs ( )
{
    FILE *fp;
    gchar filename[255] = "";
    gchar line[255];

    prefs.EchoText = prefs.KeepText = TRUE;
    prefs.AutoSave = FALSE;
    prefs.FontName = g_strdup ("fixed");
    
    g_snprintf (filename, 255, "%s%s", uid_info->pw_dir, "/.amcl");
    if (check_amcl_dir (filename) != 0)
        return;

    g_snprintf (filename, 254, "%s%s", uid_info->pw_dir, "/.amcl/preferences");
    
    fp = fopen (filename, "r");

    if ( fp == NULL )
        return;

    while ( fgets (line, 80, fp) != NULL )
    {
        gchar pref[25];
        gchar value[250];

        sscanf (line, "%s %[^\n]", pref, value);

        if ( !strcmp (pref, "EchoText") )
        {
            if ( !strcmp (value, "On") )
                prefs.EchoText = TRUE;
            else
                prefs.EchoText = FALSE;
        }

        if ( !strcmp (pref, "KeepText") )
        {
            if ( !strcmp (value, "On") )
                prefs.KeepText = TRUE;
            else
                prefs.KeepText = FALSE;
        }

        if ( !strcmp (pref, "AutoSave") )
        {
            if ( !strcmp (value, "On") )
                prefs.AutoSave = TRUE;
        }

        if ( !strcmp (pref, "FontName") )
        {
            g_free (prefs.FontName);
            prefs.FontName = g_strdup (value);
        }

        if ( !strcmp (pref, "Freeze") )
        {
            if ( !strcmp (value, "On") )
                prefs.Freeze = TRUE;
        }
    }

    if ( !prefs.FontName )
        prefs.FontName = g_strdup ("fixed");

    fclose (fp);
}

void save_prefs (GtkWidget *button, gpointer data)
{
    gchar filename[256] = "";
    FILE *fp;
    gchar buf[256];

    g_snprintf (filename, 255, "%s%s", uid_info->pw_dir, "/.amcl");
    if (check_amcl_dir (filename) != 0)
        return;

    g_snprintf (filename, 255, "%s%s", uid_info->pw_dir, "/.amcl/preferences");

    fp = fopen (filename, "w");

    if ( fp == NULL )
    {
        sprintf (buf, "You must create the directory %s/.amcl before you can save.", uid_info->pw_dir);
        popup_window (buf);
        return;
    }

    if ( prefs.EchoText )
        fprintf (fp, "EchoText On\n");

    if ( prefs.KeepText )
        fprintf (fp, "KeepText On\n");

    if ( prefs.AutoSave )
        fprintf (fp, "AutoSave On\n");

    if ( prefs.Freeze )
        fprintf (fp, "Freeze On\n");

    if ( strlen (prefs.FontName) > 0 )
        fprintf (fp, "FontName %s\n", prefs.FontName);
    
    fclose (fp);
}

void check_text_toggle (GtkWidget *widget, GtkWidget *button)
{
    if ( GTK_TOGGLE_BUTTON (button)->active )
        prefs.KeepText = TRUE;
    else
        prefs.KeepText = FALSE;
}

void prefs_autosave_cb (GtkWidget *widget, GtkWidget *check_autosave)
{
    if ( GTK_TOGGLE_BUTTON (check_autosave)->active )
        prefs.AutoSave = TRUE;
    else
        prefs.AutoSave = FALSE;
}

void prefs_freeze_cb (GtkWidget *widget, GtkWidget *check_freeze)
{
    if ( GTK_TOGGLE_BUTTON (check_freeze)->active )
        prefs.Freeze = TRUE;
    else
        prefs.Freeze = FALSE;
}

void check_callback (GtkWidget *widget, GtkWidget *check_button)
{
    if ( GTK_TOGGLE_BUTTON (check_button)->active )
        prefs.EchoText = TRUE;
    else
        prefs.EchoText = FALSE;
}

void prefs_font_selected (GtkWidget *button, GtkFontSelectionDialog *fs)
{
    gchar *temp;

    temp = gtk_font_selection_get_font_name (GTK_FONT_SELECTION(fs->fontsel));

    gtk_entry_set_text (GTK_ENTRY (entry_fontname), temp);
    g_free (prefs.FontName);
    prefs.FontName = g_strdup (temp);

    font_normal = gdk_font_load (prefs.FontName);

    g_free (temp);
}

void prefs_select_font_callback (GtkWidget *button, gpointer data)
{
    GtkWidget *fontw;

    fontw = gtk_font_selection_dialog_new ("Font Selection...");
    gtk_font_selection_dialog_set_preview_text (GTK_FONT_SELECTION_DIALOG (fontw),
                                                "How about this font?");

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

void prefs_close_window ()
{
    if ( prefs.AutoSave )
        save_prefs(NULL, NULL);

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
    GtkWidget *button_close;
    GtkWidget *button_select_font;
    GtkWidget *separator;
    GtkTooltips *tooltip;

    gtk_widget_set_sensitive (menu_option_prefs, FALSE);
                              
    prefs_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (prefs_window), "Amcl - Preferences");
    gtk_signal_connect_object (GTK_OBJECT (prefs_window), "destroy",
                               GTK_SIGNAL_FUNC(prefs_close_window), NULL );

    vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_border_width (GTK_CONTAINER (vbox), 5);
    gtk_container_add (GTK_CONTAINER (prefs_window), vbox);
    gtk_widget_show (vbox);

    tooltip = gtk_tooltips_new ();

    check_button = gtk_check_button_new_with_label ("Echo the text sent?");
    gtk_box_pack_start (GTK_BOX (vbox), check_button, FALSE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (check_button), "toggled",
                        GTK_SIGNAL_FUNC (check_callback), check_button);
    gtk_tooltips_set_tip (tooltip, check_button,
                          "With this toggled on, all the text you type and "
                          "enter will be echoed on the connection so you can "
                          "control what you are sending."
                          "\nSome people think this is annoying, and therefor this "
                          "is an options.",
                          NULL);
    GTK_WIDGET_UNSET_FLAGS (check_button, GTK_CAN_FOCUS);
    gtk_widget_show (check_button);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (check_button), prefs.EchoText);

    check_text = gtk_check_button_new_with_label ("Keep the text entered?");
    gtk_box_pack_start (GTK_BOX (vbox), check_text, FALSE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (check_text), "toggled",
                        GTK_SIGNAL_FUNC (check_text_toggle),
                        check_text);
    gtk_tooltips_set_tip (tooltip, check_text,
                          "With this toggled on, the text you have entered and sent "
                          "to the connection, will be left in the entry box but "
                          "seleceted."
                          "\nTurn this off to remove the text after it has been sent.",
                          NULL);
    GTK_WIDGET_UNSET_FLAGS (check_text, GTK_CAN_FOCUS);
    gtk_widget_show (check_text);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (check_text), prefs.KeepText);

    check_autosave = gtk_check_button_new_with_label ("AutoSave?");
    gtk_box_pack_start (GTK_BOX (vbox), check_autosave, FALSE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (check_autosave), "toggled",
                        GTK_SIGNAL_FUNC (prefs_autosave_cb), check_autosave);
    gtk_tooltips_set_tip (tooltip, check_autosave,
                          "With this toggled on, Preferences, Connection Wizard and "
                          "Alias will autosave."
                         , NULL);
    GTK_WIDGET_UNSET_FLAGS (check_autosave, GTK_CAN_FOCUS);
    gtk_widget_show (check_autosave);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (check_autosave), prefs.AutoSave);
    
    check_freeze = gtk_check_button_new_with_label ("Freeze/Thaw");
    gtk_box_pack_start (GTK_BOX (vbox), check_freeze, FALSE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (check_freeze), "toggled",
                        GTK_SIGNAL_FUNC (prefs_freeze_cb), check_freeze);
    gtk_tooltips_set_tip (tooltip, check_freeze,
                          "Using this, text will draw faster but it will not "
                          "draw every char, only whole strings. This will probably "
                          "do a lot faster if you are playing mu* with a lot of "
                          "different colours."
                          , NULL);
    GTK_WIDGET_UNSET_FLAGS (check_freeze, GTK_CAN_FOCUS);
    gtk_widget_show (check_freeze);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (check_freeze), prefs.Freeze);

    hbox_font = gtk_hbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (vbox), hbox_font);
    gtk_widget_show (hbox_font);

    entry_fontname = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (entry_fontname), prefs.FontName);
    gtk_box_pack_start (GTK_BOX (hbox_font), entry_fontname, FALSE, TRUE, 10);
    gtk_tooltips_set_tip (tooltip, entry_fontname,
                          "This is the font that AMCL is using to print the text from "
                          "the mud. If the font is unavailable when you start AMCL, "
                          "AMCL will use the default font \"fixed.\"",
                          NULL);
    gtk_widget_show (entry_fontname);
    
    button_select_font = gtk_button_new_with_label ( " select font " );

    gtk_signal_connect_object (GTK_OBJECT (button_select_font), "clicked",
                               GTK_SIGNAL_FUNC (prefs_select_font_callback),
                               NULL);
    gtk_tooltips_set_tip (tooltip, button_select_font,
                          "Use this button to start the Font selector and chose "
                          "for yourself what font you will use.",
                          NULL);

    gtk_box_pack_start (GTK_BOX (hbox_font), button_select_font, FALSE, TRUE, 10);
    gtk_widget_show (button_select_font);

    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, TRUE, 5);
    gtk_widget_show (separator);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    prefs_button_save  = gtk_button_new_with_label ( " save ");
    button_close = gtk_button_new_with_label ( " close ");
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
