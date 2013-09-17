#include <ngx_core.h>

#include "code_cache.h"

size_t ngx_lua_code_cache_core_hash(ngx_str_t key)
{
    #if __i386__
    size_t offset_basis = 2166136261U;
    size_t prime        = 16777619U;
#else // __amd64__
    size_t offset_basis = 14695981039346656037ULL;
    size_t prime        = 1099511628211ULL;
#endif
    size_t value = offset_basis;
    size_t i;

    for (i = 0; i < key.len; ++i)
    {
        value ^= (size_t)key.data[i];
        value *= prime;
    }
#if __amd64__
    value ^= value >> 32;
#endif
    return value;
}

code_cache_node_t* ngx_lua_code_cache_node_new(ngx_str_t path, ngx_str_t code)
{
    code_cache_node_t* ptr = ngx_palloc(ngx_cycle->pool, sizeof(code_cache_node_t));
    if (ptr == NULL) return NULL;

    ptr->path.data = ngx_palloc(ngx_cycle->pool, path.len + 1);
    if (ptr->path.data == NULL)
    {
        ngx_pfree(ngx_cycle->pool, ptr);
        return NULL;
    }
    memcpy(ptr->path.data, path.data, path.len);
    ptr->path.data[path.len] = 0;
    ptr->path.len = path.len;

    ptr->code.data = ngx_palloc(ngx_cycle->pool, code.len + 1);
    if (ptr->code.data == NULL)
    {
        ngx_pfree(ngx_cycle->pool, ptr->path.data);
        ngx_pfree(ngx_cycle->pool, ptr);
        return NULL;
    }
    memcpy(ptr->code.data, code.data, code.len);
    ptr->code.data[code.len] = 0;
    ptr->code.len = code.len;
    return ptr;
}

void ngx_lua_code_cache_node_free(void* data)
{
    code_cache_node_t* ptr = (code_cache_node_t*)data;
    ngx_pfree(ngx_cycle->pool, ptr->path.data);
    ngx_pfree(ngx_cycle->pool, ptr->code.data);
    ngx_pfree(ngx_cycle->pool, ptr);
}

size_t ngx_lua_code_cache_node_hash(void* data)
{
    code_cache_node_t* ptr = (code_cache_node_t*)data;
    return ngx_lua_code_cache_core_hash(ptr->path);
}

int ngx_lua_code_cache_node_compare(void* data1, void* data2)
{
    code_cache_node_t* ptr1 = (code_cache_node_t*)data1;
    code_cache_node_t* ptr2 = (code_cache_node_t*)data2;

    return strcmp((const char*)ptr1->path.data, (const char*)ptr2->path.data) == 0;
}

code_cache_node_t* ngx_lua_code_cache_key_exists(ngx_lua_hash_table_t* t, ngx_str_t key)
{
    size_t idx = ngx_lua_code_cache_core_hash(key) % t->buckets_count;
    struct ngx_lua_hash_table_bucket_node_t* current = t->buckets[idx];

    while (current)
    {
        if (strcmp((const char*)((code_cache_node_t*)current->data)->path.data, (const char*)key.data) == 0) return (code_cache_node_t*)current->data;
        current = current->next;
    }
    return NULL;
}

ngx_str_t ngx_lua_code_cache_load(ngx_str_t path)
{
    FILE* fp;
    long size;
    size_t readen = 0;
    u_char *tmp;
    ngx_str_t code;

    ngx_str_null(&code);

    fp = fopen((const char*)path.data, "rb");
    if (fp == NULL) // TODO: error log
    {
        return code;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    tmp = code.data = ngx_palloc(ngx_cycle->pool, size);
    if (code.data == NULL) // TODO: error log
    {
        return code;
    }
    code.len = size;

    while (size)
    {
        readen = fread(tmp, sizeof(u_char), size, fp);
        if (readen == 0) // TODO: error log
        {
            code.len = 0;
            ngx_pfree(ngx_cycle->pool, code.data);
            code.data = NULL;
            fclose(fp);
            return code;
        }
        else
        {
            tmp += readen;
            size -= readen;
        }
    }

    fclose(fp);
    return code;
}

