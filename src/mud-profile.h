#ifndef MUD_PROFILE_H
#define MUD_PROFILE_H

G_BEGIN_DECLS

#define MUD_TYPE_PROFILE              (mud_profile_get_type ())
#define MUD_PROFILE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_PROFILE, MudProfile))
#define MUD_PROFILE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_PROFILE, MudProfile))
#define MUD_IS_PROFILE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_PROFILE))
#define MUD_IS_PROFILE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_PROFILE))
#define MUD_PROFILE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_PROFILE, MudProfileClass))

typedef struct _MudProfile            MudProfile;
typedef struct _MudProfileClass       MudProfileClass;
typedef struct _MudProfilePrivate     MudProfilePrivate;

struct _MudProfile
{
	GObject parent_instance;

	MudProfilePrivate *priv;

	gchar *name;
};

struct _MudProfileClass
{
	GObjectClass parent_class;
};

GType mud_profile_get_type (void) G_GNUC_CONST;

MudProfile* mud_profile_new (const gchar *name);

G_END_DECLS

#endif // MUD_PROFILE_H
