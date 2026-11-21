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

    T stub;
};

template <typename T> void list_init(List<T> *list, Arena *arena, i32 cap, i32 offset_of_idx_field = -1) {
    cap = cap <= 0 ? 4 : cap;
    list->buf = (T*)arena_alloc(arena, cap * sizeof(T));
    list->cap = cap;
    list->len = 0;
    list->offset_of_idx_field = offset_of_idx_field;
    list->stub = {};
}

template <typename T> T *LIST_STUB(List<T> *list) { return &list->stub; }
template <typename T> T *LIST_BUF(List<T> *list) { return &list->buf[0]; }

template <typename T> i32 list_create(List<T> *list, T *data = nullptr, T **out_instance = nullptr) {
    if (list->len >= list->cap) {
        log_err("list_create: create too much -> 0: len = [%d]", list->len);
        if (out_instance != nullptr) { *out_instance = LIST_STUB(list); }
        return 0;
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

template <typename T> void list_destroy(List<T> *list, i32 idx) {
    if (idx < 0 || idx >= list->len) {
        log_err("list_destroy(): try to destroy invalid idx: idx = [%d]", idx);
        return;
    }

    i32 last_idx = list->len - 1;
    if (idx != last_idx) {
        T *dst = &list->buf[idx];
        T *src = &list->buf[last_idx];
        mem_copy(dst, src, sizeof(T));

        if (list->offset_of_idx_field >= 0) {
            mem_copy((char*)dst + list->offset_of_idx_field, &idx, sizeof(i32));
        }
    }

    list->len--;
}

template <typename T> T *list_get(List<T> *list, i32 idx) {
    if (idx < 0 || idx >= list->len) {
        log_err("list_get(): try to get out of bounds idx -> stub: idx = [%d]", idx);
        return LIST_STUB(list);
    }
    return &list->buf[idx];
}

template <typename T> void list_reset(List<T> *list) {
    list->len = 0;
}

template <typename T> bool list_iterate(List<T> *list, i32 *iter_idx, T **r_instance, i32 *r_idx = nullptr) {
    if (*iter_idx < 0) { *iter_idx = 0; }

    i32 idx = *iter_idx;
    T *instance = LIST_STUB(list);
    bool valid = idx < list->len;

    if (valid) {
        instance = &list->buf[idx];
        idx++;
    }

    *iter_idx = idx;
    if (r_instance != nullptr) { *r_instance = instance; }
    if (r_idx != nullptr) { *r_idx = idx - 1; }

    return valid;
}

}
