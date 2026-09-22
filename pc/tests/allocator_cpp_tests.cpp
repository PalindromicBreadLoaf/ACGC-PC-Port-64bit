#include <cstdint>
#include <cstdio>
#include "JSystem/JKernel/JKRExpHeap.h"

#define CHECK(condition)                                                                                               \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            std::fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition);                         \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

static int test_exp_heap() {
    alignas(32) unsigned char arena[4096];
    alignas(JKRExpHeap) unsigned char heapStorage[sizeof(JKRExpHeap)];
    JKRExpHeap* heap = new (heapStorage) JKRExpHeap(arena, sizeof(arena), nullptr, false);
    const s32 initialFree = static_cast<s32>(sizeof(arena) - sizeof(JKRExpHeap::CMemBlock));

    CHECK(sizeof(JKRExpHeap::CMemBlock) > 0x10u);
    CHECK(heap->do_getTotalFreeSize() == initialFree);
    CHECK(heap->check());

    void* head = heap->do_alloc(100, 32);
    void* tail = heap->do_alloc(80, -64);
    void* middle = heap->do_alloc(120, 16);
    CHECK(head != nullptr && tail != nullptr && middle != nullptr);
    CHECK((reinterpret_cast<std::uintptr_t>(head) & 31u) == 0u);
    CHECK((reinterpret_cast<std::uintptr_t>(tail) & 63u) == 0u);
    CHECK((reinterpret_cast<std::uintptr_t>(middle) & 15u) == 0u);
    CHECK(JKRExpHeap::CMemBlock::getBlock(head) == JKRExpHeap::CMemBlock::getHeapBlock(head));
    CHECK(heap->check());

    heap->do_free(middle);
    heap->do_free(head);
    CHECK(heap->check());
    heap->do_free(tail);
    CHECK(heap->do_getTotalFreeSize() == initialFree);
    CHECK(heap->check());

    void* resized = heap->do_alloc(128, 4);
    void* neighbor = heap->do_alloc(128, 4);
    CHECK(resized != nullptr && neighbor != nullptr);
    heap->do_free(neighbor);
    CHECK(heap->do_resize(resized, 256) >= 256);
    CHECK(heap->do_resize(resized, 64) >= 64);
    CHECK(heap->check());
    heap->do_free(resized);

    void* allocations[128] = {};
    size_t count = 0;
    while (count < 128 && (allocations[count] = heap->do_alloc(64, 4)) != nullptr) {
        ++count;
    }
    CHECK(count > 0 && count < 128);
    for (size_t i = 1; i < count; i += 2) {
        heap->do_free(allocations[i]);
    }
    for (size_t i = 0; i < count; i += 2) {
        heap->do_free(allocations[i]);
    }
    CHECK(heap->do_getTotalFreeSize() == initialFree);
    CHECK(heap->check());

    CHECK(heap->do_alloc(UINT32_MAX, 4) == nullptr);
    void* resizeOverflow = heap->do_alloc(32, 4);
    CHECK(resizeOverflow != nullptr);
    CHECK(heap->do_resize(resizeOverflow, UINT32_MAX) == -1);
    heap->do_free(resizeOverflow);
    CHECK(heap->check());

    void* damaged = heap->do_alloc(32, 4);
    CHECK(damaged != nullptr);
    JKRExpHeap::CMemBlock* damagedBlock = JKRExpHeap::CMemBlock::getHeapBlock(damaged);
    damagedBlock->mUsageHeader = 0;
    CHECK(!heap->check());
    damagedBlock->mUsageHeader = 0x484d;
    heap->do_free(damaged);
    CHECK(heap->check());
    return 0;
}

int main() {
    return test_exp_heap();
}
