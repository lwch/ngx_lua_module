#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_lua_code.h"

char* ngx_lua_code_cache_readconf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
    return ngx_conf_set_flag_slot(cf, cmd, conf);
}

