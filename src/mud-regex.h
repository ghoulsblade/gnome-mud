#ifndef MUD_REGEX_H
#define MUD_REGEX_H

G_BEGIN_DECLS

#define MUD_TYPE_REGEX              (mud_regex_get_type ())
#define MUD_REGEX(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_REGEX, MudRegex))
#define MUD_REGEX_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_REGEX, MudRegexClass))
#define MUD_IS_REGEX(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_REGEX))
#define MUD_IS_REGEX_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_REGEX))
#define MUD_REGEX_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_REGEX, MudRegexClass))

typedef struct _MudRegex            MudRegex;
typedef struct _MudRegexClass       MudRegexClass;
typedef struct _MudRegexPrivate     MudRegexPrivate;

struct _MudRegex
{
    GObject parent_instance;

    MudRegexPrivate *priv;
};

struct _MudRegexClass
{
    GObjectClass parent_class;
};

GType mud_regex_get_type (void) G_GNUC_CONST;

MudRegex *mud_regex_new(void);

gboolean mud_regex_check(const gchar *data, guint length, const gchar *rx, gint ovector[1020], MudRegex *regex);
const gchar **mud_regex_test(const gchar *data, guint length, const gchar *rx, gint *rc, const gchar **error, gint *errorcode, gint *erroroffset);
void mud_regex_substring_clear(const gchar **substring_list);
const gchar **mud_regex_get_substring_list(gint *count, MudRegex *regex);

G_END_DECLS

#endif // MUD_REGEX_H
