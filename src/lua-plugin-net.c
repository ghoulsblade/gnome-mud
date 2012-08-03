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
#ifdef ENABLE_LUA_NETWORK

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"


// ***** ***** ***** ***** ***** ENABLE_LUA_NETWORK headers 


//~ #include <time.h> // timeval ?
#include <errno.h>
#include <string.h> // char *strerror(int errnum);
#include <stdlib.h>
#include <stdio.h>
	
#ifdef WIN32
    #include <winsock2.h>
	#ifndef socklen_t
	#define socklen_t int
	#endif
#else
	#include <sys/select.h> // timeval ?
	#include <sys/time.h> // timeval ?
	#include <sys/types.h> // timeval ?
	#include <unistd.h> // timeval ?

    #include <netdb.h>
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
#endif

//winsock workaround
#ifndef INVALID_SOCKET
#define INVALID_SOCKET  (SOCKET)(~0)
#endif

#ifndef SOCKET
#define SOCKET int
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR	-1
#endif

fd_set	sSelectSet_Read;
fd_set	sSelectSet_Write;
fd_set	sSelectSet_Except;

// called once at startup
void	LuaNetInit () {
#ifdef WIN32
	WSADATA wsaData;
	WSAStartup( MAKEWORD( 2, 0 ), &wsaData );
#endif
}
// called once at end
void	LuaNetCleanup () {
	#ifdef WIN32
	WSACleanup();
	#endif
}

#ifndef WIN32
void closesocket (int socket) { close(socket); }
#endif

/// simulate user input. view = param from LUA_ON_DATA call
/// for lua:	res,bRead,bWrite,bExcept	  Net_Select	(socket,to_usec,to_sec)
static int 									l_Net_Select	(lua_State* L) {
	struct timeval timeout;
	int res,imax=0;
	int mysocket	= luaL_checkint(L,1);
	int to_usec		= luaL_checkint(L,2);
	int to_sec		= luaL_checkint(L,3);
	
	timeout.tv_sec = to_usec;
	timeout.tv_usec = to_sec;

	FD_ZERO(&sSelectSet_Read);
	FD_ZERO(&sSelectSet_Write);
	FD_ZERO(&sSelectSet_Except);

	if (mysocket != INVALID_SOCKET) {
		if (imax < mysocket)
			imax = mysocket;
		FD_SET((unsigned int)mysocket,&sSelectSet_Read);
		FD_SET((unsigned int)mysocket,&sSelectSet_Write);
		FD_SET((unsigned int)mysocket,&sSelectSet_Except);
	}
	
	res = select(imax+1,&sSelectSet_Read,&sSelectSet_Write,&sSelectSet_Except,&timeout);
	
	lua_pushinteger(L,res);
	lua_pushboolean(L,FD_ISSET((unsigned int)mysocket,&sSelectSet_Read));
	lua_pushboolean(L,FD_ISSET((unsigned int)mysocket,&sSelectSet_Write));
	lua_pushboolean(L,FD_ISSET((unsigned int)mysocket,&sSelectSet_Except));
	return 4;
}


void	InitLuaEnvironment_Net	(lua_State*	L) {
	// register custom functions here
	lua_register(L,"Net_Select",	l_Net_Select); 
}

// ***** ***** ***** ***** ***** end
#endif // ENABLE_LUA_NETWORK
#endif // ENABLE_LUA
