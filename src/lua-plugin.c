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
#define	LUA_COMMAND_RELOAD	"#reload" // if player writes this, lua is reloaded
#define	LUA_ON_DATA			"on_data"			// this lua function gets notified of incoming and outgoing data
#define	LUA_ON_BUTTON_PRESS	"on_button_press"	// this lua function gets notified of "button-press-event" signals
#define	LUA_ON_KEY_PRESS	"on_key_press"		// this lua function gets notified of "key-press-event" signals
#define DIR_IN  1 // server to client, message from server
#define DIR_OUT 0 // client to server, user input

void	LuaPlugin_OnData			(const gchar* buf,MudConnectionView* view,int dir);
void	LuaPlugin_ExecLuaFile		();
void	InitLuaEnvironment			(lua_State*	L);
int 	PCallWithErrFuncWrapper		(lua_State* L,int narg, int nret);

#ifdef ENABLE_LUA_NETWORK
void	LuaNetInit					();
void	LuaNetCleanup				();
#endif

/// new hook for git master after v0.11.2
void	LuaPlugin_data_hook			(MudConnectionView* view,const gchar* data,guint length,int dir) {
	GString *str = g_string_new_len(data,length);
	g_string_append_c(str,0); // zero-terminate
	gchar* datacopy = g_string_free(str,FALSE);
	LuaPlugin_OnData(datacopy,view,dir);
	g_free(datacopy);
}
	
void	LuaPlugin_Init (MudWindow* pMainWindow) {
	if (gbLuaPlugin_Init_done) return;
	gbLuaPlugin_Init_done = TRUE; // singleton, init only once.
	
	L = luaL_newstate();
	g_assert(L);
	
	InitLuaEnvironment(L);
	
#ifdef ENABLE_LUA_NETWORK
	LuaNetInit();
#endif
	
	LuaPlugin_ExecLuaFile();
}

/// TODO: call this for program shutdown
void	LuaPlugin_Cleanup () {
	if (!gbLuaPlugin_Init_done) return;
	if (L) { lua_close(L); L = NULL; }
#ifdef ENABLE_LUA_NETWORK
	LuaNetCleanup();
#endif
}

void	LuaPlugin_ExecLuaFile () {
	gchar filename[1024];
	g_snprintf(filename, 1024, "%s/%s", (const char*)g_get_home_dir(), LUA_MAIN_FILENAME);
	int res = luaL_dofile(L,filename);
	if (res) {
		fprintf(stderr,"lua: error in main script-initialisation:\n");
		fprintf(stderr,"%s\n",lua_tostring(L,-1));
	}
}

void	LuaPlugin_OnData	(const gchar* buf,MudConnectionView* view,int dir) { 
    gchar* stripped_data;
	
	stripped_data = utils_strip_ansi((const gchar *) buf); // warning: assume zero terminated v0.11.2+x(master)
	//~ stripped_data = strip_ansi(buf); // warning: assume zero terminated  v0.11.2

	// reload lua on keyword
	if (dir == DIR_OUT && strstr((const char*)stripped_data,LUA_COMMAND_RELOAD)) LuaPlugin_ExecLuaFile();
	
	// lua
	if (L) {
		const char* func = LUA_ON_DATA;
		lua_getglobal(L, func); // get function
		if (lua_isnil(L,1)) {
			lua_pop(L,1);
			//~ fprintf(stderr,"lua: function `%s' not found\n",func);
		} else {
			int narg = 4, nres = 0;
			//~ luaL_checkstack(L, narg, "too many arguments");
			lua_pushstring(L, (const char*)stripped_data); // arg 1
			lua_pushinteger(L, dir); // arg 2
			lua_pushlightuserdata(L, (void*)view); // arg 3
			lua_pushstring(L, (const char*)buf); // arg 4
			if (PCallWithErrFuncWrapper(L,narg, nres) != 0) {
				fprintf(stderr,"lua: error running function `%s': %s\n",func, lua_tostring(L, -1));
			} else {
				if (nres > 0) lua_pop(L, nres);
			}
		}
	}
	
	// free buffer
	if (stripped_data) g_free(stripped_data);
}

// ***** ***** ***** ***** ***** lua utils

lua_State*	LuaPlugin_GetMainState	() { return L; }

/// also adds a traceback to the error message in case of an error, better than a plain lua_call
/// nret=-1 for unlimited
/// don't use directly, stack has to be prepared and cleaned up
int 	PCallWithErrFuncWrapper (lua_State* L,int narg, int nret) {
	int errfunc = 0;
	return lua_pcall(L, narg, (nret==-1) ? LUA_MULTRET : nret, errfunc);
}

// ***** ***** ***** ***** ***** event handler

/// this actually calls the lua code
gboolean	LuaPlugin_LuaCall_EventHandler (GtkWidget *widget, GdkEvent *event,const char* context,void* userdata) {
	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (event != NULL, FALSE);
	//~ g_return_val_if_fail (GTK_IS_WINDOW (widget), FALSE); // fails when called from LuaPlugin_key_press_hook (gnome-mud main window)
	
	// access lua state
	lua_State* L = LuaPlugin_GetMainState();
	if (!L) return FALSE;
	
	// "button-press-event"
	if (event->type == GDK_BUTTON_PRESS)  {
		const char* func = LUA_ON_BUTTON_PRESS;
		GdkEventButton* event_button = (GdkEventButton*)event;
		lua_getglobal(L, func); // get function
		if (lua_isnil(L,1)) {
			lua_pop(L,1);
			//~ fprintf(stderr,"lua: function `%s' not found\n",func);
		} else {
			int narg = 0, nres = 1;
			++narg; lua_pushlightuserdata(L, (void*)widget);
			++narg; lua_pushinteger(L, event_button->button);
			++narg; lua_pushinteger(L, event_button->x);
			++narg; lua_pushinteger(L, event_button->y);
			++narg; lua_pushinteger(L, event_button->time);
			++narg; lua_pushstring(L, context);
			++narg; lua_pushlightuserdata(L, (void*)userdata);
			if (PCallWithErrFuncWrapper(L,narg, nres) != 0) {
				fprintf(stderr,"lua: error running function `%s': %s\n",func, lua_tostring(L, -1));
			} else {
				gboolean res = lua_toboolean(L,-1);
				if (nres > 0) lua_pop(L, nres);
				if (res) return TRUE;
			}
		}
	}
	
	// "key-press-event"
	if (event->type == GDK_KEY_PRESS) {
		const char* func = LUA_ON_KEY_PRESS;
		GdkEventKey* event_key = (GdkEventKey*)event;
	
		lua_getglobal(L, func); // get function
		if (lua_isnil(L,1)) {
			lua_pop(L,1);
			//~ fprintf(stderr,"lua: function `%s' not found\n",func);
		} else {
			int narg = 0, nres = 1;
			++narg; lua_pushlightuserdata(L, (void*)widget);
			++narg; lua_pushinteger(L, event_key->keyval);
			++narg; lua_pushinteger(L, event_key->state);
			++narg; lua_pushlstring(L, event_key->string, event_key->length);
			++narg; lua_pushinteger(L, event_key->time);
			++narg; lua_pushstring(L, context);
			++narg; lua_pushlightuserdata(L, (void*)userdata);
			if (PCallWithErrFuncWrapper(L,narg, nres) != 0) {
				fprintf(stderr,"lua: error running function `%s': %s\n",func, lua_tostring(L, -1));
			} else {
				gboolean res = lua_toboolean(L,-1);
				if (nres > 0) lua_pop(L, nres);
				if (res) return TRUE;
			}
		}
	}
	
	return FALSE;
}


/// hook called from gnome-mud code
gboolean	LuaPlugin_key_press_hook	(MudConnectionView* view,GtkWidget *widget, GdkEventKey *event) {
	return LuaPlugin_LuaCall_EventHandler(widget,(GdkEvent*)event,"gnome-mud-window",(void*)view);
}

/// gtk handler/callback for "button-press-event" and similar, calls lua callback
static gboolean LuaPlugin_SignalHandler_Event (GtkWidget *widget, GdkEvent *event) {
	return LuaPlugin_LuaCall_EventHandler(widget,event,"lua-plugin",NULL);
}

/// utility function that connects our handler to a signal
void	LuaPlugin_SignalConnectEvent	(GtkWidget* widget,const char* signalname) {
	// note: swapped and 2x widget : no idea, copy-pasted from GtkMenu popup example
	g_signal_connect_swapped(widget,signalname, G_CALLBACK(LuaPlugin_SignalHandler_Event), widget);
}

// ***** ***** ***** ***** ***** api

/// simulate user input. view = param from LUA_ON_DATA call  (or udata for LUA_ON_KEY_PRESS with context "gnome-mud-window")
/// for lua:	void	  MUD_Send		(view,txt)
static int 				l_MUD_Send		(lua_State* L) {
	MudConnectionView*	view = (MudConnectionView*)lua_touserdata(L,1);
	const char*			buf = luaL_checkstring(L,2);
	if (view) mud_connection_view_send(view, (const gchar *)buf);
	return 0;
}

/// returns filepath to user home dir
/// for lua:	string	  get_home_dir		()
static int 				l_get_home_dir		(lua_State* L) {
	lua_pushstring(L,g_get_home_dir());
	return 1;
}

void	InitLuaEnvironment_Textwindow	(lua_State* L);
void	InitLuaEnvironment_Imagewindow	(lua_State* L);
#ifdef ENABLE_LUA_NETWORK
void	InitLuaEnvironment_Net			(lua_State*	L);
#endif

void	InitLuaEnvironment	(lua_State*	L) {
	// lua standard lib
	luaL_openlibs(L);
	
	// register custom functions here
	lua_register(L,"MUD_Send",		l_MUD_Send); 
	lua_register(L,"get_home_dir",	l_get_home_dir); 
	InitLuaEnvironment_Textwindow(L);
	InitLuaEnvironment_Imagewindow(L);
#ifdef ENABLE_LUA_NETWORK
	InitLuaEnvironment_Net(L);
#endif
}

// ***** ***** ***** ***** ***** end
#endif // ENABLE_LUA
