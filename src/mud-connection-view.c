#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib-object.h>
#include <vte/vte.h>

#include "mud-connection-view.h"

static char const rcsid[] = "$Id: ";

struct _MudConnectionViewPrivate
{
	GtkWidget *terminal;
};

static void mud_connection_view_init       (MudConnectionView *connection_view);
static void mud_connection_view_class_init (MudConnectionViewClass *klass);
static void mud_connection_view_finalize   (GObject *object);

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

static void
mud_connection_view_init (MudConnectionView *connection_view)
{
	connection_view->priv = g_new0(MudConnectionViewPrivate, 1);
	connection_view->connection = mud_connection_new();

	connection_view->priv->terminal = vte_terminal_new();
	// FIXME, scrollbar
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

	connection_view = MUD_CONNECTION_VIEW(object);

	g_object_unref(connection_view->connection);
	g_free(connection_view->priv);

	parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
	parent_class->finalize(object);
}

MudConnectionView*
mud_connection_view_new ()
{
	MudConnectionView *connection_view;

	connection_view = g_object_new(MUD_TYPE_CONNECTION_VIEW, NULL);

	return connection_view;
}

MudConnectionView*
mud_connection_view_new_with_params (const gchar *hostname, const gint port)
{
	MudConnectionView *view;
	
	g_assert(hostname != NULL);
	g_assert(port > 0);

	view = mud_connection_view_new();
	mud_connection_set_hostname(view->connection, hostname);
	mud_connection_set_port(view->connection, port);

	return view;
}

GtkWidget *
mud_connection_view_get_viewport (MudConnectionView *view)
{
	// FIXME
	return view->priv->terminal;
}
