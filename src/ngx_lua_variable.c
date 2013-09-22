#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "ngx_lua_module_util.h"

#include "ngx_lua_variable.h"

int ngx_lua_var_get(lua_State* lua)
{
    ngx_http_request_t* r;
    u_char *str, *dst;
    size_t len;
    ngx_uint_t hash;
    ngx_str_t  name;
    ngx_http_variable_value_t* v;

    r = ngx_lua_module_get_req_obj(lua);

    str = (u_char*)luaL_checklstring(lua, -1, &len);

    dst = ngx_palloc(ngx_cycle->pool, len + 1);
    hash = ngx_hash_strlow(dst, str, len);
    dst[len] = 0;

    name.len = len;
    name.data = dst;

    v = ngx_http_get_variable(r, &name, hash);
    ngx_pfree(ngx_cycle->pool, dst);
    if (v == NULL || v->not_found)
    {
        lua_pushnil(lua);
        return 1;
    }

    lua_pushlstring(lua, (const char*)v->data, (size_t)v->len);
    return 1;
}

int ngx_lua_var_set(lua_State* lua)
{
    ngx_http_request_t* r;
    u_char *str, *dst = NULL, *val = NULL;
    size_t len;
    ngx_uint_t hash;
    ngx_str_t  name;
    int type;
    const char* msg;
    ngx_http_core_main_conf_t* conf;
    ngx_http_variable_t* v;
    ngx_http_variable_value_t* vv;
    int ret;

    r = ngx_lua_module_get_req_obj(lua);

    str = (u_char*)luaL_checklstring(lua, 2, &len);

    dst = ngx_pcalloc(ngx_cycle->pool, len);
    hash = ngx_hash_strlow(dst, str, len);
    dst[len] = 0;

    name.len = len;
    name.data = dst;

    type = lua_type(lua, 3);
    switch (type)
    {
    case LUA_TNUMBER:
    case LUA_TSTRING:
        str = (u_char*)luaL_checklstring(lua, 3, &len);
        val = ngx_palloc(ngx_cycle->pool, len);
        if (val == NULL)
        {
            ngx_pfree(ngx_cycle->pool, dst);
            return luaL_error(lua, "out of memory");
        }
        ngx_memcpy(val, str, len);
        break;
    case LUA_TNIL:
        str = NULL;
        len = 0;
        break;
    default:
        msg = lua_pushfstring(lua, "string, number or nil expected, but got %s", lua_typename(lua, type));
        return luaL_argerror(lua, 1, msg);
    }

    conf = ngx_http_get_module_main_conf(r, ngx_http_core_module);
    v = ngx_hash_find(&conf->variables_hash, hash, name.data, name.len);

    if (v)
    {
        if (!(v->flags && NGX_HTTP_VAR_CHANGEABLE)) return luaL_error(lua, "variable \"%s\" not changeable", dst);

        if (v->set_handler)
        {
            vv = ngx_palloc(ngx_cycle->pool, sizeof(ngx_http_variable_value_t));
            if (vv == NULL)
            {
                ngx_pfree(ngx_cycle->pool, val);
                ngx_pfree(ngx_cycle->pool, dst);
                return luaL_error(lua, "out of memory");
            }
            if (type == LUA_TNIL)
            {
                vv->valid = 0;
                vv->not_found = 1;
                vv->no_cacheable = 0;

                vv->data = NULL;
                vv->len = 0;
            }
            else
            {
                vv->valid = 1;
                vv->not_found = 0;
                vv->no_cacheable = 0;
                vv->data = val;
                vv->len = len;
            }
            v->set_handler(r, vv, v->data);
            return 0;
        }
        else if (v->flags & NGX_HTTP_VAR_INDEXED)
        {
            vv = &r->variables[v->index];
            if (type == LUA_TNIL)
            {
                vv->valid = 0;
                vv->not_found = 1;
                vv->no_cacheable = 0;

                vv->data = NULL;
                vv->len = 0;
            }
            else
            {
                vv->valid = 1;
                vv->not_found = 0;
                vv->no_cacheable = 0;
                vv->data = val;
                vv->len = len;
            }
            return 0;
        }
        ret = luaL_error(lua, "variable \"%s\" cannot be assigned a value", dst);
        ngx_pfree(ngx_cycle->pool, val);
        ngx_pfree(ngx_cycle->pool, dst);
        return ret;
    }

    ret = luaL_error(lua, "variable \"%s\" cannot be assigned a value", dst);
    ngx_pfree(ngx_cycle->pool, val);
    ngx_pfree(ngx_cycle->pool, dst);
    return ret;
}

void ngx_lua_get_var(lua_State* lua)
{
    lua_getglobal(lua, "ngx");
    if (lua_isnil(lua, -1))
    {
        lua_pop(lua, -1);
        lua_newtable(lua);
        lua_setglobal(lua, "ngx");
        lua_getglobal(lua, "ngx");
    }

    lua_pushstring(lua, "var");
    lua_newtable(lua);
    lua_settable(lua, -3);
    lua_pushstring(lua, "var");
    lua_gettable(lua, -2);

    lua_newtable(lua);
    lua_pushstring(lua, "__index");
    lua_pushcfunction(lua, ngx_lua_var_get);
    lua_settable(lua, -3);
    lua_pushstring(lua, "__newindex");
    lua_pushcfunction(lua, ngx_lua_var_set);
    lua_settable(lua, -3);
    lua_setmetatable(lua, -2);
}

