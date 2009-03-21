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
#include <glade/glade-xml.h>

#include "gnome-mud.h"
#include "mud-log.h"
#include "mud-window.h"
#include "mud-connection-view.h"
#include "mud-line-buffer.h"
#include "utils.h"

struct _MudLogPrivate
{
    GtkWidget *window;
    GtkWidget *spin_log_next;
    GtkWidget *spin_log_prev;
    GtkWidget *check_log_next;
    GtkWidget *check_log_prev;
    GtkWidget *check_append;
    GtkWidget *check_buffer;
    GtkWidget *check_color;
    GtkWidget *entry_file;
    GtkWidget *btn_select;

    gboolean active;
    gboolean append;
    gboolean color;
    gboolean buffer;
    gboolean include_next;
    gboolean include_prev;

    gboolean done;
    gboolean finalizing;

    gint next_count;
    gint prev_count;

    gint count;

    gchar *dir;
    gchar *filename;

    FILE *logfile;

    MudLineBuffer *line_buffer;

    MudWindow *parent_window;
    MudConnectionView *parent;
};

/* Property Identifiers */
enum
{
    PROP_MUD_LOG_0,
    PROP_MUD_NAME,
    PROP_PARENT,
    PROP_WINDOW
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

/* Callbacks */
static void mud_log_next_toggled_cb(GtkToggleButton *button, MudLog *self);
static void mud_log_prev_toggled_cb(GtkToggleButton *button, MudLog *self);
static void mud_log_append_toggled_cb(GtkToggleButton *button, MudLog *self);
static void mud_log_buffer_toggled_cb(GtkToggleButton *button, MudLog *self);
static void mud_log_color_toggled_cb(GtkToggleButton *button, MudLog *self);
static void mud_log_select_clicked_cb(GtkWidget *widget, MudLog *self);
static void mud_log_next_spin_changed_cb(GtkSpinButton *button, MudLog *self);
static void mud_log_prev_spin_changed_cb(GtkSpinButton *button, MudLog *self);
static gboolean mud_log_keypress_cb(GtkWidget *widget,
                                    GdkEventKey *event,
                                    MudLog *self);
static void mud_log_line_added_cb(MudLineBuffer *buffer, MudLog *self);

/* Private Methods */
static void mud_log_write(MudLog *log, const gchar *data, gsize size);
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

    g_object_class_install_property(object_class,
            PROP_WINDOW,
            g_param_spec_object("window",
                "window",
                "Parent MudWindow",
                MUD_TYPE_WINDOW,
                G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property(object_class,
            PROP_PARENT,
            g_param_spec_object("parent",
                "parent",
                "Parent MudConnectionView",
                MUD_TYPE_CONNECTION_VIEW,
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
    log->priv->parent_window = NULL;
    log->priv->parent = NULL;
    log->priv->filename = NULL;

    log->priv->append = TRUE;
    log->priv->buffer = FALSE;
    log->priv->color = FALSE;
    log->priv->include_next = FALSE;
    log->priv->include_prev = FALSE;
    log->priv->done = FALSE;
    log->priv->finalizing = FALSE;

    log->priv->count = 0; 
    log->priv->next_count = 0;
    log->priv->prev_count = 0;
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

    /* Chain up to window constructor */
    klass = MUD_LOG_CLASS( g_type_class_peek(MUD_TYPE_LOG) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    self = MUD_LOG(obj);

    if(!self->mud_name)
    {
        g_printf("ERROR: Tried to instantiate MudLog without passing mud name.\n");
        g_error("Tried to instantiate MudLog without passing mud name.");
    }

    if(!self->priv->parent_window)
    {
        g_printf("ERROR: Tried to instantiate MudLog without passing MudWindow.\n");
        g_error("Tried to instantiate MudLog without passing MudWindow.");    
    }

    if(!self->priv->parent)
    {
        g_printf("ERROR: Tried to instantiate MudLog without passing MudConnectionView.\n");
        g_error("Tried to instantiate MudLog without passing MudConnectionView.");    
    }

    return obj;
}

static void
mud_log_finalize (GObject *object)
{
    MudLog *MLog;
    GObjectClass *parent_class;

    MLog = MUD_LOG(object);

    MLog->priv->finalizing = TRUE;

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

        case PROP_WINDOW:
            self->priv->parent_window = MUD_WINDOW(g_value_get_object(value));
            break;

        case PROP_PARENT:
            self->priv->parent = MUD_CONNECTION_VIEW(g_value_get_object(value));
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

        case PROP_WINDOW:
            g_value_take_object(value, self->priv->parent_window);
            break;

        case PROP_PARENT:
            g_value_take_object(value, self->priv->parent);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

/* Callbacks */
static void
mud_log_select_clicked_cb(GtkWidget *widget,
                          MudLog *self)
{
    gchar *filename;
    GladeXML *glade;
    GtkWidget *dialog;
    gint result;

    glade = glade_xml_new(GLADEDIR "/main.glade", "save_dialog", NULL);
    dialog = glade_xml_get_widget(glade, "save_dialog");
    g_object_unref(glade);

    g_object_set(dialog, "title", _("Save log as..."), NULL);

    if(strlen(gtk_entry_get_text(GTK_ENTRY(self->priv->entry_file))) != 0)
    {
        const gchar *text = gtk_entry_get_text(GTK_ENTRY(self->priv->entry_file));
        gchar *dir = g_path_get_dirname(text);
        gchar *file = g_path_get_basename(text);

        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                            dir);
        gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog),
                                           file);

        g_free(dir);
        g_free(file);
    }
    else
    {
        gchar *name = g_strdup_printf("%s.log", self->mud_name);

        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                            g_get_home_dir());
        gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(dialog),
                                           name);

        g_free(name);
    }

    result = gtk_dialog_run(GTK_DIALOG(dialog));

    if(result == GTK_RESPONSE_OK)
    {
        if(self->priv->filename)
        {
            g_free(self->priv->filename);
            self->priv->filename = NULL;
        }

        self->priv->filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        gtk_entry_set_text(GTK_ENTRY(self->priv->entry_file), self->priv->filename);
    }

    gtk_widget_destroy(dialog);
}

static void
mud_log_next_toggled_cb(GtkToggleButton *button,
                        MudLog *self)
{
    gboolean active;

    g_object_get(button, "active", &active, NULL);

    self->priv->include_next = active;

    if(self->priv->include_next)
    {
        self->priv->next_count = gtk_spin_button_get_value_as_int(
                GTK_SPIN_BUTTON(self->priv->spin_log_next));

        gtk_widget_set_sensitive(self->priv->spin_log_next, TRUE);
        gtk_widget_grab_focus(self->priv->spin_log_next);
    }
    else
    {
        gtk_widget_set_sensitive(self->priv->spin_log_next, FALSE);
        self->priv->next_count = 0;
    }
}

static void
mud_log_prev_toggled_cb(GtkToggleButton *button,
                        MudLog *self)
{
    gboolean active;

    g_object_get(button, "active", &active, NULL);

    self->priv->include_prev = active;

    if(self->priv->include_prev)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->priv->check_buffer),
                                     FALSE);

        self->priv->prev_count = gtk_spin_button_get_value_as_int(
                GTK_SPIN_BUTTON(self->priv->spin_log_prev));

        gtk_widget_set_sensitive(self->priv->spin_log_prev, TRUE);
        gtk_widget_grab_focus(self->priv->spin_log_prev);
    }
    else
    {
        gtk_widget_set_sensitive(self->priv->spin_log_prev, FALSE);
        self->priv->prev_count = 0;
    }
}


static void
mud_log_append_toggled_cb(GtkToggleButton *button,
                          MudLog *self)
{
    gboolean active;

    g_object_get(button, "active", &active, NULL);

    self->priv->append = active;
}

static void
mud_log_buffer_toggled_cb(GtkToggleButton *button,
                          MudLog *self)
{
    gboolean active;

    g_object_get(button, "active", &active, NULL);

    self->priv->buffer = active;

    if(self->priv->buffer)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->priv->check_log_prev),
                                     FALSE);
}

static void
mud_log_color_toggled_cb(GtkToggleButton *button,
                         MudLog *self)
{
    gboolean active;

    g_object_get(button, "active", &active, NULL);

    self->priv->color = active;
}

static gboolean
mud_log_keypress_cb(GtkWidget *widget,
                    GdkEventKey *event,
                    MudLog *self)
{
    if(self->priv->filename)
    {
        g_free(self->priv->filename);
        self->priv->filename = NULL;
    }

    self->priv->filename = g_strdup(
            gtk_entry_get_text(GTK_ENTRY(self->priv->entry_file)));

    return FALSE;
}

static void
mud_log_next_spin_changed_cb(GtkSpinButton *button,
                             MudLog *self)
{
    self->priv->next_count = gtk_spin_button_get_value_as_int(
                                GTK_SPIN_BUTTON(button));
}

static void
mud_log_prev_spin_changed_cb(GtkSpinButton *button,
                             MudLog *self)
{
    self->priv->prev_count = gtk_spin_button_get_value_as_int(
                                GTK_SPIN_BUTTON(button));
}

static void
mud_log_line_added_cb(MudLineBuffer *buffer,
                      MudLog *self)
{
    gulong length;
    const gchar *line;

    if(!self->priv->done)
    {
        g_object_get(buffer, "length", &length, NULL);

        line = mud_line_buffer_get_line(buffer, length - 1);

        if(line && strlen(line) != 0) 
            mud_log_write(self, line, strlen(line));

        if(self->priv->include_next)
        {
            self->priv->count++;

            if(self->priv->count == self->priv->next_count)
            {
                self->priv->count = 0;
                self->priv->next_count = 0;

                self->priv->done = TRUE;
            }
        }
    }
}

/* Public Methods */
void
mud_log_open(MudLog *self)
{
    GladeXML *glade;
    GtkWidget *main_window;
    gchar buf[1024];
    gchar nameBuf[1024];
    time_t t;
    gint result;

    g_return_if_fail(MUD_IS_LOG(self));

    /* start glading */
    glade = glade_xml_new(GLADEDIR "/main.glade", "log_config_window", NULL);

    self->priv->window = glade_xml_get_widget(glade, "log_config_window");
    self->priv->check_log_next = glade_xml_get_widget(glade, "inc_next_check_btn");
    self->priv->spin_log_next = glade_xml_get_widget(glade, "next_spin_btn");
    self->priv->check_log_prev = glade_xml_get_widget(glade, "inc_prev_check_btn");
    self->priv->spin_log_prev = glade_xml_get_widget(glade, "prev_spin_btn");
    self->priv->check_append = glade_xml_get_widget(glade, "append_check_btn");
    self->priv->check_buffer = glade_xml_get_widget(glade, "buffer_check_btn");
    self->priv->check_color = glade_xml_get_widget(glade, "color_check_btn");
    self->priv->btn_select = glade_xml_get_widget(glade, "select_btn");
    self->priv->entry_file = glade_xml_get_widget(glade, "file_entry");

    g_object_get(self->priv->parent_window, "window", &main_window, NULL);

    gtk_window_set_transient_for(GTK_WINDOW(self->priv->window),
                                 GTK_WINDOW(main_window));

    g_signal_connect(self->priv->spin_log_next,
                     "value-changed",
                     G_CALLBACK(mud_log_next_spin_changed_cb),
                     self);

    g_signal_connect(self->priv->spin_log_prev,
                     "value-changed",
                     G_CALLBACK(mud_log_prev_spin_changed_cb),
                     self);

    g_signal_connect(self->priv->entry_file,
                     "key-press-event",
                     G_CALLBACK(mud_log_keypress_cb),
                     self);

    g_signal_connect(self->priv->btn_select,
                     "clicked",
                     G_CALLBACK(mud_log_select_clicked_cb),
                     self);

    g_signal_connect(self->priv->check_log_next,
                     "toggled",
                     G_CALLBACK(mud_log_next_toggled_cb),
                     self);

    g_signal_connect(self->priv->check_log_prev,
                     "toggled",
                     G_CALLBACK(mud_log_prev_toggled_cb),
                     self);

    g_signal_connect(self->priv->check_append,
                     "toggled",
                     G_CALLBACK(mud_log_append_toggled_cb),
                     self);

    g_signal_connect(self->priv->check_buffer,
                     "toggled",
                     G_CALLBACK(mud_log_buffer_toggled_cb),
                     self);

    g_signal_connect(self->priv->check_color,
                     "toggled",
                     G_CALLBACK(mud_log_color_toggled_cb),
                     self);

    g_object_unref(glade);

    result = gtk_dialog_run(GTK_DIALOG(self->priv->window));
    
    if(result == GTK_RESPONSE_OK)
    {
        self->priv->logfile = fopen(self->priv->filename,
                                    (self->priv->append) ? "a" : "w");
        if (self->priv->logfile)
        {
            time(&t);
            strftime(buf, 1024,
                    _("\n*** Log starts *** %d/%m/%Y %H:%M:%S\n"),
                    localtime(&t));
            fprintf(self->priv->logfile, "%s", buf);

            if(self->priv->buffer)
            {
                VteTerminal *term;
                GtkClipboard *clipboard;
                GError *err = NULL;
                gchar *term_text;

                g_object_get(self->priv->parent, "terminal", &term, NULL);

                clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
                vte_terminal_select_all(term);
                vte_terminal_copy_primary(term);
                term_text = gtk_clipboard_wait_for_text(clipboard);
                vte_terminal_select_none(term);

                if(term_text)
                {
                    fprintf(self->priv->logfile, "%s", term_text);
                    g_free(term_text);
                }
            }

            if(self->priv->include_prev)
            {
                VteTerminal *term;
                GtkClipboard *clipboard;
                GError *err = NULL;
                gchar *term_text;
                gchar *buf_text;

                MudLineBuffer *buffer = g_object_new(MUD_TYPE_LINE_BUFFER,
                                                     "maximum-line-count", self->priv->prev_count,
                                                     NULL);

                g_object_get(self->priv->parent, "terminal", &term, NULL);

                clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
                vte_terminal_select_all(term);
                vte_terminal_copy_primary(term);
                term_text = gtk_clipboard_wait_for_text(clipboard);
                vte_terminal_select_none(term);

                if(term_text)
                {
                    mud_line_buffer_add_data(buffer, term_text, strlen(term_text));
                    g_free(term_text);

                    buf_text = mud_line_buffer_get_lines(buffer);

                    fprintf(self->priv->logfile, "%s", buf_text);

                    g_free(buf_text);
                }

                g_object_unref(buffer);
            }

            if(self->priv->include_next)
                self->priv->count = 0;

            self->priv->line_buffer = g_object_new(MUD_TYPE_LINE_BUFFER,
                                                   "maximum-line-count", 5,
                                                   NULL);

            g_signal_connect(self->priv->line_buffer,
                             "line-added",
                             G_CALLBACK(mud_log_line_added_cb),
                             self);
        }
        else
        {
            GtkWidget *main_window;

            g_object_get(self->priv->window, "window", &main_window, NULL);

            utils_error_message(main_window,
                                _("Log Error"),
                                _("Could not open \"%s\" for writing."),
                                self->priv->filename);
        }

        self->priv->active = TRUE;
    }

    gtk_widget_destroy(self->priv->window);
}

void
mud_log_close(MudLog *log)
{
    gchar buf[255];
    time_t t;

    g_return_if_fail(MUD_IS_LOG(log));

    time(&t);
    strftime(buf, 255,
            _("\n *** Log stops *** %d/%m/%Y %H:%M:%S\n"),
            localtime(&t));

    fprintf(log->priv->logfile, "%s", buf);
    fclose(log->priv->logfile);

    if(log->priv->filename)
    {
        g_free(log->priv->filename);
        log->priv->filename = NULL;
    }

    g_object_unref(log->priv->line_buffer);
    log->priv->active = FALSE;

    if(!log->priv->finalizing)
    {
        g_object_set(log->priv->parent, "logging", FALSE, NULL);
        mud_window_update_logging_ui(log->priv->parent_window,
                log->priv->parent,
                FALSE);
    }
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

    if(log->priv->done)
    {
        log->priv->done = FALSE;
        mud_log_close(log);
    }
    else if(log->priv->active)
    {
        mud_line_buffer_add_data(log->priv->line_buffer, data, length);
    }
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
mud_log_write(MudLog *log, const gchar *data, gsize size)
{
    gchar *stripData;
    gint stripSize = 0;
    gsize write_size;

    g_return_if_fail(MUD_IS_LOG(log));
    g_return_if_fail(log->priv->logfile != NULL);
    g_return_if_fail(data != NULL);

    if(!log->priv->color)
    {
        stripData = utils_strip_ansi(data);
        stripSize = strlen(stripData);

        write_size = fwrite(stripData, 1, stripSize, log->priv->logfile);

        if(write_size != stripSize)
            g_critical(_("Could not write data to log file!"));

        g_free(stripData);
    }
    else
    {
        write_size = fwrite(data, 1, size, log->priv->logfile);

        if(write_size != size)
            g_critical(_("Could not write data to log file!"));

    }
}

