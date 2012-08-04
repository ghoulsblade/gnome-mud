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

#define	LUA_IMAGEWINDOW_ON_BUTTON_PRESS		"imagewindow_on_button_press" // this lua function gets notified of incoming and outgoing data

lua_State*	LuaPlugin_GetMainState	();
int 		PCallWithErrFuncWrapper (lua_State* L,int narg, int nret);

/// "button_press_event" handler, should call lua callback
static gint ImageWindow_On_Button_Press_Event (GtkWidget *widget, GdkEvent *event) {
	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_WINDOW (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);
	if (event->type != GDK_BUTTON_PRESS) return FALSE;
	
	GdkEventButton* event_button = (GdkEventButton*)event;
	//~ printf("ImageWindow_On_Button_Press_Event\n");
	
	// lua
	lua_State* L = LuaPlugin_GetMainState();
	if (L) {
		const char* func = LUA_IMAGEWINDOW_ON_BUTTON_PRESS;
		lua_getglobal(L, func); // get function
		if (lua_isnil(L,1)) {
			lua_pop(L,1);
			//~ fprintf(stderr,"lua: function `%s' not found\n",func);
		} else {
			int narg = 5, nres = 1;
			//~ luaL_checkstack(L, narg, "too many arguments");
			lua_pushlightuserdata(L, (void*)widget); // arg 1
			lua_pushinteger(L, event_button->button); // arg 2
			lua_pushinteger(L, event_button->x); // arg 3
			lua_pushinteger(L, event_button->y); // arg 4
			lua_pushinteger(L, event_button->time); // arg 5
			//~ lua_pushlightuserdata(L, (void*)event_button->time); // arg 3
			if (PCallWithErrFuncWrapper(L,narg, nres) != 0) {
				fprintf(stderr,"lua: error running function `%s': %s\n",func, lua_tostring(L, -1));
			} else {
				gboolean res = lua_toboolean(L,-1);
				if (nres > 0) lua_pop(L, nres);
				return res ? TRUE : FALSE;
			}
		}
	}
	
	return FALSE;
}

/// open image window, w,h in pixels
/// for lua:	window	  MUD_ImageWindow_Open	(title,w,h,x=680,y=0)
static int 				l_MUD_ImageWindow_Open	(lua_State* L) {
	const gchar*	title	= luaL_checkstring(L,1);
	gint			w		= luaL_checkint(L,2);
	gint			h		= luaL_checkint(L,3);
	gint			x		= lua2_isset(L,4) ? luaL_checkint(L,4) : 680;
	gint			y		= lua2_isset(L,5) ? luaL_checkint(L,5) : 0;
	
	// open window
	GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	
	// set title and properties
	gtk_window_set_title(GTK_WINDOW(window),title);
	//~ gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER); // show in screen center
	gtk_window_move(GTK_WINDOW(window),x,y);
	gtk_widget_set_size_request(window,w,h);
	GtkWidget* parent = window;
	
	// layout
	GtkWidget* layout = gtk_layout_new(NULL,NULL);
	gtk_container_add(GTK_CONTAINER(parent),layout);
	parent = layout;
	
	// bind events
	gtk_widget_set_events(window,gtk_widget_get_events(window) | GDK_BUTTON_PRESS_MASK);
	g_signal_connect_swapped(window,"button_press_event", G_CALLBACK (ImageWindow_On_Button_Press_Event), window);
	
	// present window
	gtk_widget_show_all(window);
	gtk_window_present(GTK_WINDOW(window));
	
	// return handle
	lua_pushlightuserdata(L, (void*)window);
	return 1;
}

/// for lua:	widget	  MUD_ImageWindow_AddText	(window,txt,x,y,font="monospace 12")
static int 				l_MUD_ImageWindow_AddText	(lua_State* L) {
	GtkWidget*	window	= (GtkWidget*)lua_touserdata(L,1);
	const char*	text	= luaL_checkstring(L,2);
	gint		x		= luaL_checkint(L,3);
	gint		y		= luaL_checkint(L,4);
	const char*	font	= lua2_isset(L,5) ? luaL_checkstring(L,5) : "monospace 12";
	if (!window) return 0;
	
	GtkWidget* textview = gtk_label_new(text);
	// set font
	PangoFontDescription* fontdesc = pango_font_description_from_string(font);
	gtk_widget_modify_font(textview,fontdesc);
	// add to parent
	GtkWidget* layout = gtk_bin_get_child(GTK_BIN(window));
	if (layout) gtk_layout_put(GTK_LAYOUT(layout),textview,x,y);
	gtk_widget_show(textview);
	
	lua_pushlightuserdata(L,(void*)textview);
	return 1;
}

/// for lua:	widget	  MUD_ImageWindow_AddImage	(window,path,x,y)
static int 				l_MUD_ImageWindow_AddImage	(lua_State* L) {
	GtkWidget*	window	= (GtkWidget*)lua_touserdata(L,1);
	const char*	path	= luaL_checkstring(L,2);
	gint		x		= luaL_checkint(L,3);
	gint		y		= luaL_checkint(L,4);
	if (!window) return 0;
	
	// add image
	GtkWidget* image = gtk_image_new_from_file(path);
	// add to parent
	GtkWidget* layout = gtk_bin_get_child(GTK_BIN(window));
	if (layout) gtk_layout_put(GTK_LAYOUT(layout),image,x,y);
	gtk_widget_show(image);
	
	lua_pushlightuserdata(L,(void*)image);
	return 1;
}

/// destroy image/text/...
/// for lua:	void	  MUD_ImageWindow_DestroyWidget	(widget)
static int 				l_MUD_ImageWindow_DestroyWidget	(lua_State* L) {
	GtkWidget*	widget	= (GtkWidget*)lua_touserdata(L,1);
	if (!widget) return 0;
	gtk_widget_destroy(GTK_WIDGET(widget));
	return 0;
}

void	InitLuaEnvironment_Imagewindow	(lua_State* L) {
	lua_register(L,"MUD_ImageWindow_Open",			l_MUD_ImageWindow_Open); 
	lua_register(L,"MUD_ImageWindow_AddText",		l_MUD_ImageWindow_AddText); 
	lua_register(L,"MUD_ImageWindow_AddImage",		l_MUD_ImageWindow_AddImage);
	lua_register(L,"MUD_ImageWindow_DestroyWidget",	l_MUD_ImageWindow_DestroyWidget);
}

#endif // ENABLE_LUA
