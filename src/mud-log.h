#ifndef MUD_LOG_H
#define MUD_LOG_H

G_BEGIN_DECLS

#include <gconf/gconf-client.h>

#define MUD_TYPE_LOG              (mud_log_get_type ())
#define MUD_LOG(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_LOG, MudLog))
#define MUD_LOG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_LOG, MudLogClass))
#define MUD_IS_LOG(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_LOG))
#define MUD_IS_LOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_LOG))
#define MUD_LOG_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_LOG, MudLogClass))

typedef struct _MudLog            MudLog;
typedef struct _MudLogClass       MudLogClass;
typedef struct _MudLogPrivate     MudLogPrivate;

struct _MudLog
{
	GObject parent_instance;

	MudLogPrivate *priv;
};

struct _MudLogClass
{
	GObjectClass parent_class;
};

GType mud_log_get_type (void) G_GNUC_CONST;

MudLog *mud_log_new(gchar *mudName);

void mud_log_write_hook(MudLog *log, gchar *data, gint length);

void mud_log_open(MudLog *log);
void mud_log_close(MudLog *log);
gboolean mud_log_islogging(MudLog *log);

G_END_DECLS

#endif // MUD_LOG_H
