module;

export module list;

import define;
import mem;
import log;

export namespace asapage {

template <typename T> struct List {
    T* buf;
    i32 len;
    i32 cap;

    i32 offset_of_idx_field;
};

template <typename T> void list_init(List<T> *list, Arena *arena, i32 cap, i32 offset_of_idx_field = -1) {
    cap = cap <= 0 ? 4 : cap;
    list->buf = (T*)arena_alloc(arena, cap * sizeof(T));
    list->cap = cap;
    list->len = 1; // stub
    list->offset_of_idx_field = offset_of_idx_field;
}

template <typename T> i32 list_stub_idx(List<T> *list) { return 0; }
template <typename T> T *list_stub(List<T> *list) { return &list->buf[0]; }

template <typename T> i32 list_create(List<T> *list, T *data = nullptr, T **out_instance = nullptr) {
    if (list->len >= list->cap) {
        log_err("list_create: create too much -> stub: len = [%d]", list->len);
        if (out_instance != nullptr) { *out_instance = list_stub(list); }
        return list_stub_idx(list);
    }

    i32 idx = list->len;
    T *ptr = &list->buf[idx];
    if (data == nullptr) { mem_zero(ptr, sizeof(T)); }
    else { mem_copy(ptr, data, sizeof(T)); }

    if (list->offset_of_idx_field >= 0) {
        mem_copy((char*)ptr + list->offset_of_idx_field, &list->len, sizeof(i32));
    }

    list->len++;

    if (out_instance != nullptr) { *out_instance = ptr; }

    return idx;
}

template <typename T> T *list_get(List<T> *list, i32 idx) {
    if (idx < 0 || idx >= list->len) {
        log_err("list_get(): try to get out of bounds idx -> stub: idx = [%d]", idx);
        return list_stub(list);
    }
    return &list->buf[idx];
}

template <typename T> void list_reset(List<T> *list) {
    list->len = 0;
}

template <typename T> bool list_iterate(List<T> *list, i32 *iter_idx, T **r_instance) {
    if (*iter_idx <= 0) { *iter_idx = 1; }

    i32 idx = *iter_idx;
    T *instance = list_stub(list);
    bool valid = idx < list->len;

    if (valid) {
        instance = &list->buf[idx];
        idx++;
    }

    *iter_idx = idx;
    if (r_instance != nullptr) { *r_instance = instance; }

    return valid;
}

}
