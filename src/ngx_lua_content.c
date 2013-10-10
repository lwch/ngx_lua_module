#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_lua_debug.h"

#include "../core/code_cache.h"
#include "../core/hash_table.h"

#include "ngx_lua_error.h"
#include "ngx_lua_module.h"
#include "ngx_lua_module_util.h"

#include "ngx_lua_content.h"

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
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_content_send: out of memory");
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
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_content_send: returned unsupported type(%s)", lua_typename(lua, -1));
        lua_pop(lua, 1);
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = size;

    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only)
    {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_content_send: send header error");
        lua_pop(lua, 1);
        return rc;
    }

    rc = ngx_http_output_filter(r, &out);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only)
    {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_content_send: send content error");
        lua_pop(lua, 1);
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
    ngx_str_t code;
    dbg("ngx_lua_content_handler\n");

    pmainconf = ngx_http_get_module_main_conf(r, ngx_lua_module);
    plocconf  = ngx_http_get_module_loc_conf(r, ngx_lua_module);

    if (ngx_http_complex_value(r, &plocconf->lua_content_code, &code) != NGX_OK)
    {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_content_handler: ngx_http_complex_value error");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    top = lua_gettop(pmainconf->lua);
    ngx_lua_module_set_req_obj(pmainconf->lua, r);
    ngx_lua_module_parse_args(r->pool, r->args.data, r->args.len, pmainconf->lua);

    if (code.len)
    {
        ngx_lua_module_replace_global(pmainconf->lua);
        rc = ngx_lua_content_call_code(r, pmainconf->lua, code.data, code.len);
        ngx_pfree(r->pool, code.data);
        if (rc != NGX_OK) return rc;
        rc = ngx_lua_content_send(r, pmainconf->lua);
        if (rc != NGX_OK) return rc;
    }

    if (top != lua_gettop(pmainconf->lua))
    {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_content_handler: error lua stack");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    return NGX_OK;
}

#define faild(status) do {\
        rc = ngx_lua_content_call_error(r, pmainconf->lua, status); \
        if (rc == NGX_OK) \
        { \
            rc = ngx_lua_content_send(r, pmainconf->lua); \
            if (rc != NGX_OK) return rc; \
        } \
        else return rc; \
        return rc; \
    } while (0)

ngx_int_t ngx_lua_content_by_file_handler(ngx_http_request_t* r)
{
    ngx_lua_main_conf_t* pmainconf;
    ngx_lua_loc_conf_t*  plocconf;
    ngx_int_t rc;
    int top;
    ngx_str_t src_path;
    u_char new_path[PATH_MAX];
    dbg("ngx_lua_content_by_file_handler\n");

    pmainconf = ngx_http_get_module_main_conf(r, ngx_lua_module);
    plocconf  = ngx_http_get_module_loc_conf(r, ngx_lua_module);

    // complex path
    if (ngx_http_complex_value(r, &plocconf->lua_content_file, &src_path) != NGX_OK)
    {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_content_handler: ngx_http_complex_value error");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    // new path with\0
    memcpy(new_path, src_path.data, src_path.len);
    new_path[src_path.len] = 0;
    src_path.data = new_path;

    // register ngx.req.args
    top = lua_gettop(pmainconf->lua);
    ngx_lua_module_set_req_obj(pmainconf->lua, r);
    ngx_lua_module_parse_args(r->pool, r->args.data, r->args.len, pmainconf->lua);

    if (src_path.len)
    {
        code_cache_node_t* ptr;
        char path[PATH_MAX];
        ngx_str_t strPath;
        ngx_str_t code;

        ngx_str_null(&strPath);

        // file exists
        if (access((const char*)src_path.data, 0) == -1)
        {
            lua_pushfstring(pmainconf->lua, "%s doesn't exist", src_path.data);
            faild(NGX_HTTP_NOT_FOUND);
        }

        // get realpath
        if (realpath((const char*)src_path.data, path) == NULL)
        {
            lua_pushfstring(pmainconf->lua, "can't get realpath with %s", src_path.data);
            faild(NGX_HTTP_INTERNAL_SERVER_ERROR);
        }

        ngx_pfree(r->pool, src_path.data);

        // register ngx.scp.path
        ngx_lua_module_get_scp(pmainconf->lua);
        lua_pushstring(pmainconf->lua, "path");
        lua_pushstring(pmainconf->lua, path);
        lua_settable(pmainconf->lua, -3);
        lua_pop(pmainconf->lua, 2);

        // lookup cache
        strPath.data = (u_char*)path;
        strPath.len = strlen(path);
        ptr = ngx_lua_code_cache_key_exists(pmainconf->cache_table, strPath);
        ngx_lua_module_replace_global(pmainconf->lua);
        if (ptr == NULL) // doesn't exist
        {
            code = ngx_lua_code_cache_load(strPath);
            if (code.data == NULL)
            {
                lua_pushstring(pmainconf->lua, "out of memory");
                faild(NGX_HTTP_INTERNAL_SERVER_ERROR);
            }
            if (pmainconf->enable_code_cache)
            {
                ngx_str_t cache;
                dbg("code uncached\n");
                if (ngx_lua_module_code_to_chunk(pmainconf->lua, code.data, code.len, &cache))
                {
                    faild(NGX_HTTP_INTERNAL_SERVER_ERROR);
                }
                ptr = ngx_lua_code_cache_node_new(strPath, cache);
                if (ptr == NULL)
                {
                    lua_pushstring(pmainconf->lua, "out of memory");
                    faild(NGX_HTTP_INTERNAL_SERVER_ERROR);
                }
                ngx_pfree(ngx_cycle->pool, code.data);
                code = ptr->code;
                ngx_lua_core_hash_table_insert_notfind(pmainconf->cache_table, ptr);
                rc = ngx_lua_content_call_chunk(r, pmainconf->lua, &code);
            }
            else rc = ngx_lua_content_call_code(r, pmainconf->lua, code.data, code.len);
        }
        else
        {
            dbg("code cached\n");
            code = ptr->code;
            rc = ngx_lua_content_call_chunk(r, pmainconf->lua, &code);
        }

        ngx_pfree(ngx_cycle->pool, code.data);
        if (rc != NGX_OK) return rc;
        rc = ngx_lua_content_send(r, pmainconf->lua);
        if (rc != NGX_OK) return rc;
    }

    if (top != lua_gettop(pmainconf->lua))
    {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_content_by_file_handler: error lua stack");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    return NGX_OK;
}

char* ngx_lua_content_readconf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
    ngx_str_t* value;
    ngx_http_core_loc_conf_t* pconf;
    ngx_http_compile_complex_value_t ccv;
    ngx_lua_loc_conf_t* plocconf = conf;
    dbg("ngx_lua_content_readconf\n");

    value = cf->args->elts;

    if (value[1].len == 0)
    {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "invalid location config: no runable lua code");
        return NGX_CONF_ERROR;
    }

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));
    ccv.cf = cf;
    ccv.value = &value[1];

    pconf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    if (ngx_strcmp(cmd->name.data, "lua_content") == 0)
    {
        ccv.complex_value = &plocconf->lua_content_code;
        pconf->handler = ngx_lua_content_handler;
    }
    else
    {
        ccv.complex_value = &plocconf->lua_content_file;
        pconf->handler = ngx_lua_content_by_file_handler;
    }
    if (ngx_http_compile_complex_value(&ccv) != NGX_OK)
    {
        pconf->handler = NULL;
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "invalid location config: ngx_http_compile_complex_value error");
        return NGX_CONF_ERROR;
    }
    return NGX_CONF_OK;
}

