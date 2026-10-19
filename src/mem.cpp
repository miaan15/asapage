module;

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

export module mem;

import define;

export namespace asapage {

constexpr usize MEM_MAX_ALIGN = alignof(max_align_t);

void mem_copy(void *__restrict dest, void *__restrict src, usize size) {
    memcpy(dest, src, size);
}

void mem_zero(void *buf, usize size) {
    memset(buf, 0, size);
}

usize mem_align_up(usize offset, usize align) {
    usize mod = offset & (align - 1);
    if (mod != 0) {
        offset += (align - mod);
    }
    return offset;
}

// Arena
struct arena {
    void *buf;
    usize cap;
    usize buf_head_offset;
};

void arena_init(arena *a, usize cap) {
    a->buf = malloc(cap);
    a->cap = cap;
    a->buf_head_offset = 0;
}

void *arena_alloc(arena *a, usize size, usize align = MEM_MAX_ALIGN) {
    usize offset = mem_align_up(a->buf_head_offset, align);
    if (offset + size > a->cap) {
        printf("ERROR NO MORE SPACE ARENA");
        return nullptr;
    }

    a->buf_head_offset = offset + size;

    void *res = (char*)a->buf + offset;
    mem_zero(res, size);

    return res;
}

void arena_reset(arena *a) {
    a->buf_head_offset = 0;
}

void arena_init_over(arena *a, void *base_ptr, usize cap) {
    a->buf = base_ptr;
    a->cap = cap;
    a->buf_head_offset = 0;
}

void arena_init_over_arena(arena *a, arena *base_a, usize cap) {
    usize offset = mem_align_up(base_a->buf_head_offset, MEM_MAX_ALIGN);
    if (offset + cap > base_a->cap) {
        printf("ERROR NO MORE SPACE ARENA");
        return;
    }

    base_a->buf_head_offset = offset + cap;

    void *base_ptr = (char*)a->buf + offset;

    a->buf = base_ptr;
    a->cap = cap;
    a->buf_head_offset = 0;
}

}
