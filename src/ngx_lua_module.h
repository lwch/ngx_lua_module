#ifndef _NGX_LUA_MODULE_H_
#define _NGX_LUA_MODULE_H_

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

typedef struct {
    ngx_str_t  lua_content_code;
    ngx_str_t  lua_content_file;
    ngx_str_t  lua_error_code;
    ngx_str_t  lua_error_file;
    ngx_flag_t enable_code_cache;
} ngx_lua_loc_conf_t;

typedef struct {
    ngx_str_t                    lua_init_code;
    struct ngx_lua_hash_table_t* cache_table;
    lua_State*                   lua;
} ngx_lua_main_conf_t;

extern ngx_module_t ngx_lua_module;

#endif

