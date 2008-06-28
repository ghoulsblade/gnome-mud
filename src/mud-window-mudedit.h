#ifndef MUD_EDIT_WINDOW_H
#define MUD_EDIT_WINDOW_H

G_BEGIN_DECLS

#include <gconf/gconf-client.h>

#define MUD_TYPE_EDIT_WINDOW              (mud_edit_window_get_type ())
#define MUD_EDIT_WINDOW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_EDIT_WINDOW, MudEditWindow))
#define MUD_EDIT_WINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_EDIT_WINDOW, MudEditWindowClass))
#define MUD_IS_EDIT_WINDOW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_EDIT_WINDOW))
#define MUD_IS_EDIT_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_EDIT_WINDOW))
#define MUD_EDIT_WINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_EDIT_WINDOW, MudEditWindowClass))

typedef struct _MudEditWindow            MudEditWindow;
typedef struct _MudEditWindowClass       MudEditWindowClass;
typedef struct _MudEditWindowPrivate     MudEditWindowPrivate;

struct _MudEditWindow
{
	GObject parent_instance;

	MudEditWindowPrivate *priv;
};

struct _MudEditWindowClass
{
	GObjectClass parent_class;
};

GType mud_edit_window_get_type (void) G_GNUC_CONST;

MudEditWindow *mud_window_mudedit_new(gchar *mud, MudListWindow *mudlist, gboolean NewMed);

G_END_DECLS

#endif // MUD_EDIT_WINDOW_H
