#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_lua_log.h"
#include "ngx_lua_variable.h"

#include "ngx_lua_module.h"
#include "ngx_lua_module_util.h"

void ngx_lua_module_init(lua_State* lua)
{
    printf("ngx_lua_module_init\n");

    ngx_lua_module_get_req(lua);
    lua_pop(lua, 2); // ngx and ngx.req

    ngx_lua_get_var(lua);
    lua_pop(lua, 2); // ngx and ngx.req

    ngx_lua_module_get_err(lua);
    lua_pop(lua, 1); // ngx.err

    lua_pushstring(lua, "log");
    lua_pushcfunction(lua, ngx_lua_log);
    lua_settable(lua, -3);
    lua_pop(lua, 1); // ngx
}

size_t ngx_lua_module_unescape_uri(u_char* dst, u_char* src, size_t size)
{
    u_char ch, d = 0, t = 0, *ptr;
    enum
    {
        usual,
        quoted,
        quoted_second
    } state;

    if (dst == src)
    {
        perror("dst == src");
        return 0;
    }

    ptr = dst;
    state = usual;

    while (size--)
    {
        ch = *src++;

        switch (state)
        {
        case usual:
            if (ch == '%') state = quoted;
            else if (ch == '+') *dst++ = ' ';
            else *dst++ = ch;
            break;
        case quoted:
            t = ch;
            if (ch >= '0' && ch <= '9')
            {
                d = (u_char)(ch - '0');
                state = quoted_second;
            }
            else if (ch >= 'a' && ch <= 'f')
            {
                d = (u_char)(ch - 'a' + 10);
                state = quoted_second;
            }
            else if (ch >= 'A' && ch <= 'F')
            {
                d = (u_char)(ch - 'A' + 10);
                state = quoted_second;
            }
            else
            {
                *dst++ = '%';
                *dst++ = ch;
                state = usual;
            }
            break;
        case quoted_second:
            state = usual;
            if (ch >= '0' && ch <= '9')
            {
                t = (u_char)((d << 4) + ch - '0');
                *dst++ = t;
            }
            else if (ch >= 'a' && ch <= 'f')
            {
                t = (u_char)((d << 4) + ch - 'a' + 10);
                *dst++ = t;
            }
            else if (ch >= 'A' && ch <= 'f')
            {
                t = (u_char)((d << 4) + ch - 'A' + 10);
                *dst++ = t;
            }
            else
            {
                *dst++ = '%';
                *dst++ = t;
                *dst++ = ch;
            }
            break;
        }
    }
    return dst - ptr;
}

void ngx_lua_module_get_req(lua_State* lua)
{
    lua_getglobal(lua, "ngx");
    if (lua_isnil(lua, -1))
    {
        lua_pop(lua, 1);
        lua_newtable(lua);
        lua_setglobal(lua, "ngx");
        lua_getglobal(lua, "ngx");
    }

    lua_pushstring(lua, "req");
    lua_newtable(lua);
    lua_settable(lua, -3);
    lua_pushstring(lua, "req");
    lua_gettable(lua, -2);
}

void ngx_lua_module_get_err(lua_State* lua)
{
    lua_getglobal(lua, "ngx");
    if (lua_isnil(lua, -1))
    {
        lua_pop(lua, -1);
        lua_newtable(lua);
        lua_setglobal(lua, "ngx");
        lua_getglobal(lua, "ngx");
    }

    lua_pushstring(lua, "err");
    lua_newtable(lua);
    lua_settable(lua, -3);
    lua_pushstring(lua, "err");
    lua_gettable(lua, -2);
}

void ngx_lua_module_get_var(lua_State* lua)
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
    lua_pushstring(lua, "err");
    lua_gettable(lua, -2);
}

void ngx_lua_module_parse_args(ngx_pool_t* pool, u_char* buf, size_t size, struct lua_State* lua)
{
    u_char *start, *tmp;
    size_t s;

    ngx_lua_module_get_req(lua);

    start = buf;
    lua_pushstring(lua, "args");
    lua_newtable(lua);
    while (size--)
    {
        if (*buf == '=')
        {
            if (start == buf) // no key skip
            {
                buf++;
                continue;
            }
            tmp = ngx_palloc(pool, buf - start);
            s = ngx_lua_module_unescape_uri(tmp, start, buf - start);
            lua_pushlstring(lua, (const char*)tmp, s);
            ngx_pfree(pool, tmp);
            start = buf + 1;
        }
        else if (*buf == '&')
        {
            if (start == buf) // no value
            {
                lua_pushnil(lua);
            }
            else
            {
                tmp = ngx_palloc(pool, buf - start);
                s = ngx_lua_module_unescape_uri(tmp, start, buf - start);
                lua_pushlstring(lua, (const char*)tmp, s);
                ngx_pfree(pool, tmp);
            }
            lua_settable(lua, -3);
            start = buf + 1;
        }
        buf++;
    }
    if (lua_isstring(lua, -1)) // add value
    {
        if (start == buf)
        {
            lua_pushnil(lua);
        }
        else
        {
            tmp = ngx_palloc(pool, buf - start);
            s = ngx_lua_module_unescape_uri(tmp, start, buf - start);
            lua_pushlstring(lua, (const char*)tmp, s);
            ngx_pfree(pool, tmp);
        }
        lua_settable(lua, -3);
    }
    lua_settable(lua, -3);

    lua_pop(lua, 2); // ngx and ngx.req
}

void ngx_lua_module_write_error(struct lua_State* lua)
{
    const char* msg = luaL_checkstring(lua, -1);

    ngx_lua_module_get_err(lua);
    lua_pushstring(lua, "msg");
    lua_pushstring(lua, msg);
    lua_settable(lua, -3);

    lua_pop(lua, 3); // msg, ngx and ngx.err
}

void ngx_lua_module_set_req_obj(struct lua_State* lua, ngx_http_request_t* r)
{
    lua_getglobal(lua, "ngx");
    if (lua_isnil(lua, -1))
    {
        lua_pop(lua, -1);
        lua_newtable(lua);
        lua_setglobal(lua, "ngx");
        lua_getglobal(lua, "ngx");
    }

    lua_pushstring(lua, "__req__");
    lua_pushlightuserdata(lua, r);
    lua_settable(lua, -3);
    lua_pop(lua, 1);
}

ngx_http_request_t* ngx_lua_module_get_req_obj(struct lua_State* lua)
{
    ngx_http_request_t* r;

    lua_getglobal(lua, "ngx");
    if (lua_isnil(lua, -1))
    {
        lua_pop(lua, 1);
        luaL_error(lua, "have no ngx object");
        return NULL;
    }

    lua_pushstring(lua, "__req__");
    lua_gettable(lua, -2);
    r = lua_touserdata(lua, -1);
    lua_pop(lua, 2);

    return r;
}

