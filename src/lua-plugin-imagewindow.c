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


void	LuaPlugin_SignalConnectEvent	(GtkWidget* widget,const char* signalname);

// ***** ***** ***** ***** ***** main window

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
	gtk_widget_set_events(window,gtk_widget_get_events(window) | GDK_KEY_PRESS_MASK);
	LuaPlugin_SignalConnectEvent(window,"button-press-event");
	LuaPlugin_SignalConnectEvent(window,"key-press-event");
	
	// present window
	gtk_widget_show_all(window);
	gtk_window_present(GTK_WINDOW(window));
	
	// return handle
	lua_pushlightuserdata(L, (void*)window);
	return 1;
}

// ***** ***** ***** ***** ***** main widgets

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

/// for lua:	widget	  MUD_ImageWindow_AddProgressBar	(window,x,y)
static int 				l_MUD_ImageWindow_AddProgressBar	(lua_State* L) {
	GtkWidget*	window	= (GtkWidget*)lua_touserdata(L,1);
	gint		x		= luaL_checkint(L,2);
	gint		y		= luaL_checkint(L,3);
	if (!window) return 0;
	
	// add image
	GtkWidget* pbar = gtk_progress_bar_new();
	// add to parent
	GtkWidget* layout = gtk_bin_get_child(GTK_BIN(window));
	if (layout) gtk_layout_put(GTK_LAYOUT(layout),pbar,x,y);
	gtk_widget_show(pbar);
	
	lua_pushlightuserdata(L,(void*)pbar);
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


// ***** ***** ***** ***** ***** GtkProgressBar

GtkProgressBar*	GtkProgressBar_FromLua	(lua_State* L,int i) {
	GtkWidget*	widget = (GtkWidget*)lua_touserdata(L,i);
	g_return_val_if_fail(GTK_IS_PROGRESS_BAR(widget), NULL);
	return GTK_PROGRESS_BAR(widget);
}

GtkProgress*	GtkProgress_FromLua	(lua_State* L,int i) {
	GtkWidget*	widget = (GtkWidget*)lua_touserdata(L,i);
	g_return_val_if_fail(GTK_IS_PROGRESS(widget), NULL);
	return GTK_PROGRESS(widget);
}


#define LUA_BIND(name,mytype,params)			\
	static int l_ ## name (lua_State* L) {		\
		mytype* o = mytype ## _FromLua (L,1);	\
		if (o) { name params; } 				\
		return 0;								\
	}

// http://developer.gnome.org/gtk/2.24/GtkProgressBar.html
/*lua: void gtk.. (o,..)*/	LUA_BIND(gtk_progress_bar_pulse				,GtkProgressBar,(o))
/*lua: void gtk.. (o,..)*/	LUA_BIND(gtk_progress_bar_set_text			,GtkProgressBar,(o,luaL_checkstring(L,2)))
/*lua: void gtk.. (o,..)*/	LUA_BIND(gtk_progress_bar_set_fraction		,GtkProgressBar,(o,luaL_checknumber(L,2)))
/*lua: void gtk.. (o,..)*/	LUA_BIND(gtk_progress_bar_set_pulse_step	,GtkProgressBar,(o,luaL_checknumber(L,2)))
/*lua: void gtk.. (o,..)*/	LUA_BIND(gtk_progress_bar_set_orientation	,GtkProgressBar,(o,(GtkProgressBarOrientation)luaL_checkint(L,2)))
/*lua: void gtk.. (o,..)*/	LUA_BIND(gtk_progress_bar_set_ellipsize		,GtkProgressBar,(o,(PangoEllipsizeMode)luaL_checkint(L,2)))


// ***** ***** ***** ***** ***** register

#define LUA_REG_FUN_BY_L_NAME(L,name)	{ lua_register(L,#name,l_ ## name); }
#define LUA_REG_CONSTANT(L,name)		{ lua_pushinteger(L,name); lua_setglobal(L,#name); }

void	InitLuaEnvironment_Imagewindow	(lua_State* L) {
	LUA_REG_FUN_BY_L_NAME(L,MUD_ImageWindow_Open)
	LUA_REG_FUN_BY_L_NAME(L,MUD_ImageWindow_AddText)
	LUA_REG_FUN_BY_L_NAME(L,MUD_ImageWindow_AddImage)
	LUA_REG_FUN_BY_L_NAME(L,MUD_ImageWindow_AddProgressBar)
	LUA_REG_FUN_BY_L_NAME(L,MUD_ImageWindow_DestroyWidget)
	
	LUA_REG_FUN_BY_L_NAME(L,gtk_progress_bar_pulse)
	LUA_REG_FUN_BY_L_NAME(L,gtk_progress_bar_set_text)
	LUA_REG_FUN_BY_L_NAME(L,gtk_progress_bar_set_fraction)
	LUA_REG_FUN_BY_L_NAME(L,gtk_progress_bar_set_pulse_step)
	LUA_REG_FUN_BY_L_NAME(L,gtk_progress_bar_set_orientation)
	LUA_REG_FUN_BY_L_NAME(L,gtk_progress_bar_set_ellipsize)
	
	// GtkProgressBarOrientation
	LUA_REG_CONSTANT(L,GTK_PROGRESS_LEFT_TO_RIGHT)
	LUA_REG_CONSTANT(L,GTK_PROGRESS_RIGHT_TO_LEFT)
	LUA_REG_CONSTANT(L,GTK_PROGRESS_BOTTOM_TO_TOP)
	LUA_REG_CONSTANT(L,GTK_PROGRESS_TOP_TO_BOTTOM)
	
	// PangoEllipsizeMode
	LUA_REG_CONSTANT(L,PANGO_ELLIPSIZE_NONE)
	LUA_REG_CONSTANT(L,PANGO_ELLIPSIZE_START)
	LUA_REG_CONSTANT(L,PANGO_ELLIPSIZE_MIDDLE)
	LUA_REG_CONSTANT(L,PANGO_ELLIPSIZE_END)
}

#endif // ENABLE_LUA
