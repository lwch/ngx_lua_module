#ifndef _NGX_LUA_CONTENT_H_
#define _NGX_LUA_CONTENT_H_

struct ngx_conf_t;
struct ngx_command_t;

extern char* ngx_lua_content_readconf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);

#endif

