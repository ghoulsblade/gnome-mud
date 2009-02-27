#ifndef MUD_WINDOW_H
#define MUD_WINDOW_H

G_BEGIN_DECLS

#include <gconf/gconf-client.h>

#define MUD_TYPE_WINDOW             (mud_window_get_type ())
#define MUD_WINDOW(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_WINDOW, MudWindow))
#define MUD_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_WINDOW, MudWindowClass))
#define MUD_IS_WINDOW(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_WINDOW))
#define MUD_IS_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_WINDOW))
#define MUD_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_WINDOW, MudWindowClass))

typedef struct _MudWindow           MudWindow;
typedef struct _MudWindowClass      MudWindowClass;
typedef struct _MudWindowPrivate    MudWindowPrivate;

struct _MudWindow
{
	GObject parent_instance;


	MudWindowPrivate *priv;
};

struct _MudWindowClass
{
	GObjectClass parent_class;
};

GType mud_window_get_type (void) G_GNUC_CONST;

#include "mud-connection-view.h"
MudWindow* mud_window_new (void);
void mud_window_add_connection_view(MudWindow *window, MudConnectionView *view, gchar *tabLbl);
void mud_window_handle_plugins(MudWindow *window, gint id, gchar *data, guint length, gint dir);
void mud_window_populate_profiles_menu(MudWindow *window);
void mud_window_profile_menu_set_active(gchar *name, MudWindow *window);
void mud_window_close_current_window(MudWindow *window);
void mud_window_disconnected(MudWindow *window);

GtkWidget* mud_window_get_window(MudWindow *window);

extern GtkWidget *pluginMenu;

G_END_DECLS

#endif /* MUD_WINDOW_H */
