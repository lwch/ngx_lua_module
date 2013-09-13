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

    if (ngx_strcmp(cmd->name.data, "lua_init_by_file") == 0)
    {
        u_char* ptr = NULL;
        u_char* tmp = NULL;
        FILE* fp = fopen((const char*)value[1].data, "rb");
        long size = 0;
        size_t readen = 0;

        if (fp == NULL)
        {
            ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "cannot open file %s", value[1].data);
            return NGX_CONF_ERROR;
        }

        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        tmp = ptr = ngx_palloc(cf->pool, size);

        while (size)
        {
            readen = fread(tmp, size, sizeof(u_char), fp);
            if (readen == 0)
            {
                ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "IO error");
                return NGX_CONF_ERROR;
            }
            else
            {
                tmp += readen;
                size -= readen;
            }
        }

        fclose(fp);

        pconf->lua_init_code.len = size;
        pconf->lua_init_code.data = ptr;
    }
    else
    {
        pconf->lua_init_code = value[1];
    }
    return NGX_CONF_OK;
}

