#ifndef _HASH_TABLE_H_
#define _HASH_TABLE_H_

#include <stdlib.h>

enum
{
    default_buckets_count = 11,
    default_buckets_max_length = 100
};

struct ngx_lua_hash_table_bucket_node_t
{
    void* data;
    struct ngx_lua_hash_table_bucket_node_t* next;
};

typedef struct
{
    struct ngx_lua_hash_table_bucket_node_t** buckets;
    size_t  buckets_count;            // buckets count
    size_t* buckets_capacity;         // buckets capacity
    size_t  buckets_max_length;       // max buckets length
    size_t  length;                   // elements count

    size_t (*hash)(void*);            // hash function
    void (*free)(void*); // free handler
    int (*compare)(void*, void*);     // compare function
} ngx_lua_hash_table_t;

typedef struct
{
    size_t (*hash)(void*);            // hash function
    void (*free)(void*); // free handler
    int (*compare)(void*, void*);     // compare function
} ngx_lua_hash_table_functor;

extern ngx_lua_hash_table_t* ngx_lua_core_hash_table_new(size_t buckets_count, size_t max_length, ngx_lua_hash_table_functor functor);
extern void ngx_lua_core_hash_table_free(ngx_lua_hash_table_t* t);
extern int ngx_lua_core_hash_table_insert(ngx_lua_hash_table_t* t, void* data);
extern int ngx_lua_core_hash_table_insert_notfind(ngx_lua_hash_table_t* t, void* data);

// lookup by key of data
extern void* ngx_lua_core_hash_table_lookup(ngx_lua_hash_table_t* t, void* data);

#endif

