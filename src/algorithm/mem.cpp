module;

#include <cassert>
#include <cstdlib>
#include <cstring>

export module mem;

import define;
import log;

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
    mem_zero(a->buf, cap);
    a->cap = cap;
    a->buf_head_offset = 0;
}

void arena_destroy(Arena *a) {
    free(a->buf);
    a->buf = nullptr;
    a->cap = a->buf_head_offset = 0;
}

void *arena_alloc(Arena *a, usize size, usize align = MEM_MAX_ALIGN) {
    assert(size != 0);
    assert(align != 0 && (align & (align - 1)) == 0);

    usize offset = mem_align_up(a->buf_head_offset, align);
    if (offset + size > a->cap) {
        log_err("arena_alloc: alloc too much -> null");
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

void arena_init_over_arena(Arena *a, Arena *base_a, usize cap) {
    assert(cap != 0);

    usize offset = mem_align_up(base_a->buf_head_offset, MEM_MAX_ALIGN);

    base_a->buf_head_offset = offset + cap;

    void *base_ptr = (char*)base_a->buf + offset;

    a->buf = base_ptr;
    a->cap = cap;
    a->buf_head_offset = 0;
}

// Heap (but actually Bucket Allocator)
// this is not good for everything like a normal heap, if you try to allocate, deallocate many small things and then a big thing, many memory are wasted
constexpr usize HEAP_BUCKET_SIZE_DEFAULT = 1 << 6; // 64

struct HeapBucketMeta {
    usize bucket_num;
    usize next_bucket_idx;
};
struct Heap {
    void *buf;

    usize bucket_size;
    usize bucket_num;

    HeapBucketMeta *meta_list;
    usize head_bucket_idx;
};

void heap_init(Heap *h, usize cap, usize bucket_size = HEAP_BUCKET_SIZE_DEFAULT) {
    assert(cap != 0);
    assert(bucket_size != 0 && (bucket_size & (bucket_size - 1)) == 0);

    cap = mem_align_up(cap, bucket_size);

    h->bucket_size = bucket_size;
    h->bucket_num = cap / bucket_size;

    h->buf = malloc(cap);
    h->meta_list = (HeapBucketMeta*)malloc(h->bucket_num * sizeof(HeapBucketMeta));

    h->head_bucket_idx = 0;
    mem_zero(h->meta_list, h->bucket_num * sizeof(HeapBucketMeta));
    h->meta_list[0] = { .bucket_num = h->bucket_num, .next_bucket_idx = (usize)-1 };
}

void heap_destroy(Heap *h) {
    free(h->buf);
    free(h->meta_list);
    h->buf = nullptr;
    h->meta_list = nullptr;
    h->bucket_size = h->bucket_num = h->head_bucket_idx = 0;
}

void *heap_alloc(Heap *h, usize size) {
    assert(size != 0);

    if (h->head_bucket_idx >= h->bucket_num) {
        log_err("heap_alloc: alloc too much -> null");
        return nullptr;
    }

    usize needed_size = mem_align_up(size, h->bucket_size);
    usize needed_bucket_num = needed_size / h->bucket_size;

    // iterate through bucket and watch its meta to see any fit
    HeapBucketMeta *cur_meta = &h->meta_list[h->head_bucket_idx];
    HeapBucketMeta *pre_meta = nullptr;
    while (cur_meta->next_bucket_idx != (usize)-1) {
        if (needed_bucket_num <= cur_meta->bucket_num) { break; }
        pre_meta = cur_meta;
        cur_meta = &h->meta_list[cur_meta->next_bucket_idx];
    }

    if (needed_bucket_num > cur_meta->bucket_num) {
        log_err("heap_alloc: alloc too much -> null");
        return nullptr;
    }

    usize suited_bucket_idx = pre_meta == nullptr ?
                              h->head_bucket_idx :
                              pre_meta->next_bucket_idx; // a bit of hacky here

    usize next_bucket_idx = needed_bucket_num == cur_meta->bucket_num ?
                            cur_meta->next_bucket_idx :
                            suited_bucket_idx + needed_bucket_num;
    if (pre_meta != nullptr) { pre_meta->next_bucket_idx = next_bucket_idx; }
    else { h->head_bucket_idx = next_bucket_idx; }

    if (needed_bucket_num < cur_meta->bucket_num) {
        h->meta_list[next_bucket_idx] = {
            .bucket_num = cur_meta->bucket_num - needed_bucket_num,
            .next_bucket_idx = cur_meta->next_bucket_idx
        };
    }
    h->meta_list[suited_bucket_idx].bucket_num = needed_bucket_num;

    void *res = (char*)h->buf + (suited_bucket_idx * h->bucket_size);
    mem_zero(res, needed_size);
    return res;
}

void heap_dealloc(Heap *h, void *ptr) {
    assert(ptr != nullptr);

    usize ptr_offset = (char*)ptr - (char*)h->buf;
    assert(ptr_offset < h->bucket_num * h->bucket_size && ptr_offset % h->bucket_size == 0);

    usize bucket_idx = ptr_offset / h->bucket_size;

    h->meta_list[bucket_idx].next_bucket_idx = h->head_bucket_idx;
    h->head_bucket_idx = bucket_idx;

    mem_zero(ptr, h->meta_list[bucket_idx].bucket_num * h->bucket_size);
}

void heap_reset(Heap *h) {
    h->head_bucket_idx = 0;
    mem_zero(h->meta_list, h->bucket_num * sizeof(HeapBucketMeta));
    h->meta_list[0] = { .bucket_num = h->bucket_num, .next_bucket_idx = (usize)-1 };
}

void heap_init_over_arena(Heap *h, Arena *base_a, usize cap, usize bucket_size = HEAP_BUCKET_SIZE_DEFAULT) {
    assert(cap != 0);
    assert(bucket_size != 0 && (bucket_size & (bucket_size - 1)) == 0);

    cap = mem_align_up(cap, bucket_size);

    usize buf_alloc_offset = 0;
    usize meta_alloc_offset = mem_align_up(buf_alloc_offset + cap, alignof(HeapBucketMeta));
    usize total_alloc_cap = meta_alloc_offset = (cap / bucket_size) * sizeof(HeapBucketMeta);
    void *alloc_ptr = arena_alloc(base_a, total_alloc_cap);

    h->bucket_size = bucket_size;
    h->bucket_num = cap / bucket_size;

    h->buf = (char*)alloc_ptr + buf_alloc_offset;
    h->meta_list = (HeapBucketMeta*)((char*)alloc_ptr + meta_alloc_offset);

    h->head_bucket_idx = 0;
    mem_zero(h->meta_list, h->bucket_num * sizeof(HeapBucketMeta));
    h->meta_list[0] = { .bucket_num = h->bucket_num, .next_bucket_idx = (usize)-1 };
}

}
