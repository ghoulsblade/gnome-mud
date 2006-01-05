#ifndef MUD_MCONNECT_WINDOW_H
#define MUD_MCONNECT_WINDOW_H

G_BEGIN_DECLS

#include <gconf/gconf-client.h>

#define MUD_TYPE_MCONNECT_WINDOW              (mud_mconnect_window_get_type ())
#define MUD_MCONNECT_WINDOW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_MCONNECT_WINDOW, MudMConnectWindow))
#define MUD_MCONNECT_WINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_MCONNECT_WINDOW, MudMConnectWindowClass))
#define MUD_IS_MCONNECT_WINDOW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_MCONNECT_WINDOW))
#define MUD_IS_MCONNECT_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_MCONNECT_WINDOW))
#define MUD_MCONNECT_WINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_MCONNECT_WINDOW, MudMConnectWindowClass))

typedef struct _MudMConnectWindow            MudMConnectWindow;
typedef struct _MudMConnectWindowClass       MudMConnectWindowClass;
typedef struct _MudMConnectWindowPrivate     MudMConnectWindowPrivate;

struct _MudMConnectWindow
{
	GObject parent_instance;
	
	MudMConnectWindowPrivate *priv;
};

struct _MudMConnectWindowClass
{
	GObjectClass parent_class;
};

GType mud_connect_window_get_type (void) G_GNUC_CONST;

MudMConnectWindow *mud_window_mconnect_new(MudWindow *win, GtkWidget *winwidget, MudTray *tray);

G_END_DECLS

#endif // MUD_MCONNECT_WINDOW_H
