#ifndef MUD_PROFILE_H
#define MUD_PROFILE_H

G_BEGIN_DECLS

#include <gdk/gdkcolor.h>

#define MUD_TYPE_PROFILE              (mud_profile_get_type ())
#define MUD_PROFILE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_PROFILE, MudProfile))
#define MUD_PROFILE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_PROFILE, MudProfile))
#define MUD_IS_PROFILE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_PROFILE))
#define MUD_IS_PROFILE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_PROFILE))
#define MUD_PROFILE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_PROFILE, MudProfileClass))

#define C_MAX 16

typedef struct _MudProfile            MudProfile;
typedef struct _MudProfileClass       MudProfileClass;
typedef struct _MudProfilePrivate     MudProfilePrivate;
typedef struct _MudPrefs              MudPrefs;

struct _MudPrefs
{
	gboolean   EchoText;
	gboolean   KeepText;
	gboolean   AutoSave;
	gboolean   DisableKeys;
	gboolean   ScrollOnOutput;
	gchar     *FontName;
	gchar     *CommDev;
	gchar     *TerminalType;
	gchar     *MudListFile;
	gchar     *LastLogDir;
	gchar     *TabLocation;
	gint       History;
	gint       Scrollback;
	gint       FlushInterval;
	GdkColor   Foreground;
	GdkColor   Background;

	GdkColor   Colors[C_MAX];
};

struct _MudProfile
{
	GObject parent_instance;

	MudProfilePrivate *priv;

	gchar *name;
	MudPrefs *preferences;
};

typedef struct
{
	unsigned int EchoText : 1;
	unsigned int KeepText : 1;
	unsigned int DisableKeys : 1;
	unsigned int ScrollOnOutput : 1;
	unsigned int CommDev : 1;
	unsigned int TerminalType : 1;
	unsigned int History : 1;
	unsigned int Scrollback : 1;
	unsigned int FontName : 1;
	unsigned int Foreground : 1;
	unsigned int Background : 1;
	unsigned int Colors : 1;
} MudProfileMask;

struct _MudProfileClass
{
	GObjectClass parent_class;
	
	void (* changed) (MudProfile *profile, MudProfileMask *mask, gpointer data);
};

GType mud_profile_get_type (void) G_GNUC_CONST;

MudProfile* mud_profile_new (const gchar *name);
void mud_profile_load_profiles ();
const GList* mud_profile_get_profiles ();

void mud_profile_copy_preferences (MudProfile *from, MudProfile *to);
GList* mud_profile_process_commands (MudProfile *profile, const gchar *data);

void mud_profile_set_echotext (MudProfile *profile, gboolean value);
void mud_profile_set_keeptext (MudProfile *profile, gboolean value);
void mud_profile_set_disablekeys (MudProfile *profile, gboolean value);
void mud_profile_set_scrolloutput (MudProfile *profile, gboolean value);
void mud_profile_set_commdev (MudProfile *profile, const gchar *value);
void mud_profile_set_terminal (MudProfile *profile, const gchar *value);
void mud_profile_set_history (MudProfile *profile, const gint value);
void mud_profile_set_scrollback (MudProfile *profile, const gint value);
void mud_profile_set_font (MudProfile *profile, const gchar *value);
void mud_profile_set_foreground (MudProfile *profile, guint r, guint g, guint b);
void mud_profile_set_background (MudProfile *profile, guint r, guint g, guint b);
void mud_profile_set_colors (MudProfile *profile, gint nr, guint r, guint g, guint b);

G_END_DECLS

#endif // MUD_PROFILE_H
