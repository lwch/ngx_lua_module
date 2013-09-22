#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "ngx_lua_log.h"

int ngx_lua_log(lua_State* lua)
{
    int args = lua_gettop(lua);
    int i;
    size_t size;
    const char* msg;
    u_char* buf;
    u_char *p, *q;
    ngx_uint_t level;

    if (args < 2)
    {
        luaL_error(lua, "ngx_lua_log: arguments count < 2");
        return 0;
    }

    level = (ngx_uint_t)luaL_checknumber(lua, 1);

    size = 0;
    for (i = 2; i <= args; ++i)
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

    p = buf = ngx_palloc(ngx_cycle->pool, size + 1);
    for (i = 2; i <= args; ++i)
    {
        int type = lua_type(lua, i);
        size_t len;
        switch (type)
        {
        case LUA_TNUMBER:
        case LUA_TSTRING:
            q = (u_char*)lua_tolstring(lua, i, &len);
            p = ngx_copy(p, q, len);
            break;
        case LUA_TNIL:
            p = ngx_copy(p, "nil", sizeof("nil") - 1);
            break;
        case LUA_TBOOLEAN:
            if (lua_toboolean(lua, i)) p = ngx_copy(p, "true", sizeof("true") - 1);
            else p = ngx_copy(p, "false", sizeof("false") - 1);
            break;
        default:
            return luaL_error(lua, "unknown");
        }
    }
    buf[size] = 0;

    ngx_log_error(level, ngx_cycle->log, 0, "%s", buf);
    ngx_pfree(ngx_cycle->pool, buf);

    return 0;
}

