#ifndef MUD_PROFILE_WINDOW_H
#define MUD_PROFILE_WINDOW_H

G_BEGIN_DECLS

#include <gconf/gconf-client.h>

#define MUD_TYPE_PROFILE_WINDOW              (mud_profile_window_get_type ())
#define MUD_PROFILE_WINDOW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_PROFILE_WINDOW, MudProfileWindow))
#define MUD_PROFILE_WINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_PROFILE_WINDOW, MudProfileWindowClass))
#define MUD_IS_PROFILE_WINDOW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_PROFILE_WINDOW))
#define MUD_IS_PROFILE_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_PROFILE_WINDOW))
#define MUD_PROFILE_WINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_PROFILE_WINDOW, MudProfileWindowClass))

typedef struct _MudProfileWindow            MudProfileWindow;
typedef struct _MudProfileWindowClass       MudProfileWindowClass;
typedef struct _MudProfileWindowPrivate     MudProfileWindowPrivate;

struct _MudProfileWindow
{
	GObject parent_instance;

	MudProfileWindowPrivate *priv;
};

struct _MudProfileWindowClass
{
	GObjectClass parent_class;
};

GType mud_profile_window_get_type (void) G_GNUC_CONST;

MudProfileWindow *mud_window_profile_new(MudWindow *window);

G_END_DECLS

#endif // MUD_PROFILE_WINDOW_H
