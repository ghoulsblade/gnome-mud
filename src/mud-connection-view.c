#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gconf/gconf-client.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtkmenu.h>
#include <vte/vte.h>

#include "gnome-mud.h"
#include "mud-connection-view.h"
#include "mud-profile.h"
#include "mud-window.h"

static char const rcsid[] = "$Id$";

struct _MudConnectionViewPrivate
{
	gint id;
	
	MudWindow *window;
	
	GtkWidget *terminal;
	GtkWidget *scrollbar;
	GtkWidget *box;
	GtkWidget *popup_menu;

	MudProfile *profile;

	gulong signal;
	gulong signal_profile_changed;

	gboolean connect_hook;
	gchar *connect_string;
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

	g_print ("Switching profile from '%s' to '%s'\n",
           view->priv->profile ?
           view->priv->profile->name : "none",
           profile ? profile->name : "none");


	mud_connection_view_set_profile(view, profile);
	mud_connection_view_reread_profile(view);
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

void
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
  	//FIXME connection_view->priv->prefs = mud_preferences_new(NULL);
  
	connection_view->priv->connect_hook = FALSE;
	
	box = gtk_hbox_new(FALSE, 0);
	
	connection_view->priv->terminal = vte_terminal_new();
	gtk_box_pack_start(GTK_BOX(box), connection_view->priv->terminal, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(connection_view->priv->terminal),
					 "button_press_event",
					 G_CALLBACK(mud_connection_view_button_press_event),
					 connection_view);
	g_object_set_data(G_OBJECT(connection_view->priv->terminal),
					  "connection-view", connection_view);

	connection_view->priv->scrollbar = gtk_vscrollbar_new(NULL);
	gtk_range_set_adjustment(GTK_RANGE(connection_view->priv->scrollbar), VTE_TERMINAL(connection_view->priv->terminal)->adjustment);
	gtk_box_pack_start(GTK_BOX(box), connection_view->priv->scrollbar, FALSE, FALSE, 0);
 					
	gtk_widget_show_all(box);
	g_object_set_data(G_OBJECT(box), "connection-view", connection_view);
	
	connection_view->priv->box = box;

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

	connection_view = MUD_CONNECTION_VIEW(object);

	gnetwork_connection_close(GNETWORK_CONNECTION(connection_view->connection));
	g_object_unref(connection_view->connection);
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

static void
mud_connection_view_received_cb(GNetworkConnection *cxn, gconstpointer data, gulong length, gpointer user_data)
{
	MudConnectionView *view = MUD_CONNECTION_VIEW(user_data);
	g_assert(view != NULL);

	// Give plugins first crack at it	
	mud_window_handle_plugins(view->priv->window, view->priv->id, (gchar *)data, 1);
	
	vte_terminal_feed(VTE_TERMINAL(view->priv->terminal), (gchar *) data, length);

	if (view->priv->connect_hook) {
		mud_connection_view_send (MUD_CONNECTION_VIEW(user_data), view->priv->connect_string);
		g_free(view->priv->connect_string);
		view->priv->connect_hook = FALSE;
	}
}

static void
mud_connection_view_send_cb(GNetworkConnection *cxn, gconstpointer data, gulong length, gpointer user_data)
{
	// Give plugins first crack at it
	mud_window_handle_plugins(MUD_CONNECTION_VIEW(user_data)->priv->window, MUD_CONNECTION_VIEW(user_data)->priv->id, (gchar *)data, 0);
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
	g_assert(view != NULL);
	
	gnetwork_connection_close(GNETWORK_CONNECTION(view->connection));
}

void
mud_connection_view_reconnect(MudConnectionView *view)
{
	g_assert(view != NULL);
	
	gnetwork_connection_close(GNETWORK_CONNECTION(view->connection));
	gnetwork_connection_open(GNETWORK_CONNECTION(view->connection));
}

void
mud_connection_view_send(MudConnectionView *view, const gchar *data)
{
	GList *commands, *command;
	gchar *text;

	commands = mud_profile_process_commands(view->priv->profile, data);
	
	for (command = g_list_first(commands); command != NULL; command = command->next)
	{
		text = g_strdup_printf("%s\r\n", (gchar *) command->data);
		gnetwork_connection_send(GNETWORK_CONNECTION(view->connection), text, strlen(text));
		if (view->priv->profile->preferences->EchoText)
			mud_connection_view_add_text(view, text, Sent);
		g_free(text);
	}

	g_list_free(commands);
}

static void
mud_connection_view_set_terminal_colors(MudConnectionView *view)
{
	vte_terminal_set_colors(VTE_TERMINAL(view->priv->terminal),
							&view->priv->profile->preferences->Foreground,
							&view->priv->profile->preferences->Background,
							view->priv->profile->preferences->Colors, C_MAX);
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
	vte_terminal_set_scroll_on_output(VTE_TERMINAL(view->priv->terminal),
									  view->priv->profile->preferences->ScrollOnOutput);
}

static void
mud_connection_view_set_terminal_font(MudConnectionView *view)
{
	vte_terminal_set_font_from_string(VTE_TERMINAL(view->priv->terminal),
									  view->priv->profile->preferences->FontName);
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
					_("Close tab or window, whatever :)"),
					NULL,
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
	
	append_menuitem(view->priv->popup_menu,
					_("Edit Current Profile..."),
					NULL,
					view);

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
mud_connection_view_new (const gchar *profile, const gchar *hostname, const gint port, GtkWidget *window)
{
	MudConnectionView *view;
   	GdkGeometry hints;
   	gint xpad, ypad;
   	gint char_width, char_height;

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

	mud_connection_view_set_profile(view, mud_profile_new(profile));
	
	// FIXME, move this away from here
	gnetwork_connection_open(GNETWORK_CONNECTION(view->connection));

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
