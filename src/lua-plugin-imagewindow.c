// image window for lua (icon map etc)

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef ENABLE_LUA
#include "lua-plugin.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include <glade/glade-xml.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <vte/vte.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define LUA_IMAGEWINDOW_PARAM_BLA	"bla"

/// open image window, w,h in pixels
/// for lua:	window	  MUD_ImageWindow_Open	(title,w,h,x=640,y=40)
static int 				l_MUD_ImageWindow_Open	(lua_State* L) {
	const gchar* title	= luaL_checkstring(L,1);
	gint w				= luaL_checkint(L,2);
	gint h				= luaL_checkint(L,3);
	gint x				= lua2_isset(L,4) ? luaL_checkint(L,4) : 680;
	gint y				= lua2_isset(L,5) ? luaL_checkint(L,5) : 40;
	
	// open window
	GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	
	// set title and properties
	gtk_window_set_title(GTK_WINDOW(window),title);
	//~ gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER); // show in screen center
	gtk_window_move(GTK_WINDOW(window),x,y);
	gtk_widget_set_size_request(window,w,h);
	GtkWidget* parent = window;
	
	// TODO...
	
	// present window
	gtk_widget_show_all(window);
	gtk_window_present(GTK_WINDOW(window));
	
	// return handle
	lua_pushlightuserdata(L, (void*)window);
	return 1;
}

/// for lua:	void	  MUD_ImageWindow_AddText	(txt,x,y)
static int 				l_MUD_ImageWindow_AddText	(lua_State* L) {
	GtkWidget*			window = (GtkWidget*)lua_touserdata(L,1);
	const char*			buf = luaL_checkstring(L,2);
	gint x				= luaL_checkint(L,3);
	gint y				= luaL_checkint(L,4);
	if (!window) return 0;
	
	//~ GtkTextBuffer*		textbuf = (GtkTextBuffer*)g_object_get_data(G_OBJECT(window),LUA_WINDOW_PARAM_TEXTBUF);
	//~ if (!textbuf) return 0;
	//~ gtk_text_buffer_set_text(textbuf, buf, -1);
	return 0;
}
	
void	InitLuaEnvironment_Imagewindow	(lua_State* L) {
	lua_register(L,"MUD_ImageWindow_Open",			l_MUD_ImageWindow_Open); 
	lua_register(L,"MUD_ImageWindow_AddText",		l_MUD_ImageWindow_AddText); 
}

#endif // ENABLE_LUA
