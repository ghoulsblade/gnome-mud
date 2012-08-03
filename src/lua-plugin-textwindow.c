// text window for lua (chat,asci-art map etc)

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

#define LUA_TEXTWINDOW_PARAM_TEXTBUF	"textbuf"
#define LUA_TEXTWINDOW_PARAM_TEXTVIEW	"textview"

/// open monospace readonly text window, w,h in pixels
/// for lua:	window	  MUD_TextWindow_Open	(title,w,h,font="monospace 12",x=640,y=40)
static int 				l_MUD_TextWindow_Open	(lua_State* L) {
	const gchar*	title	= luaL_checkstring(L,1);
	gint			w		= luaL_checkint(L,2);
	gint			h		= luaL_checkint(L,3);
	const char*		font	= lua2_isset(L,4) ? luaL_checkstring(L,4) : "monospace 12";
	gint			x		= lua2_isset(L,5) ? luaL_checkint(L,5) : 680;
	gint			y		= lua2_isset(L,6) ? luaL_checkint(L,6) : 40;
	
	// open window
	GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	
	// set title and properties
	gtk_window_set_title(GTK_WINDOW(window),title);
	//~ gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER); // show in screen center
	gtk_window_move(GTK_WINDOW(window),x,y);
	gtk_widget_set_size_request(window,w,h);
	GtkWidget* parent = window;
	
	// autoscroll
	GtkWidget* autoscroll = gtk_scrolled_window_new(NULL,NULL);
	gtk_container_add(GTK_CONTAINER(parent),autoscroll);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (autoscroll),
                                    GTK_POLICY_AUTOMATIC, 
                                    GTK_POLICY_AUTOMATIC);
	parent = autoscroll;
	
	
	// add text
	GtkWidget* textview = gtk_text_view_new();
	GtkTextBuffer* textbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
	gtk_text_view_set_editable(GTK_TEXT_VIEW(textview),FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(textview),FALSE);
	// set font
	PangoFontDescription* fontdesc = pango_font_description_from_string(font);
	gtk_widget_modify_font(textview,fontdesc);
	// add to parent
	gtk_container_add(GTK_CONTAINER(parent),textview);
	g_object_set_data(G_OBJECT(window),LUA_TEXTWINDOW_PARAM_TEXTBUF,(gpointer)textbuf);
	g_object_set_data(G_OBJECT(window),LUA_TEXTWINDOW_PARAM_TEXTVIEW,(gpointer)textview);
	
	// present window
	gtk_widget_show_all(window);
	gtk_window_present(GTK_WINDOW(window));
	
	// return handle
	lua_pushlightuserdata(L, (void*)window);
	return 1;
}

/// for lua:	void	  MUD_TextWindow_SetText	(window,txt)
static int 				l_MUD_TextWindow_SetText	(lua_State* L) {
	GtkWidget*	window	= (GtkWidget*)lua_touserdata(L,1);
	const char*	buf		= luaL_checkstring(L,2);
	if (!window) return 0;
	
	GtkTextBuffer*		textbuf = (GtkTextBuffer*)g_object_get_data(G_OBJECT(window),LUA_TEXTWINDOW_PARAM_TEXTBUF);
	if (!textbuf) return 0;
	
	gtk_text_buffer_set_text(textbuf, buf, -1);
	return 0;
}
	
/// for lua:	void	  MUD_TextWindow_AppendText	(window,txt)
static int 				l_MUD_TextWindow_AppendText	(lua_State* L) {
	GtkWidget*	window	= (GtkWidget*)lua_touserdata(L,1);
	const char*	buf		= luaL_checkstring(L,2);
	if (!window) return 0;
	
	GtkTextBuffer*		textbuf = (GtkTextBuffer*)g_object_get_data(G_OBJECT(window),LUA_TEXTWINDOW_PARAM_TEXTBUF);
	if (!textbuf) return 0;
	
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter(textbuf,&iter);
	gtk_text_buffer_insert(textbuf,&iter,buf,-1);
	return 0;
}

/// for lua:	void	  MUD_TextWindow_ScrollToEnd	(window)
static int 				l_MUD_TextWindow_ScrollToEnd	(lua_State* L) {
	GtkWidget*			window = (GtkWidget*)lua_touserdata(L,1);
	if (!window) return 0;
	
	GtkTextBuffer*		textbuf = (GtkTextBuffer*)g_object_get_data(G_OBJECT(window),LUA_TEXTWINDOW_PARAM_TEXTBUF);
	GtkWidget*			textview = (GtkWidget*)g_object_get_data(G_OBJECT(window),LUA_TEXTWINDOW_PARAM_TEXTVIEW);
	if (!textbuf || !textview) return 0;
	GtkTextIter end_start_iter;
	gtk_text_buffer_get_end_iter(textbuf,&end_start_iter);
	GtkTextMark* insert_mark = gtk_text_buffer_get_insert(textbuf);
	gtk_text_buffer_place_cursor(textbuf,&end_start_iter);
	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(textview),insert_mark, 0.0, TRUE, 0.0, 1.0); 
	return 0;
}


void	InitLuaEnvironment_Textwindow	(lua_State* L) {
	lua_register(L,"MUD_TextWindow_Open",			l_MUD_TextWindow_Open); 
	lua_register(L,"MUD_TextWindow_SetText",		l_MUD_TextWindow_SetText); 
	lua_register(L,"MUD_TextWindow_AppendText",		l_MUD_TextWindow_AppendText); 
	lua_register(L,"MUD_TextWindow_ScrollToEnd",	l_MUD_TextWindow_ScrollToEnd); 
}

#endif // ENABLE_LUA
