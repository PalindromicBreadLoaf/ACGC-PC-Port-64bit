#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dolphin/ar.h"

#define PC_ARAM_SIZE (16u * 1024u * 1024u)

static u8* aram_base = NULL;
static u32 aram_alloc_ptr = 0;

u32 ARInit(u32* stack_idx_addr, u32 length) {
    (void)stack_idx_addr; (void)length;
    if (!aram_base) {
        aram_base = (u8*)malloc(PC_ARAM_SIZE);
        if (aram_base) {
            memset(aram_base, 0, PC_ARAM_SIZE);
        }
        aram_alloc_ptr = 0;
    }
    return 0; /* offset-based, base is always 0 */
}

u8* pc_aram_get_base(void) { return aram_base; }

u32 ARGetBaseAddress(void) { return 0; }
u32 ARGetSize(void) { return PC_ARAM_SIZE; }

u32 ARAlloc(u32 size) {
    u32 aligned_size;
    if (size > UINT32_MAX - UINT32_C(31)) {
        return 0;
    }
    aligned_size = (size + UINT32_C(31)) & ~UINT32_C(31);
    if (aligned_size > PC_ARAM_SIZE - aram_alloc_ptr) {
        fprintf(stderr, "[PC/ARAM] Out of ARAM! Requested %u, used %u/%u\n",
                size, aram_alloc_ptr, PC_ARAM_SIZE);
        return 0;
    }
    u32 addr = aram_alloc_ptr;
    aram_alloc_ptr += aligned_size;
    return addr;
}

u32 ARFree(u32* addr) {
    (void)addr; /* bump allocator, no-op */
    return 0;
}

BOOL pc_aram_transfer(u32 type, void* mram_addr, aram_addr_t aram_addr, u32 length) {
    if (aram_base == NULL || (type != ARAM_DIR_MRAM_TO_ARAM && type != ARAM_DIR_ARAM_TO_MRAM)) {
        return FALSE;
    }
    if (length != 0 && mram_addr == NULL) {
        return FALSE;
    }
    if (length > PC_ARAM_SIZE || aram_addr > PC_ARAM_SIZE - length) {
        return FALSE;
    }

    if (type == ARAM_DIR_MRAM_TO_ARAM) {
        memmove(aram_base + aram_addr, mram_addr, length);
    } else {
        memmove(mram_addr, aram_base + aram_addr, length);
    }
    return TRUE;
}

void ARStartDMA(u32 type, void* mram_addr, aram_addr_t aram_addr, u32 length) {
    (void)pc_aram_transfer(type, mram_addr, aram_addr, length);
}

u32 ARGetInternalSize(void) { return PC_ARAM_SIZE; }
BOOL ARCheckInit(void) { return aram_base != NULL; }

void ARQInit(void) {}
void ARQPostRequest(ARQRequest* req, ARQOwner owner, u32 type, u32 prio, void* mram_addr,
                    aram_addr_t aram_addr, u32 length, ARQCallback callback) {
    if (req != NULL) {
        req->owner = owner;
        req->type = type;
        req->priority = prio;
        req->mramAddress = mram_addr;
        req->aramAddress = aram_addr;
        req->length = length;
        req->callback = callback;
    }
    ARStartDMA(type, mram_addr, aram_addr, length);
    if (callback != NULL) {
        callback((uintptr_t)req);
    }
}

void ARQFlushQueue(void) {}
