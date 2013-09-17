#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_lua_error.h"

char* ngx_lua_error_readconf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
    printf("ngx_lua_error_readconf\n");
    ngx_conf_set_str_slot(cf, cmd, conf);
    return NGX_CONF_OK;
}

