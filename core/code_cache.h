#ifndef _CODE_CACHE_H_
#define _CODE_CACHE_H_

typedef struct
{
    char* path;
    char* code;
} code_cache_node_t;

extern code_cache_node_t* ngx_lua_code_cache_node_new(const char* path, const char* code);
extern void ngx_lua_code_cache_node_free(void* data);
extern size_t ngx_lua_code_cache_node_hash(void* data);
extern int ngx_lua_code_cache_node_compare(void* data1, void* data2);

#endif

