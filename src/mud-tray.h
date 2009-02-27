#ifndef MUD_TRAY_H
#define MUD_TRAY_H

G_BEGIN_DECLS

#include <gconf/gconf-client.h>

#define MUD_TYPE_TRAY              (mud_tray_get_type ())
#define MUD_TRAY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_TRAY, MudTray))
#define MUD_TRAY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_TRAY, MudTrayClass))
#define MUD_IS_TRAY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_TRAY))
#define MUD_IS_TRAY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_TRAY))
#define MUD_TRAY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_TRAY, MudTrayClass))

typedef struct _MudTray            MudTray;
typedef struct _MudTrayClass       MudTrayClass;
typedef struct _MudTrayPrivate     MudTrayPrivate;

struct _MudTray
{
    GObject parent_instance;

    MudTrayPrivate *priv;
};

struct _MudTrayClass
{
    GObjectClass parent_class;
};

enum mud_tray_status
{
    offline,
    offline_connecting,
    online,
    online_connecting
        // could use a few more
};

GType mud_tray_get_type (void) G_GNUC_CONST;

MudTray *mud_tray_new(MudWindow *mainWindow, GtkWidget *window);

void mud_tray_update_icon(MudTray *tray, enum mud_tray_status icon);

G_END_DECLS

#endif // MUD_TRAY_H
