/* AMCL - A simple Mud CLient
 * Copyright (C) 1998-1999 Robin Ericsson <lobbin@localhost.nu>
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

#ifndef __AMCL_H__
#define __AMCL_H__

#include <stdio.h>
/*
 * Different type of message, so I'll know what color to use.
 */
#define MESSAGE_ERR     0
#define MESSAGE_NORMAL  1
#define MESSAGE_ANSI    2
#define MESSAGE_NONE    3
#define MESSAGE_SENT    4

/*
 * Save file version
 */
#define WIZARD_SAVEFILE_VERSION 1

/*
 * Typedefs
 */
typedef struct connection_data CONNECTION_DATA;
typedef struct alias_data      ALIAS_DATA;
typedef struct action_data     ACTION_DATA;
typedef struct wizard_data     WIZARD_DATA;
typedef struct system_data     SYSTEM_DATA;
typedef struct keybind_data    KEYBIND_DATA;
typedef        gint            bool;

/*
 * Structures
 */
struct connection_data {
  gchar      *host;
  gchar      *port;
  gint        data_ready;
  gint        sockfd;
  gint        connected;
  gint        notebook;
  gboolean    echo;
  GtkWidget  *window;
  GtkWidget  *vscrollbar;
};

struct alias_data {
    ALIAS_DATA *next;
    gchar      *alias;
    gchar      *replace;
};

struct action_data {
    ACTION_DATA *next;
    gchar       *trigger;
    gchar       *action;
};

struct system_data {
    bool       EchoText;
    bool       KeepText;
    bool       AutoSave;
    bool       Freeze;
    gchar     *FontName;
    gchar     *CommDev;
};

struct wizard_data {
    WIZARD_DATA *next;
    gchar      *name;
    gchar      *hostname;
    gchar      *port;
    gchar      *cstring;
    gchar      *playername;
    gchar      *password;
    bool       autologin;
};

struct keybind_data {
    gint	 state;
    gint	 keyv;
    gchar	 *data;
    KEYBIND_DATA *next;
};

/*
 * Function declares
 */

/* action.c */
void              load_actions (void);
int               check_actions (gchar *, gchar *);
void              window_action (GtkWidget *, gpointer);

/* alias.c */
void              load_aliases (void);
int               check_aliases (gchar *, gchar *);
void              window_alias  (GtkWidget *, gpointer);

/* net.c */
CONNECTION_DATA  *make_connection (gchar *, gchar *);
void              disconnect (GtkWidget *, CONNECTION_DATA *);
void              send_to_connection (GtkWidget *, gpointer);
void              connection_send (CONNECTION_DATA *, gchar *);

/* init.c */
void              init_window (void);
void              destroy (GtkWidget *);
void              close_window (GtkWidget *, gpointer);

/* color.c */
void  create_color_box( void                               );
void  window_color    ( GtkWidget *widget, gpointer data   );
void  save_colors     ( void				   );	
void  load_colors     ( void				   );
void  load_color_to_c_structs ( void			   );

/* dialog.c */
void  doc_dialog      ( GtkWidget *widget, gint index      );

/* keybind.c */
void window_keybind   ( void				   );
void save_keys	      ( void				   );	
void load_keys	      ( void				   );

/* map.c */
void  window_automap  ( GtkWidget *widget, gpointer data   );

/* misc.c */
void  init_uid        ( void                               );

/* modules.c */
void  do_plugin_information (GtkWidget *w, gpointer data   );
int   init_modules    ( char *path                         );
void  save_plugins    ( void                               );

/* prefs.c */
void  load_prefs      ( void                               );
void  save_prefs      ( void                               );
void  window_prefs    ( GtkWidget *widget, gpointer data   );
FILE *open_file       ( gchar *filename, gchar *mode       );

/* telnet.c */
char *pre_process     ( char *buf, CONNECTION_DATA *connection       );

/* window.c */
void  popup_window    ( const gchar *message                         );
void  switch_page_cb  ( GtkNotebook *, gpointer, guint, gpointer     );
void	grab_focus_cb   ( GtkWidget* widget, gpointer user_data        );
void  textfield_add   ( GtkWidget *widget, gchar *me, gint colortype );

/* wizard.c */
void  free_connection_data (CONNECTION_DATA *c             );
void  load_wizard        ( void                            );
void  add_wizard_connect ( gchar **entry, bool al, WIZARD_DATA *w   );
void  window_wizard      ( GtkWidget *widget,gpointer data );

/* version.c */
char * get_version_info (char *buf);

#endif /* __AMCL_H__ */

