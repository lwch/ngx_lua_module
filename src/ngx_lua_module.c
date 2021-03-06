#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "../core/hash_table.h"
#include "../core/code_cache.h"

#include "ngx_lua_debug.h"

#include "ngx_lua_module_util.h"
#include "ngx_lua_error.h"
#include "ngx_lua_init.h"
#include "ngx_lua_content.h"
#include "ngx_lua_code.h"
#include "ngx_lua_exit.h"
#include "ngx_lua_access.h"

#include "ngx_lua_module.h"

// http module
ngx_int_t ngx_lua_post_conf(ngx_conf_t* cf);
void* ngx_lua_create_main_conf(ngx_conf_t* cf);
char* ngx_lua_init_main_conf(ngx_conf_t* cf, void* conf);
void* ngx_lua_create_loc_conf(ngx_conf_t* cf);
char* ngx_lua_merge_loc_conf(ngx_conf_t* cf, void* parent, void* child);

// module
ngx_int_t ngx_lua_init_process(ngx_cycle_t* cycle);
void ngx_lua_exit_process(ngx_cycle_t* cycle);

ngx_command_t ngx_lua_commands[] = {
    {
        ngx_string("lua_code_cache"),
        NGX_HTTP_MAIN_CONF | NGX_CONF_FLAG,
        ngx_lua_code_cache_readconf,
        NGX_HTTP_MAIN_CONF_OFFSET,
        offsetof(ngx_lua_main_conf_t, enable_code_cache),
        NULL
    },
    {
        ngx_string("lua_init"),
        NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
        ngx_lua_init_readconf,
        NGX_HTTP_MAIN_CONF_OFFSET,
        offsetof(ngx_lua_main_conf_t, lua_init_code),
        NULL
    },
    {
        ngx_string("lua_init_by_file"),
        NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
        ngx_lua_init_readconf,
        NGX_HTTP_MAIN_CONF_OFFSET,
        offsetof(ngx_lua_main_conf_t, lua_init_file),
        NULL
    },
    {
        ngx_string("lua_content"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_lua_content_readconf,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    {
        ngx_string("lua_content_by_file"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_lua_content_readconf,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    {
        ngx_string("lua_access_by_file"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_http_set_complex_value_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_lua_loc_conf_t, lua_access_file),
        NULL
    },
    {
        ngx_string("lua_access"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_http_set_complex_value_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_lua_loc_conf_t, lua_access_code),
        NULL
    },
    {
        ngx_string("lua_error"),
        NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_MAIN_CONF_OFFSET,
        offsetof(ngx_lua_main_conf_t, lua_error_code),
        NULL
    },
    {
        ngx_string("lua_error_by_file"),
        NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_MAIN_CONF_OFFSET,
        offsetof(ngx_lua_main_conf_t, lua_error_file),
        NULL
    },
    {
        ngx_string("lua_exit"),
        NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
        ngx_lua_exit_readconf,
        NGX_HTTP_MAIN_CONF_OFFSET,
        offsetof(ngx_lua_main_conf_t, lua_exit_code),
        NULL
    },
    {
        ngx_string("lua_exit_by_file"),
        NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
        ngx_lua_exit_readconf,
        NGX_HTTP_MAIN_CONF_OFFSET,
        offsetof(ngx_lua_main_conf_t, lua_exit_file),
        NULL
    },
    ngx_null_command
};

ngx_http_module_t ngx_lua_module_ctx = {
    NULL,                     // pre configuration
    ngx_lua_post_conf,        // post configuration
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

ngx_int_t ngx_lua_access_phase_handler(ngx_http_request_t *r)
{
    ngx_int_t rc;
    dbg("ngx_lua_access_phase_handler\n");

    rc = ngx_lua_access_handler(r);

    if (rc == NGX_OK) return NGX_DECLINED;
    else return rc;
}

ngx_int_t ngx_lua_post_conf(ngx_conf_t* cf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;
    dbg("ngx_lua_post_conf\n");

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_lua_access_phase_handler;

    return NGX_OK;
}

void* ngx_lua_create_main_conf(ngx_conf_t* cf)
{
    ngx_lua_main_conf_t* pconf;
    dbg("ngx_lua_create_main_conf\n");

    pconf = ngx_pcalloc(cf->pool, sizeof(ngx_lua_main_conf_t));
    if (pconf == NULL) return NGX_CONF_ERROR;

    ngx_str_null(&pconf->lua_init_code);
    ngx_str_null(&pconf->lua_init_file);
    ngx_str_null(&pconf->lua_exit_code);
    ngx_str_null(&pconf->lua_exit_file);
    ngx_str_null(&pconf->lua_error_code);
    ngx_str_null(&pconf->lua_error_file);
    pconf->enable_code_cache = NGX_CONF_UNSET_UINT;
    pconf->cache_table = NULL;
    pconf->lua = NULL;
    return pconf;
}

char* ngx_lua_init_main_conf(ngx_conf_t* cf, void* conf)
{
    ngx_lua_main_conf_t* pconf = conf;
    dbg("ngx_lua_init_main_conf\n");

    if (pconf->enable_code_cache == (ngx_flag_t)NGX_CONF_UNSET_UINT) pconf->enable_code_cache = 1;

    return NGX_CONF_OK;
}

void* ngx_lua_create_loc_conf(ngx_conf_t* cf)
{
    ngx_lua_loc_conf_t* conf;
    dbg("ngx_lua_create_loc_conf\n");

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_lua_loc_conf_t));
    if (conf == NULL) return NGX_CONF_ERROR;

    return conf;
}

char* ngx_lua_merge_loc_conf(ngx_conf_t* cf, void* parent, void* child)
{
    /*ngx_lua_loc_conf_t* prev = parent;
    ngx_lua_loc_conf_t* conf = child;*/
    dbg("ngx_lua_merge_loc_conf\n");

    /*if (conf->lua_content_code.len == 0)
    {
        conf->lua_content_code = prev->lua_content_code;
    }*/

    return NGX_CONF_OK;
}

ngx_int_t ngx_lua_init_process(ngx_cycle_t* cycle)
{
    ngx_lua_main_conf_t* pconf;
    ngx_lua_hash_table_functor functor;
    dbg("ngx_lua_init_process\n");

    pconf = ngx_http_cycle_get_module_main_conf(cycle, ngx_lua_module);
    pconf->lua = luaL_newstate();

    luaL_openlibs(pconf->lua);
    ngx_lua_module_init(pconf->lua);

    functor.hash = ngx_lua_code_cache_node_hash;
    functor.free = ngx_lua_code_cache_node_free;
    functor.compare = ngx_lua_code_cache_node_compare;
    pconf->cache_table = ngx_lua_core_hash_table_new(default_buckets_count, default_buckets_max_length, functor);
    if (pconf->cache_table == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "out of memory");
        return NGX_ERROR;
    }

    if (pconf->lua_init_code.len)
    {
        if (luaL_loadbuffer(pconf->lua, (const char*)pconf->lua_init_code.data, pconf->lua_init_code.len, "@lua_init"))
        {
            ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "ngx_lua_init_process: luaL_loadbuffer error");
            ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "ngx_lua_init_process: %s", lua_tostring(pconf->lua, -1));
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
        if (lua_pcall(pconf->lua, 0, 0, 0))
        {
            ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "ngx_lua_init_process: lua_pcall error");
            ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "ngx_lua_init_process: %s", lua_tostring(pconf->lua, -1));
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
    }
    else if (pconf->lua_init_file.len)
    {
        char path[PATH_MAX];
        ngx_str_t strPath;
        ngx_str_t code;

        if (access((const char*)pconf->lua_init_file.data, 0) == -1)
        {
            ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_content_by_file_handler: file %s doesn't exist", pconf->lua_init_file.data);
            return NGX_ERROR;
        }

        if (realpath((const char*)pconf->lua_init_file.data, path) == NULL)
        {
            ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_content_by_file_handler: can't get realpath with %s", pconf->lua_init_file.data);
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        strPath.data = (u_char*)path;
        strPath.len = strlen(path);
        code = ngx_lua_code_cache_load(strPath);
        if (code.data == NULL)
        {
            ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "ngx_lua_init_process: out of memory");
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        ngx_lua_module_get_scp(pconf->lua);
        lua_pushstring(pconf->lua, "path");
        lua_pushstring(pconf->lua, path);
        lua_settable(pconf->lua, -3);
        lua_pop(pconf->lua, 2);

        if (luaL_loadbuffer(pconf->lua, (const char*)code.data, code.len, "@lua_init_by_file"))
        {
            ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "ngx_lua_init_process: luaL_loadbuffer error");
            ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "ngx_lua_init_process: %s", lua_tostring(pconf->lua, -1));
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
        if (lua_pcall(pconf->lua, 0, 0, 0))
        {
            ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "ngx_lua_init_process: lua_pcall error");
            ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "ngx_lua_init_process: %s", lua_tostring(pconf->lua, -1));
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
    }

    return NGX_OK;
}

void ngx_lua_exit_process(ngx_cycle_t* cycle)
{
    ngx_lua_main_conf_t* pconf;
    dbg("ngx_lua_exit_process\n");

    pconf = ngx_http_cycle_get_module_main_conf(cycle, ngx_lua_module);

    if (pconf->lua_exit_code.len)
    {
        if (luaL_loadbuffer(pconf->lua, (const char*)pconf->lua_exit_code.data, pconf->lua_exit_code.len, "@lua_exit") == 0)
        {
            if (lua_pcall(pconf->lua, 0, 0, 0))
            {
                ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "ngx_lua_exit_process: lua_pcall error");
                ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "ngx_lua_exit_process: %s", lua_tostring(pconf->lua, -1));
            }
        }
        else
        {
            ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "ngx_lua_exit_process: luaL_loadbuffer error");
            ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "ngx_lua_exit_process: %s", lua_tostring(pconf->lua, -1));
        }
    }
    else if (pconf->lua_exit_file.len)
    {
        char path[PATH_MAX];
        ngx_str_t strPath;
        ngx_str_t code;

        if (access((const char*)pconf->lua_exit_file.data, 0) != -1)
        {
            if (realpath((const char*)pconf->lua_exit_file.data, path))
            {
                strPath.data = (u_char*)path;
                strPath.len = strlen(path);
                code = ngx_lua_code_cache_load(strPath);
                if (code.data)
                {
                    ngx_lua_module_get_scp(pconf->lua);
                    lua_pushstring(pconf->lua, "path");
                    lua_pushstring(pconf->lua, path);
                    lua_settable(pconf->lua, -3);
                    lua_pop(pconf->lua, 2);
                    if (luaL_loadbuffer(pconf->lua, (const char*)code.data, code.len, "@lua_exit_by_file") == 0)
                    {
                        if (lua_pcall(pconf->lua, 0, 0, 0))
                        {
                            ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "ngx_lua_exit_process: lua_pcall error");
                            ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "ngx_lua_exit_process: %s", lua_tostring(pconf->lua, -1));
                        }
                    }
                    else
                    {
                        ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "ngx_lua_exit_process: luaL_loadbuffer error");
                        ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "ngx_lua_exit_process: %s", lua_tostring(pconf->lua, -1));
                    }
                }
                else ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "ngx_lua_exit_process: out of memory");
            }
            else ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_content_by_file_handler: can't get realpath with %s", pconf->lua_exit_file.data);
        }
        else ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ngx_lua_content_by_file_handler: file %s doesn't exist", pconf->lua_exit_file.data);
    }

    if (pconf->cache_table) ngx_lua_core_hash_table_free(pconf->cache_table);
    if (pconf->lua) lua_close(pconf->lua);
}
