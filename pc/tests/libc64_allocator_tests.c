#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "libc64/__osMalloc.h"

#define CHECK(condition)                                                                                               \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition);                              \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

static int test_libc64_allocator(void) {
    _Alignas(32) u8 storage[8193];
    OSArena arena;
    void* head;
    void* tail;
    void* aligned;
    u32 initialFree;

    __osMallocInit(&arena, storage + 1, 8192);
    CHECK(__osMallocIsInitalized(&arena));
    CHECK(!__osCheckArena(&arena));
    initialFree = __osGetTotalFreeSize(&arena);

    head = __osMalloc(&arena, 73);
    tail = __osMallocR(&arena, 91);
    aligned = __osMallocAlign(&arena, 55, 256);
    CHECK(head != NULL && tail != NULL && aligned != NULL);
    CHECK(((uintptr_t)head & 31u) == 0u);
    CHECK(((uintptr_t)tail & 31u) == 0u);
    CHECK(((uintptr_t)aligned & 255u) == 0u);
    CHECK(__osGetMemBlockSize(&arena, head) >= 73);
    CHECK(!__osCheckArena(&arena));

    memset(head, 0x5a, 73);
    head = __osRealloc(&arena, head, 200);
    CHECK(head != NULL);
    for (size_t i = 0; i < 73; ++i) {
        CHECK(((u8*)head)[i] == 0x5a);
    }
    head = __osRealloc(&arena, head, 40);
    CHECK(head != NULL);
    CHECK(__osGetMemBlockSize(&arena, head) >= 40);
    CHECK(!__osCheckArena(&arena));

    __osFree(&arena, aligned);
    __osFree(&arena, head);
    __osFree(&arena, tail);
    CHECK(!__osCheckArena(&arena));
    CHECK(__osGetTotalFreeSize(&arena) == initialFree);

    head = __osMalloc(&arena, 288);
    CHECK(head != NULL);
    CHECK(__osRealloc(&arena, head, 224) == head);
    CHECK(__osGetMemBlockSize(&arena, head) >= 224);
    __osFree(&arena, head);
    CHECK(__osGetTotalFreeSize(&arena) == initialFree);

    void* allocations[128] = {0};
    size_t count = 0;
    while (count < 128 && (allocations[count] = __osMalloc(&arena, 64)) != NULL) {
        ++count;
    }
    CHECK(count > 0 && count < 128);
    for (size_t i = 1; i < count; i += 2) {
        __osFree(&arena, allocations[i]);
    }
    for (size_t i = 0; i < count; i += 2) {
        __osFree(&arena, allocations[i]);
    }
    CHECK(__osGetTotalFreeSize(&arena) == initialFree);

    CHECK(__osMalloc(&arena, UINT32_MAX) == NULL);
    CHECK(__osMallocR(&arena, UINT32_MAX) == NULL);
    CHECK(__osRealloc(&arena, NULL, UINT32_MAX) == NULL);
    __osFree(&arena, NULL);

    __osMallocCleanup(&arena);
    CHECK(!__osMallocIsInitalized(&arena));
    return 0;
}

int main(void) {
    return test_libc64_allocator();
}
