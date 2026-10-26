module;

#include <cassert>
#include <cstdio>
#include <cstring>

export module mapstr;

import define;
import mem;
import pool;
import log;

export namespace asapage {

constexpr usize MAX_MAPSTR_STRING_SIZE = 256;

struct MapStrMeta {
    const char *key;
    i32 probe_num : 31;
    i32 is_alive : 1;
};

struct MapStrStringBuf {
    char buf[MAX_MAPSTR_STRING_SIZE];
};

template <typename T> struct MapStr {
    T *data_buf;
    MapStrMeta *meta_buf;
    i32 cap;
    i32 len;

    i32 max_probe_num;

    Pool<MapStrStringBuf> string_buf_pool;

    i32 offset_of_key_field;
};

template <typename T> void mapstr_init(MapStr<T> *mapstr, Arena *arena, i32 cap, i32 offset_of_key_field = -1) {
    cap = cap <= 0 ? 4 : cap;
    mapstr->data_buf = (T*)arena_alloc(arena, cap * sizeof(T));
    mapstr->meta_buf = (MapStrMeta*)arena_alloc(arena, cap * sizeof(MapStrMeta));
    mapstr->cap = cap;
    mapstr->len = 0;
    mapstr->max_probe_num = 0;
    pool_init(&mapstr->string_buf_pool, arena, cap);
    mapstr->offset_of_key_field = offset_of_key_field;
}

template <typename T> T *mapstr_stub(MapStr<T> *mapstr) {
    return &mapstr->data_buf[0];
}

template <typename T> i32 mapstr_key_idx(MapStr<T> *mapstr, const char *key) {
    u32 hash = 0x811c9dc5;
    while (*key) {
        hash *= 0x01000193;
        hash ^= (u8)*key;
        key++;
    }
    return hash % (mapstr->cap - 1) + 1;
}

bool mapstr_match_key(const char *a, const char *b) {
    assert(a != nullptr && b != nullptr);
    return strcmp(a, b) == 0;
}

template <typename T> i32 mapstr_calc_probe_num(MapStr<T> *mapstr, i32 from_idx, i32 to_idx) {
    return to_idx >= from_idx ? to_idx - from_idx :
                                mapstr->cap - from_idx + to_idx - 1;
}

template <typename T> T *mapstr_create(MapStr<T> *mapstr, const char *key, T *instance_data = nullptr) {
    if (mapstr->len >= mapstr->cap) {
        log_debug("mapstr_create: create too much -> stub");
        return mapstr_stub(mapstr);
    }

    const char *cur_key = (const char*)pool_create(&mapstr->string_buf_pool, (MapStrStringBuf*)key);
    T cur_instance_data = {};
    if (instance_data != nullptr) { cur_instance_data = *instance_data; }

    i32 idx = mapstr_key_idx(mapstr, cur_key);
    while (true) {
        MapStrMeta meta = mapstr->meta_buf[idx];

        i32 cur_probe_num = mapstr_calc_probe_num(mapstr, mapstr_key_idx(mapstr, cur_key), idx);

        if (!meta.is_alive) {
            T *res = &mapstr->data_buf[idx];
            MapStrMeta *res_meta = &mapstr->meta_buf[idx];

            mem_copy(res, &cur_instance_data, sizeof(T));

            if (mapstr->offset_of_key_field >= 0) {
                mem_copy((char*)res + mapstr->offset_of_key_field, &cur_key, sizeof(u64));
            }

            res_meta->key = cur_key;
            res_meta->is_alive = true;
            res_meta->probe_num = cur_probe_num;
            mapstr->max_probe_num = cur_probe_num > mapstr->max_probe_num ? cur_probe_num : mapstr->max_probe_num;

            mapstr->len++;

            return res;
        }

        if (mapstr_match_key(meta.key, cur_key)) { // if the key already existed
            T *res = &mapstr->data_buf[idx];
            MapStrMeta *res_meta = &mapstr->meta_buf[idx];

            if (instance_data != nullptr) { mem_copy(res, &cur_instance_data, sizeof(T)); }

            if (mapstr->offset_of_key_field >= 0) {
                mem_copy((char*)res + mapstr->offset_of_key_field, &meta.key, sizeof(u64));
            }

            pool_destroy(&mapstr->string_buf_pool, (MapStrStringBuf*)cur_key);

            return res;
        }

        if (meta.probe_num < cur_probe_num) {
            T temp_instance = mapstr->data_buf[idx];
            mapstr->data_buf[idx] = cur_instance_data;
            cur_instance_data = temp_instance;

            MapStrMeta temp_meta = mapstr->meta_buf[idx];
            mapstr->meta_buf[idx] = (MapStrMeta) { .key = cur_key, .probe_num = cur_probe_num, .is_alive = true };
            cur_key = temp_meta.key;

            mapstr->max_probe_num = cur_probe_num > mapstr->max_probe_num ? cur_probe_num : mapstr->max_probe_num;
        }

        idx++;
        if (idx >= mapstr->cap) { idx = 1; }
    }
}

template <typename T> bool mapstr_destroy(MapStr<T> *mapstr, const char *key) {
    i32 idx = mapstr_key_idx(mapstr, key);
    i32 cur_probe_num = 0;
    while (true) {
        MapStrMeta meta = mapstr->meta_buf[idx];

        if (!meta.is_alive || meta.probe_num < cur_probe_num) { return false; }

        if (mapstr_match_key(meta.key, key)) {
            mapstr_destroy_as_idx(mapstr, idx);
            return true;
        }

        cur_probe_num++;

        idx++;
        if (idx >= mapstr->cap) { idx = 1; }
    }
}

template <typename T> void mapstr_destroy_as_value(MapStr<T> *mapstr, T *value) {
    mapstr_destroy_as_idx(mapstr, value - mapstr->data_buf);
}

template <typename T> void mapstr_destroy_as_idx(MapStr<T> *mapstr, i32 idx) {
    if (idx < 1 || idx >= mapstr->cap) { return; }
    if (!mapstr->meta_buf[idx].is_alive) { return; }

    pool_destroy(&mapstr->string_buf_pool, (MapStrStringBuf*)mapstr->meta_buf[idx].key);

    while (true) {
        i32 next_idx = idx + 1;
        if (next_idx >= mapstr->cap) { next_idx = 1; }

        if (mapstr->meta_buf[next_idx].probe_num == 0) {
            mem_zero(&mapstr->data_buf[idx], sizeof(T));
            mem_zero(&mapstr->meta_buf[idx], sizeof(MapStrMeta));
            mapstr->len--;
            return;
        }

        mapstr->data_buf[idx] = mapstr->data_buf[next_idx];
        mapstr->meta_buf[idx] = mapstr->meta_buf[next_idx];
        mapstr->meta_buf[idx].probe_num--;

        idx = next_idx;
    }
}

template <typename T> T *mapstr_get(MapStr<T> *mapstr, const char *key) {
    i32 idx = mapstr_key_idx(mapstr, key);
    i32 cur_probe_num = 0;
    while (true) {
        MapStrMeta meta = mapstr->meta_buf[idx];

        if (!meta.is_alive || meta.probe_num < cur_probe_num) { return mapstr_stub(mapstr); }

        if (mapstr_match_key(meta.key, key)) { return &mapstr->data_buf[idx]; }

        cur_probe_num++;

        idx++;
        if (idx >= mapstr->cap) { idx = 1; }
    }
}

template <typename T> bool mapstr_try_get(MapStr<T> *mapstr, const char *key, T **r_instance) {
    i32 idx = mapstr_key_idx(mapstr, key);
    i32 cur_probe_num = 0;
    while (true) {
        MapStrMeta meta = mapstr->meta_buf[idx];

        if (!meta.is_alive || meta.probe_num < cur_probe_num) {
            return false;
        }

        if (mapstr_match_key(meta.key, key)) {
            if (r_instance != nullptr) { *r_instance = &mapstr->data_buf[idx]; }
            return true;
        }

        cur_probe_num++;

        idx++;
        if (idx >= mapstr->cap) { idx = 1; }
    }
}

template <typename T> bool mapstr_iterate(MapStr<T> *mapstr, i32 *iter_idx, T **r_instance, const char **r_key) {
    if (*iter_idx <= 0) { *iter_idx = 1; }

    i32 idx = *iter_idx;
    T *instance = mapstr_stub(mapstr);
    const char *key = 0;
    bool valid = false;

    while (idx < mapstr->cap && !mapstr->meta_buf[idx].is_alive) { idx++; }

    valid = idx < mapstr->cap;

    if (valid) {
        instance = &mapstr->data_buf[idx];
        key = mapstr->meta_buf[idx].key;
        idx++;
    }

    *iter_idx = idx;
    if (r_instance != nullptr) { *r_instance = instance; }
    if (r_key != nullptr) { *r_key = key; }

    return valid;
}

}
