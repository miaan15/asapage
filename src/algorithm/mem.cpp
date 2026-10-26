module;

#include <cstdio>
#include <cstdlib>
#include <cstring>

export module mem;

import define;

export namespace asapage {

constexpr usize MEM_MAX_ALIGN = 16;

void mem_copy(void *__restrict dest, const void *__restrict src, usize size) {
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

void mem_swap(void *__restrict a, void *__restrict b, usize size) {
    u8 *A = (u8*)a;
    u8 *B = (u8*)b;

    while (size >= sizeof(u64)) {
        u64 temp;
        memcpy(&temp, A, sizeof(u64));
        memcpy(A, B, sizeof(u64));
        memcpy(B, &temp, sizeof(u64));
        A += sizeof(u64);
        B += sizeof(u64);
        size -= sizeof(u64);
    }

    while (size > 0) {
        u8 tmp = *A;
        *A = *B;
        *B = tmp;
        A++;
        B++;
        size--;
    }
}

i32 mem_most_significant_bit_32(u32 x) {
#if defined(__GNUC__) && (__GNUC__ >= 4 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))
    if (x == 0) {
        return -1;
    }
    return 31 - __builtin_clz(x);
#elif defined(__WATCOMC__) && defined(__386__)
    if (x == 0) {
        return -1;
    }
    return _SDL_bsr_watcom(x);
#elif defined(_MSC_VER) && _MSC_VER >= 1400
    u32 index;
    if (_BitScanReverse(&index, x)) {
        return (i32)index;
    }
    return -1;
#else
    const u32 b[] = {0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000};
    const i32 S[] = {1, 2, 4, 8, 16};

    i32 msb_idx = 0;
    i32 i;

    if (x == 0) {
        return -1;
    }

    for (i = 4; i >= 0; i--) {
        if (x & b[i]) {
            x >>= S[i];
            msb_idx |= S[i];
        }
    }

    return msb_idx;
#endif
}

// Arena
struct Arena {
    void *buf;
    usize cap;
    usize buf_head_offset;
};

void arena_init(Arena *a, usize cap) {
    a->buf = malloc(cap);
    a->cap = cap;
    a->buf_head_offset = 0;
}

void arena_destroy(Arena *a) {
    free(a->buf);
    a->buf = nullptr;
    a->cap = a->buf_head_offset = 0;
}

void *arena_alloc(Arena *a, usize size, usize align = MEM_MAX_ALIGN) {
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

void arena_reset(Arena *a) {
    a->buf_head_offset = 0;
}

void arena_init_over(Arena *a, void *base_ptr, usize cap) {
    a->buf = base_ptr;
    a->cap = cap;
    a->buf_head_offset = 0;
}

void arena_init_over_arena(Arena *a, Arena *base_a, usize cap) {
    usize offset = mem_align_up(base_a->buf_head_offset, MEM_MAX_ALIGN);
    if (offset + cap > base_a->cap) {
        printf("ERROR NO MORE SPACE ARENA");
        return;
    }

    base_a->buf_head_offset = offset + cap;

    void *base_ptr = (char*)base_a->buf + offset;

    a->buf = base_ptr;
    a->cap = cap;
    a->buf_head_offset = 0;
}

}
