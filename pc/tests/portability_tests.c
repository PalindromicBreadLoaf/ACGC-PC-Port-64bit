#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "jaudio_NES/audiocommon.h"
#include "PR/gbi.h"
#include "PR/ultratypes.h"
#include "TwoHeadArena.h"
#include "gamealloc.h"
#include "libforest/gbi_extensions.h"
#include "dolphin/card.h"
#include "dolphin/gx/GXStruct.h"
#include "dolphin/mtx.h"
#include "pc_portability.h"
#include "pc_acmd_runtime.h"
#include "pc_emu64_address.h"
#include "pc_pointer_token.h"

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
_Static_assert(sizeof(runtime_id_t) == sizeof(void*), "runtime IDs must hold pointers");
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

static int test_pointer_tokens(void) {
    uint32_t gbi_token;
    uint32_t duplicate_token;
    uint32_t acmd_token;
    uintptr_t resolved = 0u;
    uintptr_t gbi_pointer = UINT64_C(0x123456789abcdef0);
    uintptr_t acmd_pointer = UINT64_C(0x0fedcba987654321);

    pc_pointer_token_reset(PC_POINTER_TOKEN_DOMAIN_GBI);
    pc_pointer_token_reset(PC_POINTER_TOKEN_DOMAIN_ACMD);

    CHECK(pc_pointer_token_pack(PC_POINTER_TOKEN_DOMAIN_GBI, gbi_pointer, &gbi_token) == PC_POINTER_TOKEN_OK);
    CHECK(pc_pointer_token_is_token(gbi_token));
    CHECK(pc_pointer_token_pack(PC_POINTER_TOKEN_DOMAIN_GBI, gbi_pointer, &duplicate_token) == PC_POINTER_TOKEN_OK);
    CHECK(duplicate_token == gbi_token);
    CHECK(pc_pointer_token_active_count(PC_POINTER_TOKEN_DOMAIN_GBI) == 1u);
    CHECK(pc_pointer_token_resolve(PC_POINTER_TOKEN_DOMAIN_GBI, gbi_token, &resolved) == PC_POINTER_TOKEN_OK);
    CHECK(resolved == gbi_pointer);

    CHECK(pc_pointer_token_pack(PC_POINTER_TOKEN_DOMAIN_ACMD, acmd_pointer, &acmd_token) == PC_POINTER_TOKEN_OK);
    CHECK(acmd_token != gbi_token);
    CHECK(pc_pointer_token_resolve(PC_POINTER_TOKEN_DOMAIN_GBI, acmd_token, &resolved) ==
          PC_POINTER_TOKEN_WRONG_DOMAIN);
    CHECK(pc_pointer_token_resolve(PC_POINTER_TOKEN_DOMAIN_ACMD, gbi_token, &resolved) ==
          PC_POINTER_TOKEN_WRONG_DOMAIN);
    CHECK(pc_pointer_token_resolve(PC_POINTER_TOKEN_DOMAIN_GBI, UINT32_C(0x03000000), &resolved) ==
          PC_POINTER_TOKEN_NOT_TOKEN);
    CHECK(pc_pointer_token_resolve(PC_POINTER_TOKEN_DOMAIN_GBI, UINT32_C(0xfc000001), &resolved) ==
          PC_POINTER_TOKEN_INVALID_DOMAIN);
    CHECK(strcmp(pc_pointer_token_result_name(PC_POINTER_TOKEN_STALE), "stale token") == 0);

    CHECK(pc_pointer_token_release(PC_POINTER_TOKEN_DOMAIN_GBI, gbi_token) == PC_POINTER_TOKEN_OK);
    CHECK(pc_pointer_token_resolve(PC_POINTER_TOKEN_DOMAIN_GBI, gbi_token, &resolved) == PC_POINTER_TOKEN_STALE);
    CHECK(pc_pointer_token_pack(PC_POINTER_TOKEN_DOMAIN_GBI, gbi_pointer, &duplicate_token) == PC_POINTER_TOKEN_OK);
    CHECK(duplicate_token != gbi_token);
    CHECK(pc_pointer_token_resolve(PC_POINTER_TOKEN_DOMAIN_GBI, gbi_token, &resolved) == PC_POINTER_TOKEN_STALE);

    pc_pointer_token_reset(PC_POINTER_TOKEN_DOMAIN_GBI);
    CHECK(pc_pointer_token_resolve(PC_POINTER_TOKEN_DOMAIN_GBI, duplicate_token, &resolved) == PC_POINTER_TOKEN_STALE);
    CHECK(pc_pointer_token_resolve(PC_POINTER_TOKEN_DOMAIN_ACMD, acmd_token, &resolved) == PC_POINTER_TOKEN_OK);
    CHECK(resolved == acmd_pointer);
    pc_pointer_token_reset(PC_POINTER_TOKEN_DOMAIN_ACMD);
    return 0;
}

static int test_pointer_token_exhaustion(void) {
    uint32_t tokens[PC_POINTER_TOKEN_CAPACITY];
    uint32_t overflow_token = 0u;
    uintptr_t resolved;
    uint32_t i;

    pc_pointer_token_reset(PC_POINTER_TOKEN_DOMAIN_GBI);
    for (i = 0u; i < PC_POINTER_TOKEN_CAPACITY; i++) {
        CHECK(pc_pointer_token_pack(PC_POINTER_TOKEN_DOMAIN_GBI, (uintptr_t)i + UINT32_C(0x10000), &tokens[i]) ==
              PC_POINTER_TOKEN_OK);
    }
    CHECK(pc_pointer_token_active_count(PC_POINTER_TOKEN_DOMAIN_GBI) == PC_POINTER_TOKEN_CAPACITY);
    CHECK(pc_pointer_token_pack(PC_POINTER_TOKEN_DOMAIN_GBI, UINT32_C(0xdeadbeef), &overflow_token) ==
          PC_POINTER_TOKEN_EXHAUSTED);
    CHECK(overflow_token == 0u);

    CHECK(pc_pointer_token_release(PC_POINTER_TOKEN_DOMAIN_GBI, tokens[0]) == PC_POINTER_TOKEN_OK);
    CHECK(pc_pointer_token_pack(PC_POINTER_TOKEN_DOMAIN_GBI, UINT32_C(0xdeadbeef), &overflow_token) ==
          PC_POINTER_TOKEN_OK);
    CHECK(pc_pointer_token_resolve(PC_POINTER_TOKEN_DOMAIN_GBI, tokens[0], &resolved) == PC_POINTER_TOKEN_STALE);
    pc_pointer_token_reset(PC_POINTER_TOKEN_DOMAIN_GBI);
    return 0;
}

static int test_gbi_pointer_tokens(void) {
    uintptr_t pointer = UINT64_C(0x1234567887654321);
    uintptr_t resolved = 0u;
    unsigned int token;

    pc_pointer_token_reset(PC_POINTER_TOKEN_DOMAIN_GBI);
    token = pc_gbi_pack_runtime_ptr(pointer, 1, "pointer", __FILE__, __LINE__);
    CHECK(pc_pointer_token_is_token(token));
    CHECK(pc_gbi_unpack_runtime_ptr(token, &resolved) == PC_POINTER_TOKEN_OK);
    CHECK(resolved == pointer);
    CHECK(pc_gbi_pack_runtime_ptr(UINT32_C(0x03000000), 0, "segment", __FILE__, __LINE__) ==
          UINT32_C(0x03000000));
    pc_pointer_token_reset(PC_POINTER_TOKEN_DOMAIN_GBI);
    CHECK(pc_gbi_unpack_runtime_ptr(token, &resolved) == PC_POINTER_TOKEN_STALE);
    return 0;
}

static int test_acmd_pointer_tokens(void) {
    Acmd command;
    uintptr_t pointer = UINT64_C(0x1234567887654321);
    uintptr_t resolved = 0u;

    pc_pointer_token_reset(PC_POINTER_TOKEN_DOMAIN_ACMD);
    aLoadBuffer2(&command, pointer, 0x380, 16);
    CHECK(pc_pointer_token_is_token(command.words.w1));
    CHECK(pc_acmd_unpack_runtime_ptr(command.words.w1, &resolved) == PC_POINTER_TOKEN_OK);
    CHECK(resolved == pointer);
    CHECK((command.words.w0 >> 24) == A_CMD_LOADBUFFER2);
    pc_pointer_token_reset(PC_POINTER_TOKEN_DOMAIN_ACMD);
    CHECK(pc_acmd_unpack_runtime_ptr(command.words.w1, &resolved) == PC_POINTER_TOKEN_STALE);
    return 0;
}

static int test_emu64_address_resolution(void) {
    uintptr_t segments[PC_EMU64_SEGMENT_COUNT] = {0u};
    uintptr_t resolved = 0u;
    uintptr_t pointer = UINT64_C(0x1234567887654321);
    uint32_t token;
    uint32_t segment;

    pc_pointer_token_reset(PC_POINTER_TOKEN_DOMAIN_GBI);
    CHECK(pc_pointer_token_pack(PC_POINTER_TOKEN_DOMAIN_GBI, pointer, &token) == PC_POINTER_TOKEN_OK);
    CHECK(pc_emu64_resolve_address(token, segments, &resolved) == PC_POINTER_TOKEN_OK);
    CHECK(resolved == pointer);

    for (segment = PC_EMU64_FIRST_SEGMENT; segment < PC_EMU64_SEGMENT_COUNT; segment++) {
        segments[segment] = pointer + (uintptr_t)segment * UINT32_C(0x01000000);
        CHECK(pc_emu64_resolve_address((segment << 24), segments, &resolved) == PC_POINTER_TOKEN_OK);
        CHECK(resolved == segments[segment]);
        CHECK(pc_emu64_resolve_address((segment << 24) | UINT32_C(0x00ffffff), segments, &resolved) ==
              PC_POINTER_TOKEN_OK);
        CHECK(resolved == segments[segment] + UINT32_C(0x00ffffff));
    }

    CHECK(pc_emu64_resolve_address(UINT32_C(0x02ffffff), segments, &resolved) == PC_POINTER_TOKEN_OK);
    CHECK(resolved == UINT32_C(0x02ffffff));
    segments[3] = 0u;
    CHECK(pc_emu64_resolve_address(UINT32_C(0x03123456), segments, &resolved) == PC_POINTER_TOKEN_OK);
    CHECK(resolved == UINT32_C(0x03123456));
    segments[3] = UINTPTR_MAX;
    CHECK(pc_emu64_resolve_address(UINT32_C(0x03000001), segments, &resolved) ==
          PC_POINTER_TOKEN_INVALID_DOMAIN);

    CHECK(pc_pointer_token_release(PC_POINTER_TOKEN_DOMAIN_GBI, token) == PC_POINTER_TOKEN_OK);
    CHECK(pc_emu64_resolve_address(token, segments, &resolved) == PC_POINTER_TOKEN_STALE);
    pc_pointer_token_reset(PC_POINTER_TOKEN_DOMAIN_GBI);
    return 0;
}

#if UINTPTR_MAX > UINT32_MAX
static Vtx static_vertices[1];
static Mtx static_matrix;
static uint8_t static_texture[16];
static Gfx static_nested_dl[] = { gsSPEndDisplayList() };
static Gfx static_relocation_dl[] = {
    gsSPVertex(static_vertices, 1, 0),
    gsSPMatrix(&static_matrix, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH),
    gsSPDisplayList(static_nested_dl),
    gsSPSegment(3, UINT32_C(0x00123000)),
    gsDPSetTextureImage_Dolphin(G_IM_FMT_RGBA, G_IM_SIZ_16b, 2, 4, static_texture),
    gsSPEndDisplayList()
};

static int check_static_relocation(size_t command_index, uintptr_t expected) {
    uintptr_t resolved = 0u;

    CHECK(pc_gbi_relocate_static_command(&static_relocation_dl[command_index]) == PC_POINTER_TOKEN_OK);
    CHECK(pc_gbi_unpack_runtime_ptr(static_relocation_dl[command_index].words.w1, &resolved) ==
          PC_POINTER_TOKEN_OK);
    CHECK(resolved == expected);
    CHECK(static_relocation_dl[command_index + 1u].words.w0 == 0u);
    CHECK(static_relocation_dl[command_index + 1u].words.w1 == 0u);
    return 0;
}

static int test_static_gbi_relocations(void) {
    pc_pointer_token_reset(PC_POINTER_TOKEN_DOMAIN_GBI);
    CHECK(sizeof(static_relocation_dl) / sizeof(static_relocation_dl[0]) == 11u);
    CHECK(check_static_relocation(0u, (uintptr_t)static_vertices) == 0);
    CHECK(check_static_relocation(2u, (uintptr_t)&static_matrix) == 0);
    CHECK(check_static_relocation(4u, (uintptr_t)static_nested_dl) == 0);
    CHECK(pc_gbi_relocate_static_command(&static_relocation_dl[6]) == PC_POINTER_TOKEN_OK);
    CHECK(static_relocation_dl[6].words.w1 == UINT32_C(0x00123000));
    CHECK(static_relocation_dl[7].words.w0 == 0u);
    CHECK(static_relocation_dl[7].words.w1 == 0u);
    CHECK(check_static_relocation(8u, (uintptr_t)static_texture) == 0);
    CHECK(pc_gbi_relocate_static_command(&static_relocation_dl[10]) == PC_POINTER_TOKEN_NOT_TOKEN);
    pc_pointer_token_reset(PC_POINTER_TOKEN_DOMAIN_GBI);
    return 0;
}
#else
static int test_static_gbi_relocations(void) {
    return 0;
}
#endif

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

static int test_gamealloc(void) {
    GameAlloc gamealloc;
    void* first;
    void* second;
    void* third;

    gamealloc_init(&gamealloc);
    CHECK(gamealloc.tail == &gamealloc.head);
    CHECK(gamealloc.head.next == &gamealloc.head);
    CHECK(gamealloc.head.prev == &gamealloc.head);

    first = gamealloc_malloc(&gamealloc, 1u);
    second = gamealloc_malloc(&gamealloc, 32u);
    third = gamealloc_malloc(&gamealloc, 7u);
    CHECK(first != NULL);
    CHECK(second != NULL);
    CHECK(third != NULL);
    CHECK(((uintptr_t)first % _Alignof(max_align_t)) == 0u);
    CHECK((((GameAllocList*)first) - 1)->alloc_size == 1u);
    CHECK((((GameAllocList*)second) - 1)->alloc_size == 32u);
    CHECK(gamealloc.tail == ((GameAllocList*)third) - 1);

    gamealloc_free(&gamealloc, second);
    CHECK((((GameAllocList*)first) - 1)->next == ((GameAllocList*)third) - 1);
    CHECK((((GameAllocList*)third) - 1)->prev == ((GameAllocList*)first) - 1);
    gamealloc_free(&gamealloc, third);
    CHECK(gamealloc.tail == ((GameAllocList*)first) - 1);
    gamealloc_free(&gamealloc, first);
    CHECK(gamealloc.tail == &gamealloc.head);
    CHECK(gamealloc.head.next == &gamealloc.head);

    CHECK(gamealloc_malloc(&gamealloc, SIZE_MAX) == NULL);
    CHECK(gamealloc_malloc(&gamealloc, SIZE_MAX - sizeof(GameAllocList) + 1u) == NULL);
#if SIZE_MAX > UINT32_MAX
    CHECK(gamealloc_malloc(&gamealloc, (size_t)UINT32_MAX + 1u) == NULL);
#endif

    first = gamealloc_malloc(&gamealloc, 8u);
    second = gamealloc_malloc(&gamealloc, 16u);
    CHECK(first != NULL && second != NULL);
    gamealloc_cleanup(&gamealloc);
    CHECK(gamealloc.tail == &gamealloc.head);
    CHECK(gamealloc.head.next == &gamealloc.head);
    CHECK(gamealloc.head.prev == &gamealloc.head);
    return 0;
}

int main(void) {
    CHECK(test_big_endian_loads() == 0);
    CHECK(test_big_endian_stores() == 0);
    CHECK(test_checked_narrowing() == 0);
    CHECK(test_pointer_tokens() == 0);
    CHECK(test_pointer_token_exhaustion() == 0);
    CHECK(test_gbi_pointer_tokens() == 0);
    CHECK(test_acmd_pointer_tokens() == 0);
    CHECK(test_emu64_address_resolution() == 0);
    CHECK(test_static_gbi_relocations() == 0);
    CHECK(test_two_head_arena() == 0);
    CHECK(test_gamealloc() == 0);
    return 0;
}
