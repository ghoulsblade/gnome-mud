#ifndef MUD_CONNECTION_VIEW_H
#define MUD_CONNECTION_VIEW_H

G_BEGIN_DECLS

#include <gtk/gtkwidget.h>

#include "mud-connection.h"

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

	MudConnection *connection;
};

struct _MudConnectionViewClass
{
	GObjectClass parent_class;
};

GType mud_connection_view_get_type (void) G_GNUC_CONST;

MudConnectionView* mud_connection_view_new ();
MudConnectionView* mud_connection_view_new_with_params (const gchar *hostname, const gint port);
GtkWidget* mud_connection_view_get_viewport (MudConnectionView *view);

G_END_DECLS

#endif /* MUD_CONNECTION_VIEW_H */
