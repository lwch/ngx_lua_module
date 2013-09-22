#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "hash_table.h"

ngx_lua_hash_table_t* ngx_lua_core_hash_table_new(size_t buckets_count, size_t max_length, ngx_lua_hash_table_functor functor)
{
    ngx_lua_hash_table_t* t = ngx_palloc(ngx_cycle->pool, sizeof(ngx_lua_hash_table_t));
    if (t == NULL) return NULL;

    t->buckets = ngx_palloc(ngx_cycle->pool, sizeof(struct ngx_lua_hash_table_bucket_node_t*) * buckets_count);
    if (t->buckets == NULL)
    {
        ngx_pfree(ngx_cycle->pool, t);
        return NULL;
    }
    t->buckets_capacity = ngx_palloc(ngx_cycle->pool, sizeof(size_t) * buckets_count);
    if (t->buckets_capacity == NULL)
    {
        ngx_pfree(ngx_cycle->pool, t->buckets);
        ngx_pfree(ngx_cycle->pool, t);
        return NULL;
    }

    t->buckets_count = buckets_count;
    t->buckets_max_length = max_length;
    t->length = 0;
    t->hash = functor.hash;
    t->free = functor.free;
    t->compare = functor.compare;
    memset(t->buckets, 0, sizeof(struct ngx_lua_hash_table_bucket_node_t*) * buckets_count);
    memset(t->buckets_capacity, 0, sizeof(size_t) * buckets_count);

    return t;
}

void ngx_lua_core_hash_table_free(ngx_lua_hash_table_t* t)
{
    size_t i;
    if (t == NULL) return;

    for (i = 0; i < t->buckets_count; ++i)
    {
        struct ngx_lua_hash_table_bucket_node_t* current = t->buckets[i];
        struct ngx_lua_hash_table_bucket_node_t* next = NULL;
        while (current)
        {
            next = current->next;
            t->free(current->data);
            ngx_pfree(ngx_cycle->pool, current);
            current = next;
        }
    }

    ngx_pfree(ngx_cycle->pool, t->buckets);
    ngx_pfree(ngx_cycle->pool, t->buckets_capacity);
    ngx_pfree(ngx_cycle->pool, t);
}

int ngx_lua_core_hash_table_insert(ngx_lua_hash_table_t* t, void* data)
{
    size_t idx = t->hash(data) % t->buckets_count;
    struct ngx_lua_hash_table_bucket_node_t* current = t->buckets[idx];
    while (current)
    {
        if (t->compare(current->data, data)) // already exists
        {
            t->free(current->data);
            current->data = data;
            return 2;
        }
    }
    if (t->buckets_capacity[idx] < t->buckets_max_length)
    {
        current = ngx_palloc(ngx_cycle->pool, sizeof(struct ngx_lua_hash_table_bucket_node_t));
        if (current == NULL) return 0;
        current->data = data;
        current->next = t->buckets[idx];
        t->buckets[idx] = current;
        ++t->buckets_capacity[idx];
        ++t->length;
    }
    else
    {
        size_t new_buckets_count = t->buckets_count << 1;
        struct ngx_lua_hash_table_bucket_node_t** new_buckets;
        struct ngx_lua_hash_table_bucket_node_t* new_node;
        size_t* new_buckets_capacity;
        size_t i;

        // alloc memory
        new_buckets = ngx_palloc(ngx_cycle->pool, sizeof(struct ngx_lua_hash_table_bucket_node_t*) * new_buckets_count);
        if (new_buckets == NULL) return 0;
        memset(new_buckets, 0, sizeof(struct ngx_lua_hash_table_bucket_node_t*) * new_buckets_count);

        new_buckets_capacity = ngx_palloc(ngx_cycle->pool, sizeof(size_t) * new_buckets_count);
        if (new_buckets_capacity == NULL)
        {
            ngx_pfree(ngx_cycle->pool, new_buckets);
            return 0;
        }
        memset(new_buckets_capacity, 0, sizeof(size_t) * new_buckets_count);

        new_node = ngx_palloc(ngx_cycle->pool, sizeof(struct ngx_lua_hash_table_bucket_node_t));
        if (new_node == NULL)
        {
            ngx_pfree(ngx_cycle->pool, new_buckets_capacity);
            ngx_pfree(ngx_cycle->pool, new_buckets);
            return 0;
        }

        // move to new buckets
        for (i = 0; i < t->buckets_count; ++i)
        {
            struct ngx_lua_hash_table_bucket_node_t* current = t->buckets[i];
            struct ngx_lua_hash_table_bucket_node_t* next;
            while (current)
            {
                idx = t->hash(current->data) % new_buckets_count;
                next = current->next;
                current->next = new_buckets[idx];
                new_buckets[idx] = current;
                ++new_buckets_capacity[idx];
                current = next;
            }
        }

        // last
        ngx_pfree(ngx_cycle->pool, t->buckets);
        ngx_pfree(ngx_cycle->pool, t->buckets_capacity);
        idx = t->hash(data) % new_buckets_count;
        new_node->data = data;
        new_node->next = new_buckets[idx];
        new_buckets[idx] = new_node;
        ++new_buckets_capacity[idx];
        t->buckets_count = new_buckets_count;
        t->buckets = new_buckets;
        t->buckets_capacity = new_buckets_capacity;
        ++t->length;
    }
    return 1;
}

int ngx_lua_core_hash_table_insert_notfind(ngx_lua_hash_table_t* t, void* data)
{
    size_t idx = t->hash(data) % t->buckets_count;
    struct ngx_lua_hash_table_bucket_node_t* current = t->buckets[idx];

    if (t->buckets_capacity[idx] < t->buckets_max_length)
    {
        current = ngx_palloc(ngx_cycle->pool, sizeof(struct ngx_lua_hash_table_bucket_node_t));
        if (current == NULL) return 0;
        current->data = data;
        current->next = t->buckets[idx];
        t->buckets[idx] = current;
        ++t->buckets_capacity[idx];
        ++t->length;
    }
    else
    {
        size_t new_buckets_count = t->buckets_count << 1;
        struct ngx_lua_hash_table_bucket_node_t** new_buckets;
        struct ngx_lua_hash_table_bucket_node_t* new_node;
        size_t* new_buckets_capacity;
        size_t i;

        // alloc memory
        new_buckets = ngx_palloc(ngx_cycle->pool, sizeof(struct ngx_lua_hash_table_bucket_node_t*) * new_buckets_count);
        if (new_buckets == NULL) return 0;
        memset(new_buckets, 0, sizeof(struct ngx_lua_hash_table_bucket_node_t*) * new_buckets_count);

        new_buckets_capacity = ngx_palloc(ngx_cycle->pool, sizeof(size_t) * new_buckets_count);
        if (new_buckets_capacity == NULL)
        {
            ngx_pfree(ngx_cycle->pool, new_buckets);
            return 0;
        }
        memset(new_buckets_capacity, 0, sizeof(size_t) * new_buckets_count);

        new_node = ngx_palloc(ngx_cycle->pool, sizeof(struct ngx_lua_hash_table_bucket_node_t));
        if (new_node == NULL)
        {
            ngx_pfree(ngx_cycle->pool, new_buckets_capacity);
            ngx_pfree(ngx_cycle->pool, new_buckets);
            return 0;
        }

        // move to new buckets
        for (i = 0; i < t->buckets_count; ++i)
        {
            struct ngx_lua_hash_table_bucket_node_t* current = t->buckets[i];
            struct ngx_lua_hash_table_bucket_node_t* next;
            while (current)
            {
                idx = t->hash(current->data) % new_buckets_count;
                next = current->next;
                current->next = new_buckets[idx];
                new_buckets[idx] = current;
                ++new_buckets_capacity[idx];
                current = next;
            }
        }

        // last
        ngx_pfree(ngx_cycle->pool, t->buckets);
        ngx_pfree(ngx_cycle->pool, t->buckets_capacity);
        idx = t->hash(data) % new_buckets_count;
        new_node->data = data;
        new_node->next = new_buckets[idx];
        new_buckets[idx] = new_node;
        ++new_buckets_capacity[idx];
        t->buckets_count = new_buckets_count;
        t->buckets = new_buckets;
        t->buckets_capacity = new_buckets_capacity;
        ++t->length;
    }
    return 1;
}

void* ngx_lua_core_hash_table_lookup(ngx_lua_hash_table_t* t, void* data)
{
    size_t idx = t->hash(data) % t->buckets_count;
    struct ngx_lua_hash_table_bucket_node_t* current = t->buckets[idx];

    while (current)
    {
        if (t->compare(current->data, data)) return current->data;
        current = current->next;
    }
    return NULL;
}

