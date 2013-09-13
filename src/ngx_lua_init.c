#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_lua_init.h"

char* ngx_lua_init_readconf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
    printf("ngx_lua_init_readconf\n");
    return NGX_CONF_OK;
}

