#include <ngx_core.h>

#include "code_cache.h"

code_cache_node_t* ngx_lua_code_cache_node_new(const char* path, const char* code)
{
    code_cache_node_t* ptr = ngx_palloc(ngx_cycle->pool, sizeof(code_cache_node_t));
    size_t size;
    if (ptr == NULL) return NULL;

    size = strlen(path);
    ptr->path = ngx_palloc(ngx_cycle->pool, size + 1);
    if (ptr->path == NULL)
    {
        ngx_pfree(ngx_cycle->pool, ptr);
        return NULL;
    }
    memcpy(ptr->path, path, size);
    ptr->path[size] = 0;

    size = strlen(code);
    ptr->code = ngx_palloc(ngx_cycle->pool, size + 1);
    if (ptr->code == NULL)
    {
        ngx_pfree(ngx_cycle->pool, ptr->path);
        ngx_pfree(ngx_cycle->pool, ptr);
        return NULL;
    }
    memcpy(ptr->code, code, size);
    ptr->code[size] = 0;
    return ptr;
}

void ngx_lua_code_cache_node_free(void* data)
{
    code_cache_node_t* ptr = (code_cache_node_t*)data;
    ngx_pfree(ngx_cycle->pool, ptr->path);
    ngx_pfree(ngx_cycle->pool, ptr->code);
    ngx_pfree(ngx_cycle->pool, ptr);
}

size_t ngx_lua_code_cache_node_hash(void* data)
{
    code_cache_node_t* ptr = (code_cache_node_t*)data;
#if __i386__
    size_t offset_basis = 2166136261U;
    size_t prime        = 16777619U;
#else // __amd64__
    size_t offset_basis = 14695981039346656037ULL;
    size_t prime        = 1099511628211ULL;
#endif
    size_t len = strlen(ptr->path);
    size_t value = offset_basis;
    size_t i;

    for (i = 0; i < len; ++i)
    {
        value ^= (size_t)ptr->path[i];
        value *= prime;
    }
#if __amd64__
    value ^= value >> 32;
#endif
    return value;
}

int ngx_lua_code_cache_node_compare(void* data1, void* data2)
{
    code_cache_node_t* ptr1 = (code_cache_node_t*)data1;
    code_cache_node_t* ptr2 = (code_cache_node_t*)data2;

    return strcmp(ptr1->path, ptr2->path) == 0;
}

