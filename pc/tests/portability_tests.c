#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "PR/gbi.h"
#include "PR/ultratypes.h"
#include "TwoHeadArena.h"
#include "dolphin/card.h"
#include "dolphin/gx/GXStruct.h"
#include "dolphin/mtx.h"
#include "pc_portability.h"

#define CHECK(condition)                                                                                               \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition);                              \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

_Static_assert(CHAR_BIT == 8, "the port requires 8-bit bytes");
_Static_assert(sizeof(u8) == 1, "u8 layout changed");
_Static_assert(sizeof(s8) == 1, "s8 layout changed");
_Static_assert(sizeof(u16) == 2, "u16 layout changed");
_Static_assert(sizeof(s16) == 2, "s16 layout changed");
_Static_assert(sizeof(u32) == 4, "u32 layout changed");
_Static_assert(sizeof(s32) == 4, "s32 layout changed");
_Static_assert(sizeof(u64) == 8, "u64 layout changed");
_Static_assert(sizeof(s64) == 8, "s64 layout changed");
_Static_assert(sizeof(pc_host_addr_t) == sizeof(void*), "host addresses must hold pointers");
_Static_assert(sizeof(gc_addr32_t) == 4, "guest addresses must remain 32-bit");
_Static_assert(sizeof(n64_segaddr_t) == 4, "segmented addresses must remain 32-bit");
_Static_assert(sizeof(aram_addr_t) == 4, "ARAM addresses must remain 32-bit");
_Static_assert(sizeof(dvd_offset_t) == 4, "DVD offsets must remain 32-bit");
_Static_assert(sizeof(pc_gbi_handle_t) == 4, "GBI handles must remain 32-bit");
_Static_assert(sizeof(Gfx) == 8, "Gfx layout changed");
_Static_assert(sizeof(Vtx) == 16, "Vtx layout changed");
_Static_assert(sizeof(Mtx) == 64, "N64 matrix layout changed");
_Static_assert(sizeof(Mtx34) == 48, "Dolphin 3x4 matrix layout changed");
_Static_assert(sizeof(Mtx44) == 64, "Dolphin 4x4 matrix layout changed");
_Static_assert(sizeof(GXColor) == 4, "GXColor layout changed");
_Static_assert(sizeof(GXColorS10) == 8, "GXColorS10 layout changed");
_Static_assert(sizeof(CARDFileInfo) == 20, "CARDFileInfo layout changed");
_Static_assert(sizeof(CARDDir) == 64, "CARDDir layout changed");
_Static_assert(sizeof(CARDDirCheck) == 64, "CARDDirCheck layout changed");
_Static_assert(sizeof(CARDID) == 512, "CARDID layout changed");
_Static_assert(sizeof(CARDStat) == 108, "CARDStat layout changed");

static int test_big_endian_loads(void) {
    const uint8_t bytes[] = {0x00, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0};

    CHECK(pc_load_be16(bytes + 1) == UINT16_C(0x1234));
    CHECK(pc_load_be32(bytes + 1) == UINT32_C(0x12345678));
    CHECK(pc_load_be64(bytes + 1) == UINT64_C(0x123456789abcdef0));
    return 0;
}

static int test_big_endian_stores(void) {
    uint8_t bytes[10];
    const uint8_t expected16[] = {0x12, 0x34};
    const uint8_t expected32[] = {0x12, 0x34, 0x56, 0x78};
    const uint8_t expected64[] = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0};

    memset(bytes, 0, sizeof(bytes));
    pc_store_be16(bytes + 1, UINT16_C(0x1234));
    CHECK(memcmp(bytes + 1, expected16, sizeof(expected16)) == 0);

    pc_store_be32(bytes + 1, UINT32_C(0x12345678));
    CHECK(memcmp(bytes + 1, expected32, sizeof(expected32)) == 0);

    pc_store_be64(bytes + 1, UINT64_C(0x123456789abcdef0));
    CHECK(memcmp(bytes + 1, expected64, sizeof(expected64)) == 0);
    return 0;
}

static int test_checked_narrowing(void) {
    uint32_t result = 0;

    CHECK(pc_u32_from_host_addr((pc_host_addr_t)UINT32_MAX, &result));
    CHECK(result == UINT32_MAX);

#if UINTPTR_MAX > UINT32_MAX
    result = UINT32_C(0xfeedface);
    CHECK(!pc_u32_from_host_addr((pc_host_addr_t)UINT32_MAX + 1u, &result));
    CHECK(result == UINT32_C(0xfeedface));
#endif

    return 0;
}

static uintptr_t align_down(uintptr_t value, uintptr_t alignment) {
    return value & ~(alignment - 1u);
}

static uintptr_t align_up(uintptr_t value, uintptr_t alignment) {
    return (value + alignment - 1u) & ~(alignment - 1u);
}

static int test_two_head_arena(void) {
    char storage[257];
    TwoHeadArena arena;
    uintptr_t start = (uintptr_t)(storage + 1);
    uintptr_t end = start + 256u;
    uintptr_t expected;

    THA_ct(&arena, storage + 1, 256u);
    CHECK(arena.head_p == storage + 1);
    CHECK(arena.tail_p == storage + 257);
    CHECK(THA_getFreeBytes(&arena) == 256);

    expected = align_down(align_down(end, 16u) - 17u, 16u);
    CHECK((uintptr_t)THA_alloc16(&arena, 17u) == expected);
    CHECK((uintptr_t)arena.tail_p == expected);
    CHECK(((uintptr_t)arena.tail_p & 15u) == 0u);
    CHECK(THA_getFreeBytes16(&arena) == (int)(expected - align_up(start, 16u)));

    expected = align_down(align_down(expected, 32u) - 1u, 32u);
    CHECK((uintptr_t)THA_allocAlign(&arena, 1u, ~31) == expected);
    CHECK(((uintptr_t)arena.tail_p & 31u) == 0u);
    CHECK(THA_getFreeBytesAlign(&arena, ~31) == (int)(expected - align_up(start, 32u)));

    arena.head_p = arena.tail_p + 1;
    CHECK(THA_getFreeBytes(&arena) == -1);
    CHECK(THA_isCrash(&arena));

    THA_init(&arena);
    CHECK(arena.head_p == storage + 1);
    CHECK(arena.tail_p == storage + 257);

#if UINTPTR_MAX > UINT32_MAX
    CHECK(((uintptr_t)arena.tail_p >> 32) != 0u);
    CHECK(((uintptr_t)THA_alloc16(&arena, 16u) >> 32) == (end >> 32));
#endif

    THA_dt(&arena);
    CHECK(arena.size == 0u);
    CHECK(arena.buf_p == NULL);
    CHECK(arena.head_p == NULL);
    CHECK(arena.tail_p == NULL);
    return 0;
}

int main(void) {
    CHECK(test_big_endian_loads() == 0);
    CHECK(test_big_endian_stores() == 0);
    CHECK(test_checked_narrowing() == 0);
    CHECK(test_two_head_arena() == 0);
    return 0;
}
