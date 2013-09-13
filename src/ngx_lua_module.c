#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_lua_init.h"

#include "ngx_lua_module.h"

// for test
char* ngx_lua_readconf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);

// http module
void* ngx_lua_create_main_conf(ngx_conf_t* cf);
char* ngx_lua_init_main_conf(ngx_conf_t* cf, void* conf);
void* ngx_lua_create_loc_conf(ngx_conf_t* cf);
char* ngx_lua_merge_loc_conf(ngx_conf_t* cf, void* parent, void* child);

// module
ngx_int_t ngx_lua_init_process(ngx_cycle_t* cycle);
void ngx_lua_exit_process(ngx_cycle_t* cycle);

ngx_command_t ngx_lua_commands[] = {
    {
        ngx_string("echo"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_lua_readconf,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_lua_loc_conf_t, ecdata),
        NULL
    },
    {
        ngx_string("lua_init"),
        NGX_HTTP_SRV_CONF | NGX_CONF_TAKE1,
        ngx_lua_init_readconf,
        NGX_HTTP_SRV_CONF_OFFSET,
        0,
        NULL
    },
    ngx_null_command
};

ngx_http_module_t ngx_lua_module_ctx = {
    NULL,                     // pre configuration
    NULL,                     // post configuration
    ngx_lua_create_main_conf, // create main conf
    ngx_lua_init_main_conf,   // init main conf
    NULL,                     // create srv conf
    NULL,                     // merge srv conf
    ngx_lua_create_loc_conf,  // create loc conf
    ngx_lua_merge_loc_conf    // merge loc conf
};

ngx_module_t ngx_lua_module = {
    NGX_MODULE_V1,
    &ngx_lua_module_ctx,
    ngx_lua_commands,
    NGX_HTTP_MODULE,
    NULL,                 // init master
    NULL,                 // init module
    ngx_lua_init_process, // init process
    NULL,                 // init thread
    NULL,                 // exit thread
    ngx_lua_exit_process, // exit process
    NULL,                 // exit master
    NGX_MODULE_V1_PADDING
};

ngx_int_t ngx_lua_handler(ngx_http_request_t* r)
{
    ngx_int_t   rc;
    ngx_buf_t*  b;
    ngx_chain_t out;
    //ngx_lua_main_conf_t* pmainconf;
    ngx_lua_loc_conf_t* cf;
    printf("ngx_lua_handler\n");

    //pmainconf = ngx_http_get_module_main_conf(r, ngx_lua_module);
    cf = ngx_http_get_module_loc_conf(r, ngx_lua_module);

    if (!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD))) return NGX_HTTP_NOT_ALLOWED;
    if (r->headers_in.if_modified_since) return NGX_HTTP_NOT_MODIFIED;

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = cf->ecdata.len;

    if (r->method == NGX_HTTP_HEAD)
    {
        rc = ngx_http_send_header(r);
        if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) return rc;
    }

    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if (b == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Failed to allocate response buffer.");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    out.buf = b;
    out.next = NULL;

    b->pos = cf->ecdata.data;
    b->last = cf->ecdata.data + cf->ecdata.len;

    b->memory = 1;
    b->last_buf = 1;
    rc = ngx_http_send_header(r);

    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) return rc;

    return ngx_http_output_filter(r, &out);
}

char* ngx_lua_readconf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
    ngx_http_core_loc_conf_t* clc;
    printf("ngx_lua_readconf\n");

    clc = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clc->handler = ngx_lua_handler;
    ngx_conf_set_str_slot(cf, cmd, conf);
    return NGX_CONF_OK;
}

void* ngx_lua_create_main_conf(ngx_conf_t* cf)
{
    ngx_lua_main_conf_t* pconf;
    printf("ngx_lua_create_main_conf\n");

    pconf = ngx_pcalloc(cf->pool, sizeof(ngx_lua_main_conf_t));
    if (pconf == NULL) return NGX_CONF_ERROR;

    return pconf;
}

char* ngx_lua_init_main_conf(ngx_conf_t* cf, void* conf)
{
    ngx_lua_main_conf_t* pconf = conf;
    printf("ngx_lua_init_main_conf\n");

    ngx_str_null(&pconf->lua_init_code);
    pconf->lua = 0;
    return NGX_CONF_OK;
}

void* ngx_lua_create_loc_conf(ngx_conf_t* cf)
{
    ngx_lua_loc_conf_t* conf;
    printf("ngx_lua_create_loc_conf\n");
    
    conf = ngx_pcalloc(cf->pool, sizeof(ngx_lua_loc_conf_t));
    if (conf == NULL) return NGX_CONF_ERROR;

    conf->ecdata.len = 0;
    conf->ecdata.data = NULL;
    conf->enable = NGX_CONF_UNSET;
    return conf;
}

char* ngx_lua_merge_loc_conf(ngx_conf_t* cf, void* parent, void* child)
{
    ngx_lua_loc_conf_t* prev = parent;
    ngx_lua_loc_conf_t* conf = child;
    printf("ngx_lua_merge_loc_conf\n");

    ngx_conf_merge_str_value(conf->ecdata, prev->ecdata, 10);
    ngx_conf_merge_value(conf->enable, prev->enable, 0);

    return NGX_CONF_OK;
}

ngx_int_t ngx_lua_init_process(ngx_cycle_t* cycle)
{
    ngx_lua_main_conf_t* pconf;
    printf("ngx_lua_init_process\n");

    pconf = ngx_http_cycle_get_module_main_conf(cycle, ngx_lua_module);
    pconf->lua = NULL; // create lua object
    return NGX_OK;
}

void ngx_lua_exit_process(ngx_cycle_t* cycle)
{
    printf("ngx_lua_exit_process\n");
}
