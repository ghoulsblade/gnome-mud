#ifndef MUD_CONNECTION_H
#define MUD_CONNECTION_H

G_BEGIN_DECLS

#define MUD_TYPE_CONNECTION                (mud_connection_get_type ())
#define MUD_CONNECTION(object)             (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_CONNECTION, MudConnection))
#define MUD_CONNECTION_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_CONNECTION, MudConnectionClass))
#define MUD_IS_CONNECTION(object)          (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_CONNECTION))
#define MUD_IS_CONNECTION_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_CONNECTION))
#define MUD_CONNECTIO_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_CONNECTION, MudConnectionClass))

typedef struct _MudConnection           MudConnection;
typedef struct _MudConnectionClass      MudConnectionClass;
typedef struct _MudConnectionPrivate    MudConnectionPrivate;

struct _MudConnection
{
	GObject parent_instance;

	MudConnectionPrivate *priv;
};

struct _MudConnectionClass
{
	GObjectClass parent_class;
};

GType mud_connection_view_get_type (void) G_GNUC_CONST;

MudConnection* mud_connection_new ();
void mud_connection_set_hostname(MudConnection *connection, const gchar *hostname);
void mud_connection_set_port(MudConnection *connection, const gint port);

G_END_DECLS

#endif /* MUD_CONNECTION_H */
