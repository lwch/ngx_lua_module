#ifndef _CODE_CACHE_H_
#define _CODE_CACHE_H_

#include <ngx_string.h>

#include "hash_table.h"

typedef struct
{
    ngx_str_t path;
    ngx_str_t code;
} code_cache_node_t;

extern size_t ngx_lua_code_cache_core_hash(ngx_str_t key);
extern code_cache_node_t* ngx_lua_code_cache_node_new(ngx_str_t path, ngx_str_t code);
extern void ngx_lua_code_cache_node_free(void* data);
extern size_t ngx_lua_code_cache_node_hash(void* data);
extern int ngx_lua_code_cache_node_compare(void* data1, void* data2);
extern code_cache_node_t* ngx_lua_code_cache_key_exists(ngx_lua_hash_table_t* t, ngx_str_t key);
extern ngx_str_t ngx_lua_code_cache_load(ngx_str_t path);

#endif

