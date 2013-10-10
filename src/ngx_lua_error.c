#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "../core/code_cache.h"

#include "ngx_lua_module.h"
#include "ngx_lua_debug.h"
#include "ngx_lua_module_util.h"

#include "ngx_lua_error.h"

#define faild do { \
                  if (!pconf->enable_code_cache) ngx_pfree(ngx_cycle->pool, code.data); \
                  ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_content_call_error: %s", lua_tostring(lua, -1)); \
                  lua_pop(lua, 1); \
                  return NGX_HTTP_INTERNAL_SERVER_ERROR; \
              } while (0)

ngx_int_t ngx_lua_content_call_error(ngx_http_request_t* r, lua_State* lua, ngx_uint_t status)
{
    ngx_lua_main_conf_t* pconf;

    pconf = ngx_http_get_module_main_conf(r, ngx_lua_module);

    ngx_lua_module_write_error(lua, status);
    if (pconf->lua_error_code.len)
    {
        if (luaL_loadbuffer(lua, (const char*)pconf->lua_error_code.data, pconf->lua_error_code.len, "@lua_error"))
        {
            ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_content_call_error(luaL_loadbuffer): %s", lua_tostring(lua, -1));
            lua_pop(lua, 1);
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
        if (lua_pcall(lua, 0, 1, 0))
        {
            ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_content_call_error(lua_pcall): %s", lua_tostring(pconf->lua, -1));
            lua_pop(lua, 1);
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

        if (access((const char*)pconf->lua_error_file.data, 0) == -1)
        {
            ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_content_call_error: %s doesn't exist", pconf->lua_error_file.data);
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        if (realpath((const char*)pconf->lua_error_file.data, path) == NULL)
        {
            ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_content_call_error: can't get realpath with %s", pconf->lua_error_file.data);
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        ngx_lua_module_get_scp(pconf->lua);
        lua_pushstring(pconf->lua, "path");
        lua_pushstring(pconf->lua, path);
        lua_settable(pconf->lua, -3);
        lua_pop(pconf->lua, 2);

        strPath.data = (u_char*)path;
        strPath.len = strlen(path);
        ptr = ngx_lua_code_cache_key_exists(pconf->cache_table, strPath);
        if (ptr == NULL) // doesn't exists
        {
            code = ngx_lua_code_cache_load(strPath);
            if (code.data == NULL)
            {
                ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "out of memory");
                return NGX_HTTP_INTERNAL_SERVER_ERROR;
            }
            if (pconf->enable_code_cache)
            {
                ngx_str_t cache;
                dbg("code uncached\n");
                if (ngx_lua_module_code_to_chunk(lua, code.data, code.len, &cache))
                {
                    ngx_pfree(ngx_cycle->pool, code.data);
                    ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_content_call_error: %s", lua_tostring(lua, -1));
                    return NGX_HTTP_INTERNAL_SERVER_ERROR;
                }
                ptr = ngx_lua_code_cache_node_new(strPath, cache);
                if (ptr == NULL)
                {
                    ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "out of memory");
                    return NGX_HTTP_INTERNAL_SERVER_ERROR;
                }
                ngx_pfree(ngx_cycle->pool, code.data);
                code = ptr->code;
                ngx_lua_core_hash_table_insert_notfind(pconf->cache_table, ptr);

                if (ngx_lua_module_chunk_load(lua, &code)) faild;
            }
            else
            {
                if (luaL_loadbuffer(lua, (const char*)code.data, code.len, "@lua_error")) faild;
            }
        }
        else
        {
            dbg("code cached\n");
            code = ptr->code;
            if (ngx_lua_module_chunk_load(lua, &code)) faild;
        }

        if (lua_pcall(lua, 0, 1, 0))
        {
            if (!pconf->enable_code_cache) ngx_pfree(ngx_cycle->pool, code.data);
            ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_content_call_error(lua_pcall): %s", lua_tostring(lua, -1));
            lua_pop(lua, 1);
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
        if (!pconf->enable_code_cache) ngx_pfree(ngx_cycle->pool, code.data);
        return NGX_OK;
    }

    // undefined lua_error or lua_error_by_file return nil
    lua_pushnil(lua);
    ngx_log_error(NGX_LOG_WARN, ngx_cycle->log, 0, "undefined lua_error or lua_error_by_file default return nil for client");
    return NGX_OK;
}

