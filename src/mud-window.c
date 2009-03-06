/* GNOME-Mud - A simple Mud Client
 * mud-window.c
 * Copyright (C) 1998-2005 Robin Ericsson <lobbin@localhost.nu>
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

#include <glade/glade-xml.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <vte/vte.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>

#include "gnome-mud.h"
#include "gnome-mud-icons.h"
#include "mud-connection-view.h"
#include "mud-preferences-window.h"
#include "mud-window.h"
#include "mud-tray.h"
#include "mud-profile.h"
#include "mud-window-profile.h"
#include "mud-parse-base.h"
#include "mud-connections.h"

struct _MudWindowPrivate
{
    GSList *mud_views_list;

    GtkWidget *notebook;
    GtkWidget *textview;
    GtkWidget *textviewscroll;

    GtkWidget *startlog;
    GtkWidget *stoplog;
    GtkWidget *bufferdump;

    GtkWidget *menu_close;
    GtkWidget *menu_reconnect;
    GtkWidget *menu_disconnect;
    GtkWidget *toolbar_disconnect;
    GtkWidget *toolbar_reconnect;

    GtkWidget *blank_label;
    GtkWidget *current_view;

    GtkWidget *mi_profiles;

    GtkWidget *image;

    GSList *profile_menu_list;

    gint nr_of_tabs;
    gint textview_line_height;
};

typedef struct MudViewEntry
{
	gint id;
	MudConnectionView *view;
} MudViewEntry;

/* Create the Type */
G_DEFINE_TYPE(MudWindow, mud_window, G_TYPE_OBJECT);

/* Property Identifiers */
enum
{
    PROP_MUD_WINDOW_0,
    PROP_WINDOW,
    PROP_TRAY
};

/* Class Function Prototypes */
static void mud_window_init       (MudWindow *self);
static void mud_window_class_init (MudWindowClass *klass);
static void mud_window_finalize   (GObject *object);
static void mud_window_set_property(GObject *object,
                                    guint prop_id,
                                    const GValue *value,
                                    GParamSpec *pspec);
static void mud_window_get_property(GObject *object,
                                    guint prop_id,
                                    GValue *value,
                                    GParamSpec *pspec);

/* Callback Prototypes */
static int mud_window_close(GtkWidget *widget, MudWindow *self);
static gboolean mud_window_grab_entry_focus_cb(GtkWidget *widget,
                               GdkEventFocus *event,
                               gpointer user_data);
static void mud_window_disconnect_cb(GtkWidget *widget, MudWindow *self);
static void mud_window_reconnect_cb(GtkWidget *widget, MudWindow *self);
static void mud_window_closewindow_cb(GtkWidget *widget, MudWindow *self);
static void mud_window_textview_buffer_changed(GtkTextBuffer *buffer, 
                                               MudWindow *self);
static gboolean mud_window_textview_keypress(GtkWidget *widget,
                                             GdkEventKey *event, 
                                             MudWindow *self);
static void mud_window_notebook_page_change(GtkNotebook *notebook, 
                                            GtkNotebookPage *page, 
                                            gint arg, 
                                            MudWindow *self);
static void mud_window_preferences_cb(GtkWidget *widget, MudWindow *self);
static void mud_window_profiles_cb(GtkWidget *widget, MudWindow *self);
static void mud_window_about_cb(GtkWidget *widget, MudWindow *self);
static void mud_window_mconnect_dialog(GtkWidget *widget, MudWindow *self);

static gboolean mud_window_configure_event(GtkWidget *widget,
                                           GdkEventConfigure *event,
                                           gpointer user_data);
static gboolean save_dialog_vte_cb (VteTerminal *terminal, 
                                    glong column,
                                    glong row,
                                    gpointer data);
static void mud_window_buffer_cb(GtkWidget *widget, MudWindow *self);
static void mud_window_select_profile(GtkWidget *widget, MudWindow *self);
static void mud_window_profile_menu_set_cb(GtkWidget *widget, gpointer data);
static void mud_window_startlog_cb(GtkWidget *widget, MudWindow *self);
static void mud_window_stoplog_cb(GtkWidget *widget, MudWindow *self);

/* Private Method Prototypes */
static void mud_window_remove_connection_view(MudWindow *self, gint nr);
static gint mud_window_textview_get_display_line_count(GtkTextView *textview);
static void mud_window_textview_ensure_height(MudWindow *self, guint max_lines);
static void mud_window_clear_profiles_menu(GtkWidget *widget, gpointer data);

/* Class Functions */
static void
mud_window_class_init (MudWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override parent's finalize */
    object_class->finalize = mud_window_finalize;

    /* Override base object property methods */
    object_class->set_property = mud_window_set_property;
    object_class->get_property = mud_window_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(MudWindowPrivate));

    /* Create and Install Properties */
    g_object_class_install_property(object_class,
            PROP_WINDOW,
            g_param_spec_object("window",
                "gtk window",
                "the gtk window of mud window",
                GTK_TYPE_WINDOW,
                G_PARAM_READABLE));

    g_object_class_install_property(object_class,
            PROP_TRAY,
            g_param_spec_object("tray",
                "mud tray",
                "mud status tray icon",
                MUD_TYPE_TRAY,
                G_PARAM_READABLE));

}

static void
mud_window_init (MudWindow *self)
{
    GladeXML *glade;
    GtkTextIter iter;
    gint y;

    /* Get our private data */
    self->priv = MUD_WINDOW_GET_PRIVATE(self);

    /* start glading */
    glade = glade_xml_new(GLADEDIR "/main.glade", "main_window", NULL);

    /* set public properties */
    self->window = GTK_WINDOW(glade_xml_get_widget(glade, "main_window"));
    self->tray = g_object_new(MUD_TYPE_TRAY,
                              "parent-window", self->window,
                              NULL); 

    /* set priate members */
    self->priv->nr_of_tabs = 0;
    self->priv->current_view = NULL;
    self->priv->mud_views_list = NULL;
    self->priv->profile_menu_list = NULL;
    self->priv->menu_disconnect = glade_xml_get_widget(glade, "menu_disconnect");
    self->priv->toolbar_disconnect = glade_xml_get_widget(glade, "toolbar_disconnect");
    self->priv->menu_reconnect = glade_xml_get_widget(glade, "menu_reconnect");
    self->priv->toolbar_reconnect = glade_xml_get_widget(glade, "toolbar_reconnect");
    self->priv->menu_close = glade_xml_get_widget(glade, "menu_closewindow");
    self->priv->startlog = glade_xml_get_widget(glade, "menu_start_logging");
    self->priv->stoplog = glade_xml_get_widget(glade, "menu_stop_logging");
    self->priv->bufferdump = glade_xml_get_widget(glade, "menu_dump_buffer");
    self->priv->mi_profiles = glade_xml_get_widget(glade, "mi_profiles_menu");
    self->priv->notebook = glade_xml_get_widget(glade, "notebook");
    self->priv->textviewscroll = glade_xml_get_widget(glade, "text_view_scroll");
    self->priv->textview = glade_xml_get_widget(glade, "text_view");
    self->priv->image = glade_xml_get_widget(glade, "image");

    /* connect quit buttons */
    g_signal_connect(self->window,
                     "destroy",
                     G_CALLBACK(mud_window_close),
                     self);

    g_signal_connect(glade_xml_get_widget(glade, "menu_quit"),
                     "activate",
                     G_CALLBACK(mud_window_close),
                     self);

    /* connect connect buttons */
    g_signal_connect(glade_xml_get_widget(glade, "main_connect"),
                     "activate",
                     G_CALLBACK(mud_window_mconnect_dialog),
                     self);

    g_signal_connect(glade_xml_get_widget(glade, "toolbar_connect"),
                     "clicked",
                     G_CALLBACK(mud_window_mconnect_dialog),
                     self);

    /* connect disconnect buttons */
    g_signal_connect(self->priv->menu_disconnect,
                     "activate",
                     G_CALLBACK(mud_window_disconnect_cb),
                     self);

    g_signal_connect(self->priv->toolbar_disconnect,
                     "clicked",
                     G_CALLBACK(mud_window_disconnect_cb),
                     self);

    /* connect reconnect buttons */
    g_signal_connect(self->priv->menu_reconnect,
                     "activate",
                     G_CALLBACK(mud_window_reconnect_cb),
                     self);

    g_signal_connect(self->priv->toolbar_reconnect,
                     "clicked",
                     G_CALLBACK(mud_window_reconnect_cb),
                     self);

    /* connect close window button */
    g_signal_connect(self->priv->menu_close,
                     "activate",
                     G_CALLBACK(mud_window_closewindow_cb),
                     self);

    /* logging */
    g_signal_connect(self->priv->startlog,
                     "activate",
                     G_CALLBACK(mud_window_startlog_cb),
                     self);

    g_signal_connect(self->priv->stoplog,
                     "activate", G_CALLBACK(mud_window_stoplog_cb),
                     self);

    g_signal_connect(self->priv->bufferdump,
                     "activate",
                     G_CALLBACK(mud_window_buffer_cb),
                     self);

    /* preferences window button */
    g_signal_connect(glade_xml_get_widget(glade, "menu_preferences"),
                     "activate",
                     G_CALLBACK(mud_window_preferences_cb),
                     self);

    g_signal_connect(glade_xml_get_widget(glade, "menu_about"),
                     "activate",
                     G_CALLBACK(mud_window_about_cb),
                     self);

    /* other objects */
    g_signal_connect(self->priv->notebook,
                     "switch-page",
                     G_CALLBACK(mud_window_notebook_page_change),
                     self);

    g_signal_connect(self->window,
                     "configure-event",
                     G_CALLBACK(mud_window_configure_event),
                     self);

    g_signal_connect(self->priv->textview,
                     "key_press_event",
                     G_CALLBACK(mud_window_textview_keypress),
                     self);

    g_signal_connect(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(self->priv->textview)),
            "changed",
            G_CALLBACK(mud_window_textview_buffer_changed),
            self);

    /* Setup TextView */
    gtk_text_view_set_wrap_mode(
            GTK_TEXT_VIEW(self->priv->textview), GTK_WRAP_WORD_CHAR);

    /* Set the initial height of the input box equal to the height of one line */
    gtk_text_buffer_get_start_iter(
            gtk_text_view_get_buffer(
                GTK_TEXT_VIEW(self->priv->textview)),
                &iter);

    gtk_text_view_get_line_yrange(GTK_TEXT_VIEW(self->priv->textview),
                                  &iter, &y,
                                  &self->priv->textview_line_height);

    gtk_widget_set_size_request(self->priv->textview, -1,
                                self->priv->textview_line_height*1);
    gtk_widget_set_size_request(
            GTK_SCROLLED_WINDOW(self->priv->textviewscroll)->vscrollbar,
            -1, 1);

    if (GTK_WIDGET_VISIBLE(self->priv->textviewscroll))
        gtk_widget_queue_resize(self->priv->textviewscroll);

    mud_window_populate_profiles_menu(self);

    g_object_unref(glade);
}

static void
mud_window_finalize (GObject *object)
{

    GSList *entry;
    MudWindow    *self;
    GObjectClass *parent_class;

    self = MUD_WINDOW(object);

    entry = self->priv->mud_views_list;

    while(entry != NULL)
    {
        g_object_unref( ( (MudViewEntry *)entry->data )->view );
        entry = g_slist_next(entry);
    }

    g_slist_free(self->priv->mud_views_list);
    
    g_object_unref(self->tray);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);

    gtk_main_quit();
}

static void
mud_window_set_property(GObject *object,
                        guint prop_id,
                        const GValue *value,
                        GParamSpec *pspec)
{
    MudWindow *self;

    self = MUD_WINDOW(object);

    switch(prop_id)
    {
        /* These properties aren't writeable. If we get here
         * something is really screwy. */
        case PROP_WINDOW:
            g_return_if_reached();
            break;

        case PROP_TRAY:
            g_return_if_reached();
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
mud_window_get_property(GObject *object,
                        guint prop_id,
                        GValue *value,
                        GParamSpec *pspec)
{
    MudWindow *self;

    self = MUD_WINDOW(object);

    switch(prop_id)
    {
        case PROP_WINDOW:
            g_value_take_object(value, self->window);
            break;

        case PROP_TRAY:
            g_value_take_object(value, self->tray);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

/* Callbacks */
static int
mud_window_close(GtkWidget *widget, MudWindow *self)
{
    g_object_unref(self);

    return TRUE;
}

static gboolean
mud_window_grab_entry_focus_cb(GtkWidget *widget,
                               GdkEventFocus *event,
                               gpointer user_data)
{
    MudWindow *self = MUD_WINDOW(user_data);
    gtk_widget_grab_focus(self->priv->textview);

    return TRUE;
}

static void
mud_window_disconnect_cb(GtkWidget *widget, MudWindow *self)
{
    if (self->priv->current_view != NULL)
    {
        gtk_widget_set_sensitive(self->priv->startlog, FALSE);
        gtk_widget_set_sensitive(self->priv->menu_disconnect, FALSE);
        gtk_widget_set_sensitive(self->priv->toolbar_disconnect, FALSE);
        mud_connection_view_disconnect(MUD_CONNECTION_VIEW(self->priv->current_view));
    }
}

static void
mud_window_reconnect_cb(GtkWidget *widget, MudWindow *self)
{
    if (self->priv->current_view != NULL)
    {
        gtk_widget_set_sensitive(self->priv->startlog, TRUE);
        gtk_widget_set_sensitive(self->priv->menu_disconnect, TRUE);
        gtk_widget_set_sensitive(self->priv->toolbar_disconnect, TRUE);
        mud_connection_view_reconnect(MUD_CONNECTION_VIEW(self->priv->current_view));
    }
}

static void
mud_window_closewindow_cb(GtkWidget *widget, MudWindow *self)
{
    mud_window_close_current_window(self);
}

static void
mud_window_textview_buffer_changed(GtkTextBuffer *buffer, MudWindow *self)
{
    mud_window_textview_ensure_height(self, 5);
}

static gboolean
mud_window_textview_keypress(GtkWidget *widget, GdkEventKey *event, MudWindow *self)
{
    gchar *text;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self->priv->textview));
    GtkTextIter start, end;
    MudParseBase *base;
    GConfClient *client = gconf_client_get_default();

    if ((event->keyval == GDK_Return || event->keyval == GDK_KP_Enter) &&
            (event->state & gtk_accelerator_get_default_mod_mask()) == 0)
    {
        gtk_text_buffer_get_bounds(buffer, &start, &end);

        if (self->priv->current_view)
        {
            text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

            if (g_str_equal(text, ""))
                text = g_strdup(" ");

            base = mud_connection_view_get_parsebase(MUD_CONNECTION_VIEW(self->priv->current_view));

            if(mud_parse_base_do_aliases(base, text))
                mud_connection_view_send(MUD_CONNECTION_VIEW(self->priv->current_view), text);

            g_free(text);
        }

        if (gconf_client_get_bool(client,
                    "/apps/gnome-mud/functionality/keeptext", NULL) == FALSE)
            gtk_text_buffer_delete(buffer, &start, &end);
        else
            gtk_text_buffer_select_range(buffer, &start, &end);

        g_object_unref(client);

        return TRUE;
    }

    g_object_unref(client);

    if(self->priv->current_view)
    {
        if(event->keyval == GDK_Up)
        {
            text = mud_connection_view_get_history_item(
                    MUD_CONNECTION_VIEW(self->priv->current_view), HISTORY_UP);

            if(text)
            {
                gtk_text_buffer_set_text(buffer, text, strlen(text));
                gtk_text_buffer_get_bounds(buffer, &start, &end);
                gtk_text_buffer_select_range(buffer, &start, &end);
            }

            return TRUE;
        }

        if(event->keyval == GDK_Down)
        {
            text = mud_connection_view_get_history_item(
                    MUD_CONNECTION_VIEW(self->priv->current_view), HISTORY_DOWN);

            if(text)
            {
                gtk_text_buffer_set_text(buffer, text, strlen(text));
                gtk_text_buffer_get_bounds(buffer, &start, &end);
                gtk_text_buffer_select_range(buffer, &start, &end);
            }

            return TRUE;
        }
    }

    return FALSE;
}

static void
mud_window_notebook_page_change(GtkNotebook *notebook, GtkNotebookPage *page, gint arg, MudWindow *self)
{
    gchar *name;
    gboolean connected;

    self->priv->current_view =
        g_object_get_data(
                G_OBJECT(gtk_notebook_get_nth_page(notebook, arg)),
                "connection-view");

    if (self->priv->nr_of_tabs != 0)
    {
        name = mud_profile_get_name(
                mud_connection_view_get_current_profile(
                    MUD_CONNECTION_VIEW(self->priv->current_view)));

        mud_window_profile_menu_set_active(self, name);

        connected = mud_connection_view_is_connected(
                MUD_CONNECTION_VIEW(self->priv->current_view));

        if(mud_connection_view_islogging(MUD_CONNECTION_VIEW(self->priv->current_view)))
        {
            gtk_widget_set_sensitive(self->priv->startlog, FALSE);
            gtk_widget_set_sensitive(self->priv->stoplog, TRUE);
        }
        else
        {
            gtk_widget_set_sensitive(self->priv->startlog, TRUE);
            gtk_widget_set_sensitive(self->priv->stoplog, FALSE);
        }

        if(!connected)
        {
            gtk_widget_set_sensitive(self->priv->startlog, FALSE);
            gtk_widget_set_sensitive(self->priv->stoplog, FALSE);
        }

        gtk_widget_set_sensitive(self->priv->menu_disconnect, connected);
        gtk_widget_set_sensitive(self->priv->toolbar_disconnect, connected);
    }
    else
    {
        gtk_widget_set_sensitive(self->priv->startlog, FALSE);
        gtk_widget_set_sensitive(self->priv->stoplog, FALSE);
        gtk_widget_set_sensitive(self->priv->bufferdump, FALSE);
        gtk_widget_set_sensitive(self->priv->menu_close, FALSE);
        gtk_widget_set_sensitive(self->priv->menu_reconnect, FALSE);
        gtk_widget_set_sensitive(self->priv->menu_disconnect, FALSE);
        gtk_widget_set_sensitive(self->priv->toolbar_disconnect, FALSE);
        gtk_widget_set_sensitive(self->priv->toolbar_reconnect, FALSE);
    }

    gtk_widget_grab_focus(self->priv->textview);
}

static void
mud_window_preferences_cb(GtkWidget *widget, MudWindow *self)
{
    mud_preferences_window_new("Default");
}

static void
mud_window_profiles_cb(GtkWidget *widget, MudWindow *self)
{
    mud_window_profile_new(self);
}

static void
mud_window_about_cb(GtkWidget *widget, MudWindow *self)
{
    static const gchar * const authors[] = {
        "Robin Ericsson <lobbin@localhost.nu>",
        "Jordi Mallach <jordi@sindominio.net>",
        "Daniel Patton <seven-nation@army.com>",
        "Les Harris <lharris@gnome.org>",
        "Mart Raudsepp <leio@gentoo.org>",
        NULL
    };

    static const gchar * const documenters[] = {
        "Jordi Mallach <jordi@sindominio.net>",
        "Petter E. Stokke <gibreel@project23.no>",
        NULL
    };

    static const gchar * const artists[] = {
        "Daniel Taylor <DanTaylor@web.de>",
        "Andreas Nilsson <andreasn@gnome.org>",
        NULL
    };

    static const gchar copyright[] = "Copyright \xc2\xa9 1998-2005 Robin Ericsson\n"
                                     "Copyright \xc2\xa9 2005-2009 Les Harris";

    static const gchar comments[] = N_("A Multi-User Dungeon (MUD) client for GNOME");

    GdkPixbuf *logo = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), "gnome-mud",
            128, GTK_ICON_LOOKUP_FORCE_SVG, NULL);

    gtk_show_about_dialog(GTK_WINDOW(self->window),
            "artists", artists,
            "authors", authors,
            "comments", _(comments),
            "copyright", copyright,
            "documenters", documenters,
            "logo", logo,
            "translator-credits", _("translator-credits"),
            "version", VERSION,
            "website", "http://live.gnome.org/GnomeMud",
            "name", "Gnome-Mud",
            NULL);

    if(logo)
        g_object_unref(logo);
}

static void
mud_window_mconnect_dialog(GtkWidget *widget, MudWindow *self)
{
    MudConnections *connections = g_object_new(MUD_TYPE_CONNECTIONS,
                                               "parent-window", self,
                                               NULL);
}

static gboolean
mud_window_configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data)
{
    gint i;
    GSList *view;
    MudWindow *self = (MudWindow *)user_data;

    if (self->priv->nr_of_tabs == 0)
    {
        GError *err = NULL;
        GdkPixbuf *buf;

        buf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                "gnome-mud", event->width >> 1, GTK_ICON_LOOKUP_FORCE_SVG, &err);

        gtk_image_set_from_pixbuf(GTK_IMAGE(self->priv->image), buf);

        g_object_unref(buf);
    }

    for(i = 0; i < self->priv->nr_of_tabs; ++i)
    {
        MudConnectionView *iter =
            g_object_get_data(
                    G_OBJECT(
                        gtk_notebook_get_nth_page(
                            GTK_NOTEBOOK(self->priv->notebook),
                            i)),
                    "connection-view");

        if(mud_connection_view_is_connected(iter))
            mud_connection_view_send_naws(iter);
    }

    gtk_widget_grab_focus(self->priv->textview);

    return FALSE;
}

static gboolean
save_dialog_vte_cb (VteTerminal *terminal,glong column,glong row,gpointer data)
{
    return TRUE;
}

static void
mud_window_buffer_cb(GtkWidget *widget, MudWindow *self)
{
    GladeXML *glade;
    GtkWidget *dialog;
    gint result;

    glade = glade_xml_new(GLADEDIR "/main.glade", "save_dialog", NULL);
    dialog = glade_xml_get_widget(glade, "save_dialog");

    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), "buffer.txt");
    result = gtk_dialog_run(GTK_DIALOG(dialog));
    if(result == GTK_RESPONSE_OK)
    {
        gchar *filename;
        FILE *file;

        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        file = fopen(filename, "w");

        if(!file)
        {
            GtkWidget *errDialog;

            errDialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Could not save the file in specified location!"));

            gtk_dialog_run(GTK_DIALOG(errDialog));
            gtk_widget_destroy(errDialog);
        }
        else
        {
            gchar *bufferText;
            GtkWidget *term;

            term = mud_connection_view_get_terminal(MUD_CONNECTION_VIEW(self->priv->current_view));

            bufferText = vte_terminal_get_text_range(VTE_TERMINAL(term),0,0,
                    vte_terminal_get_row_count(VTE_TERMINAL(term)),
                    vte_terminal_get_column_count(VTE_TERMINAL(term)),
                    save_dialog_vte_cb,
                    NULL,
                    NULL);

            if(!fwrite(bufferText, 1, strlen(bufferText), file))
                g_critical(_("Could not write buffer to disk!"));

            fclose(file);
        }

        g_free(filename);
    }

    gtk_widget_destroy(dialog);
    g_object_unref(glade);
}

static void
mud_window_select_profile(GtkWidget *widget, MudWindow *self)
{
    MudProfile *profile;
    GtkWidget *profileLabel;

    profileLabel = gtk_bin_get_child(GTK_BIN(widget));

    if (self->priv->current_view)
    {
        profile = get_profile(gtk_label_get_text(GTK_LABEL(profileLabel)));

        if (profile)
            mud_connection_view_set_profile(MUD_CONNECTION_VIEW(self->priv->current_view), profile);
    }
}

static void
mud_window_profile_menu_set_cb(GtkWidget *widget, gpointer data)
{
    gchar *name = (gchar *)data;
    GtkWidget *label;

    label = gtk_bin_get_child(GTK_BIN(widget));


    if (GTK_IS_LABEL(label) && !strcmp(name,gtk_label_get_text(GTK_LABEL(label))))
    {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), TRUE);
    }
}

static void
mud_window_startlog_cb(GtkWidget *widget, MudWindow *self)
{
    mud_connection_view_start_logging(MUD_CONNECTION_VIEW(self->priv->current_view));
    gtk_widget_set_sensitive(self->priv->startlog, FALSE);
    gtk_widget_set_sensitive(self->priv->stoplog, TRUE);

}

static void
mud_window_stoplog_cb(GtkWidget *widget, MudWindow *self)
{
    mud_connection_view_stop_logging(MUD_CONNECTION_VIEW(self->priv->current_view));
    gtk_widget_set_sensitive(self->priv->stoplog, FALSE);
    gtk_widget_set_sensitive(self->priv->startlog, TRUE);
}

/* Private Methods */
static void
mud_window_remove_connection_view(MudWindow *self, gint nr)
{
    GSList *entry, *rementry;

    rementry = NULL;
    rementry = g_slist_append(rementry, NULL);

    g_object_unref(self->priv->current_view);
    gtk_notebook_remove_page(GTK_NOTEBOOK(self->priv->notebook), nr);

    for(entry = self->priv->mud_views_list; entry != NULL; entry = g_slist_next(entry))
        if(((MudViewEntry *)entry->data)->id == nr)
        {
            rementry->data = entry->data;
            break;
        }

    self->priv->mud_views_list =
        g_slist_remove(self->priv->mud_views_list, rementry->data);

    if (--self->priv->nr_of_tabs < 2)
    {
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(self->priv->notebook), FALSE);
    }

    if (self->priv->nr_of_tabs == 0)
    {
        gint w, h;
        GdkPixbuf *buf;
        GError *err = NULL;

        gtk_window_get_size(GTK_WINDOW(self->window), &w, &h);

        if(self->priv->image)
            g_object_unref(self->priv->image);

        buf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                "gnome-mud", w >> 1, GTK_ICON_LOOKUP_FORCE_SVG, &err);

        self->priv->image = gtk_image_new_from_pixbuf(buf);
        gtk_widget_show(self->priv->image);

        if(buf)
            g_object_unref(buf);

        gtk_notebook_append_page(GTK_NOTEBOOK(self->priv->notebook), self->priv->image, NULL);
    }
}

static gint
mud_window_textview_get_display_line_count(GtkTextView *textview)
{
    gint result = 1;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview);
    GtkTextIter iter;

    gtk_text_buffer_get_start_iter(buffer, &iter);
    while (gtk_text_view_forward_display_line(textview, &iter))
        ++result;

    if (gtk_text_buffer_get_line_count(buffer) != 1)
    {
        GtkTextIter iter2;
        gtk_text_buffer_get_end_iter(buffer, &iter2);
        if (gtk_text_iter_get_chars_in_line(&iter) == 0)
            ++result;
    }

    return result;
}

static void
mud_window_textview_ensure_height(MudWindow *self, guint max_lines)
{
    gint lines = mud_window_textview_get_display_line_count(GTK_TEXT_VIEW(self->priv->textview));
    gtk_widget_set_size_request(self->priv->textview, -1,
            self->priv->textview_line_height * MIN(lines, max_lines));
    gtk_widget_queue_resize(gtk_widget_get_parent(self->priv->textview));
}


static void
mud_window_clear_profiles_menu(GtkWidget *widget, gpointer data)
{
    gtk_widget_destroy(widget);
}

/* Public Methods */
void
mud_window_close_current_window(MudWindow *self)
{
    g_return_if_fail(IS_MUD_WINDOW(self));

    if (self->priv->nr_of_tabs > 0)
    {
        gint nr = gtk_notebook_get_current_page(GTK_NOTEBOOK(self->priv->notebook));

        mud_window_remove_connection_view(self, nr);

        if(self->priv->nr_of_tabs == 0)
            mud_tray_update_icon(MUD_TRAY(self->tray), offline_connecting);
    }
}

void
mud_window_profile_menu_set_active(MudWindow *self, gchar *name)
{
    g_return_if_fail(IS_MUD_WINDOW(self));

    gtk_container_foreach(GTK_CONTAINER(self->priv->mi_profiles),mud_window_profile_menu_set_cb,(gpointer)name);
}

void
mud_window_populate_profiles_menu(MudWindow *self)
{
    const GList *profiles;
    GList *entry;
    GtkWidget *profile;
    GtkWidget *sep;
    GtkWidget *manage;
    GtkWidget *icon;

    g_return_if_fail(IS_MUD_WINDOW(self));

    profiles = mud_profile_get_profiles();

    gtk_container_foreach(GTK_CONTAINER(self->priv->mi_profiles),
                          mud_window_clear_profiles_menu,
                          NULL);

    for (entry = (GList *)profiles; entry != NULL; entry = g_list_next(entry))
    {
        profile = gtk_radio_menu_item_new_with_label(
                self->priv->profile_menu_list,
                (gchar *)MUD_PROFILE(entry->data)->name);

        gtk_widget_show(profile);
        
        self->priv->profile_menu_list = 
            gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(profile));

        gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->mi_profiles), profile);

        g_signal_connect(profile,
                         "activate",
                         G_CALLBACK(mud_window_select_profile),
                        self);
    }

    sep = gtk_separator_menu_item_new();
    gtk_widget_show(sep);
    gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->mi_profiles), sep);

    icon = gtk_image_new_from_stock("gtk-edit", GTK_ICON_SIZE_MENU);

    manage = gtk_image_menu_item_new_with_mnemonic(_("_Manage Profiles..."));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(manage), icon);
    gtk_widget_show(manage);

    gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->mi_profiles), manage);

    g_signal_connect(manage,
                     "activate",
                     G_CALLBACK(mud_window_profiles_cb),
                     self);
}
    
GtkWidget*
mud_window_get_window(MudWindow *self)
{
    if(!IS_MUD_WINDOW(self))
        return NULL;

    return GTK_WIDGET(self->window);
}

void
mud_window_add_connection_view(MudWindow *self, GObject *cview, gchar *tabLbl)
{
    gint nr;
    MudViewEntry *entry;
    GtkWidget *terminal;
    MudConnectionView *view = MUD_CONNECTION_VIEW(cview);

    g_return_if_fail(IS_MUD_WINDOW(self));
    g_return_if_fail(IS_MUD_CONNECTION_VIEW(view));

    entry = g_new(MudViewEntry, 1);

    if (self->priv->nr_of_tabs++ == 0)
    {
        gtk_notebook_remove_page(GTK_NOTEBOOK(self->priv->notebook), 0);
        self->priv->image = NULL;
    }

    nr = gtk_notebook_append_page(GTK_NOTEBOOK(self->priv->notebook), mud_connection_view_get_viewport(view), gtk_label_new(tabLbl));
    gtk_notebook_set_current_page(GTK_NOTEBOOK(self->priv->notebook), nr);

    gtk_widget_set_sensitive(self->priv->startlog, TRUE);
    gtk_widget_set_sensitive(self->priv->bufferdump, TRUE);
    gtk_widget_set_sensitive(self->priv->menu_close, TRUE);
    gtk_widget_set_sensitive(self->priv->menu_reconnect, TRUE);
    gtk_widget_set_sensitive(self->priv->menu_disconnect, TRUE);
    gtk_widget_set_sensitive(self->priv->toolbar_disconnect, TRUE);
    gtk_widget_set_sensitive(self->priv->toolbar_reconnect, TRUE);

    mud_connection_view_set_id(view, nr);
    mud_connection_view_set_parent(view, self);

    terminal = mud_connection_view_get_terminal(view);
    g_signal_connect(terminal,
                     "focus-in-event",
                     G_CALLBACK(mud_window_grab_entry_focus_cb),
                     self);

    entry->id = nr;
    entry->view = view;

    self->priv->mud_views_list = g_slist_append(self->priv->mud_views_list, entry);

    if (self->priv->nr_of_tabs > 1)
    {
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(self->priv->notebook), TRUE);
    }
}

void
mud_window_disconnected(MudWindow *self)
{
    g_return_if_fail(IS_MUD_WINDOW(self));

    gtk_widget_set_sensitive(self->priv->startlog, FALSE);
    gtk_widget_set_sensitive(self->priv->menu_disconnect, FALSE);
    gtk_widget_set_sensitive(self->priv->toolbar_disconnect, FALSE);
}

