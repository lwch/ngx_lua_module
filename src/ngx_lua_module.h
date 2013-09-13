#ifndef _NGX_LUA_MODULE_H_
#define _NGX_LUA_MODULE_H_

typedef struct {
    ngx_str_t ecdata;
    ngx_flag_t enable;
} ngx_lua_loc_conf_t;

typedef struct {
    ngx_str_t lua_init_code;
    void*     lua; // lua object
} ngx_lua_main_conf_t;

#endif

