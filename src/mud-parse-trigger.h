#ifndef MUD_PARSE_TRIGGER_H
#define MUD_PARSE_TRIGGER_H

G_BEGIN_DECLS

#define MUD_TYPE_PARSE_TRIGGER              (mud_parse_trigger_get_type ())
#define MUD_PARSE_TRIGGER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_PARSE_TRIGGER, MudParseTrigger))
#define MUD_PARSE_TRIGGER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_PARSE_TRIGGER, MudParseTriggerClass))
#define MUD_IS_PARSE_TRIGGER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_PARSE_TRIGGER))
#define MUD_IS_PARSE_TRIGGER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_PARSE_TRIGGER))
#define MUD_PARSE_TRIGGER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_PARSE_TRIGGER, MudParseTriggerClass))

typedef struct _MudParseTrigger            MudParseTrigger;
typedef struct _MudParseTriggerClass       MudParseTriggerClass;
typedef struct _MudParseTriggerPrivate     MudParseTriggerPrivate;

struct _MudParseTrigger
{
	GObject parent_instance;

	MudParseTriggerPrivate *priv;
};

struct _MudParseTriggerClass
{
	GObjectClass parent_class;
};

GType mud_parse_trigger_get_type (void) G_GNUC_CONST;

MudParseTrigger *mud_parse_trigger_new(void);

#include "mud-regex.h"
#include "mud-connection-view.h"
gboolean mud_parse_trigger_do(gchar *data, MudConnectionView *view, MudRegex *regex, MudParseTrigger *trigger);

G_END_DECLS

#endif // MUD_PARSE_TRIGGER_H
