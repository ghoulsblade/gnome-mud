#ifndef MUD_CONNECTION_VIEW_H
#define MUD_CONNECTION_VIEW_H

G_BEGIN_DECLS

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <gnet.h>
#include <vte/vte.h>

#define MUD_TYPE_CONNECTION_VIEW               (mud_connection_view_get_type ())
#define MUD_CONNECTION_VIEW(object)            (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_CONNECTION_VIEW, MudConnectionView))
#define MUD_CONNECTION_VIEW_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_CONNECTION_VIEW, MudConnectionViewClass))
#define IS_MUD_CONNECTION_VIEW(object)         (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_CONNECTION_VIEW))
#define IS_MUD_CONNECTION_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_CONNECTION_VIEW))
#define MUD_CONNECTION_VIEW_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_CONNECTION, MudConnectionViewClass))
#define MUD_CONNECTION_VIEW_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MUD_TYPE_CONNECTION_VIEW, MudConnectionViewPrivate))

typedef struct _MudConnectionView           MudConnectionView;
typedef struct _MudConnectionViewClass      MudConnectionViewClass;
typedef struct _MudConnectionViewPrivate    MudConnectionViewPrivate;

#include "mud-telnet.h"
#include "mud-parse-base.h"
#include "mud-profile.h"
#include "mud-window.h"
#include "mud-log.h"
#include "mud-tray.h"
#include "mud-subwindow.h"

struct _MudConnectionViewClass
{
    GObjectClass parent_class;
};

struct _MudConnectionView
{
    GObject parent_instance;

    /*< Private >*/
    MudConnectionViewPrivate *priv;

    /*< Public >*/
    GConn *connection;

    // Flags
    gboolean local_echo;
    gboolean remote_encode;   
    gboolean connect_hook;
    gboolean connected;
    gboolean logging;

    gchar *connect_string;
    gchar *remote_encoding;
    gchar *profile_name;

    gint port;
    gchar *mud_name;
    gchar *hostname;

    MudLog *log;
    MudTray *tray;
    MudTelnet *telnet;
    MudWindow *window;
    MudProfile *profile;
    MudParseBase *parse;

    VteTerminal *terminal;
    GtkVBox *ui_vbox;
};

enum MudConnectionColorType
{
    Error,
    Normal,
    Sent,
    System
};

enum MudConnectionHistoryDirection
{
    HISTORY_UP,
    HISTORY_DOWN
};

GType mud_connection_view_get_type (void) G_GNUC_CONST;

void mud_connection_view_send (MudConnectionView *view, const gchar *data);
void mud_connection_view_add_text(MudConnectionView *view, gchar *message, enum MudConnectionColorType type);

void mud_connection_view_reconnect (MudConnectionView *view);
void mud_connection_view_disconnect (MudConnectionView *view);

const gchar *mud_connection_view_get_history_item(MudConnectionView *view, enum
                                   MudConnectionHistoryDirection direction);

#ifdef ENABLE_GST
void mud_connection_view_queue_download(MudConnectionView *view, gchar *url, gchar *file);
#endif

void mud_connection_view_set_profile(MudConnectionView *view, MudProfile *profile);

void mud_connection_view_start_logging(MudConnectionView *view);
void mud_connection_view_stop_logging(MudConnectionView *view);

MudSubwindow *mud_connection_view_create_subwindow(MudConnectionView *view,
                                                   const gchar *title,
                                                   const gchar *identifier,
                                                   guint width,
                                                   guint height);

gboolean mud_connection_view_has_subwindow(MudConnectionView *view,
                                           const gchar *identifier);

void mud_connection_view_set_output(MudConnectionView *view,
                                    const gchar *identifier);

void mud_connection_view_enable_subwindow_input(MudConnectionView *view,
                                                const gchar *identifier,
                                                gboolean enable);

void mud_connection_view_enable_subwindow_scroll(MudConnectionView *view,
                                                 const gchar *identifier,
                                                 gboolean enable);

void mud_connection_view_show_subwindow(MudConnectionView *view,
                                        const gchar *identifier);

void mud_connection_view_remove_subwindow(MudConnectionView *view,
                                          const gchar *identifier);

MudSubwindow *mud_connection_view_get_subwindow(MudConnectionView *view,
                                                const gchar *identifier);

void mud_connection_view_hide_subwindows(MudConnectionView *view);
void mud_connection_view_show_subwindows(MudConnectionView *view);

G_END_DECLS

#endif /* MUD_CONNECTION_VIEW_H */

