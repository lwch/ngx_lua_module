#ifndef _NGX_LUA_INIT_H_
#define _NGX_LUA_INIT_H_

struct ngx_conf_t;
struct ngx_command_t;

extern char* ngx_lua_init_readconf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);

#endif

