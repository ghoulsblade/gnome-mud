#ifndef MUD_PARSE_BASE_H
#define MUD_PARSE_BASE_H

G_BEGIN_DECLS

#define MUD_TYPE_PARSE_BASE              (mud_parse_base_get_type ())
#define MUD_PARSE_BASE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_PARSE_BASE, MudParseBase))
#define MUD_PARSE_BASE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_PARSE_BASE, MudParseBaseClass))
#define MUD_IS_PARSE_BASE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_PARSE_BASE))
#define MUD_IS_PARSE_BASE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_PARSE_BASE))
#define MUD_PARSE_BASE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_PARSE_BASE, MudParseBaseClass))

typedef struct _MudParseBase            MudParseBase;
typedef struct _MudParseBaseClass       MudParseBaseClass;
typedef struct _MudParseBasePrivate     MudParseBasePrivate;

struct _MudParseBase
{
	GObject parent_instance;
	
	MudParseBasePrivate *priv;
};

struct _MudParseBaseClass
{
	GObjectClass parent_class;
};

GType mud_parse_base_get_type (void) G_GNUC_CONST;

#include "mud-connection-view.h"
#include "mud-regex.h"
MudParseBase *mud_parse_base_new(MudConnectionView *parentview);

gboolean mud_parse_base_do_triggers(MudParseBase *base, gchar *data);
gboolean mud_parse_base_do_aliases(MudParseBase *base, gchar *data);
void mud_parse_base_parse(const gchar *data, gchar *stripped_data, gint ovector[1020], MudConnectionView *view, MudRegex *regex);

G_END_DECLS

#endif // MUD_PARSE_BASE_H
