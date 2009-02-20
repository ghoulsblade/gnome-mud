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

#include <gconf/gconf-client.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <vte/vte.h>
#define GNET_EXPERIMENTAL
#include <gnet.h>
#include <string.h>

#include "gnome-mud.h"
#include "mud-connection-view.h"
#include "mud-profile.h"
#include "mud-window.h"
#include "mud-tray.h"
#include "mud-log.h"
#include "mud-parse-base.h"
#include "mud-telnet.h"
#include "mud-telnet-zmp.h"
#include "mud-telnet-msp.h"

#ifdef ENABLE_MCCP
#include "mud-telnet-mccp.h"
#endif

#ifdef ENABLE_GST
#include "mud-telnet-msp.h"
#endif

/* Hack, will refactor with plugin rewrite -lh */
extern gboolean PluginGag;

struct _MudConnectionViewPrivate
{
    gint id;

    MudWindow *window;
    MudTray *tray;

    GtkWidget *terminal;
    GtkWidget *scrollbar;
    GtkWidget *box;
    GtkWidget *popup_menu;
    GtkWidget *progressbar;
    GtkWidget *dl_label;
    GtkWidget *dl_button;

    MudProfile *profile;
    MudLog *log;

    gulong signal;
    gulong signal_profile_changed;

    gboolean connect_hook;
    gboolean connected;
    gchar *connect_string;

    MudParseBase *parse;

    GQueue *history;
    guint current_history_index;

    MudTelnet *telnet;
    gchar *hostname;
    guint port;

    gchar *mud_name;

#ifdef ENABLE_GST
    GQueue *download_queue;
    GConnHttp *dl_conn;
    gboolean downloading;
#endif

    GString *processed;

    guint width;
    guint height;
};

static void mud_connection_view_init                     (MudConnectionView *connection_view);
static void mud_connection_view_class_init               (MudConnectionViewClass *klass);
static void mud_connection_view_finalize                 (GObject *object);
static void mud_connection_view_set_terminal_colors      (MudConnectionView *view);
static void mud_connection_view_set_terminal_scrollback  (MudConnectionView *view);
static void mud_connection_view_set_terminal_scrolloutput(MudConnectionView *view);
static void mud_connection_view_set_terminal_font        (MudConnectionView *view);
static void mud_connection_view_set_terminal_type        (MudConnectionView *view);
static void mud_connection_view_profile_changed_cb       (MudProfile *profile, MudProfileMask *mask, MudConnectionView *view);
static gboolean mud_connection_view_button_press_event   (GtkWidget *widget, GdkEventButton *event, MudConnectionView *view);
static void mud_connection_view_popup                    (MudConnectionView *view, GdkEventButton *event);
static void mud_connection_view_reread_profile           (MudConnectionView *view);
static void mud_connection_view_network_event_cb(GConn *conn, GConnEvent *event, gpointer data);

#ifdef ENABLE_GST
static void mud_connection_view_http_cb(GConnHttp *conn, GConnHttpEvent *event, gpointer data);
static void mud_connection_view_cancel_dl_cb(GtkWidget *widget, MudConnectionView *view);
#endif

GType
mud_connection_view_get_type (void)
{
    static GType object_type = 0;

    g_type_init();

    if (!object_type)
    {
        static const GTypeInfo object_info =
            {
                sizeof (MudConnectionViewClass),
                NULL,
                NULL,
                (GClassInitFunc) mud_connection_view_class_init,
                NULL,
                NULL,
                sizeof (MudConnectionView),
                0,
                (GInstanceInitFunc) mud_connection_view_init,
            };

        object_type = g_type_register_static(G_TYPE_OBJECT, "MudConnectionView", &object_info, 0);
    }

    return object_type;
}

static GtkWidget*
append_stock_menuitem(GtkWidget *menu, const gchar *text, GCallback callback, gpointer data)
{
    GtkWidget *menu_item;
    GtkWidget *image;
    GConfClient *client;
    GError *error;
    gboolean use_image;

    menu_item = gtk_image_menu_item_new_from_stock(text, NULL);
    image = gtk_image_menu_item_get_image(GTK_IMAGE_MENU_ITEM(menu_item));

    client = gconf_client_get_default();
    error = NULL;

    use_image = gconf_client_get_bool(client,
                                      "/desktop/gnome/interface/menus_have_icons",
                                      &error);
    if (error)
    {
        g_printerr(_("There was an error loading config value for whether to use image in menus. (%s)\n"),
                   error->message);
        g_error_free(error);
    }
    else
    {
        if (use_image)
            gtk_widget_show(image);
        else
            gtk_widget_hide(image);
    }

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

    if (callback)
        g_signal_connect(G_OBJECT(menu_item),
                         "activate",
                         callback, data);

    return menu_item;
}

static GtkWidget*
append_menuitem(GtkWidget *menu, const gchar *text, GCallback callback, gpointer data)
{
    GtkWidget *menu_item;

    menu_item = gtk_menu_item_new_with_mnemonic(text);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

    if (callback)
        g_signal_connect(G_OBJECT(menu_item), "activate", callback, data);

    return menu_item;
}

static void
popup_menu_detach(GtkWidget *widget, GtkMenu *menu)
{
    MudConnectionView *view;

    view = g_object_get_data(G_OBJECT(widget), "connection-view");
    view->priv->popup_menu = NULL;
}

static void
choose_profile_callback(GtkWidget *menu_item, MudConnectionView *view)
{
    MudProfile *profile;

    if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item)))
        return;

    profile = g_object_get_data(G_OBJECT(menu_item), "profile");

    g_assert(profile);

    mud_connection_view_set_profile(view, profile);
    mud_connection_view_reread_profile(view);
}

static void
mud_connection_view_close_current_cb(GtkWidget *menu_item, MudConnectionView *view)
{
    mud_window_close_current_window(view->priv->window);
}


static void
mud_connection_view_str_replace (gchar *buf, const gchar *s, const gchar *repl)
{
    gchar out_buf[4608];
    gchar *pc, *out;
    gint  len = strlen (s);
    gboolean found = FALSE;

    for ( pc = buf, out = out_buf; *pc && (out-out_buf) < (4608-len-4);)
        if ( !strncasecmp(pc, s, len))
        {
            out += sprintf (out, "%s", repl);
            pc += len;
            found = TRUE;
        }
        else
            *out++ = *pc++;

    if ( found)
    {
        *out = '\0';
        strcpy (buf, out_buf);
    }
}

static void
mud_connection_view_feed_text(MudConnectionView *view, gchar *message)
{
    gint rlen = strlen(message);
    gchar buf[rlen*2];

    g_stpcpy(buf, message);
    mud_connection_view_str_replace(buf, "\r", "");
    mud_connection_view_str_replace(buf, "\n", "\n\r");

    vte_terminal_feed(VTE_TERMINAL(view->priv->terminal), buf, strlen(buf));
}

void
mud_connection_view_add_text(MudConnectionView *view, gchar *message, enum MudConnectionColorType type)
{
    gchar *encoding, *text;
    const gchar *local_codeset;
    gchar *profile_name;
    GConfClient *client;
    gboolean remote;
    gsize bytes_read, bytes_written;
    GError *error = NULL;

    gchar key[2048];
    gchar extra_path[512] = "";

    client = gconf_client_get_default();

    text = g_strdup(message);

    g_snprintf(key, 2048, "/apps/gnome-mud/%s%s", extra_path, "functionality/remote_encoding");
    remote = gconf_client_get_bool(client, key, NULL);

    if(view->remote_encode && remote)
        encoding = view->remote_encoding;
    else
    {
        profile_name = mud_profile_get_name(view->priv->profile);

        if (strcmp(profile_name, "Default"))
        {
            g_snprintf(extra_path, 512, "profiles/%s/", profile_name);
        }

        g_snprintf(key, 2048, "/apps/gnome-mud/%s%s", extra_path, "functionality/encoding");
        encoding = gconf_client_get_string(client, key, NULL);
    }

    g_get_charset(&local_codeset);

    text = g_convert(message, -1,
            encoding,
            local_codeset, 
            &bytes_read, &bytes_written, &error);

    vte_terminal_set_encoding(VTE_TERMINAL(view->priv->terminal), encoding);

    g_free(encoding);

    switch (type)
    {
        case Sent:
            mud_connection_view_feed_text(view, "\e[1;33m");
            break;

        case Error:
            mud_connection_view_feed_text(view, "\e[1;31m");
            break;

        case System:
            mud_connection_view_feed_text(view, "\e[1;32m");
            break;

        case Normal:
        default:
            break;
    }

    if(!error)
    {
        if(view->local_echo)
            mud_connection_view_feed_text(view, text);
        mud_connection_view_feed_text(view, "\e[0m");
    }

    g_free(text);
}


static void
mud_connection_view_init (MudConnectionView *connection_view)
{
    GtkWidget *box;
    GtkWidget *dl_vbox;
    GtkWidget *dl_hbox;
    GtkWidget *term_box;

    connection_view->priv = g_new0(MudConnectionViewPrivate, 1);

    connection_view->priv->history = g_queue_new();
    connection_view->priv->current_history_index = 0;

#ifdef ENABLE_GST
    connection_view->priv->download_queue = g_queue_new();
    connection_view->priv->dl_conn = NULL;
#endif

    connection_view->priv->processed = NULL;

    connection_view->priv->parse = mud_parse_base_new(connection_view);

    connection_view->priv->connect_hook = FALSE;

    box = gtk_vbox_new(FALSE, 0);
    dl_vbox = gtk_vbox_new(FALSE, 0);
    dl_hbox = gtk_hbox_new(FALSE, 0);
    term_box = gtk_hbox_new(FALSE, 0);

    connection_view->priv->dl_label = gtk_label_new("Downloading...");
    connection_view->priv->progressbar = gtk_progress_bar_new();
    gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR(connection_view->priv->progressbar), 0.1);
    connection_view->priv->dl_button = gtk_button_new_from_stock("gtk-cancel");

#ifdef ENABLE_GST
    connection_view->priv->downloading = FALSE;
#endif

    gtk_box_pack_start(GTK_BOX(dl_vbox), connection_view->priv->dl_label, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(dl_hbox), connection_view->priv->progressbar, TRUE, TRUE, 0);

    gtk_box_pack_end(GTK_BOX(dl_hbox), connection_view->priv->dl_button, FALSE, FALSE, 0);

    gtk_box_pack_end(GTK_BOX(dl_vbox), dl_hbox, TRUE, TRUE, 0);

    connection_view->priv->terminal = vte_terminal_new();
    vte_terminal_set_encoding(VTE_TERMINAL(connection_view->priv->terminal), "ISO-8859-1");
    vte_terminal_set_emulation(VTE_TERMINAL(connection_view->priv->terminal), "xterm");
        
    connection_view->priv->width = VTE_TERMINAL(connection_view->priv->terminal)->column_count;
    connection_view->priv->height = VTE_TERMINAL(connection_view->priv->terminal)->row_count;
        
    gtk_box_pack_start(GTK_BOX(term_box), connection_view->priv->terminal, TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(connection_view->priv->terminal),
                     "button_press_event",
                     G_CALLBACK(mud_connection_view_button_press_event),
                     connection_view);
    g_object_set_data(G_OBJECT(connection_view->priv->terminal),
                      "connection-view", connection_view);

#ifdef ENABLE_GST                                                       
    g_signal_connect(connection_view->priv->dl_button, "clicked",
    G_CALLBACK(mud_connection_view_cancel_dl_cb), connection_view);
#endif

    connection_view->priv->scrollbar = gtk_vscrollbar_new(NULL);
    gtk_range_set_adjustment(
            GTK_RANGE(connection_view->priv->scrollbar),
            VTE_TERMINAL(connection_view->priv->terminal)->adjustment);
    gtk_box_pack_end(GTK_BOX(term_box), connection_view->priv->scrollbar, 
            FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box), dl_vbox, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(box), term_box, TRUE, TRUE, 0);

    gtk_widget_show_all(box);

    gtk_widget_hide(connection_view->priv->progressbar);
    gtk_widget_hide(connection_view->priv->dl_label);
    gtk_widget_hide(connection_view->priv->dl_button);

    g_object_set_data(G_OBJECT(box), "connection-view", connection_view);

    connection_view->priv->box = box;
    connection_view->priv->connected = FALSE;

}

static void
mud_connection_view_reread_profile(MudConnectionView *view)
{
    mud_connection_view_set_terminal_colors(view);
    mud_connection_view_set_terminal_scrollback(view);
    mud_connection_view_set_terminal_scrolloutput(view);
    mud_connection_view_set_terminal_font(view);
    mud_connection_view_set_terminal_type(view);
}

static void
mud_connection_view_class_init (MudConnectionViewClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = mud_connection_view_finalize;
}

static void
mud_connection_view_finalize (GObject *object)
{
    MudConnectionView *connection_view;
    GObjectClass *parent_class;
    gchar *history_item;

#ifdef ENABLE_GST                               
    MudMSPDownloadItem *item;
#endif

    connection_view = MUD_CONNECTION_VIEW(object);

    if(connection_view->priv->history && !g_queue_is_empty(connection_view->priv->history))
        while((history_item = (gchar *)g_queue_pop_head(connection_view->priv->history)) != NULL)
            g_free(history_item);

    if(connection_view->priv->history)
        g_queue_free(connection_view->priv->history);

#ifdef ENABLE_GST                               
    if(connection_view->priv->download_queue
            && !g_queue_is_empty(connection_view->priv->download_queue))
        while((item = (MudMSPDownloadItem *)
                    g_queue_pop_head(connection_view->priv->download_queue)) != NULL)
            mud_telnet_msp_download_item_free(item);

    if(connection_view->priv->download_queue)
        g_queue_free(connection_view->priv->download_queue);
#endif

    if(connection_view->connection && 
            gnet_conn_is_connected(connection_view->connection))
        gnet_conn_disconnect(connection_view->connection);

    if(connection_view->connection)
        gnet_conn_unref(connection_view->connection);

    g_free(connection_view->priv);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}

void
mud_connection_view_set_connect_string(MudConnectionView *view, gchar *connect_string)
{
    g_assert(view != NULL);
    view->priv->connect_hook = TRUE;
    view->priv->connect_string = g_strdup(connect_string);
}

void
mud_connection_view_disconnect(MudConnectionView *view)
{
#ifdef ENABLE_GST
    MudMSPDownloadItem *item;
#endif

    g_assert(view != NULL);

    if(view->connection && gnet_conn_is_connected(view->connection))
    {
#ifdef ENABLE_GST
        if(view->priv->download_queue)
            while((item = (MudMSPDownloadItem *)g_queue_pop_head(view->priv->download_queue)) != NULL)
                mud_telnet_msp_download_item_free(item);

        if(view->priv->download_queue)
            g_queue_free(view->priv->download_queue);

        view->priv->download_queue = NULL;
#endif

        view->priv->processed = NULL;

        gnet_conn_disconnect(view->connection);
        gnet_conn_unref(view->connection);
        view->connection = NULL;

        if(view->priv->telnet)
            g_object_unref(view->priv->telnet);

        mud_connection_view_add_text(view, _("\n*** Connection closed.\n"), System);
    }
}

void
mud_connection_view_reconnect(MudConnectionView *view)
{
    gchar *buf, *profile_name, *proxy_host, *version;
    gchar key[2048];
    gchar extra_path[512] = "";
    gboolean use_proxy;
    GConfClient *client;

#ifdef ENABLE_GST
    MudMSPDownloadItem *item;
#endif

    g_assert(view != NULL);

    if(view->connection && gnet_conn_is_connected(view->connection))
    {

#ifdef ENABLE_GST
        while((item = (MudMSPDownloadItem *)
                    g_queue_pop_head(view->priv->download_queue)) != NULL)
            mud_telnet_msp_download_item_free(item);

        if(view->priv->download_queue)
            g_queue_free(view->priv->download_queue);

        view->priv->download_queue = NULL;
#endif

        view->priv->processed = NULL;

        gnet_conn_disconnect(view->connection);
        gnet_conn_unref(view->connection);
        view->connection = NULL;

        g_object_unref(view->priv->telnet);

        mud_connection_view_add_text(view,
                _("\n*** Connection closed.\n"), System);
    }

    view->connection = gnet_conn_new(view->priv->hostname, view->priv->port,
            mud_connection_view_network_event_cb, view);
    gnet_conn_ref(view->connection);
    gnet_conn_set_watch_error(view->connection, TRUE);

    profile_name = mud_profile_get_name(view->priv->profile);

    if (strcmp(profile_name, "Default") != 0)
    {
        g_snprintf(extra_path, 512, "profiles/%s/", profile_name);
    }

    g_snprintf(key, 2048, "/apps/gnome-mud/%s%s", extra_path, "functionality/use_proxy");
    client = gconf_client_get_default();
    use_proxy = gconf_client_get_bool(client, key, NULL);

    g_snprintf(key, 2048, "/apps/gnome-mud/%s%s", extra_path, "functionality/proxy_hostname");
    proxy_host = gconf_client_get_string(client, key, NULL);

    g_snprintf(key, 2048, "/apps/gnome-mud/%s%s", extra_path, "functionality/proxy_version");
    version = gconf_client_get_string(client, key, NULL);

    if(use_proxy)
    {
        if(proxy_host && version)
        {
            gnet_socks_set_enabled(TRUE);
            gnet_socks_set_server(gnet_inetaddr_new(proxy_host,GNET_SOCKS_PORT));
            gnet_socks_set_version((strcmp(version, "4") == 0) ? 4 : 5);
        }
    }
    else
        gnet_socks_set_enabled(FALSE);

    if(proxy_host)
        g_free(proxy_host);

    if(version)
        g_free(version);

#ifdef ENABLE_GST
    view->priv->download_queue = g_queue_new();
#endif

    view->naws_enabled = FALSE;
    view->local_echo = TRUE;

    view->priv->telnet = mud_telnet_new(view,
            view->connection, view->priv->mud_name);

    buf = g_strdup_printf(_("*** Making connection to %s, port %d.\n"),
            view->priv->hostname, view->priv->port);
    mud_connection_view_add_text(view, buf, System);
    g_free(buf);

    gnet_conn_connect(view->connection);
}

void
mud_connection_view_send(MudConnectionView *view, const gchar *data)
{
    GList *commands, *command;
    gchar *text, *encoding, *conv_text;
    const gchar *local_codeset;
    gchar *profile_name;
    GConfClient *client;
    gboolean remote;
    gsize bytes_read, bytes_written;
    GError *error = NULL;

    gchar key[2048];
    gchar extra_path[512] = "";

    if(view->connection && gnet_conn_is_connected(view->connection))
    {
        if(view->local_echo) // Prevent password from being cached.
        {
            gchar *head = g_queue_peek_head(view->priv->history);

            if( (head && strcmp(head, data) != 0 && head[0] != '\n') 
                    || g_queue_is_empty(view->priv->history))
                g_queue_push_head(view->priv->history,
                        (gpointer)g_strdup(data));
        }
        else
            g_queue_push_head(view->priv->history,
                    (gpointer)g_strdup(_("<password removed>")));

        client = gconf_client_get_default();

        g_snprintf(key, 2048, "/apps/gnome-mud/%s%s", extra_path, "functionality/remote_encoding");
        remote = gconf_client_get_bool(client, key, NULL);

        if(view->remote_encode && remote)
            encoding = view->remote_encoding;
        else
        {
            profile_name = mud_profile_get_name(view->priv->profile);

            if (strcmp(profile_name, "Default"))
            {
                g_snprintf(extra_path, 512, "profiles/%s/", profile_name);
            }

            g_snprintf(key, 2048, "/apps/gnome-mud/%s%s", extra_path, "functionality/encoding");
            encoding = gconf_client_get_string(client, key, NULL);
        }

        g_get_charset(&local_codeset);

        view->priv->current_history_index = 0;
        commands = mud_profile_process_commands(view->priv->profile, data);

        for (command = g_list_first(commands); command != NULL; command = command->next)
        {
            text = g_strdup_printf("%s\r\n", (gchar *) command->data);

            conv_text = g_convert(text, -1,
                encoding,
                local_codeset, 
                &bytes_read, &bytes_written, &error);

            if(error)
            {
                conv_text = NULL;
                error = NULL;
            }

            // Give plugins first crack at it.
            mud_window_handle_plugins(view->priv->window, view->priv->id,
                    (gchar *)text, strlen(text), 0);

            if(conv_text == NULL)
                gnet_conn_write(view->connection, text, strlen(text));
            else
                gnet_conn_write(view->connection, conv_text, strlen(conv_text));

            if (view->priv->profile->preferences->EchoText && view->local_echo)
                mud_connection_view_add_text(view, text, Sent);

            if(conv_text != NULL)
                g_free(conv_text);
            g_free(text);
        }

        g_list_free(commands);
    }
}

static void
mud_connection_view_set_terminal_colors(MudConnectionView *view)
{
    vte_terminal_set_colors(VTE_TERMINAL(view->priv->terminal),
            &view->priv->profile->preferences->Foreground,
            &view->priv->profile->preferences->Background,
            view->priv->profile->preferences->Colors, C_MAX);
}

void
mud_connection_view_start_logging(MudConnectionView *view)
{
    mud_log_open(view->priv->log);
}

void
mud_connection_view_stop_logging(MudConnectionView *view)
{
    mud_log_close(view->priv->log);
}

gboolean
mud_connection_view_islogging(MudConnectionView *view)
{
    return mud_log_islogging(view->priv->log);
}

static void
mud_connection_view_set_terminal_scrollback(MudConnectionView *view)
{
    vte_terminal_set_scrollback_lines(VTE_TERMINAL(view->priv->terminal),
            view->priv->profile->preferences->Scrollback);
}

static void
mud_connection_view_set_terminal_scrolloutput(MudConnectionView *view)
{
    if(view->priv->terminal)
        vte_terminal_set_scroll_on_output(VTE_TERMINAL(view->priv->terminal),
                view->priv->profile->preferences->ScrollOnOutput);
}

static void
mud_connection_view_set_terminal_font(MudConnectionView *view)
{
    PangoFontDescription *desc = NULL;
    char *name;

    name = view->priv->profile->preferences->FontName;

    if(name)
        desc = pango_font_description_from_string(name);

    if(!desc)
        desc = pango_font_description_from_string("Monospace 10");

    vte_terminal_set_font(VTE_TERMINAL(view->priv->terminal), desc);
}

static void
mud_connection_view_set_terminal_type(MudConnectionView *view)
{
    vte_terminal_set_emulation(VTE_TERMINAL(view->priv->terminal),
            view->priv->profile->preferences->TerminalType);
}

static void
mud_connection_view_profile_changed_cb(MudProfile *profile, MudProfileMask *mask, MudConnectionView *view)
{
    if (mask->ScrollOnOutput)
        mud_connection_view_set_terminal_scrolloutput(view);
    if (mask->TerminalType)
        mud_connection_view_set_terminal_type(view);
    if (mask->Scrollback)
        mud_connection_view_set_terminal_scrollback(view);
    if (mask->FontName)
        mud_connection_view_set_terminal_font(view);
    if (mask->Foreground || mask->Background || mask->Colors)
        mud_connection_view_set_terminal_colors(view);
}

static gboolean
mud_connection_view_button_press_event(GtkWidget *widget, GdkEventButton *event, MudConnectionView *view)
{
    if ((event->button == 3) &&
            !(event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK)))
    {
        mud_connection_view_popup(view, event);
        return TRUE;
    }

    return FALSE;
}

static void
mud_connection_view_popup(MudConnectionView *view, GdkEventButton *event)
{
    GtkWidget *im_menu;
    GtkWidget *menu_item;
    GtkWidget *profile_menu;
    const GList *profiles;
    const GList *profile;
    GSList *group;

    if (view->priv->popup_menu)
        gtk_widget_destroy(view->priv->popup_menu);

    g_assert(view->priv->popup_menu == NULL);

    view->priv->popup_menu = gtk_menu_new();
    gtk_menu_attach_to_widget(GTK_MENU(view->priv->popup_menu),
            view->priv->terminal,
            popup_menu_detach);

    append_menuitem(view->priv->popup_menu,
            _("Close"),
            G_CALLBACK(mud_connection_view_close_current_cb),
            view);

    menu_item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(view->priv->popup_menu), menu_item);

    append_stock_menuitem(view->priv->popup_menu,
            GTK_STOCK_COPY,
            NULL,
            view);
    append_stock_menuitem(view->priv->popup_menu,
            GTK_STOCK_PASTE,
            NULL,
            view);

    menu_item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(view->priv->popup_menu), menu_item);

    profile_menu = gtk_menu_new();
    menu_item = gtk_menu_item_new_with_mnemonic(_("Change P_rofile"));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), profile_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(view->priv->popup_menu), menu_item);

    group = NULL;
    profiles = mud_profile_get_profiles();
    profile = profiles;
    while (profile != NULL)
    {
        MudProfile *prof;

        prof = profile->data;

        /* Profiles can go away while the menu is up. */
        g_object_ref(G_OBJECT(prof));

        menu_item = gtk_radio_menu_item_new_with_label(group,
                prof->name);
        group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
        gtk_menu_shell_append(GTK_MENU_SHELL(profile_menu), menu_item);

        if (prof == view->priv->profile)
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), TRUE);

        g_signal_connect(G_OBJECT(menu_item),
                "toggled",
                G_CALLBACK(choose_profile_callback),
                view);
        g_object_set_data_full(G_OBJECT(menu_item),
                "profile",
                prof,
                (GDestroyNotify) g_object_unref);
        profile = profile->next;
    }

    menu_item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(view->priv->popup_menu), menu_item);

    im_menu = gtk_menu_new();
    menu_item = gtk_menu_item_new_with_mnemonic(_("_Input Methods"));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), im_menu);
    vte_terminal_im_append_menuitems(VTE_TERMINAL(view->priv->terminal),
            GTK_MENU_SHELL(im_menu));
    gtk_menu_shell_append(GTK_MENU_SHELL(view->priv->popup_menu), menu_item);

    gtk_widget_show_all(view->priv->popup_menu);
    gtk_menu_popup(GTK_MENU(view->priv->popup_menu),
            NULL, NULL,
            NULL, NULL,
            event ? event->button : 0,
            event ? event->time : gtk_get_current_event_time());
}

void
mud_connection_view_set_id(MudConnectionView *view, gint id)
{
    view->priv->id = id;
}

void
mud_connection_view_set_parent(MudConnectionView *view, MudWindow *window)
{
    view->priv->window = window;
}


MudConnectionView*
mud_connection_view_new (const gchar *profile, const gchar *hostname,
        const gint port, GtkWidget *window, GtkWidget *tray, gchar *name)
{
    gchar *profile_name;
    GConfClient *client;

    gchar key[2048];
    gchar extra_path[512] = "";
    gboolean use_proxy;
    gchar *proxy_host;
    gchar *version;

    MudConnectionView *view;
    GdkGeometry hints;
    gint xpad, ypad;
    gint char_width, char_height;
    gchar *buf;

    g_assert(hostname != NULL);
    g_assert(port > 0);

    view = g_object_new(MUD_TYPE_CONNECTION_VIEW, NULL);

    view->priv->hostname = g_strdup(hostname);
    view->priv->port = port;

    view->connection = gnet_conn_new(hostname, port,
            mud_connection_view_network_event_cb, view);
    gnet_conn_ref(view->connection);
    gnet_conn_set_watch_error(view->connection, TRUE);

    view->naws_enabled = FALSE;

    view->priv->telnet = mud_telnet_new(view, view->connection, name);

    view->local_echo = TRUE;

    mud_connection_view_set_profile(view, mud_profile_new(profile));

    /* Let us resize the gnome-mud window */
    vte_terminal_get_padding(VTE_TERMINAL(view->priv->terminal), &xpad, &ypad);
    char_width = VTE_TERMINAL(view->priv->terminal)->char_width;
    char_height = VTE_TERMINAL(view->priv->terminal)->char_height;

    hints.base_width = xpad;
    hints.base_height = ypad;
    hints.width_inc = char_width;
    hints.height_inc = char_height;

    hints.min_width =  hints.base_width + hints.width_inc * 4;
    hints.min_height = hints.base_height+ hints.height_inc * 2;

    gtk_window_set_geometry_hints(GTK_WINDOW(window),
            GTK_WIDGET(view->priv->terminal),
            &hints,
            GDK_HINT_RESIZE_INC |
            GDK_HINT_MIN_SIZE |
            GDK_HINT_BASE_SIZE);

    view->priv->tray = MUD_TRAY(tray);

    view->priv->log = mud_log_new(name);

    buf = g_strdup_printf(_("*** Making connection to %s, port %d.\n"),
            view->priv->hostname, view->priv->port);
    mud_connection_view_add_text(view, buf, System);
    g_free(buf);
    buf = NULL;

    profile_name = mud_profile_get_name(view->priv->profile);

    if (strcmp(profile_name, "Default") != 0)
    {
        g_snprintf(extra_path, 512, "profiles/%s/", profile_name);
    }

    g_snprintf(key, 2048, "/apps/gnome-mud/%s%s", extra_path, "functionality/use_proxy");
    client = gconf_client_get_default();
    use_proxy = gconf_client_get_bool(client, key, NULL);

    g_snprintf(key, 2048, "/apps/gnome-mud/%s%s", extra_path, "functionality/proxy_hostname");
    proxy_host = gconf_client_get_string(client, key, NULL);

    g_snprintf(key, 2048, "/apps/gnome-mud/%s%s", extra_path, "functionality/proxy_version");
    version = gconf_client_get_string(client, key, NULL);

    if(use_proxy)
    {
        if(proxy_host && version)
        {
            gnet_socks_set_enabled(TRUE);
            gnet_socks_set_server(gnet_inetaddr_new(proxy_host,GNET_SOCKS_PORT));
            gnet_socks_set_version((strcmp(version, "4") == 0) ? 4 : 5);
        }
    }
    else
        gnet_socks_set_enabled(FALSE);

    if(proxy_host)
        g_free(proxy_host);

    if(version)
        g_free(version);

    gnet_conn_connect(view->connection);

    return view;
}

void
mud_connection_view_set_profile(MudConnectionView *view, MudProfile *profile)
{
    if (profile == view->priv->profile)
        return;

    if (view->priv->profile)
    {
        g_signal_handler_disconnect(view->priv->profile, view->priv->signal_profile_changed);
        g_object_unref(G_OBJECT(view->priv->profile));
    }

    view->priv->profile = profile;
    g_object_ref(G_OBJECT(profile));
    view->priv->signal_profile_changed =
        g_signal_connect(G_OBJECT(view->priv->profile), "changed",
                         G_CALLBACK(mud_connection_view_profile_changed_cb),
                         view);
    mud_connection_view_reread_profile(view);
}

MudProfile *
mud_connection_view_get_current_profile(MudConnectionView *view)
{
    return view->priv->profile;
}

GtkWidget *
mud_connection_view_get_viewport (MudConnectionView *view)
{
    return view->priv->box;
}

GtkWidget *
mud_connection_view_get_terminal(MudConnectionView *view)
{
    return view->priv->terminal;
}

MudParseBase *
mud_connection_view_get_parsebase(MudConnectionView *view)
{
    return view->priv->parse;
}

gchar *
mud_connection_view_get_history_item(MudConnectionView *view, enum
                                     MudConnectionHistoryDirection direction)
{
    gchar *history_item;

    if(direction == HISTORY_DOWN)
        if(view->priv->current_history_index != 0)
            view->priv->current_history_index--;

    history_item = (gchar *)g_queue_peek_nth(view->priv->history,
            view->priv->current_history_index);

    if(direction == HISTORY_UP)
        if(view->priv->current_history_index <
                g_queue_get_length(view->priv->history) - 1)
            view->priv->current_history_index++;

    return history_item;
}

static void
mud_connection_view_network_event_cb(GConn *conn, GConnEvent *event, gpointer pview)
{
    gint gag;
    gint pluggag;
    gchar *buf;
    gboolean temp;
    MudConnectionView *view = MUD_CONNECTION_VIEW(pview);
    gint length;

#ifdef ENABLE_GST
    MudMSPDownloadItem *item;
#endif

    g_assert(view != NULL);

    switch(event->type)
    {
        case GNET_CONN_ERROR:
            mud_connection_view_add_text(view, _("*** Could not connect.\n"), Error);
            break;

        case GNET_CONN_CONNECT:
            mud_connection_view_add_text(view, _("*** Connected.\n"), System);
            gnet_conn_read(view->connection);
            break;

        case GNET_CONN_CLOSE:
#ifdef ENABLE_GST
            if(view->priv->download_queue)
                while((item = (MudMSPDownloadItem *)g_queue_pop_head(view->priv->download_queue)) != NULL)
                    mud_telnet_msp_download_item_free(item);

            if(view->priv->download_queue)
                g_queue_free(view->priv->download_queue);

            view->priv->download_queue = NULL;
#endif

            view->priv->processed = NULL;

            gnet_conn_disconnect(view->connection);
            gnet_conn_unref(view->connection);
            view->connection = NULL;

            if(view->priv->telnet)
                g_object_unref(view->priv->telnet);

            mud_connection_view_add_text(view, _("*** Connection closed.\n"), Error);
            break;

        case GNET_CONN_TIMEOUT:
            break;

        case GNET_CONN_READ:
            if(!view->priv->connected)
            {
                view->priv->connected = TRUE;
                mud_tray_update_icon(view->priv->tray, online);
            }

            view->priv->processed =
                mud_telnet_process(view->priv->telnet, (guchar *)event->buffer,
                        event->length, &length);

            if(view->priv->processed != NULL)
            {
#ifdef ENABLE_GST
                if(view->priv->telnet->msp_parser.enabled)
                {
                    view->priv->processed = mud_telnet_msp_parse(
                            view->priv->telnet, view->priv->processed, &length);
                }
#endif
                if(view->priv->processed != NULL)
                {
#ifdef ENABLE_GST
                    mud_telnet_msp_parser_clear(view->priv->telnet);
#endif 
                    buf = view->priv->processed->str;

                    temp = view->local_echo;
                    view->local_echo = FALSE;
                    gag = mud_parse_base_do_triggers(view->priv->parse,
                            buf);
                    view->local_echo = temp;

                    mud_window_handle_plugins(view->priv->window, view->priv->id,
                            buf, length, 1);

                    pluggag = PluginGag;
                    PluginGag = FALSE;

                    if(!gag && !pluggag)
                    {
                        vte_terminal_feed(VTE_TERMINAL(view->priv->terminal),
                                buf, length);
                        mud_log_write_hook(view->priv->log, buf, length);
                    }

                    if (view->priv->connect_hook) {
                        mud_connection_view_send (view, view->priv->connect_string);
                        view->priv->connect_hook = FALSE;
                    }

                    if(view->priv->processed != NULL)
                    {
                        g_string_free(view->priv->processed, TRUE);
                        view->priv->processed = NULL;
                    }

                    buf = NULL;
                }
            }

            gnet_conn_read(view->connection);
            break;

        case GNET_CONN_WRITE:
            break;

        case GNET_CONN_READABLE:
            break;

        case GNET_CONN_WRITABLE:
            break;
    }
}

void
mud_connection_view_get_term_size(MudConnectionView *view, gint *w, gint *h)
{
    VteTerminal *term = VTE_TERMINAL(view->priv->terminal);
    *w = term->column_count;
    *h = term->row_count;
}

void
mud_connection_view_set_naws(MudConnectionView *view, gint enabled)
{
    view->naws_enabled = enabled;
}

void
mud_connection_view_send_naws(MudConnectionView *view)
{
    if(view && view->connection 
            && gnet_conn_is_connected(view->connection) 
            && view->naws_enabled)
    {
        guint curr_width = VTE_TERMINAL(view->priv->terminal)->column_count;
        guint curr_height = VTE_TERMINAL(view->priv->terminal)->row_count;

        if(curr_width != view->priv->width || curr_height != view->priv->height)
            mud_telnet_send_naws(view->priv->telnet, curr_width, curr_height);

        view->priv->width = curr_width;
        view->priv->height = curr_height;
    }
}

#ifdef ENABLE_GST
static void
mud_connection_view_start_download(MudConnectionView *view)
{
    MudMSPDownloadItem *item;

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(view->priv->progressbar), 0.0);
    gtk_label_set_text(GTK_LABEL(view->priv->dl_label), _("Connecting..."));
    gtk_widget_show(view->priv->progressbar);
    gtk_widget_show(view->priv->dl_label);
    gtk_widget_show(view->priv->dl_button);

    if(!view->priv->downloading && view->priv->dl_conn)
        gnet_conn_http_delete(view->priv->dl_conn);

    item = g_queue_peek_head(view->priv->download_queue);
    view->priv->dl_conn = gnet_conn_http_new();
    gnet_conn_http_set_uri(view->priv->dl_conn, item->url);
    gnet_conn_http_set_user_agent (view->priv->dl_conn, "gnome-mud");
    // 30 minute timeout, if the file didn't download in 30 minutes, its not going to happen.
    gnet_conn_http_set_timeout(view->priv->dl_conn, 1800000);

    view->priv->downloading = TRUE;

    gnet_conn_http_run_async(view->priv->dl_conn,
            mud_connection_view_http_cb, view);
}

void
mud_connection_view_queue_download(MudConnectionView *view, gchar *url, gchar *file)
{
    MudMSPDownloadItem *item;
    guint i, size;
    GConfClient *client;
    gboolean download;

    gchar key[2048];
    gchar extra_path[512] = "";

    client = gconf_client_get_default();

    g_snprintf(key, 2048, "/apps/gnome-mud/%s%s", extra_path, "functionality/remote_download");
    download = gconf_client_get_bool(client, key, NULL);

    if(download)
    {
        size = g_queue_get_length(view->priv->download_queue);

        for(i = 0; i < size; ++i) // Don't add items twice.
        {
            item = (MudMSPDownloadItem *)g_queue_peek_nth(view->priv->download_queue, i);

            if(strcmp(item->url, url) == 0)
                return;
        }

        item = NULL;
        item = g_malloc(sizeof(MudMSPDownloadItem));

        item->url = g_strdup(url);
        item->file = g_strdup(file);

        g_queue_push_tail(view->priv->download_queue, item);

        item = NULL;

        if(view->priv->downloading == FALSE)
            mud_connection_view_start_download(view);
    }
}

static void
mud_connection_view_http_cb(GConnHttp *conn, GConnHttpEvent *event, gpointer data)
{
    MudConnectionView *view = (MudConnectionView *)data;
    MudMSPDownloadItem *item;
    gchar **uri;
    GString *file_name;
    GConnHttpEventData *event_data;

    switch(event->type)
    {
        case GNET_CONN_HTTP_CONNECTED:
            break;

        case GNET_CONN_HTTP_DATA_PARTIAL:
            event_data = (GConnHttpEventData *)event;

            if(event_data->content_length == 0)
                gtk_progress_bar_pulse(GTK_PROGRESS_BAR(view->priv->progressbar));
            else
                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(view->priv->progressbar),
                        (gdouble)((gdouble)event_data->data_received / (gdouble)event_data->content_length));
            break;

        case GNET_CONN_HTTP_DATA_COMPLETE:
            event_data = (GConnHttpEventData *)event;

            gtk_widget_hide(view->priv->progressbar);
            gtk_widget_hide(view->priv->dl_label);
            gtk_widget_hide(view->priv->dl_button);

            item = g_queue_pop_head(view->priv->download_queue);

            g_file_set_contents(item->file, event_data->buffer,
                    event_data->buffer_length, NULL);

            mud_telnet_msp_download_item_free(item);
            view->priv->downloading = FALSE;

            if(!g_queue_is_empty(view->priv->download_queue))
                mud_connection_view_start_download(view);
            break;

        case GNET_CONN_HTTP_TIMEOUT:
            if(!view->priv->downloading)
                break;

            gtk_widget_hide(view->priv->progressbar);
            gtk_widget_hide(view->priv->dl_label);
            gtk_widget_hide(view->priv->dl_button);

            g_warning(_("Connection timed out."));

            item = g_queue_pop_head(view->priv->download_queue);
            mud_telnet_msp_download_item_free(item);

            view->priv->downloading = FALSE;

            if(!g_queue_is_empty(view->priv->download_queue))
                mud_connection_view_start_download(view);
            break;

        case GNET_CONN_HTTP_ERROR:
            gtk_widget_hide(view->priv->progressbar);
            gtk_widget_hide(view->priv->dl_label);
            gtk_widget_hide(view->priv->dl_button);

            g_warning(_("There was an internal http connection error."));

            item = g_queue_pop_head(view->priv->download_queue);
            mud_telnet_msp_download_item_free(item);

            view->priv->downloading = FALSE;

            if(!g_queue_is_empty(view->priv->download_queue))
                mud_connection_view_start_download(view);

            break;

        case GNET_CONN_HTTP_RESOLVED:
            break;

        case GNET_CONN_HTTP_RESPONSE:
            item = g_queue_peek_head(view->priv->download_queue);

            uri = g_strsplit(item->url, "/", 0);

            file_name = g_string_new(NULL);

            g_string_append(file_name, _("Downloading"));
            g_string_append_c(file_name, ' ');
            g_string_append(file_name, uri[g_strv_length(uri) - 1]);
            g_string_append(file_name, "...");

            gtk_label_set_text(GTK_LABEL(view->priv->dl_label), file_name->str);

            g_string_free(file_name, TRUE);
            g_strfreev(uri);
            break;

        case GNET_CONN_HTTP_REDIRECT:
            break;
    }
}

static void
mud_connection_view_cancel_dl_cb(GtkWidget *widget, MudConnectionView *view)
{
    MudMSPDownloadItem *item;

    gtk_widget_hide(view->priv->progressbar);
    gtk_widget_hide(view->priv->dl_label);
    gtk_widget_hide(view->priv->dl_button);

    if(view->priv->dl_conn)
    {
        gnet_conn_http_delete(view->priv->dl_conn);
        view->priv->dl_conn = NULL;
    }

    item = g_queue_pop_head(view->priv->download_queue);
    mud_telnet_msp_download_item_free(item);

    view->priv->downloading = FALSE;

    if(!g_queue_is_empty(view->priv->download_queue))
        mud_connection_view_start_download(view);
}
#endif
