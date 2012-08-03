// lua scripting and some utils by ghoulsblade@schattenkind.net 2012-07

// example lua file : ~/gnome-mud.lua		(LUA_MAIN_FILENAME)
/*
DIR_IN  = 1
DIR_OUT = 0
function on_data (txt,dir,view,rawdata)
	print("on_data",txt,dir)
	if (dir == DIR_IN and string.find(txt,"test01")) then MUD_Send("blub!",view) end
end
*/

#define LUA_NETWORK

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef ENABLE_LUA

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

#include "gnome-mud.h"
#include "gnome-mud-icons.h"
#include "mud-connection-view.h"
#include "mud-window.h"
#include "mud-tray.h"
#include "mud-profile.h"
#include "mud-window-profile.h"
#include "mud-parse-base.h"
#include "mud-connections.h"
#include "utils.h" // utils_strip_ansi


// ***** ***** ***** ***** ***** rest

lua_State*	L = NULL;
gboolean	gbLuaPlugin_Init_done = FALSE;
#define	LUA_MAIN_FILENAME	"gnome-mud.lua"
#define	LUA_ON_DATA		"on_data" // this lua function gets notified of incoming and outgoing data
#define	LUA_MYRELOAD	"myreload" // if player writes this, lua is reloaded
#define DIR_IN  1 // server to client, message from server
#define DIR_OUT 0 // client to server, user input


void	LuaPlugin		(const gchar* buf, guint len, MudConnectionView* view,int dir);
void	LuaNetInit		();
void	LuaNetCleanup	();
void	InitLuaEnvironment			(lua_State*	L);
int 	PCallWithErrFuncWrapper		(lua_State *L,int narg, int nret);
void	LuaPlugin_ExecLuaFile		();

/// new hook for git master after v0.11.2
void	LuaPlugin_data_hook			(MudConnectionView* view,const gchar *data,guint length,int dir) {
	GString *buf = g_string_new(NULL);
	int i;

	for(i = 0; i < length; i++)
		g_string_append_c(buf, data[i]);
	
	LuaPlugin(buf->str, length, view, dir);
	
	g_string_free(buf, FALSE);
}
	
void	LuaPlugin_Init () {
	if (gbLuaPlugin_Init_done) return;
	gbLuaPlugin_Init_done = TRUE; // singleton, init only once. 
	// TODO: better place in code than on first use ?
	// TODO: cleanup at exit ?
	
	L = luaL_newstate();
	g_assert(L);
	
	InitLuaEnvironment(L);
	LuaPlugin_ExecLuaFile();
	
#ifdef LUA_NETWORK
	LuaNetInit();
#endif
}

/// TODO: call this for program shutdown
void	LuaPlugin_Cleanup () {
	if (!gbLuaPlugin_Init_done) return;
	if (L) { lua_close(L); L = NULL; }
#ifdef LUA_NETWORK
	LuaNetCleanup();
#endif
}

void	LuaPlugin_ExecLuaFile () {
	gchar filename[1024];
	g_snprintf(filename, 1024, "%s/%s", g_get_home_dir(), LUA_MAIN_FILENAME);
	int res = luaL_dofile(L,filename);
	if (res) {
		fprintf(stderr,"lua: error in main script-initialisation:\n");
		fprintf(stderr,"%s\n",lua_tostring(L,-1));
	}
}

void	LuaPlugin	(const gchar* buf, guint len, MudConnectionView* view,int dir) { 
    gchar* stripped_data;

	g_assert(!buf[len]);
	
	stripped_data = utils_strip_ansi((const gchar *) buf); // warning: assume zero terminated v0.11.2+x(master)
	//~ stripped_data = strip_ansi(buf); // warning: assume zero terminated  v0.11.2
	
	LuaPlugin_Init();

	// reload lua on keyword
	if (dir == DIR_OUT && strstr((const char*)stripped_data,LUA_MYRELOAD)) LuaPlugin_ExecLuaFile();
	
	// lua
	if (L) {
		int narg = 4, nres = 0;
		lua_getglobal(L, LUA_ON_DATA); // get function
		//~ luaL_checkstack(L, narg, "too many arguments");
		lua_pushstring(L, (const char*)stripped_data); // arg 1
		lua_pushnumber(L, dir); // arg 2
		lua_pushlightuserdata(L, (void*)view); // arg 3
		lua_pushstring(L, (const char*)buf); // arg 4
		if (PCallWithErrFuncWrapper(L,narg, nres) != 0) {
			fprintf(stderr,"lua: error running function `%s': %s\n",LUA_ON_DATA, lua_tostring(L, -1));
		}
		if (nres > 0) lua_pop(L, nres);
	}
	
	// free buffer
	if (stripped_data) g_free(stripped_data);
}

// ***** ***** ***** ***** ***** lua utils

/// also adds a traceback to the error message in case of an error, better than a plain lua_call
/// nret=-1 for unlimited
/// don't use directly, stack has to be prepared and cleaned up
int 	PCallWithErrFuncWrapper (lua_State *L,int narg, int nret) {
	int errfunc = 0;
	return lua_pcall(L, narg, (nret==-1) ? LUA_MULTRET : nret, errfunc);
}

// ***** ***** ***** ***** ***** api

#define lua2_isset(L,i) (lua_gettop(L) >= i && !lua_isnil(L,i))

/// simulate user input. view = param from LUA_ON_DATA call
/// for lua:	void	MUD_Send	(txt,view)
static int 				l_MUD_Send	(lua_State *L) {
	const char*			buf = luaL_checkstring(L,1);
	MudConnectionView*	view = (MudConnectionView*)lua_touserdata(L,2);
	if (view) mud_connection_view_send(view, (const gchar *)buf);
	return 0;
}

#define LUA_WINDOW_PARAM_TEXTBUF	"textbuf"
#define LUA_WINDOW_PARAM_TEXTVIEW	"textview"

/// open monospace readonly text window, w,h in pixels
/// for lua:	window	  MUD_TextWindow_Open	(title,w,h,font="monospace 12",x=640,y=40)
static int 				l_MUD_TextWindow_Open	(lua_State *L) {
	const gchar* title	= luaL_checkstring(L,1);
	int w				= luaL_checkint(L,2);
	int h				= luaL_checkint(L,3);
	const char*	font	= lua2_isset(L,4) ? luaL_checkstring(L,4) : "monospace 12";
	gint x				= lua2_isset(L,5) ? luaL_checkint(L,5) : 680;
	gint y				= lua2_isset(L,6) ? luaL_checkint(L,6) : 40;
	
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
	GtkWidget* view = gtk_text_view_new();
	GtkTextBuffer* textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	gtk_text_view_set_editable(GTK_TEXT_VIEW(view),FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view),FALSE);
	// set font
	PangoFontDescription* fontdesc = pango_font_description_from_string(font);
	gtk_widget_modify_font(view,fontdesc);
	// add to parent
	gtk_container_add(GTK_CONTAINER(parent),view);
	g_object_set_data(G_OBJECT(window),LUA_WINDOW_PARAM_TEXTBUF,(gpointer)textbuf);
	g_object_set_data(G_OBJECT(window),LUA_WINDOW_PARAM_TEXTVIEW,(gpointer)view);
	
	// present window
	gtk_widget_show_all(window);
	gtk_window_present(GTK_WINDOW(window));
	
	// return handle
	lua_pushlightuserdata(L, (void*)window);
	return 1;
}

/// for lua:	void	  MUD_TextWindow_SetText	(txt)
static int 				l_MUD_TextWindow_SetText	(lua_State *L) {
	GtkWidget*			window = (GtkWidget*)lua_touserdata(L,1);
	const char*			buf = luaL_checkstring(L,2);
	if (!window) return 0;
	
	GtkTextBuffer*		textbuf = (GtkTextBuffer*)g_object_get_data(G_OBJECT(window),LUA_WINDOW_PARAM_TEXTBUF);
	if (!textbuf) return 0;
	
	gtk_text_buffer_set_text(textbuf, buf, -1);
	return 0;
}
	
/// for lua:	void	  MUD_TextWindow_AppendText	(txt)
static int 				l_MUD_TextWindow_AppendText	(lua_State *L) {
	GtkWidget*			window = (GtkWidget*)lua_touserdata(L,1);
	const char*			buf = luaL_checkstring(L,2);
	if (!window) return 0;
	
	GtkTextBuffer*		textbuf = (GtkTextBuffer*)g_object_get_data(G_OBJECT(window),LUA_WINDOW_PARAM_TEXTBUF);
	if (!textbuf) return 0;
	
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter(textbuf,&iter);
	gtk_text_buffer_insert(textbuf,&iter,buf,-1);
	return 0;
}

/// for lua:	void	  MUD_TextWindow_ScrollToEnd	()
static int 				l_MUD_TextWindow_ScrollToEnd	(lua_State *L) {
	GtkWidget*			window = (GtkWidget*)lua_touserdata(L,1);
	if (!window) return 0;
	
	GtkTextBuffer*		textbuf = (GtkTextBuffer*)g_object_get_data(G_OBJECT(window),LUA_WINDOW_PARAM_TEXTBUF);
	GtkWidget*			view = (GtkWidget*)g_object_get_data(G_OBJECT(window),LUA_WINDOW_PARAM_TEXTVIEW);
	if (!textbuf || !view) return 0;
	GtkTextIter end_start_iter;
	gtk_text_buffer_get_end_iter(textbuf,&end_start_iter);
	GtkTextMark* insert_mark = gtk_text_buffer_get_insert(textbuf);
	gtk_text_buffer_place_cursor(textbuf, &end_start_iter);
	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(view),insert_mark, 0.0, TRUE, 0.0, 1.0); 
	return 0;
}


void	InitLuaEnvironment	(lua_State*	L) {
	// lua standard lib
	luaL_openlibs(L);
	
	// register custom functions here
	lua_register(L,"MUD_Send",						l_MUD_Send); 
	lua_register(L,"MUD_TextWindow_Open",			l_MUD_TextWindow_Open); 
	lua_register(L,"MUD_TextWindow_SetText",		l_MUD_TextWindow_SetText); 
	lua_register(L,"MUD_TextWindow_AppendText",		l_MUD_TextWindow_AppendText); 
	lua_register(L,"MUD_TextWindow_ScrollToEnd",	l_MUD_TextWindow_ScrollToEnd); 
#ifdef LUA_NETWORK
	InitLuaEnvironment_Net(L);
#endif
}

// ***** ***** ***** ***** ***** end
#endif // ENABLE_LUA
