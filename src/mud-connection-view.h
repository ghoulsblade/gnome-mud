#ifndef MUD_CONNECTION_VIEW_H
#define MUD_CONNECTION_VIEW_H

G_BEGIN_DECLS

#include <gtk/gtkwidget.h>
#include <libgnetwork/gnetwork.h>


#define MUD_TYPE_CONNECTION_VIEW               (mud_connection_view_get_type ())
#define MUD_CONNECTION_VIEW(object)            (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_CONNECTION_VIEW, MudConnectionView))
#define MUD_CONNECTION_VIEW_TYPE_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_CONNECTION_VIEW, MudConnectionViewClass))
#define MUD_IS_CONNECTION_VIEW(object)         (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_CONNECTION_VIEW))
#define MUD_IS_CONNECTION_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_CONNECTION_VIEW))
#define MUD_CONNECTION_VIEW_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_CONNECTION, MudConnectionViewClass))

typedef struct _MudConnectionView           MudConnectionView;
typedef struct _MudConnectionViewClass      MudConnectionViewClass;
typedef struct _MudConnectionViewPrivate    MudConnectionViewPrivate;

struct _MudConnectionView
{
	GObject parent_instance;

	MudConnectionViewPrivate *priv;

	GNetworkTcpConnection *connection;
};

struct _MudConnectionViewClass
{
	GObjectClass parent_class;
};

enum MudConnectionColorType
{
	Error,
	Normal,
	Sent,
	System
};

GType mud_connection_view_get_type (void) G_GNUC_CONST;

MudConnectionView* mud_connection_view_new (const gchar *profile, const gchar *hostname, const gint port, GtkWidget *window);
GtkWidget* mud_connection_view_get_viewport (MudConnectionView *view);
void mud_connection_view_disconnect (MudConnectionView *view);
void mud_connection_view_reconnect (MudConnectionView *view);
void mud_connection_view_send (MudConnectionView *view, const gchar *data);
void mud_connection_view_set_connect_string(MudConnectionView *view, gchar *connect_string);
void mud_connection_view_set_id(MudConnectionView *view, gint id);
void mud_connection_view_add_text(MudConnectionView *view, gchar *message, enum MudConnectionColorType type);

#include "mud-profile.h"
void mud_connection_view_set_profile(MudConnectionView *view, MudProfile *profile);
MudProfile *mud_connection_view_get_current_profile(MudConnectionView *view);

#include "mud-window.h"
void mud_connection_view_set_parent(MudConnectionView *view, MudWindow *window);

G_END_DECLS

#endif /* MUD_CONNECTION_VIEW_H */
