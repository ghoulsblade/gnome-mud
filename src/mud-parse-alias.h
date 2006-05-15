#ifndef MUD_PARSE_ALIAS_H
#define MUD_PARSE_ALIAS_H

G_BEGIN_DECLS

#define MUD_TYPE_PARSE_ALIAS              (mud_parse_alias_get_type ())
#define MUD_PARSE_ALIAS(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_PARSE_ALIAS, MudParseAlias))
#define MUD_PARSE_ALIAS_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_PARSE_ALIAS, MudParseAliasClass))
#define MUD_IS_PARSE_ALIAS(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_PARSE_ALIAS))
#define MUD_IS_PARSE_ALIAS_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_PARSE_ALIAS))
#define MUD_PARSE_ALIAS_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_PARSE_ALIAS, MudParseAliasClass))

typedef struct _MudParseAlias            MudParseAlias;
typedef struct _MudParseAliasClass       MudParseAliasClass;
typedef struct _MudParseAliasPrivate     MudParseAliasPrivate;

struct _MudParseAlias
{
	GObject parent_instance;
	
	MudParseAliasPrivate *priv;
};

struct _MudParseAliasClass
{
	GObjectClass parent_class;
};

GType mud_parse_alias_get_type (void) G_GNUC_CONST;

MudParseAlias *mud_parse_alias_new(void);

#include "mud-connection-view.h"
#include "mud-regex.h"
gboolean mud_parse_alias_do(gchar *data, MudConnectionView *view, MudRegex *regex, MudParseAlias *alias);

G_END_DECLS

#endif // MUD_PARSE_ALIAS_H
