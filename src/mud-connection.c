#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib-object.h>

#include "mud-connection.h"

static char const rcsid[] = "$Id: ";

struct _MudConnectionPrivate
{
	gchar *hostname;
	gint port;
};

static void mud_connection_init       (MudConnection *connection);
static void mud_connection_class_init (MudConnectionClass *klass);
static void mud_connection_finalize   (GObject *object);

GType
mud_connection_get_type (void)
{
	static GType object_type = 0;

	g_type_init();

	if (!object_type)
	{
		static const GTypeInfo object_info =
		{
			sizeof (MudConnectionClass),
			NULL,
			NULL,
			(GClassInitFunc) mud_connection_class_init,
			NULL,
			NULL,
			sizeof (MudConnection),
			0,
			(GInstanceInitFunc) mud_connection_init,
		};

		object_type = g_type_register_static(G_TYPE_OBJECT, "MudConnection", &object_info, 0);
	}

	return object_type;
}

static void
mud_connection_init (MudConnection *connection)
{
	connection->priv = g_new0(MudConnectionPrivate, 1);
}

static void
mud_connection_class_init (MudConnectionClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = mud_connection_finalize;
}

static void
mud_connection_finalize (GObject *object)
{
	MudConnection *connection;
	GObjectClass  *parent_class;

	connection = MUD_CONNECTION(object);

	g_free(connection->priv);

	parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
	parent_class->finalize(object);
}

MudConnection*
mud_connection_new ()
{
	MudConnection *connection;

	connection = g_object_new(MUD_TYPE_CONNECTION, NULL);

	return connection;
}

void
mud_connection_set_hostname (MudConnection *connection, const gchar *hostname)
{
	g_assert(connection != NULL);
	g_assert(hostname != NULL);

	g_free(connection->priv->hostname);
	connection->priv->hostname = g_strdup(hostname);
}

void
mud_connection_set_port (MudConnection *connection, const gint port)
{
	g_assert(connection != NULL);
	g_assert(port > 0);

	connection->priv->port = port;
}
