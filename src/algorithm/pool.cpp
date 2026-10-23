module;

#include <climits>
#include <cstdio>

export module pool;

import define;
import mem;

export namespace asapage {

template <typename T> struct Pool {
    T *data_buf;
    i32 *flag_buf;
    i32 *validation_buf;

    i32 cap;
    i32 len;
    i32 freelist_head;
    i32 max_idx;

    i32 offset_of_idx_field;
    i32 offset_of_validation_field;
};

template <typename T> void pool_init(Pool<T> *pool, Arena *arena, i32 cap, i32 offset_of_idx_field = -1, i32 offset_of_validation_field = -1) {
    cap = cap <= 0 ? 4 : cap;
    pool->data_buf = (T*)arena_alloc(arena, cap * sizeof(T));
    pool->flag_buf = (i32*)arena_alloc(arena, cap * sizeof(i32));
    pool->validation_buf = (i32*)arena_alloc(arena, cap * sizeof(i32));
    pool->cap = cap;
    pool->len = 0;
    pool->freelist_head = pool->max_idx = 1; // stub
    pool->offset_of_idx_field = offset_of_idx_field;
    pool->offset_of_validation_field = offset_of_validation_field;
}

template <typename T> T *pool_stub(Pool<T> *pool) { return &pool->data_buf[0]; }

template <typename T> T *pool_create(Pool<T> *pool, T *instance_data = nullptr, i32 *r_validation = nullptr) {
    if (pool->len + 1 >= pool->cap) {
        printf("POOL TOO MUCH");
        return pool_stub(pool);
    }

    i32 target_idx = pool->freelist_head;
    if (target_idx == pool->max_idx) {
        pool->max_idx++;
        pool->freelist_head++;
    } else {
        pool->freelist_head = pool->flag_buf[target_idx];
    }

    pool->flag_buf[target_idx] = -1;
    pool->validation_buf[target_idx] = (pool->validation_buf[target_idx] + 1) % INT_MAX;
    pool->len++;

    void *data_ptr = &pool->data_buf[target_idx];
    if (instance_data != nullptr) {
        mem_copy(data_ptr, instance_data, sizeof(T));
    } else {
        mem_zero(data_ptr, sizeof(T));
    }

    if (pool->offset_of_idx_field >= 0) {
        mem_copy((char*)data_ptr + pool->offset_of_idx_field, &target_idx, sizeof(i32));
    }
    if (pool->offset_of_validation_field >= 0) {
        mem_copy((char*)data_ptr + pool->offset_of_validation_field, &pool->validation_buf[target_idx], sizeof(i32));
    }

    if (r_validation != nullptr) { *r_validation = pool->validation_buf[target_idx]; }

    return (T*)data_ptr;
}

template <typename T> bool pool_is_alive(Pool<T> *pool, T *instance, i32 validation = -1) {
    if (instance < &pool->data_buf[1] || instance >= &pool->data_buf[pool->max_idx]) { return false; }

    i32 idx = instance - pool->data_buf;
    if (pool->flag_buf[idx] >= 0) { return false; }
    if (validation >= 0 && validation != pool->validation_buf[idx]) { return false; }

    return true;
}

template <typename T> bool pool_destroy(Pool<T> *pool, T *instance, i32 validation = -1) {
    if (!pool_is_alive(pool, instance, validation)) { return false; }

    i32 idx = instance - pool->data_buf;
    pool->flag_buf[idx] = pool->freelist_head;
    pool->freelist_head = idx;
    pool->len--;

    mem_zero(instance, sizeof(T));

    return true;
}

template <typename T> bool pool_iterate(Pool<T> *pool, i32 *iter_idx, T **r_instance, bool include_dead = false) {
    if (*iter_idx <= 0) { *iter_idx = 1; }

    i32 idx = *iter_idx;
    T *instance = pool_stub(pool);
    bool valid = false;

    if (!include_dead) {
        while (idx < pool->max_idx && pool->flag_buf[idx] >= 0) { idx++; }
    }

    valid = idx < pool->max_idx;

    if (valid) {
        instance = &pool->data_buf[idx];
        idx++;
    }

    *iter_idx = idx;
    if (r_instance != nullptr) { *r_instance = instance; }

    return valid;
}

}
