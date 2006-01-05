/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1998-2005 Robin Ericsson <lobbin@localhost.nu>
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

#ifndef __GNOME_MUD_H__
#define __GNOME_MUD_H__

#include "mccpDecompress.h"
#include <stdio.h>
#include <vte/vte.h>
#include <libgnetwork/gnetwork.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

/*
 * Different type of message, so I'll know what color to use.
 */
#define MESSAGE_ERR     0
#define MESSAGE_NORMAL  1
#define MESSAGE_SENT    2
#define MESSAGE_SYSTEM	3

/*
 * Different location of the connection tabs
 */
#define TAB_LOCATION_TOP		0
#define TAB_LOCATION_RIGHT		1
#define TAB_LOCATION_BOTTOM		2
#define TAB_LOCATION_LEFT		3

/*
 * Maximum number of connections possible
 */
#define MAX_CONNECTIONS 16

/*
 * Maximum number of colors
 */
#define C_MAX 16

/*
 * Save file version
 */
#define WIZARD_SAVEFILE_VERSION 1

/*
 * Typedefs
 */
typedef struct connection_data CONNECTION_DATA;
typedef struct wizard_data2    WIZARD_DATA2;
typedef struct system_data     SYSTEM_DATA;
typedef struct keybind_data    KEYBIND_DATA;
typedef	struct profile_data	   PROFILE_DATA;
#ifndef WITHOUT_MAPPER
	typedef struct _AutoMap        AutoMap;
	typedef struct _AutoMapConfig  AutoMapConfig;
#endif
typedef        gint            bool;

/*
 * Structures
 */
struct connection_data 
{
	PROFILE_DATA	*profile;
	FILE			*log;
	time_t			 last_log_flush;
	mc_state		*mccp;
	gint			 mccp_timer;
	gchar			*host;
	gchar			*port;
	gchar			*log_filename;
	gint			 data_ready;
	gint			 sockfd;
	gint			 connected;
	gint			 notebook;
	gint			 logging;
	gboolean		 echo;
	gboolean		 naws;			/* whether mud supports NAWS */
	GtkWidget		*window;
	GtkWidget		*vscrollbar;
  	gint             telnet_state;
  	gint             telnet_subneg;
	gint             conn_id;
};

struct system_data {
	bool       EchoText;
	bool       KeepText;
	bool       AutoSave;
	bool       DisableKeys;
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

struct wizard_data2 {
	gchar   *name;
	gchar   *hostname;
	gchar   *port;
	gchar   *playername;
	gchar   *password;
	gchar   *profile;
};


struct keybind_data {
    gint	 state;
    gint	 keyv;
    gchar	 *data;
    KEYBIND_DATA *next;
};

struct profile_data {
	gchar 			*name;
	GList			*alias;
	GList			*variables;
	GList			*triggers;
	GtkCList		*keys;
	GSList			*directions;
	KEYBIND_DATA	*kd;
};

#ifndef WITHOUT_MAPPER
struct _AutoMapConfig
{
	GList* unusual_exits;
};
#endif


/*
 * Function declares
 */

/* data.c */
void		 load_data (GtkCList *, gchar *);
void		 window_data(PROFILE_DATA *, gint);
gchar		*check_actions (GList *, gchar *);
gchar		*check_alias (GList *, gchar *);
gchar		*check_vars (GList *, gchar *);

/* net.c */
CONNECTION_DATA	*make_connection (gchar *, gchar *, gchar *);
void		 action_send_to_connection(gchar *, CONNECTION_DATA *);
void		 disconnect (GtkWidget *, CONNECTION_DATA *);
void		 send_to_connection (GtkWidget *, gpointer);
void		 connection_send (CONNECTION_DATA *, gchar *);
void		 connection_send_secret (CONNECTION_DATA *, gchar *);
void		 connection_send_data (CONNECTION_DATA *, gchar *, int, gboolean);
void		 connection_send_telnet_control (CONNECTION_DATA *, int len, ...);
void		 open_connection (CONNECTION_DATA *);

/* init.c */
void		 main_window (void);
gboolean	 destroy (GtkWidget *);
void		 close_window (GtkWidget *, gpointer);

/* color.c */
void  create_color_box( void                               );
void  window_color    ( GtkWidget *widget, gpointer data   );
void  save_colors     ( void				   );	
void  load_colors     ( void				   );
void  load_color_to_c_structs ( void			   );

/* keybind.c */
void window_keybind   ( PROFILE_DATA *		   );

/* directions.c */
void window_directions( PROFILE_DATA *		   );

/* log.c */
void  window_menu_file_start_logging_cb ( GtkWidget *widget, gpointer data );
void  window_menu_file_stop_logging_cb  ( GtkWidget *widget, gpointer data );
void  window_menu_file_save_buffer_cb   ( GtkWidget *widget, gpointer data );
void  stop_logging_connection           ( CONNECTION_DATA *connection      );

#ifndef WITHOUT_MAPPER
/* map.c */
void window_automap   ( GtkWidget *widget, gpointer data);
void user_command     ( AutoMap* automap, const gchar* command);
#endif

#endif /* __GNOME_MUD_H__ */
