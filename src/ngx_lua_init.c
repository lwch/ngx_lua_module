#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_lua_module.h"
#include "ngx_lua_init.h"

char* ngx_lua_init_readconf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
    ngx_lua_main_conf_t* pconf;
    ngx_str_t*           value;
    printf("ngx_lua_init_readconf\n");

    value = cf->args->elts;

    if (value[1].len == 0)
    {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "invalid location config: no runable Lua code");
        return NGX_CONF_ERROR;
    }

    pconf = ngx_http_conf_get_module_main_conf(cf, ngx_lua_module);

    if (ngx_strcmp(cmd->name.data, "lua_init_by_file") == 0) pconf->lua_init_file = value[1];
    else pconf->lua_init_code = value[1];

    return NGX_CONF_OK;
}

