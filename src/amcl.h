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

/*
 * Different type of message, so I'll know what color to use.
 */
#define MESSAGE_ERR     0
#define MESSAGE_NORMAL  1
#define MESSAGE_ANSI    2
#define MESSAGE_NONE    3
#define MESSAGE_SENT    4

/*
 * Typedefs
 */
typedef struct connection_data CONNECTION_DATA;
typedef struct alias_data      ALIAS_DATA;
typedef struct action_data     ACTION_DATA;
typedef struct wizard_data     WIZARD_DATA;
typedef struct system_data     SYSTEM_DATA;
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
  GtkWidget  *window;
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

/*
 * Function declares
 */
#define AD ALIAS_DATA
#define WD WIZARD_DATA

/* action.c */
void  load_actions    ( void                               );
void  save_actions    ( GtkWidget *button, gpointer data   );
void  add_action      ( gchar *trigger, gchar *action      );
void  insert_actions  ( ACTION_DATA *a, GtkCList *clist    );
int   check_actions   ( gchar *incoming, gchar *outgoing   );
void  window_action   ( GtkWidget *widget, gpointer data   );

/* alias.c */
void  load_aliases    ( void                               );
void  save_aliases    ( GtkWidget *button, gpointer data   );
void  add_alias       ( gchar *alias, gchar *replacement   );
void  insert_aliases  ( GtkWidget *clist                   );

/* color.c */
void  create_color_box( void                               );
void  window_color    ( GtkWidget *widget, gpointer data   );

/* dialog.c */
void  doc_dialog      ( GtkWidget *widget, gint index      );

/* init.c */
void  init_window     ( void                               );
void  init_colors     ( void                               );
void  destroy         ( GtkWidget *widget                  );
void  close_window    ( GtkWidget *widget, gpointer data   );

/* map.c */
void  window_automap  ( GtkWidget *widget, gpointer data   );

/* modules.c */
int   init_modules    ( char *path                         );

/* net.c */
void  make_connection ( GtkWidget *widget, gpointer data    );
void  disconnect      ( GtkWidget *widget, CONNECTION_DATA *);
void  open_connection ( CONNECTION_DATA *connection         );
void  read_from_connection (CONNECTION_DATA *connection,
                            gint source,
                            GdkInputCondition condition     );
void  send_to_connection (GtkWidget *widget, gpointer data  );
void  connection_send ( gchar *message                      );

/* prefs.c */
void  load_prefs      ( void                               );
void  window_prefs    ( GtkWidget *widget, gpointer data   );
int   check_amcl_dir  ( gchar *dirname                     );

/* window.c */
void  window_alias    ( GtkWidget *widget, gpointer data             );
void  popup_window    ( const gchar *message                         );
void  switch_page_cb  ( GtkNotebook *, gpointer, guint, gpointer     );
void  textfield_add   ( GtkWidget *widget, gchar *me, gint colortype );

/* wizard.c */
void  load_wizard        ( void                            );
void  add_wizard_connect ( gchar **entry, bool al, WD *w   );
void  window_wizard      ( GtkWidget *widget,gpointer data );

#undef AD
#undef WD

/*
 * Variables declared somewhere else...
 */

/* init.c */
extern CONNECTION_DATA *main_connection;
extern GList *Connections;

extern GtkWidget *main_notebook;
extern GtkWidget *text_entry;
extern GtkWidget *entry_host;
extern GtkWidget *entry_port;
extern GtkWidget *menu_plugin_menu;
extern GtkWidget *menu_main_wizard;
extern GtkWidget *menu_main_connect;
extern GtkWidget *menu_main_disconnect;
extern GtkWidget *menu_option_prefs;
extern GtkWidget *menu_option_alias;
extern GtkWidget *menu_option_colors;
extern GtkWidget *menu_option_action;
extern GtkWidget *window;
extern GtkWidget *color_window;

extern GdkColor color_white;
extern GdkColor color_black;
extern GdkColor color_blue;
extern GdkColor color_green;
extern GdkColor color_red;
extern GdkColor color_brown;
extern GdkColor color_magenta;
extern GdkColor color_lightred;
extern GdkColor color_yellow;
extern GdkColor color_lightgreen;
extern GdkColor color_cyan;
extern GdkColor color_lightcyan;
extern GdkColor color_lightblue;
extern GdkColor color_lightmagenta;
extern GdkColor color_grey;
extern GdkColor color_lightgrey;
extern GdkColor *foreground;
extern GdkColor *background;

extern GdkFont  *font_normal;
extern GdkFont  *font_bold;


/* net.c */
extern bool      echo;
extern gchar     *host;
extern gchar     *port;

/* prefs.c */
extern SYSTEM_DATA prefs;

/* wizard.c */
extern WIZARD_DATA *wizard_connection_list;
#endif /* __AMCL_H__ */

