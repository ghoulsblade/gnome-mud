/* GNOME-Mud - A simple Mud Client
 * mud-profile.h
 * Copyright (C) 1998-2005 Robin Ericsson <lobbin@localhost.nu>
 * Copyright (C) 2005-2009 Les Harris <lharris@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef MUD_PROFILE_H
#define MUD_PROFILE_H

G_BEGIN_DECLS

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#define MUD_TYPE_PROFILE              (mud_profile_get_type ())
#define MUD_PROFILE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MUD_TYPE_PROFILE, MudProfile))
#define MUD_PROFILE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MUD_TYPE_PROFILE, MudProfileClass))
#define IS_MUD_PROFILE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MUD_TYPE_PROFILE))
#define IS_MUD_PROFILE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MUD_TYPE_PROFILE))
#define MUD_PROFILE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MUD_TYPE_PROFILE, MudProfileClass))
#define MUD_PROFILE_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MUD_TYPE_PROFILE, MudProfilePrivate))

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
    gchar     *LastLogDir;
    gchar     *Encoding;
    gchar     *ProxyVersion;
    gchar     *ProxyHostname;

    gint       History;
    gint       Scrollback;
    gint       FlushInterval;

    GdkColor   Foreground;
    GdkColor   Background;

    GSList	  *alias_names;
    GSList	  *trigger_names;

    gboolean UseRemoteEncoding;
    gboolean UseProxy;
    gboolean UseRemoteDownload;

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
    unsigned int CommDev : 1;
    unsigned int History : 1;
    unsigned int ScrollOnOutput : 1;
    unsigned int Scrollback : 1;
    unsigned int FontName : 1;
    unsigned int Foreground : 1;
    unsigned int Background : 1;
    unsigned int Colors : 1;
    unsigned int UseRemoteEncoding : 1;
    unsigned int UseProxy : 1;
    unsigned int Encoding : 1;
    unsigned int ProxyVersion : 1;
    unsigned int ProxyHostname : 1;
    unsigned int UseRemoteDownload : 1;
} MudProfileMask;

struct _MudProfileClass
{
    GObjectClass parent_class;

    void (* changed) (MudProfile *profile, MudProfileMask *mask, gpointer data);
};

GType mud_profile_get_type (void);

void mud_profile_copy_preferences (MudProfile *from, MudProfile *to);

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
void mud_profile_set_encoding_combo(MudProfile *profile, const gchar *e);
void mud_profile_set_encoding_check (MudProfile *profile, const gint value);
void mud_profile_set_proxy_check (MudProfile *profile, const gint value);
void mud_profile_set_proxy_combo(MudProfile *profile, GtkComboBox *combo);
void mud_profile_set_proxy_entry (MudProfile *profile, const gchar *value);
void mud_profile_set_msp_check (MudProfile *profile, const gint value);

G_END_DECLS

#endif // MUD_PROFILE_H

