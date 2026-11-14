module;

#include <climits>

export module pool;

import define;
import mem;
import log;

export namespace asapage {

struct PoolKey {
    i32 idx;
    i8 validate;
};

template <typename T> struct Pool {
    T *data_buf;
    i32 *flag_buf; // flag: if >= 0 -> instance is dead and 'flag' is 'next in freelist'; if < 0 -> instance is alive
    i8 *validate_buf;

    i32 cap;
    i32 len;
    i32 freelist_head;
    i32 max_idx;

    i32 offset_of_idx_field;
    i32 offset_of_validate_field;
};

template <typename T> void pool_init(Pool<T> *pool, Arena *arena, i32 cap,
                                     i32 offset_of_idx_field = -1, i32 offset_of_validate_field = -1) {
    cap = cap <= 0 ? 4 : cap;
    pool->data_buf = (T*)arena_alloc(arena, cap * sizeof(T));
    pool->flag_buf = (i32*)arena_alloc(arena, cap * sizeof(i32));
    pool->validate_buf = (i8*)arena_alloc(arena, cap * sizeof(i8));
    pool->cap = cap;
    pool->len = 0;
    pool->freelist_head = pool->max_idx = 1; // stub
    pool->offset_of_idx_field = offset_of_idx_field;
    pool->offset_of_validate_field = offset_of_validate_field;
}

constexpr PoolKey POOL_STUB_KEY = PoolKey{0};
template <typename T> constexpr T *POOL_STUB(Pool<T> *pool) { return &pool->data_buf[0]; }

template <typename T> PoolKey pool_create(Pool<T> *pool, T *data = nullptr, T **out_instance = nullptr) {
    if (pool->len + 1 >= pool->cap) {
        log_err("pool_create: create too much -> stub: len = [%d]", pool->len);
        if (out_instance != nullptr) { *out_instance = POOL_STUB(pool); }
        return POOL_STUB_KEY;
    }

    i32 target_idx = pool->freelist_head;
    if (target_idx == pool->max_idx) {
        pool->max_idx++;
        pool->freelist_head++;
    } else {
        pool->freelist_head = pool->flag_buf[target_idx];
    }

    pool->flag_buf[target_idx] = -1;
    pool->validate_buf[target_idx] = (pool->validate_buf[target_idx] + 1) % INT_MAX;
    pool->len++;

    T *data_ptr = &pool->data_buf[target_idx];
    if (data != nullptr) {
        mem_copy(data_ptr, data, sizeof(T));
    } else {
        mem_zero(data_ptr, sizeof(T));
    }

    if (pool->offset_of_idx_field >= 0) {
        mem_copy((char*)data_ptr + pool->offset_of_idx_field, &target_idx, sizeof(i32));
    }
    if (pool->offset_of_validate_field >= 0) {
        mem_copy((char*)data_ptr + pool->offset_of_validate_field, &pool->validate_buf[target_idx], sizeof(i32));
    }

    if (out_instance != nullptr) { *out_instance = data_ptr; }

    return PoolKey{target_idx, pool->validate_buf[target_idx]};
}

template <typename T> T *pool_get(Pool<T> *pool, PoolKey key) {
    if (key.idx <= 0 || key.idx >= pool->max_idx) {
        log_err("pool_get(): try to get with key has out of bounds idx -> stub: key = [%d:%d]", key.idx, key.validate);
        return POOL_STUB(pool);
    }
    if (pool->flag_buf[key.idx] >= 0) {
        log_err("pool_get(): try to get dead instance -> stub: key = [%d:%d]", key.idx, key.validate);
        return POOL_STUB(pool);
    }
    if (key.validate != pool->validate_buf[key.idx]) {
        log_err("pool_get(): try to get with key not matching 'validate' -> stub: key = [%d:%d]", key.idx, key.validate);
        return POOL_STUB(pool);
    }

    return &pool->data_buf[key.idx];
}

template <typename T> bool pool_is_alive(Pool<T> *pool, PoolKey key) {
    if (key.idx <= 0 || key.idx >= pool->max_idx) { return false; }
    if (pool->flag_buf[key.idx] >= 0) { return false; }
    if (key.validate != pool->validate_buf[key.idx]) { return false; }
    return true;
}

template <typename T> bool pool_destroy(Pool<T> *pool, PoolKey key) {
    if (!pool_is_alive(pool, key)) { return false; }

    pool->flag_buf[key.idx] = pool->freelist_head;
    pool->freelist_head = key.idx;
    pool->len--;

    mem_zero(&pool->data_buf[key.idx], sizeof(T));

    return true;
}

template <typename T> bool pool_iterate(Pool<T> *pool, i32 *iter_idx, T **r_instance, i8 *r_validate, bool is_include_dead = false) {
    if (*iter_idx <= 0) { *iter_idx = 1; }

    i32 idx = *iter_idx;
    T *instance = POOL_STUB(pool);
    bool valid = false;

    if (!is_include_dead) {
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
