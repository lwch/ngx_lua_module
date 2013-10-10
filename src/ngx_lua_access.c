#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_lua_debug.h"

#include "../core/code_cache.h"

#include "ngx_lua_error.h"
#include "ngx_lua_module.h"
#include "ngx_lua_module_util.h"

#include "ngx_lua_access.h"

extern ngx_int_t ngx_lua_access_inline_handler(ngx_http_request_t* r, ngx_str_t code);
extern ngx_int_t ngx_lua_access_by_file_handler(ngx_http_request_t* r, ngx_str_t path);

ngx_int_t ngx_lua_access_handler(ngx_http_request_t* r)
{
    ngx_lua_loc_conf_t* pconf;
    ngx_str_t code;
    ngx_int_t rc;
    dbg("ngx_lua_access_handler\n");

    pconf = ngx_http_get_module_loc_conf(r, ngx_lua_module);

    if (pconf->lua_access_code)
    {
        if (ngx_http_complex_value(r, pconf->lua_access_code, &code) != NGX_OK)
        {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
        if (code.len)
        {
            rc = ngx_lua_access_inline_handler(r, code);
            ngx_pfree(r->pool, code.data);
            return rc;
        }
    }
    if (pconf->lua_access_file)
    {
        if (ngx_http_complex_value(r, pconf->lua_access_file, &code) != NGX_OK)
        {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
        if (code.len)
        {
            rc = ngx_lua_access_by_file_handler(r, code);
            ngx_pfree(r->pool, code.data);
            return rc;
        }
    }
    return NGX_OK;
}

ngx_int_t ngx_lua_access_inline_handler(ngx_http_request_t* r, ngx_str_t code)
{
    ngx_lua_main_conf_t* pconf;
    int top;
    int rc;
    int ret = 0;
    dbg("ngx_lua_access_inline_handler\n");

    pconf = ngx_http_get_module_main_conf(r, ngx_lua_module);

    top = lua_gettop(pconf->lua);
    ngx_lua_module_set_req_obj(pconf->lua, r);
    ngx_lua_module_parse_args(r->pool, r->args.data, r->args.len, pconf->lua);

    ngx_lua_module_replace_global(pconf->lua);
    rc = ngx_lua_content_call_code(r, pconf->lua, code.data, code.len);
    if (rc != NGX_OK) return rc;
    if (lua_isboolean(pconf->lua, -1)) ret = lua_toboolean(pconf->lua, -1);
    lua_pop(pconf->lua, 1);

    dbg("access: %d\n", ret);

    if (top != lua_gettop(pconf->lua))
    {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_access_inline_handler: error lua stack");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    return ret ? NGX_OK : NGX_HTTP_FORBIDDEN;
}

#define faild(status) do {\
                          rc = ngx_lua_content_call_error(r, pconf->lua, status); \
                          if (rc == NGX_OK) \
                          { \
                              if (lua_isboolean(pconf->lua, -1)) ret = lua_toboolean(pconf->lua, -1); \
                              lua_pop(pconf->lua, 1); \
                              dbg("access: %d\n", ret); \
                              return ret ? NGX_OK : NGX_HTTP_FORBIDDEN; \
                          } \
                          return rc; \
                      } while (0)
ngx_int_t ngx_lua_access_by_file_handler(ngx_http_request_t* r, ngx_str_t path)
{
    ngx_lua_main_conf_t* pconf;
    ngx_int_t rc;
    int top;
    int ret = 0;
    u_char new_path[PATH_MAX];
    ngx_str_t src_path = path;
    dbg("ngx_lua_access_by_file_handler\n");

    pconf = ngx_http_get_module_main_conf(r, ngx_lua_module);

    // new path with\0
    memcpy(new_path, path.data, path.len);
    new_path[path.len] = 0;
    src_path.data = new_path;

    // register ngx.req.args
    top = lua_gettop(pconf->lua);
    ngx_lua_module_set_req_obj(pconf->lua, r);
    ngx_lua_module_parse_args(r->pool, r->args.data, r->args.len, pconf->lua);

    {
        code_cache_node_t* ptr;
        char path[PATH_MAX];
        ngx_str_t strPath;
        ngx_str_t code;

        ngx_str_null(&strPath);

        // file exists
        if (access((const char*)src_path.data, 0) == -1)
        {
            lua_pushfstring(pconf->lua, "%s doesn't exist", src_path.data);
            faild(NGX_HTTP_NOT_FOUND);
        }

        // get realpath
        if (realpath((const char*)src_path.data, path) == NULL)
        {
            lua_pushfstring(pconf->lua, "can't get realpath with %s", src_path.data);
            faild(NGX_HTTP_INTERNAL_SERVER_ERROR);
        }

        ngx_pfree(r->pool, src_path.data);

        // register ngx.scp.path
        ngx_lua_module_get_scp(pconf->lua);
        lua_pushstring(pconf->lua, "path");
        lua_pushstring(pconf->lua, path);
        lua_settable(pconf->lua, -3);
        lua_pop(pconf->lua, 2);

        // lookup cache
        strPath.data = (u_char*)path;
        strPath.len = strlen(path);
        ptr = ngx_lua_code_cache_key_exists(pconf->cache_table, strPath);
        ngx_lua_module_replace_global(pconf->lua);
        if (ptr == NULL) // doesn't exist
        {
            code = ngx_lua_code_cache_load(strPath);
            if (code.data == NULL)
            {
                lua_pushstring(pconf->lua, "out of memory");
                faild(NGX_HTTP_INTERNAL_SERVER_ERROR);
            }
            if (pconf->enable_code_cache)
            {
                ngx_str_t cache;
                dbg("code uncached\n");
                if (ngx_lua_module_code_to_chunk(pconf->lua, code.data, code.len, &cache))
                {
                    faild(NGX_HTTP_INTERNAL_SERVER_ERROR);
                }
                ptr = ngx_lua_code_cache_node_new(strPath, cache);
                if (ptr == NULL)
                {
                    lua_pushstring(pconf->lua, "out of memory");
                    faild(NGX_HTTP_INTERNAL_SERVER_ERROR);
                }
                ngx_pfree(ngx_cycle->pool, code.data);
                code = ptr->code;
                ngx_lua_core_hash_table_insert_notfind(pconf->cache_table, ptr);
                rc = ngx_lua_content_call_chunk(r, pconf->lua, &code);
            }
            else rc = ngx_lua_content_call_code(r, pconf->lua, code.data, code.len);
        }
        else
        {
            dbg("code cached\n");
            code = ptr->code;
            rc = ngx_lua_content_call_chunk(r, pconf->lua, &code);
        }

        ngx_pfree(ngx_cycle->pool, code.data);
        if (rc != NGX_OK) return rc;
        if (lua_isboolean(pconf->lua, -1)) ret = lua_toboolean(pconf->lua, -1);
        lua_pop(pconf->lua, 1);

        dbg("access: %d\n", ret);
    }

    if (top != lua_gettop(pconf->lua))
    {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_access_by_file_handler: error lua stack");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    return ret ? NGX_OK : NGX_HTTP_FORBIDDEN;
}

