// commonly used items and defines in lua-plugin

#define ENABLE_LUA_NETWORK

#define lua2_isset(L,i) (lua_gettop(L) >= i && !lua_isnil(L,i))
