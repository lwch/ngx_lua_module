#ifndef _NGX_LUA_MODULE_H_
#define _NGX_LUA_MODULE_H_

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../core/hash_table.h"

typedef struct {
    ngx_http_complex_value_t lua_content_code;
    ngx_http_complex_value_t lua_content_file;
} ngx_lua_loc_conf_t;

typedef struct {
    ngx_str_t             lua_init_code;
    ngx_str_t             lua_init_file;
    ngx_str_t             lua_exit_code;
    ngx_str_t             lua_exit_file;
    ngx_str_t             lua_error_code;
    ngx_str_t             lua_error_file;
    ngx_lua_hash_table_t* cache_table;
    ngx_flag_t            enable_code_cache;
    lua_State*            lua;
} ngx_lua_main_conf_t;

extern ngx_module_t ngx_lua_module;

#endif

