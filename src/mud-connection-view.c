#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib-object.h>
#include <glib/gi18n.h>
#include <vte/vte.h>

#include "mud-connection-view.h"

static char const rcsid[] = "$Id: ";

struct _MudConnectionViewPrivate
{
	GtkWidget *terminal;
	GtkWidget *scrollbar;
	GtkWidget *box;
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
mud_connection_view_str_replace (gchar *buf, const gchar *s, const gchar *repl)
{
	gchar out_buf[4608];
	gchar *pc, *out;
	gint  len = strlen (s);
	gboolean found = FALSE;

	for ( pc = buf, out = out_buf; *pc && (out-out_buf) < (4608-len-4);)
		if ( !strncasecmp(pc, s, len))
		{
			out += sprintf (out, repl);
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

static void
mud_connection_view_add_text(MudConnectionView *view, gchar *message, enum MudConnectionColorType type)
{
	
	/*GtkWidget *text_widget = cd->window;
	gint       i;

    if ( message[0] == '\0' )
    {
        return;
    }

	i = gtk_notebook_get_current_page (GTK_NOTEBOOK(main_notebook));
	if (connections[i]->logging
		&& colortype != MESSAGE_ERR
		&& colortype != MESSAGE_SYSTEM)
	{
		struct timeval tv;
		
		fputs(message, connections[i]->log);

		gettimeofday(&tv, NULL);
		if ((connections[i]->last_log_flush + (unsigned) prefs.FlushInterval) < tv.tv_sec)
		{
			fflush(connections[i]->log);
			connections[i]->last_log_flush = tv.tv_sec;
		}
	}

	switch (colortype)
    {
	    case MESSAGE_SENT:
			terminal_feed(text_widget, "\e[1;33m");
			break;
		
	    case MESSAGE_ERR:
			terminal_feed(text_widget, "\e[1;31m");
			break;
		
		case MESSAGE_SYSTEM:
			terminal_feed(text_widget, "\e[1;32m");
			break;
		
		default:
			break;
    }

 	terminal_feed(text_widget, message);
	terminal_feed(text_widget, "\e[0m");*/

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

	mud_connection_view_feed_text(view, message);
	mud_connection_view_feed_text(view, "\e[0m");
}


static void
mud_connection_view_init (MudConnectionView *connection_view)
{
	GtkWidget *box;
	connection_view->priv = g_new0(MudConnectionViewPrivate, 1);

	box = gtk_hbox_new(FALSE, 0);
	
	connection_view->priv->terminal = vte_terminal_new();
	gtk_box_pack_start(GTK_BOX(box), connection_view->priv->terminal, TRUE, TRUE, 0);

	connection_view->priv->scrollbar = gtk_vscrollbar_new(NULL);
	gtk_range_set_adjustment(GTK_RANGE(connection_view->priv->scrollbar), VTE_TERMINAL(connection_view->priv->terminal)->adjustment);
	gtk_box_pack_start(GTK_BOX(box), connection_view->priv->scrollbar, FALSE, FALSE, 0);
	
	gtk_widget_show_all(box);
	g_object_set_data(G_OBJECT(box), "connection-view", connection_view);
	
	connection_view->priv->box = box;
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

static void
mud_connection_view_received_cb(GNetworkConnection *cxn, gconstpointer data, gulong length, gpointer user_data)
{
	g_print ("Client Connection: Received: %lu bytes\n\"%s\"\n", length, (gchar *) data);

	vte_terminal_feed(VTE_TERMINAL(MUD_CONNECTION_VIEW(user_data)->priv->terminal), (gchar *) data, length);
}

static void
mud_connection_view_send_cb(GNetworkConnection *cxn, gconstpointer data, gulong length, gpointer user_data)
{
	g_print ("Client Connection: Sent: %lu bytes\n\"%s\"\n", length, (gchar *) data);
}

static void
mud_connection_view_notify_cb(GObject *cxn, GParamSpec *pspec, gpointer user_data)
{
	GNetworkTcpConnectionStatus status;

	g_object_get (cxn, "tcp-status", &status, NULL);

	switch (status)
	{
		case GNETWORK_TCP_CONNECTION_CLOSING:
			break;
			
		case GNETWORK_TCP_CONNECTION_CLOSED:
			mud_connection_view_add_text(MUD_CONNECTION_VIEW(user_data), _("*** Connection closed.\n"), System);
	        break;

		case GNETWORK_TCP_CONNECTION_LOOKUP:
			break;
			
		case GNETWORK_TCP_CONNECTION_OPENING:
			{
				gchar *buf, *address;
				gint port;

				g_object_get(GNETWORK_CONNECTION(cxn), "address", &address, "port", &port, NULL);
				
				buf = g_strdup_printf(_("*** Making connection to %s, port %d.\n"), address, port);
				mud_connection_view_add_text(MUD_CONNECTION_VIEW(user_data), buf, System);
				g_free(buf);
				break;
			}
			
		case GNETWORK_TCP_CONNECTION_PROXYING:
			break;
		
		case GNETWORK_TCP_CONNECTION_AUTHENTICATING:
			break;

		case GNETWORK_TCP_CONNECTION_OPEN:
			break;
	}
}

static void
mud_connection_view_error_cb(GNetworkConnection *cxn, GError *error, gpointer user_data)
{
	g_print ("Client Connection: Error:\n\tDomain\t= %s\n\tCode\t= %d\n\tMessage\t= %s\n",
			 g_quark_to_string (error->domain), error->code, error->message);
}

void
mud_connection_view_disconnect(MudConnectionView *view)
{
	gnetwork_connection_close(GNETWORK_CONNECTION(view->connection));
}

void
mud_connection_view_reconnect(MudConnectionView *view)
{
	gnetwork_connection_close(GNETWORK_CONNECTION(view->connection));
	gnetwork_connection_open(GNETWORK_CONNECTION(view->connection));
}

void
mud_connection_view_send(MudConnectionView *view, const gchar *data)
{
	gchar *text;

	text = g_strdup_printf("%s\r\n", data);
	gnetwork_connection_send(GNETWORK_CONNECTION(view->connection), text, strlen(text));
	mud_connection_view_add_text(view, text, Sent);
	g_free(text);
}

MudConnectionView*
mud_connection_view_new (const gchar *hostname, const gint port)
{
	MudConnectionView *view;

	g_assert(hostname != NULL);
	g_assert(port > 0);
	
	view = g_object_new(MUD_TYPE_CONNECTION_VIEW, NULL);
	view->connection = g_object_new(GNETWORK_TYPE_TCP_CONNECTION,
									"address", hostname,
									"port", port,
									"authentication-type", GNETWORK_SSL_AUTH_ANONYMOUS,
									"proxy-type", GNETWORK_TCP_PROXY_NONE, NULL);
	g_object_add_weak_pointer(G_OBJECT(view->connection), (gpointer *) &view->connection);
	g_signal_connect(view->connection, "received", G_CALLBACK(mud_connection_view_received_cb), view);
	g_signal_connect(view->connection, "sent", G_CALLBACK(mud_connection_view_send_cb), view);
	g_signal_connect(view->connection, "error", G_CALLBACK(mud_connection_view_error_cb), view);
	g_signal_connect(view->connection, "notify::tcp-status", G_CALLBACK(mud_connection_view_notify_cb), view);
	
	// FIXME, move this away from here
	gnetwork_connection_open(GNETWORK_CONNECTION(view->connection));
	
	return view;
}

GtkWidget *
mud_connection_view_get_viewport (MudConnectionView *view)
{
	return view->priv->box;
}
