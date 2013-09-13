#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_lua_module.h"
#include "ngx_lua_content.h"

// TODO: 传参
ngx_int_t ngx_lua_content_handler(ngx_http_request_t* r)
{
    ngx_lua_main_conf_t* pmainconf;
    ngx_lua_loc_conf_t*  plocconf;
    ngx_int_t            rc;
    ngx_buf_t*           b;
    ngx_chain_t          out;
    size_t               size;
    printf("ngx_lua_content_handler\n");

    pmainconf = ngx_http_get_module_main_conf(r, ngx_lua_module);
    plocconf  = ngx_http_get_module_loc_conf(r, ngx_lua_module);

    if (plocconf->lua_content_code.len)
    {
        if (luaL_loadbuffer(pmainconf->lua, (const char*)plocconf->lua_content_code.data, plocconf->lua_content_code.len, "@lua_content"))
        {
            ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_init_process: luaL_loadbuffer error");
            ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_init_process: %s", lua_tostring(pmainconf->lua, -1));
            return NGX_ERROR;
        }
        if (lua_pcall(pmainconf->lua, 0, 1, 0))
        {
            ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_init_process: lua_pcall error");
            ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_init_process: %s", lua_tostring(pmainconf->lua, -1));
            return NGX_ERROR;
        }
    }

    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if (b == NULL) return NGX_HTTP_INTERNAL_SERVER_ERROR;
    out.buf = b;
    out.next = NULL;

    if (lua_isnil(pmainconf->lua, -1))
    {
        b->pos = NULL;
        b->last = NULL;
        b->memory = 0;
        b->last_buf = 1;
        size = 0;
    }
    else
    {
        const char* result = luaL_checklstring(pmainconf->lua, -1, &size);
        b->pos = (u_char*)result;  /* the begin offset of the buffer */
        b->last = (u_char*)result + size; /* the end offset of the buffer */
        b->memory = 1;    /* this buffer is in memory */
        b->last_buf = 1;  /* this is the last buffer in the buffer chain */
    }
 
    //还是完善HTTP头
    /* set the status line */
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = size;
 
    //输出HTTP头
    /* send the headers of your response */
    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) return rc;

    rc = ngx_http_output_filter(r, &out);

    lua_pop(pmainconf->lua, 1);

    return rc;
}

// TODO: cache
// TODO: 传参
ngx_int_t ngx_lua_content_by_file_handler(ngx_http_request_t* r)
{
    printf("ngx_lua_content_by_file_handler\n");

    return NGX_OK;
}

char* ngx_lua_content_readconf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
    ngx_http_core_loc_conf_t* pconf;
    printf("ngx_lua_content_readconf\n");

    pconf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    if (ngx_strcmp(cmd->name.data, "lua_content_by_file") == 0) pconf->handler = ngx_lua_content_by_file_handler;
    else pconf->handler = ngx_lua_content_handler;
    ngx_conf_set_str_slot(cf, cmd, conf);
    return NGX_CONF_OK;
}

