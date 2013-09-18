#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "ngx_lua_log.h"

int ngx_lua_log(lua_State* lua)
{
    // TODO
    int args = lua_gettop(lua);
    int i;
    size_t size;
    const char* msg;

    if (args < 2)
    {
        luaL_error(lua, "ngx_lua_log: arguments count < 2");
        return 0;
    }

    size = 0;
    for (i = 1; i <= args; ++i)
    {
        int type = lua_type(lua, i);
        size_t len;
        switch (type)
        {
        case LUA_TNUMBER:
        case LUA_TSTRING:
            lua_tolstring(lua, i, &len);
            size += len;
            break;
        case LUA_TNIL:
            size += sizeof("nil") - 1;
            break;
        case LUA_TBOOLEAN:
            if (lua_toboolean(lua, i)) size += sizeof("true") - 1;
            else size += sizeof("false") - 1;
            break;
        default:
            msg = lua_pushfstring(lua, "string, number, boolean or nil expected, got %s", lua_typename(lua, type));
            return luaL_argerror(lua, i, msg);
        }
    }
    return 0;
}

