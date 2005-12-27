#ifndef MUD_LIST_WINDOW_H
#define MUD_LIST_WINDOW_H

G_BEGIN_DECLS

#include <gconf/gconf-client.h>

#define MUD_TYPE_LIST_WINDOW              (mud_list_window_get_type ())
#define MUD_LIST_WINDOW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_LIST_WINDOW, MudListWindow))
#define MUD_LIST_WINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_LIST_WINDOW, MudListWindowClass))
#define MUD_IS_LIST_WINDOW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_LIST_WINDOW))
#define MUD_IS_LIST_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_LIST_WINDOW))
#define MUD_LIST_WINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_LIST_WINDOW, MudListWindowClass))

typedef struct _MudListWindow            MudListWindow;
typedef struct _MudListWindowClass       MudListWindowClass;
typedef struct _MudListWindowPrivate     MudListWindowPrivate;

struct _MudListWindow
{
	GObject parent_instance;

	MudListWindowPrivate *priv;
};

struct _MudListWindowClass
{
	GObjectClass parent_class;
};

GType mud_list_window_get_type (void) G_GNUC_CONST;

MudListWindow *mud_window_mudlist_new(void);
void mud_list_window_populate_treeview(MudListWindow *mudlist);

G_END_DECLS

#endif // MUD_LIST_WINDOW_H
