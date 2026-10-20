module;

#include <cstdio>

export module map;

import define;
import mem;

export namespace asapage {

struct MapMeta {
    u64 key;
    i32 probe_num : 31;
    i32 is_alive : 1;
};

template <typename T> struct Map {
    T *data_buf;
    MapMeta *meta_buf;
    i32 cap;
    i32 len;

    i32 max_probe_num;

    i32 offset_of_key_field;
};

template <typename T> void map_init(Map<T> *map, Arena *arena, i32 cap, i32 offset_of_key_field = -1) {
    cap = cap <= 0 ? 4 : cap;
    map->data_buf = (T*)arena_alloc(arena, cap * sizeof(T));
    map->meta_buf = (MapMeta*)arena_alloc(arena, cap * sizeof(MapMeta));
    map->cap = cap;
    map->len = 0;
    map->max_probe_num = 0;
    map->offset_of_key_field = offset_of_key_field;
}

template <typename T> T *map_stub(Map<T> *map) {
    return &map->data_buf[0];
}

template <typename T> i32 map_key_idx(Map<T> *map, u64 key) {
    return (key * (u64)0x9e3779b9) % (map->cap - 1) + 1;
}

template <typename T> i32 map_calc_probe_num(Map<T> *map, i32 from_idx, i32 to_idx) {
    return to_idx >= from_idx ? to_idx - from_idx :
                                map->cap - from_idx + to_idx - 1;
}

template <typename T> T *map_create(Map<T> *map, u64 key, T *instance_data = nullptr) {
    if (map->len >= map->cap) {
        printf("MAP TOO MUCHCHDSA");
        return map_stub(map);
    }

    u64 cur_key = key;
    T cur_instance_data = {};
    if (instance_data != nullptr) { cur_instance_data = *instance_data; }

    i32 idx = map_key_idx(map, cur_key);
    while (true) {
        MapMeta meta = map->meta_buf[idx];

        i32 cur_probe_num = map_calc_probe_num(map, map_key_idx(map, cur_key), idx);

        if (!meta.is_alive) {
            T *res = &map->data_buf[idx];
            MapMeta *res_meta = &map->meta_buf[idx];

            mem_copy(res, &cur_instance_data, sizeof(T));

            if (map->offset_of_key_field >= 0) {
                mem_copy((char*)res + map->offset_of_key_field, &meta.key, sizeof(u64));
            }

            res_meta->key = cur_key;
            res_meta->is_alive = true;
            res_meta->probe_num = cur_probe_num;
            map->max_probe_num = cur_probe_num > map->max_probe_num ? cur_probe_num : map->max_probe_num;

            map->len++;

            return res;
        }

        if (meta.key == cur_key) { // if the key already existed
            T *res = &map->data_buf[idx];
            MapMeta *res_meta = &map->meta_buf[idx];

            if (instance_data != nullptr) { mem_copy(res, &cur_instance_data, sizeof(T)); }

            if (map->offset_of_key_field >= 0) {
                mem_copy((char*)res + map->offset_of_key_field, &cur_key, sizeof(u64));
            }

            return res;
        }

        if (meta.probe_num < cur_probe_num) {
            T temp_instance = map->data_buf[idx];
            map->data_buf[idx] = cur_instance_data;
            cur_instance_data = temp_instance;

            MapMeta temp_meta = map->meta_buf[idx];
            map->meta_buf[idx] = (MapMeta) { .key = cur_key, .probe_num = cur_probe_num, .is_alive = true };
            cur_key = temp_meta.key;

            map->max_probe_num = cur_probe_num > map->max_probe_num ? cur_probe_num : map->max_probe_num;
        }

        idx++;
        if (idx >= map->cap) { idx = 1; }
    }
}

template <typename T> void map_destroy_as_idx(Map<T> *map, i32 idx) {
    if (idx < 1 || idx >= map->cap) { return; }
    if (!map->meta_buf[idx].is_alive) { return; }

    while (true) {
        i32 next_idx = idx + 1;
        if (next_idx >= map->cap) { next_idx = 1; }

        if (map->meta_buf[next_idx].probe_num == 0) {
            mem_zero(&map->data_buf[idx], sizeof(T));
            mem_zero(&map->meta_buf[idx], sizeof(MapMeta));
            map->len--;
            return;
        }

        map->data_buf[idx] = map->data_buf[next_idx];
        map->meta_buf[idx] = map->meta_buf[next_idx];
        map->meta_buf[idx].probe_num--;

        idx = next_idx;
    }
}

template <typename T> bool map_destroy(Map<T> *map, u64 key) {
    i32 idx = map_key_idx(map, key);
    i32 cur_probe_num = 0;
    while (true) {
        MapMeta meta = map->meta_buf[idx];

        if (!meta.is_alive || meta.probe_num < cur_probe_num) { return false; }

        if (meta.key == key) {
            map_destroy_as_idx(map, idx);
            return true;
        }

        cur_probe_num++;

        idx++;
        if (idx >= map->cap) { idx = 1; }
    }
}

template <typename T> void map_destroy_as_value(Map<T> *map, T *value) {
    map_destroy_as_idx(map, value - map->data_buf);
}

template <typename T> T *map_get(Map<T> *map, u64 key) {
    i32 idx = map_key_idx(map, key);
    i32 cur_probe_num = 0;
    while (true) {
        MapMeta meta = map->meta_buf[idx];

        if (!meta.is_alive || meta.probe_num < cur_probe_num) { return map_stub(map); }

        if (meta.key == key) { return &map->data_buf[idx]; }

        cur_probe_num++;

        idx++;
        if (idx >= map->cap) { idx = 1; }
    }
}

template <typename T> bool map_try_get(Map<T> *map, u64 key, T **r_instance) {
    i32 idx = map_key_idx(map, key);
    i32 cur_probe_num = 0;
    while (true) {
        MapMeta meta = map->meta_buf[idx];

        if (!meta.is_alive || meta.probe_num < cur_probe_num) {
            return false;
        }

        if (meta.key == key) {
            if (r_instance != nullptr) { *r_instance = &map->data_buf[idx]; }
            return true;
        }

        cur_probe_num++;

        idx++;
        if (idx >= map->cap) { idx = 1; }
    }
}

template <typename T> bool map_iterate(Map<T> *map, i32 *iter_idx, T **r_instance, u64 *r_key) {
    if (*iter_idx <= 0) { *iter_idx = 1; }

    i32 idx = *iter_idx;
    T *instance = map_stub(map);
    u64 key = 0;
    bool valid = false;

    while (idx < map->cap && !map->meta_buf[idx].is_alive) { idx++; }

    valid = idx < map->cap;

    if (valid) {
        instance = &map->data_buf[idx];
        key = map->meta_buf[idx].key;
        idx++;
    }

    *iter_idx = idx;
    if (r_instance != nullptr) { *r_instance = instance; }
    if (r_key != nullptr) { *r_key = key; }

    return valid;
}

}
