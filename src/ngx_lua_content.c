#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "../core/code_cache.h"
#include "../core/hash_table.h"

#include "ngx_lua_module.h"
#include "ngx_lua_module_util.h"

#include "ngx_lua_content.h"

ngx_int_t ngx_lua_content_call_error(ngx_http_request_t* r, lua_State* lua)
{
    ngx_lua_main_conf_t* pconf;

    pconf = ngx_http_get_module_main_conf(r, ngx_lua_module);

    ngx_lua_module_write_error(lua);
    if (pconf->lua_error_code.len)
    {
        if (luaL_loadbuffer(lua, (const char*)pconf->lua_error_code.data, pconf->lua_error_code.len, "@lua_error")) // TODO: error log
        {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
        if (lua_pcall(lua, 0, 1, 0)) // TODO: error log
        {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
        return NGX_OK;
    }
    else if (pconf->lua_error_file.len)
    {
        code_cache_node_t* ptr;
        char path[PATH_MAX];
        ngx_str_t strPath;
        ngx_str_t code;

        ngx_str_null(&strPath);

        if (access((const char*)pconf->lua_error_file.data, 0) == -1) // TODO: error log
        {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        if (realpath((const char*)pconf->lua_error_file.data, path) == NULL) // TODO: error log
        {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        strPath.data = (u_char*)path;
        strPath.len = strlen(path);
        ptr = ngx_lua_code_cache_key_exists(pconf->cache_table, strPath);
        if (ptr == NULL) // doesn't exists
        {
            code = ngx_lua_code_cache_load(strPath);
            if (code.data == NULL) // TODO: error log
            {
                return NGX_HTTP_INTERNAL_SERVER_ERROR;
            }
            ptr = ngx_lua_code_cache_node_new(strPath, code);
            ngx_pfree(ngx_cycle->pool, code.data);
            ngx_lua_core_hash_table_insert_notfind(pconf->cache_table, ptr);
        }

        if (luaL_loadbuffer(lua, (const char*)ptr->code.data, ptr->code.len, "@lua_error")) // TODO: error log
        {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
        if (lua_pcall(lua, 0, 1, 0)) // TODO: error log
        {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
        return NGX_OK;
    }

    // TODO: no file log
    return NGX_HTTP_INTERNAL_SERVER_ERROR;
}

ngx_int_t ngx_lua_content_call_code(ngx_http_request_t* r, lua_State* lua, u_char* code, size_t len)
{
    if (luaL_loadbuffer(lua, (const char*)code, len, "@lua_content")) return ngx_lua_content_call_error(r, lua);
    if (lua_pcall(lua, 0, 1, 0)) return ngx_lua_content_call_error(r, lua);
    return NGX_OK;
}

// send content to client
// error: 1 out of memory
//        2 returned unsupported type
//        3 send header error
//        4 send content error
ngx_int_t ngx_lua_content_send(ngx_http_request_t* r, lua_State* lua)
{
    ngx_int_t            rc;
    ngx_buf_t*           b;
    ngx_chain_t          out;
    size_t               size;

    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if (b == NULL)
    {
        lua_pop(lua, 1);
        lua_pushnumber(lua, 1);
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    out.buf = b;
    out.next = NULL;

    if (lua_isnil(lua, -1))
    {
        b->pos = NULL;
        b->last = NULL;
        b->memory = 0;
        b->last_buf = 1;
        size = 0;
    }
    else if (lua_isstring(lua, -1))
    {
        const char* result = luaL_checklstring(lua, -1, &size);
        b->pos = (u_char*)result;  /* the begin offset of the buffer */
        b->last = (u_char*)result + size; /* the end offset of the buffer */
        b->memory = 1;    /* this buffer is in memory */
        b->last_buf = 1;  /* this is the last buffer in the buffer chain */
    }
    else
    {
        lua_pop(lua, 1);
        lua_pushnumber(lua, 2);
        return NGX_ERROR;
    }

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = size;

    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only)
    {
        lua_pop(lua, 1);
        lua_pushnumber(lua, 3);
        return rc;
    }

    rc = ngx_http_output_filter(r, &out);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only)
    {
        lua_pop(lua, 1);
        lua_pushnumber(lua, 4);
        return rc;
    }

    lua_pop(lua, 1);
    return NGX_OK;
}

ngx_int_t ngx_lua_content_handler(ngx_http_request_t* r)
{
    ngx_lua_main_conf_t* pmainconf;
    ngx_lua_loc_conf_t*  plocconf;
    ngx_int_t rc;
    int top;
    printf("ngx_lua_content_handler\n");

    pmainconf = ngx_http_get_module_main_conf(r, ngx_lua_module);
    plocconf  = ngx_http_get_module_loc_conf(r, ngx_lua_module);
    top = lua_gettop(pmainconf->lua);

    ngx_lua_module_parse_args(r->pool, r->args.data, r->args.len, pmainconf->lua);

    if (plocconf->lua_content_code.len)
    {
        if (ngx_lua_content_call_code(r, pmainconf->lua, plocconf->lua_content_code.data, plocconf->lua_content_code.len) == NGX_OK)
        {
            rc = ngx_lua_content_send(r, pmainconf->lua);
            if (rc != NGX_OK) // TODO: error log
            {
                return NGX_HTTP_INTERNAL_SERVER_ERROR;
            }
        }
        else // TODO: error log
        {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
    }

    if (top != lua_gettop(pmainconf->lua)) // TODO: error log
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    return NGX_OK;
}

ngx_int_t ngx_lua_content_by_file_handler(ngx_http_request_t* r)
{
    ngx_lua_main_conf_t* pmainconf;
    ngx_lua_loc_conf_t*  plocconf;
    ngx_int_t rc;
    int top;
    printf("ngx_lua_content_by_file_handler\n");

    pmainconf = ngx_http_get_module_main_conf(r, ngx_lua_module);
    plocconf  = ngx_http_get_module_loc_conf(r, ngx_lua_module);
    top = lua_gettop(pmainconf->lua);

    ngx_lua_module_parse_args(r->pool, r->args.data, r->args.len, pmainconf->lua);

    if (plocconf->lua_content_file.len)
    {
        code_cache_node_t* ptr;
        char path[PATH_MAX];
        ngx_str_t strPath;
        ngx_str_t code;

        ngx_str_null(&strPath);

        if (access((const char*)plocconf->lua_content_file.data, 0) == -1) return NGX_HTTP_NOT_FOUND;

        if (realpath((const char*)plocconf->lua_content_file.data, path) == NULL) // TODO: error log
        {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        strPath.data = (u_char*)path;
        strPath.len = strlen(path);
        ptr = ngx_lua_code_cache_key_exists(pmainconf->cache_table, strPath);
        if (ptr == NULL) // doesn't exists
        {
            code = ngx_lua_code_cache_load(strPath);
            if (code.data == NULL) // TODO: error log
            {
                return NGX_HTTP_INTERNAL_SERVER_ERROR;
            }
            ptr = ngx_lua_code_cache_node_new(strPath, code);
            ngx_pfree(ngx_cycle->pool, code.data);
            ngx_lua_core_hash_table_insert_notfind(pmainconf->cache_table, ptr);
        }

        if (ngx_lua_content_call_code(r, pmainconf->lua, ptr->code.data, ptr->code.len) == NGX_OK)
        {
            rc = ngx_lua_content_send(r, pmainconf->lua);
            if (rc != NGX_OK) // TODO: error log
            {
                return NGX_HTTP_INTERNAL_SERVER_ERROR;
            }
        }
        else // TODO: error log
        {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
    }

    if (top != lua_gettop(pmainconf->lua)) // TODO: error log
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    return NGX_OK;
}

char* ngx_lua_content_readconf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
    ngx_http_core_loc_conf_t* pconf;
    printf("ngx_lua_content_readconf\n");

    pconf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    if (ngx_strcmp(cmd->name.data, "lua_content") == 0) pconf->handler = ngx_lua_content_handler;
    else pconf->handler = ngx_lua_content_by_file_handler;
    ngx_conf_set_str_slot(cf, cmd, conf);
    return NGX_CONF_OK;
}

