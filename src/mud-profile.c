#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib-object.h>

#include "mud-profile.h"

static char const rcsid[] = "$Id: ";

struct _MudProfilePrivate
{
};

static void mud_profile_init        (MudProfile *profile);
static void mud_profile_class_init  (MudProfileClass *profile);
static void mud_profile_finalize    (GObject *object);

GType
mud_profile_get_type (void)
{
	static GType object_type = 0;

	g_type_init();

	if (!object_type)
	{
		static const GTypeInfo object_info =
		{
			sizeof (MudProfileClass),
			NULL,
			NULL,
			(GClassInitFunc) mud_profile_class_init,
			NULL,
			NULL,
			sizeof (MudProfile),
			0,
			(GInstanceInitFunc) mud_profile_init,
		};

		object_type = g_type_register_static(G_TYPE_OBJECT, "MudProfile", &object_info, 0);
	}

	return object_type;
}

static void
mud_profile_init (MudProfile *profile)
{
	profile->priv = g_new0(MudProfilePrivate, 1);
}

static void
mud_profile_class_init (MudProfileClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->finalize = mud_profile_finalize;
}

static void
mud_profile_finalize (GObject *object)
{
	MudProfile *profile;
	GObjectClass *parent_class;

	profile = MUD_PROFILE(object);

	g_free(profile->priv);

	g_message("finalizing profile %s",  profile->name);

	parent_class = g_type_class_peek_parent(G_OBJECT_GET_CLASS(object));
	parent_class->finalize(object);
}

MudProfile*
mud_profile_new (const gchar *name)
{
	MudProfile *profile;

	g_assert(name != NULL);
	
	profile = g_object_new(MUD_TYPE_PROFILE, NULL);
	profile->name = g_strdup(name);

	return profile;
}
