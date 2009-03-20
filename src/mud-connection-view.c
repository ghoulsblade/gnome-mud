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
#include "mud-subwindow.h"

#include "handlers/mud-telnet-handlers.h"

struct _MudConnectionViewPrivate
{
    GtkWidget *scrollbar;
    GtkWidget *popup_menu;
    GtkWidget *progressbar;
    GtkWidget *dl_label;
    GtkWidget *dl_button;

    GString *processed;
    
    gulong signal;
    gulong signal_profile_changed;

    GQueue *history;
    gint current_history_index;

    gchar *current_output;
    GList *subwindows;

#ifdef ENABLE_GST
    GQueue *download_queue;
    GConnHttp *dl_conn;
    gboolean downloading;
#endif
};

/* Property Identifiers */
enum
{
    PROP_MUD_CONNECTION_VIEW_0,
    PROP_CONNECTION,
    PROP_LOCAL_ECHO,
    PROP_REMOTE_ENCODE,
    PROP_CONNECT_HOOK,
    PROP_CONNECTED,
    PROP_CONNECT_STRING,
    PROP_REMOTE_ENCODING,
    PROP_PORT,
    PROP_MUD_NAME,
    PROP_HOSTNAME,
    PROP_LOG,
    PROP_TRAY,
    PROP_PROFILE,
    PROP_PARSE_BASE,
    PROP_TELNET,
    PROP_WINDOW,
    PROP_PROFILE_NAME,
    PROP_LOGGING,
    PROP_TERMINAL,
    PROP_VBOX
};

/* Create the Type */
G_DEFINE_TYPE(MudConnectionView, mud_connection_view, G_TYPE_OBJECT);

/* Class Functions */
static void mud_connection_view_init (MudConnectionView *connection_view);
static void mud_connection_view_class_init (MudConnectionViewClass *klass);
static void mud_connection_view_finalize (GObject *object);

static GObject *mud_connection_view_constructor(GType gtype,
                                                guint n_properties,
                                                GObjectConstructParam *props);

static void mud_connection_view_set_property(GObject *object,
                                             guint prop_id,
                                             const GValue *value,
                                             GParamSpec *pspec);

static void mud_connection_view_get_property(GObject *object,
                                             guint prop_id,
                                             GValue *value,
                                             GParamSpec *pspec);

/* Callbacks */
static void mud_connection_view_profile_changed_cb (MudProfile *profile, 
                                                    MudProfileMask *mask,
                                                    MudConnectionView *view);
static gboolean mud_connection_view_button_press_event(GtkWidget *widget, 
                                                       GdkEventButton *event,
                                                       MudConnectionView *view);
static void mud_connection_view_popup(MudConnectionView *view, 
                                      GdkEventButton *event);

static void popup_menu_detach(GtkWidget *widget, GtkMenu *menu);
static void mud_connection_view_network_event_cb(GConn *conn, 
                                                 GConnEvent *event,
                                                 gpointer data);
static void mud_connection_view_close_current_cb(GtkWidget *menu_item,
                                                 MudConnectionView *view);
static void mud_connection_view_profile_changed_cb(MudProfile *profile,
                                                   MudProfileMask *mask,
                                                   MudConnectionView *view);


/* Private Methods */
static void mud_connection_view_set_terminal_colors(MudConnectionView *view);
static void mud_connection_view_set_terminal_scrollback(MudConnectionView *view);
static void mud_connection_view_set_terminal_scrolloutput(MudConnectionView *view);
static void mud_connection_view_set_terminal_font(MudConnectionView *view);
static GtkWidget* append_stock_menuitem(GtkWidget *menu, 
                                        const gchar *text,
                                        GCallback callback,
                                        gpointer data);
static GtkWidget* append_menuitem(GtkWidget *menu,
                                  const gchar *text,
                                  GCallback callback,
                                  gpointer data);
static void choose_profile_callback(GtkWidget *menu_item,
                                    MudConnectionView *view);
static void mud_connection_view_reread_profile(MudConnectionView *view);
static void mud_connection_view_feed_text(MudConnectionView *view,
                                          gchar *message);
static void mud_connection_view_set_size_force_grid (MudConnectionView *window,
                                                     VteTerminal *screen,
                                                     gboolean        even_if_mapped,
                                                     int             force_grid_width,
                                                     int             force_grid_height);

static void mud_connection_view_update_geometry (MudConnectionView *window);

#ifdef ENABLE_GST
static void mud_connection_view_http_cb(GConnHttp *conn,
                                        GConnHttpEvent *event,
                                        gpointer data);
static void mud_connection_view_cancel_dl_cb(GtkWidget *widget,
                                             MudConnectionView *view);
#endif

/* Class Functions */
static void
mud_connection_view_class_init (MudConnectionViewClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /* Override base object constructor */
    object_class->constructor = mud_connection_view_constructor;

    /* Override base object's finalize */
    object_class->finalize = mud_connection_view_finalize;

    /* Override base object property methods */
    object_class->set_property = mud_connection_view_set_property;
    object_class->get_property = mud_connection_view_get_property;

    /* Add private data to class */
    g_type_class_add_private(klass, sizeof(MudConnectionViewPrivate));

    /* Create and install properties */
    
    // Required construction properties
    g_object_class_install_property(object_class,
            PROP_CONNECT_STRING,
            g_param_spec_string("connect-string",
                "connect string",
                "string to send to the mud on connect",
                NULL,
                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property(object_class,
            PROP_PORT,
            g_param_spec_int("port",
                "port",
                "the port of the mud",
                1, 200000,
                23,
                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property(object_class,
            PROP_MUD_NAME,
            g_param_spec_string("mud-name",
                "mud name",
                "the name of the mud",
                NULL,
                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property(object_class,
            PROP_HOSTNAME,
            g_param_spec_string("hostname",
                "hostname",
                "the host of the mud",
                NULL,
                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property(object_class,
            PROP_WINDOW,
            g_param_spec_object("window",
                "window",
                "the parent mud window object",
                MUD_TYPE_WINDOW,
                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property(object_class,
            PROP_PROFILE_NAME,
            g_param_spec_string("profile-name",
                "profile name",
                "the name of the current profile",
                NULL,
                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    // Setable properties
    g_object_class_install_property(object_class,
            PROP_REMOTE_ENCODE,
            g_param_spec_boolean("remote-encode",
                "remote encode",
                "do we accept encoding negotiation",
                FALSE,
                G_PARAM_READWRITE));

    g_object_class_install_property(object_class,
            PROP_REMOTE_ENCODING,
            g_param_spec_string("remote-encoding",
                "remote encoding",
                "the negotiated encoding of the terminal",
                NULL,
                G_PARAM_READWRITE));

    g_object_class_install_property(object_class,
            PROP_LOCAL_ECHO,
            g_param_spec_boolean("local-echo",
                "local echo",
                "do we display the text sent to the mud",
                TRUE,
                G_PARAM_READWRITE));

    // Readable Properties
    g_object_class_install_property(object_class,
            PROP_TRAY,
            g_param_spec_object("tray",
                "mud tray",
                "mud status tray icon",
                MUD_TYPE_TRAY,
                G_PARAM_READABLE));

    g_object_class_install_property(object_class,
            PROP_CONNECTION,
            g_param_spec_pointer("connection",
                "connection",
                "the connection to the mud",
                G_PARAM_READABLE));

    g_object_class_install_property(object_class,
            PROP_LOGGING,
            g_param_spec_boolean("logging",
                "logging",
                "are we currently logging",
                FALSE,
                G_PARAM_READABLE));

    g_object_class_install_property(object_class,
            PROP_CONNECT_HOOK,
            g_param_spec_boolean("connect-hook",
                "connect hook",
                "do we need to send the connection string",
                FALSE,
                G_PARAM_READABLE));
    
    g_object_class_install_property(object_class,
            PROP_CONNECTED,
            g_param_spec_boolean("connected",
                "connected",
                "are we connected to the mud",
                FALSE,
                G_PARAM_READABLE));

    g_object_class_install_property(object_class,
            PROP_LOG,
            g_param_spec_object("log",
                "log",
                "the mud log object",
                MUD_TYPE_LOG,
                G_PARAM_READABLE));

    g_object_class_install_property(object_class,
            PROP_PROFILE,
            g_param_spec_object("profile",
                "profile",
                "the mud profile object",
                MUD_TYPE_PROFILE,
                G_PARAM_READABLE));

    g_object_class_install_property(object_class,
            PROP_TELNET,
            g_param_spec_object("telnet",
                "telnet",
                "the mud telnet object",
                MUD_TYPE_TELNET,
                G_PARAM_READABLE));

    g_object_class_install_property(object_class,
            PROP_PARSE_BASE,
            g_param_spec_object("parse-base",
                "parse base",
                "the parse base object",
                MUD_TYPE_PARSE_BASE,
                G_PARAM_READABLE));

    g_object_class_install_property(object_class,
            PROP_TERMINAL,
            g_param_spec_object("terminal",
                "terminal",
                "the terminal widget",
                VTE_TYPE_TERMINAL,
                G_PARAM_READABLE));

    g_object_class_install_property(object_class,
            PROP_VBOX,
            g_param_spec_object("ui-vbox",
                "ui vbox",
                "main ui vbox",
                GTK_TYPE_VBOX,
                G_PARAM_READABLE));
}

static void
mud_connection_view_init (MudConnectionView *self)
{
    /* Get our private data */
    self->priv = MUD_CONNECTION_VIEW_GET_PRIVATE(self);

    /* Set some defaults */
    self->priv->history = g_queue_new();
    self->priv->current_history_index = 0;

    self->priv->processed = NULL;

    self->connection = NULL;

    self->local_echo = TRUE;
    self->remote_encode = FALSE;   
    self->connect_hook = FALSE;
    self->connected = FALSE;;

    self->connect_string = NULL;
    self->remote_encoding = NULL;

    self->port = 23;
    self->mud_name = NULL;
    self->hostname = NULL;

    self->log = NULL;
    self->tray = NULL;
    self->profile = NULL;
    self->parse = NULL;
    self->window = NULL;
    self->telnet = NULL;

    self->terminal = NULL;
    self->ui_vbox = NULL;

    self->priv->subwindows = NULL;
    self->priv->current_output = g_strdup("main");
}

static GObject *
mud_connection_view_constructor (GType gtype,
                                 guint n_properties,
                                 GObjectConstructParam *properties)
{
    GtkWidget *box;
    GtkWidget *dl_vbox;
    GtkWidget *dl_hbox;
    GtkWidget *term_box;
    GtkWidget *main_window;
    MudTray *tray;
    GConfClient *client;

    gchar key[2048];
    gchar extra_path[512] = "";
    gchar *buf;
    gchar *proxy_host;
    gchar *version;
    gboolean use_proxy;
    
    MudConnectionView *self;
    GObject *obj;

    MudConnectionViewClass *klass;
    GObjectClass *parent_class;

    /* Chain up to parent constructor */
    klass = MUD_CONNECTION_VIEW_CLASS( g_type_class_peek(MUD_TYPE_CONNECTION_VIEW) );
    parent_class = G_OBJECT_CLASS( g_type_class_peek_parent(klass) );
    obj = parent_class->constructor(gtype, n_properties, properties);

    /* Construct the View */
    self = MUD_CONNECTION_VIEW(obj);

    /* Check for construction properties */
    if(!self->mud_name)
    {
        g_printf("ERROR: Tried to instantiate MudConnectionView without passing mud-name.\n");
        g_error("Tried to instantiate MudConnectionView without passing mud-name.");
    }
    
    if(!self->hostname)
    {
        g_printf("ERROR: Tried to instantiate MudConnectionView without passing hostname.\n");
        g_error("Tried to instantiate MudConnectionView without passing hostname.");
    }

    if(!self->window)
    {
        g_printf("ERROR: Tried to instantiate MudConnectionView without passing parent MudWindow.\n");
        g_error("Tried to instantiate MudConnectionView without passing parent MudWindow.");
    }

    /* Create main VBox, VTE, and scrollbar */
    box = gtk_vbox_new(FALSE, 0);
    self->ui_vbox = GTK_VBOX(box);
    self->terminal = VTE_TERMINAL(vte_terminal_new());
    self->priv->scrollbar = gtk_vscrollbar_new(NULL);
    term_box = gtk_hbox_new(FALSE, 0);

#ifdef ENABLE_GST
    /* Setup Download UI */
    dl_vbox = gtk_vbox_new(FALSE, 0);
    dl_hbox = gtk_hbox_new(FALSE, 0);

    self->priv->dl_label = gtk_label_new("Downloading...");
    self->priv->progressbar = gtk_progress_bar_new();
    gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR(self->priv->progressbar), 0.1);
    self->priv->dl_button = gtk_button_new_from_stock("gtk-cancel");

    /* Pack the Download UI */
    gtk_box_pack_start(GTK_BOX(dl_vbox), self->priv->dl_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(dl_hbox), self->priv->progressbar, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(dl_hbox), self->priv->dl_button, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(dl_vbox), dl_hbox, TRUE, TRUE, 0);

    /* Pack into Main UI */
    gtk_box_pack_start(GTK_BOX(box), dl_vbox, FALSE, FALSE, 0);

    /* Set defaults and create download queue */
    self->priv->downloading = FALSE;
    self->priv->download_queue = g_queue_new();
    self->priv->dl_conn = NULL;

    /* Connect Download Cancel Signal */
    g_signal_connect(self->priv->dl_button,
                     "clicked",
                     G_CALLBACK(mud_connection_view_cancel_dl_cb),
                     self);
#endif

    /* Pack the Terminal UI */
    gtk_box_pack_start(GTK_BOX(term_box),
                       GTK_WIDGET(self->terminal),
                       TRUE,
                       TRUE,
                       0);

    gtk_box_pack_end(GTK_BOX(term_box),
                     self->priv->scrollbar, 
                     FALSE,
                     FALSE,
                     0);

    /* Pack into Main UI */
    gtk_box_pack_end(GTK_BOX(box), term_box, TRUE, TRUE, 0);

    /* Connect signals and set data */
    g_signal_connect(G_OBJECT(self->terminal),
                     "button_press_event",
                     G_CALLBACK(mud_connection_view_button_press_event),
                     self);

    g_object_set_data(G_OBJECT(self->terminal),
                      "connection-view",
                      self);
    g_object_set_data(G_OBJECT(box),
                      "connection-view",
                      self);

    /* Setup scrollbar */
    gtk_range_set_adjustment(
            GTK_RANGE(self->priv->scrollbar),
            self->terminal->adjustment);

    /* Setup VTE */
    vte_terminal_set_encoding(self->terminal, "ISO-8859-1");
    vte_terminal_set_emulation(self->terminal, "xterm");

    g_object_get(self->window,
                 "window", &main_window,
                 "tray", &tray,
                 NULL);

    self->connection = gnet_conn_new(self->hostname, self->port,
            mud_connection_view_network_event_cb, self);
    gnet_conn_ref(self->connection);
    gnet_conn_set_watch_error(self->connection, TRUE);

    self->telnet = g_object_new(MUD_TYPE_TELNET,
                                "parent-view", self,
                                NULL);

    mud_connection_view_set_profile(self, mud_profile_new(self->profile_name));

    self->tray = tray;

    self->parse = g_object_new(MUD_TYPE_PARSE_BASE,
                               "parent-view", self,
                               NULL);

    self->log = g_object_new(MUD_TYPE_LOG,
                             "mud-name", self->mud_name,
                              NULL);

    buf = g_strdup_printf(_("*** Making connection to %s, port %d.\n"),
            self->hostname, self->port);
    mud_connection_view_add_text(self, buf, System);
    g_free(buf);
    buf = NULL;

    if (strcmp(self->profile_name, "Default") != 0)
    {
        g_snprintf(extra_path, 512, "profiles/%s/", self->profile_name);
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

    g_object_unref(client);

    /* Show everything */
    gtk_widget_show_all(box);

    mud_connection_view_update_geometry (self);

#ifdef ENABLE_GST
    /* Hide UI until download starts */
    gtk_widget_hide(self->priv->progressbar);
    gtk_widget_hide(self->priv->dl_label);
    gtk_widget_hide(self->priv->dl_button);
#endif

    gnet_conn_connect(self->connection);

    return obj;
}

static void
mud_connection_view_finalize (GObject *object)
{
    MudConnectionView *connection_view;
    GObjectClass *parent_class;
    gchar *history_item;
    GList *entry;

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

    if(connection_view->hostname)
        g_free(connection_view->hostname);

    if(connection_view->connect_string)
        g_free(connection_view->connect_string);
    
    if(connection_view->mud_name)
        g_free(connection_view->mud_name);
    
    if(connection_view->priv->processed)
        g_string_free(connection_view->priv->processed, TRUE);
    
    if(connection_view->telnet)
        g_object_unref(connection_view->telnet);
  
    g_object_unref(connection_view->log);
    g_object_unref(connection_view->parse);
    g_object_unref(connection_view->profile);

    entry = g_list_first(connection_view->priv->subwindows);

    while(entry)
    {
        MudSubwindow *sub = MUD_SUBWINDOW(entry->data);

        g_object_unref(sub);

        entry = g_list_next(entry);
    }

    g_list_free(connection_view->priv->subwindows);

    parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
    parent_class->finalize(object);
}


static void
mud_connection_view_set_property(GObject *object,
                                 guint prop_id,
                                 const GValue *value,
                                 GParamSpec *pspec)
{
    MudConnectionView *self;
    gboolean new_boolean;
    gint new_int;
    gchar *new_string;

    self = MUD_CONNECTION_VIEW(object);

    switch(prop_id)
    {
        case PROP_REMOTE_ENCODE:
            new_boolean = g_value_get_boolean(value);

            if(new_boolean != self->remote_encode)
                self->remote_encode = new_boolean;
            break;

        case PROP_CONNECT_STRING:
            new_string = g_value_dup_string(value);

            if(!self->connect_string)
                self->connect_string = 
                    (new_string) ? g_strdup(new_string) : NULL;
            else if( strcmp(self->connect_string, new_string) != 0)
            {
                if(self->connect_string)
                    g_free(self->connect_string);
                self->connect_string = 
                    (new_string) ? g_strdup(new_string) : NULL;
            }

            if(self->connect_string)
                self->connect_hook = TRUE;

            g_free(new_string);
            break;

        case PROP_REMOTE_ENCODING:
            new_string = g_value_dup_string(value);

            if(!self->remote_encoding)
                self->remote_encoding = 
                    (new_string) ? g_strdup(new_string) : NULL;
            else if( strcmp(self->remote_encoding, new_string) != 0)
            {
                if(self->remote_encoding)
                    g_free(self->remote_encoding);
                self->remote_encoding = 
                    (new_string) ? g_strdup(new_string) : NULL;
            }

            g_free(new_string);
            break;

        case PROP_PORT:
            new_int = g_value_get_int(value);

            if(new_int != self->port)
                self->port = new_int;
            break;

        case PROP_LOCAL_ECHO:
            new_boolean = g_value_get_boolean(value);

            if(new_boolean != self->local_echo)
                self->local_echo = new_boolean;
            break;

        case PROP_MUD_NAME:
            new_string = g_value_dup_string(value);

            if(!self->mud_name)
                self->mud_name = 
                    (new_string) ? g_strdup(new_string) : NULL;
            else if( strcmp(self->mud_name, new_string) != 0)
            {
                if(self->mud_name)
                    g_free(self->mud_name);
                self->mud_name = 
                    (new_string) ? g_strdup(new_string) : NULL;
            }

            g_free(new_string);
            break;

        case PROP_HOSTNAME:
            new_string = g_value_dup_string(value);

            if(!self->hostname)
                self->hostname = 
                    (new_string) ? g_strdup(new_string) : NULL;
            else if( strcmp(self->hostname, new_string) != 0)
            {
                if(self->hostname)
                    g_free(self->hostname);
                self->hostname = 
                    (new_string) ? g_strdup(new_string) : NULL;
            }

            g_free(new_string);
            break;

        case PROP_WINDOW:
            self->window = MUD_WINDOW(g_value_get_object(value));
            break;

        case PROP_TRAY:
            self->tray = MUD_TRAY(g_value_get_object(value));
            break;

        case PROP_PROFILE_NAME:
            new_string = g_value_dup_string(value);

            if(!self->profile_name)
                self->profile_name = 
                    (new_string) ? g_strdup(new_string) : NULL;
            else if( strcmp(self->profile_name, new_string) != 0)
            {
                if(self->profile_name)
                    g_free(self->profile_name);
                self->profile_name = 
                    (new_string) ? g_strdup(new_string) : NULL;
            }

            g_free(new_string);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
mud_connection_view_get_property(GObject *object,
                                 guint prop_id,
                                 GValue *value,
                                 GParamSpec *pspec)
{
    MudConnectionView *self;

    self = MUD_CONNECTION_VIEW(object);

    switch(prop_id)
    {
        case PROP_CONNECTION:
            g_value_set_pointer(value, self->connection);
            break;

        case PROP_LOCAL_ECHO:
            g_value_set_boolean(value, self->local_echo);
            break;

        case PROP_REMOTE_ENCODE:
            g_value_set_boolean(value, self->remote_encode);
            break;

        case PROP_CONNECT_HOOK:
            g_value_set_boolean(value, self->connect_hook);
            break;

        case PROP_CONNECTED:
            g_value_set_boolean(value, self->connected);
            break;

        case PROP_CONNECT_STRING:
            g_value_set_string(value, self->connect_string);
            break;

        case PROP_REMOTE_ENCODING:
            g_value_set_string(value, self->remote_encoding);
            break;

        case PROP_PORT:
            g_value_set_int(value, self->port);
            break;

        case PROP_MUD_NAME:
            g_value_set_string(value, self->mud_name);
            break;

        case PROP_HOSTNAME:
            g_value_set_string(value, self->hostname);
            break;

        case PROP_LOG:
            g_value_take_object(value, self->log);
            break;

        case PROP_TRAY:
            g_value_take_object(value, self->tray);
            break;

        case PROP_PROFILE:
            g_value_take_object(value, self->profile);
            break;

        case PROP_PARSE_BASE:
            g_value_take_object(value, self->parse);
            break;

        case PROP_WINDOW:
            g_value_take_object(value, self->window);
            break;

        case PROP_TELNET:
            g_value_take_object(value, self->telnet);
            break;

        case PROP_LOGGING:
            g_value_set_boolean(value, self->logging);
            break;

        case PROP_PROFILE_NAME:
            g_value_set_string(value, self->profile_name);
            break;

        case PROP_TERMINAL:
            g_value_take_object(value, self->terminal);
            break;

        case PROP_VBOX:
            g_value_take_object(value, self->ui_vbox);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

/* Callbacks */
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
    mud_window_close_current_window(view->window);
}

static void
mud_connection_view_profile_changed_cb(MudProfile *profile, MudProfileMask *mask, MudConnectionView *view)
{
    GList *entry;

    if (mask->ScrollOnOutput)
        mud_connection_view_set_terminal_scrolloutput(view);
    if (mask->Scrollback)
        mud_connection_view_set_terminal_scrollback(view);
    if (mask->FontName)
        mud_connection_view_set_terminal_font(view);
    if (mask->Foreground || mask->Background || mask->Colors)
        mud_connection_view_set_terminal_colors(view);

    entry = g_list_first(view->priv->subwindows);

    while(entry)
    {
        MudSubwindow *sub = MUD_SUBWINDOW(entry->data);

        mud_subwindow_reread_profile(sub);

        entry = g_list_next(entry);
    }
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
            GTK_WIDGET(view->terminal),
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

        if (prof == view->profile)
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
    vte_terminal_im_append_menuitems(view->terminal,
            GTK_MENU_SHELL(im_menu));
    gtk_menu_shell_append(GTK_MENU_SHELL(view->priv->popup_menu), menu_item);

    gtk_widget_show_all(view->priv->popup_menu);
    gtk_menu_popup(GTK_MENU(view->priv->popup_menu),
            NULL, NULL,
            NULL, NULL,
            event ? event->button : 0,
            event ? event->time : gtk_get_current_event_time());
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
    MudTelnetMsp *msp_handler;
    gboolean msp_parser_enabled;
#endif

    g_assert(view != NULL);

    switch(event->type)
    {
        case GNET_CONN_ERROR:
            mud_connection_view_add_text(view, _("*** Could not connect.\n"), Error);
            mud_window_disconnected(view->window);
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

            if(view->telnet)
            {
                g_object_unref(view->telnet);
                view->telnet = NULL;
            }

            mud_connection_view_add_text(view, _("*** Connection closed.\n"), Error);

            mud_window_disconnected(view->window);
            break;

        case GNET_CONN_TIMEOUT:
            break;

        case GNET_CONN_READ:
            if(!view->connected)
            {
                view->connected = TRUE;
                mud_tray_update_icon(view->tray, online);
            }

            view->priv->processed =
                mud_telnet_process(view->telnet, (guchar *)event->buffer,
                        event->length, &length);

            if(view->priv->processed != NULL)
            {
#ifdef ENABLE_GST
                msp_handler =
                    MUD_TELNET_MSP(mud_telnet_get_handler(view->telnet,
                                                          TELOPT_MSP));

                g_object_get(msp_handler,
                             "enabled", &msp_parser_enabled,
                             NULL);

                if(msp_parser_enabled)
                {
                    view->priv->processed =
                        mud_telnet_msp_parse(msp_handler,
                                             view->priv->processed,
                                             &length);
                }
#endif
                if(view->priv->processed != NULL)
                {
#ifdef ENABLE_GST
                    if(msp_parser_enabled)
                        mud_telnet_msp_parser_clear(msp_handler);
#endif 
                    buf = view->priv->processed->str;

                    temp = view->local_echo;
                    view->local_echo = FALSE;
                    gag = mud_parse_base_do_triggers(view->parse,
                            buf);
                    view->local_echo = temp;

                    if(!gag)
                    {
                        if(g_str_equal(view->priv->current_output, "main"))
                            vte_terminal_feed(view->terminal,
                                              buf,
                                              length);
                        else
                        {
                            MudSubwindow *sub =
                                mud_connection_view_get_subwindow(view,
                                        view->priv->current_output);

                            if(!sub)
                                vte_terminal_feed(view->terminal,
                                        buf,
                                        length);
                            else
                                mud_subwindow_feed(sub, buf, length);
                        }

                        mud_log_write_hook(view->log, buf, length);
                    }

                    if (view->connect_hook) {
                        mud_connection_view_send (view, view->connect_string);
                        view->connect_hook = FALSE;
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


static void
popup_menu_detach(GtkWidget *widget, GtkMenu *menu)
{
    MudConnectionView *view;

    view = g_object_get_data(G_OBJECT(widget), "connection-view");
    view->priv->popup_menu = NULL;
}

/* Private Methods */
static void
mud_connection_view_set_size_force_grid (MudConnectionView *window,
                                         VteTerminal *screen,
                                         gboolean        even_if_mapped,
                                         int             force_grid_width,
                                         int             force_grid_height)
{
    /* Owen's hack from gnome-terminal */
    GtkWidget *widget;
    GtkWidget *app;
    GtkWidget *mainwindow;
    GtkRequisition toplevel_request;
    GtkRequisition widget_request;
    int w, h;
    int char_width;
    int char_height;
    int grid_width;
    int grid_height;
    int xpad;
    int ypad;

    g_return_if_fail(IS_MUD_CONNECTION_VIEW(window));

    /* be sure our geometry is up-to-date */
    mud_connection_view_update_geometry (window);
    widget = GTK_WIDGET (screen);

    g_object_get(window->window, "window", &app, NULL);

    gtk_widget_size_request (app, &toplevel_request);
    gtk_widget_size_request (widget, &widget_request);

    w = toplevel_request.width - widget_request.width;
    h = toplevel_request.height - widget_request.height;

    char_width = VTE_TERMINAL(screen)->char_width;
    char_height = VTE_TERMINAL(screen)->char_height;

    grid_width = VTE_TERMINAL(screen)->column_count;
    grid_height = VTE_TERMINAL(screen)->row_count;

    if (force_grid_width >= 0)
        grid_width = force_grid_width;
    if (force_grid_height >= 0)
        grid_height = force_grid_height;

    vte_terminal_get_padding (VTE_TERMINAL (screen), &xpad, &ypad);

    w += xpad * 2 + char_width * grid_width;
    h += ypad * 2 + char_height * grid_height;

    if (even_if_mapped && GTK_WIDGET_MAPPED (app)) {
        gtk_window_resize (GTK_WINDOW (app), w, h);
    }
    else {
        gtk_window_set_default_size (GTK_WINDOW (app), w, h);
    }
}

static void
mud_connection_view_update_geometry (MudConnectionView *window)
{
    GtkWidget *widget = GTK_WIDGET(window->terminal);
    GtkWidget *mainwindow;
    GdkGeometry hints;
    gint char_width;
    gint char_height;
    gint xpad, ypad;

    char_width = VTE_TERMINAL(widget)->char_width;
    char_height = VTE_TERMINAL(widget)->char_height;

    vte_terminal_get_padding (VTE_TERMINAL (window->terminal), &xpad, &ypad);

    hints.base_width = xpad;
    hints.base_height = ypad;

#define MIN_WIDTH_CHARS 4
#define MIN_HEIGHT_CHARS 2

    hints.width_inc = char_width;
    hints.height_inc = char_height;

    /* min size is min size of just the geometry widget, remember. */
    hints.min_width = hints.base_width + hints.width_inc * MIN_WIDTH_CHARS;
    hints.min_height = hints.base_height + hints.height_inc * MIN_HEIGHT_CHARS;

    g_object_get(window->window, "window", &mainwindow, NULL);
    gtk_window_set_geometry_hints (GTK_WINDOW (mainwindow),
            widget,
            &hints,
            GDK_HINT_RESIZE_INC |
            GDK_HINT_MIN_SIZE |
            GDK_HINT_BASE_SIZE);
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

    g_object_unref(client);

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
mud_connection_view_feed_text(MudConnectionView *view, gchar *message)
{
    gint rlen = strlen(message);
    gchar buf[rlen*2];

    g_return_if_fail(IS_MUD_CONNECTION_VIEW(view));

    g_stpcpy(buf, message);
    utils_str_replace(buf, "\r", "");
    utils_str_replace(buf, "\n", "\n\r");

    vte_terminal_feed(view->terminal, buf, strlen(buf));
}

static void
mud_connection_view_reread_profile(MudConnectionView *view)
{
    g_return_if_fail(IS_MUD_CONNECTION_VIEW(view));

    mud_connection_view_set_terminal_colors(view);
    mud_connection_view_set_terminal_scrollback(view);
    mud_connection_view_set_terminal_scrolloutput(view);
    mud_connection_view_set_terminal_font(view);
}

static void
mud_connection_view_set_terminal_colors(MudConnectionView *view)
{
    g_return_if_fail(IS_MUD_CONNECTION_VIEW(view));

    vte_terminal_set_colors(view->terminal,
            &view->profile->preferences->Foreground,
            &view->profile->preferences->Background,
            view->profile->preferences->Colors, C_MAX);
}

static void
mud_connection_view_set_terminal_scrollback(MudConnectionView *view)
{
    g_return_if_fail(IS_MUD_CONNECTION_VIEW(view));

    vte_terminal_set_scrollback_lines(view->terminal,
            view->profile->preferences->Scrollback);
}

static void
mud_connection_view_set_terminal_scrolloutput(MudConnectionView *view)
{
    g_return_if_fail(IS_MUD_CONNECTION_VIEW(view));

    if(view->terminal)
        vte_terminal_set_scroll_on_output(view->terminal,
                view->profile->preferences->ScrollOnOutput);
}

static void
mud_connection_view_set_terminal_font(MudConnectionView *view)
{
    PangoFontDescription *desc = NULL;
    char *name;

    g_return_if_fail(IS_MUD_CONNECTION_VIEW(view));

    name = view->profile->preferences->FontName;

    if(name)
        desc = pango_font_description_from_string(name);

    if(!desc)
        desc = pango_font_description_from_string("Monospace 10");

    vte_terminal_set_font(view->terminal, desc);
}

/* Public Methods */
MudSubwindow *
mud_connection_view_create_subwindow(MudConnectionView *view,
                                     const gchar *title,
                                     const gchar *identifier,
                                     guint width,
                                     guint height)
{
    MudSubwindow *sub;

    if(!IS_MUD_CONNECTION_VIEW(view))
        return NULL;

    if(mud_connection_view_has_subwindow(view, identifier))
        return mud_connection_view_get_subwindow(view, identifier);

    sub = g_object_new(MUD_TYPE_SUBWINDOW,
                       "title", title,
                       "identifier", identifier,
                       "width", width,
                       "height", height,
                       "parent-view", view,
                       NULL);

    view->priv->subwindows = g_list_append(view->priv->subwindows, sub);

    return sub;
}

gboolean
mud_connection_view_has_subwindow(MudConnectionView *view,
                                  const gchar *identifier)
{
    GList *entry;
    gchar *ident;

    if(!IS_MUD_CONNECTION_VIEW(view))
        return FALSE;

    entry = g_list_first(view->priv->subwindows);

    while(entry)
    {
        MudSubwindow *sub = MUD_SUBWINDOW(entry->data);

        g_object_get(sub, "identifier", &ident, NULL);

        if(g_str_equal(identifier, ident))
        {
            g_free(ident);

            return TRUE;
        }

        g_free(ident);

        entry = g_list_next(entry);
    }

    return FALSE;
}

void
mud_connection_view_set_output(MudConnectionView *view,
                               const gchar *identifier)
{
    g_return_if_fail(IS_MUD_CONNECTION_VIEW(view));

    if(mud_connection_view_has_subwindow(view, identifier) ||
       g_str_equal(identifier, "main"))
    {
        g_free(view->priv->current_output);
        view->priv->current_output = g_strdup(identifier);
    }
}

void
mud_connection_view_enable_subwindow_input(MudConnectionView *view,
                                           const gchar *identifier,
                                           gboolean enable)
{
    g_return_if_fail(IS_MUD_CONNECTION_VIEW(view));

    if(mud_connection_view_has_subwindow(view, identifier))
    {
        MudSubwindow *sub =
            mud_connection_view_get_subwindow(view, identifier);

        mud_subwindow_enable_input(sub, enable);
    }
}


void
mud_connection_view_show_subwindow(MudConnectionView *view,
                                   const gchar *identifier)
{
    g_return_if_fail(IS_MUD_CONNECTION_VIEW(view));

    if(mud_connection_view_has_subwindow(view, identifier))
    {
        MudSubwindow *sub =
            mud_connection_view_get_subwindow(view, identifier);

        mud_subwindow_show(sub);
    }
}

MudSubwindow *
mud_connection_view_get_subwindow(MudConnectionView *view,
                                  const gchar *identifier)
{
    GList *entry;
    gchar *ident;

    if(!IS_MUD_CONNECTION_VIEW(view))
        return NULL;

    if(!mud_connection_view_has_subwindow(view, identifier))
        return NULL;

    entry = view->priv->subwindows;

    while(entry)
    {
        MudSubwindow *sub = MUD_SUBWINDOW(entry->data);

        g_object_get(sub, "identifier", &ident, NULL);

        if(g_str_equal(ident, identifier))
        {
            g_free(ident);

            return sub;
        }

        g_free(ident);

        entry = g_list_next(entry);
    }

    return NULL;
}

void
mud_connection_view_remove_subwindow(MudConnectionView *view,
                                     const gchar *identifier)
{
    MudSubwindow *sub;

    g_return_if_fail(IS_MUD_CONNECTION_VIEW(view));

    if(!mud_connection_view_has_subwindow(view, identifier))
        return;

    sub = mud_connection_view_get_subwindow(view, identifier);

    if(g_str_equal(view->priv->current_output, identifier))
    {
        g_free(view->priv->current_output);
        view->priv->current_output = g_strdup("main");
    }

    view->priv->subwindows = g_list_remove(view->priv->subwindows, sub);

    g_object_unref(sub);

}

void
mud_connection_view_hide_subwindows(MudConnectionView *view)
{
    GList *entry;
    gboolean visible;

    g_return_if_fail(IS_MUD_CONNECTION_VIEW(view));

    entry = g_list_first(view->priv->subwindows);

    while(entry)
    {
        MudSubwindow *sub = MUD_SUBWINDOW(entry->data);

        g_object_get(sub, "visible", &visible, NULL);

        if(visible)
        {
            g_object_set(sub, "view-hidden", TRUE, NULL);
            mud_subwindow_hide(sub);
        }

        entry = g_list_next(entry);
    }
}

void
mud_connection_view_show_subwindows(MudConnectionView *view)
{
    GList *entry;
    gboolean view_hidden;

    g_return_if_fail(IS_MUD_CONNECTION_VIEW(view));

    entry = g_list_first(view->priv->subwindows);

    while(entry)
    {
        MudSubwindow *sub = MUD_SUBWINDOW(entry->data);

        g_object_get(sub, "view-hidden", &view_hidden, NULL);

        if(view_hidden)
        {
            g_object_set(sub, "view-hidden", FALSE, NULL);
            mud_subwindow_show(sub);
        }

        entry = g_list_next(entry);
    }
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

    g_return_if_fail(IS_MUD_CONNECTION_VIEW(view));

    client = gconf_client_get_default();

    text = g_strdup(message);

    g_snprintf(key, 2048, "/apps/gnome-mud/%s%s", extra_path, "functionality/remote_encoding");
    remote = gconf_client_get_bool(client, key, NULL);

    if(view->remote_encode && remote)
        encoding = view->remote_encoding;
    else
    {
        profile_name = mud_profile_get_name(view->profile);

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

    if(error)
    {
        text = NULL;
    }

    vte_terminal_set_encoding(view->terminal, encoding);

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

    if(view->local_echo)
        mud_connection_view_feed_text(view, (!error) ? text : message);

    mud_connection_view_feed_text(view, "\e[0m");

    if(text != NULL)
        g_free(text);

    g_object_unref(client);
}

void
mud_connection_view_disconnect(MudConnectionView *view)
{
#ifdef ENABLE_GST
    MudMSPDownloadItem *item;
#endif

    g_return_if_fail(IS_MUD_CONNECTION_VIEW(view));

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

        if(view->telnet)
        {
            g_object_unref(view->telnet);
            view->telnet = NULL;
        }

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
    g_return_if_fail(IS_MUD_CONNECTION_VIEW(view));

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

        g_object_unref(view->telnet);
        view->telnet = NULL;

        mud_connection_view_add_text(view,
                _("\n*** Connection closed.\n"), System);
    }

    view->connection = gnet_conn_new(view->hostname, view->port,
            mud_connection_view_network_event_cb, view);
    gnet_conn_ref(view->connection);
    gnet_conn_set_watch_error(view->connection, TRUE);

    profile_name = mud_profile_get_name(view->profile);

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

    view->local_echo = TRUE;

    view->telnet = g_object_new(MUD_TYPE_TELNET,
                                "parent-view", view,
                                NULL);

    buf = g_strdup_printf(_("*** Making connection to %s, port %d.\n"),
            view->hostname, view->port);
    mud_connection_view_add_text(view, buf, System);
    g_free(buf);

    g_object_unref(client);

    gnet_conn_connect(view->connection);
}

void
mud_connection_view_send(MudConnectionView *view, const gchar *data)
{
    GConfClient *client;
    MudTelnetZmp *zmp_handler;
    GList *commands, *command;
    const gchar *local_codeset;
    gboolean remote, zmp_enabled;
    gsize bytes_read, bytes_written;
    gchar *text, *encoding, *conv_text, *profile_name;

    gchar key[2048];
    gchar extra_path[512] = "";

    GError *error = NULL;
    
    g_return_if_fail(IS_MUD_CONNECTION_VIEW(view));

    if(view->connection && gnet_conn_is_connected(view->connection))
    {
        if(view->local_echo) // Prevent password from being cached.
        {
            gchar *head = g_queue_peek_head(view->priv->history);

            if( (head && !g_str_equal(head, data)) ||
                 g_queue_is_empty(view->priv->history))
            {
                gchar *history_item = g_strdup(data);
                g_strstrip(history_item);

                /* Don't queue empty lines */
                if(strlen(history_item) != 0) 
                    g_queue_push_head(view->priv->history,
                                      history_item);
                else
                    g_free(history_item);
            }
        }
        else
            g_queue_push_head(view->priv->history,
                    (gpointer)g_strdup(_("<password removed>")));

        view->priv->current_history_index = -1;

        client = gconf_client_get_default();

        g_snprintf(key, 2048, "/apps/gnome-mud/%s%s", extra_path, "functionality/remote_encoding");
        remote = gconf_client_get_bool(client, key, NULL);

        if(view->remote_encode && remote)
            encoding = view->remote_encoding;
        else
        {
            profile_name = mud_profile_get_name(view->profile);

            if (!g_str_equal(profile_name, "Default"))
                g_snprintf(extra_path, 512, "profiles/%s/", profile_name);

            g_snprintf(key, 2048, "/apps/gnome-mud/%s%s", extra_path, "functionality/encoding");
            encoding = gconf_client_get_string(client, key, NULL);
        }

        g_get_charset(&local_codeset);

        commands = mud_profile_process_commands(view->profile, data);

        for (command = g_list_first(commands); command != NULL; command = command->next)
        {
            gchar *text = (gchar *)command->data;
            g_strstrip(text);

            conv_text = g_convert(text, -1,
                encoding,
                local_codeset, 
                &bytes_read, &bytes_written, &error);

            if(error)
            {
                conv_text = NULL;
                error = NULL;
            }

            zmp_handler = MUD_TELNET_ZMP(mud_telnet_get_handler(view->telnet,
                                                                TELOPT_ZMP));
            if(!zmp_handler)
                zmp_enabled = FALSE;
            else
                g_object_get(zmp_handler, "enabled", &zmp_enabled, NULL);

            if(!zmp_enabled)
            {
                gchar *line = (conv_text == NULL) ? text : conv_text;

                gnet_conn_write(view->connection, line, strlen(line));
                gnet_conn_write(view->connection, "\r\n", 2);
            }
            else // ZMP is enabled, use zmp.input.
            {
                gchar *line = (conv_text == NULL) ? text : conv_text;

                mud_zmp_send_command(zmp_handler, 2,
                                     "zmp.input",
                                     line);
            }

            if (view->profile->preferences->EchoText && view->local_echo)
            {
                mud_connection_view_add_text(view, text, Sent);
                mud_connection_view_add_text(view, "\r\n", Sent);
            }

            if(conv_text != NULL)
                g_free(conv_text);
            g_free(text);
        }

        g_list_free(commands);
        g_object_unref(client);
    }
}

void
mud_connection_view_start_logging(MudConnectionView *view)
{
    g_return_if_fail(IS_MUD_CONNECTION_VIEW(view));

    view->logging = TRUE;
    mud_log_open(view->log);
}

void
mud_connection_view_stop_logging(MudConnectionView *view)
{
    g_return_if_fail(IS_MUD_CONNECTION_VIEW(view));

    view->logging = FALSE;
    mud_log_close(view->log);
}

void
mud_connection_view_set_profile(MudConnectionView *view, MudProfile *profile)
{
    g_return_if_fail(IS_MUD_CONNECTION_VIEW(view));

    if(profile == view->profile)
        return;

    if (view->profile)
    {
        g_signal_handler_disconnect(view->profile, view->priv->signal_profile_changed);
        g_object_unref(G_OBJECT(view->profile));
    }

    view->profile = profile;
    g_object_ref(G_OBJECT(profile));
    view->priv->signal_profile_changed =
        g_signal_connect(G_OBJECT(view->profile), "changed",
                         G_CALLBACK(mud_connection_view_profile_changed_cb),
                         view);
    mud_connection_view_reread_profile(view);
}

const gchar *
mud_connection_view_get_history_item(MudConnectionView *view, enum
                                     MudConnectionHistoryDirection direction)
{
    gchar *history_item;

    if(direction == HISTORY_DOWN)
        if( !(view->priv->current_history_index <= 0) )
            view->priv->current_history_index--;

    if(direction == HISTORY_UP)
        if(view->priv->current_history_index < 
                (gint)g_queue_get_length(view->priv->history) - 1)
            view->priv->current_history_index++;

    history_item = (gchar *)g_queue_peek_nth(view->priv->history,
            view->priv->current_history_index);

    return history_item;
}

/* MSP Download Code */
#ifdef ENABLE_GST
static void
mud_connection_view_start_download(MudConnectionView *view)
{
    MudMSPDownloadItem *item;

    g_return_if_fail(IS_MUD_CONNECTION_VIEW(view));

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

    g_return_if_fail(IS_MUD_CONNECTION_VIEW(view));

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
            {
                g_object_unref(client);
                return;
            }
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

    g_object_unref(client);
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
