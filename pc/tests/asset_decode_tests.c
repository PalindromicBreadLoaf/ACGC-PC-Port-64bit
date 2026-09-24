#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pc_asset_decode.h"

static int failures;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        failures++; \
    } \
} while (0)

static void test_raw_texture(void) {
    const uint8_t disk[] = { 0x10, 0x20, 0x30, 0x40, 0x50 };
    uint8_t runtime[sizeof(disk)] = { 0 };

    CHECK(pc_asset_decode(runtime, sizeof(runtime), disk, sizeof(disk), PC_ASSET_RAW));
    CHECK(memcmp(runtime, disk, sizeof(disk)) == 0);
}

static void test_big_endian_scalars(void) {
    const uint8_t disk16[] = { 0x12, 0x34, 0xab, 0xcd };
    const uint8_t disk32[] = { 0x12, 0x34, 0x56, 0x78, 0xde, 0xad, 0xbe, 0xef };
    const uint8_t expected16[] = { 0x12, 0x34, 0xab, 0xcd };
    const uint8_t expected32[] = { 0x12, 0x34, 0x56, 0x78, 0xde, 0xad, 0xbe, 0xef };
    uint16_t runtime16[2] = { 0 };
    uint32_t runtime32[2] = { 0 };

    CHECK(pc_asset_decode(runtime16, sizeof(runtime16), disk16, sizeof(disk16), PC_ASSET_BE16));
    CHECK(runtime16[0] == UINT16_C(0x1234));
    CHECK(runtime16[1] == UINT16_C(0xabcd));
    CHECK(memcmp(disk16, expected16, sizeof(disk16)) == 0);

    CHECK(pc_asset_decode(runtime32, sizeof(runtime32), disk32, sizeof(disk32), PC_ASSET_BE32));
    CHECK(runtime32[0] == UINT32_C(0x12345678));
    CHECK(runtime32[1] == UINT32_C(0xdeadbeef));
    CHECK(memcmp(disk32, expected32, sizeof(disk32)) == 0);
}

static void test_vertex(void) {
    const uint8_t disk[] = {
        0xff, 0xfe, 0x12, 0x34, 0x80, 0x00, 0xab, 0xcd,
        0x01, 0x02, 0xfe, 0xdc, 0x11, 0x22, 0x33, 0x44
    };
    uint8_t runtime[16] = { 0 };
    uint16_t fields[6];
    size_t i;

    CHECK(pc_asset_decode(runtime, sizeof(runtime), disk, sizeof(disk), PC_ASSET_VTX_BE));
    for (i = 0; i < 6; i++) memcpy(&fields[i], runtime + i * 2, sizeof(fields[i]));
    CHECK(fields[0] == UINT16_C(0xfffe));
    CHECK(fields[1] == UINT16_C(0x1234));
    CHECK(fields[2] == UINT16_C(0x8000));
    CHECK(fields[3] == UINT16_C(0xabcd));
    CHECK(fields[4] == UINT16_C(0x0102));
    CHECK(fields[5] == UINT16_C(0xfedc));
    CHECK(memcmp(runtime + 12, disk + 12, 4) == 0);
}

static void test_display_list(void) {
    const uint8_t disk[] = {
        0xde, 0x00, 0x00, 0x00, 0x06, 0x12, 0x34, 0x56,
        0xdf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    uint32_t runtime[4] = { 0 };

    CHECK(pc_asset_decode(runtime, sizeof(runtime), disk, sizeof(disk), PC_ASSET_GFX_BE));
    CHECK(runtime[0] == UINT32_C(0xde000000));
    CHECK(runtime[1] == UINT32_C(0x06123456));
    CHECK(runtime[2] == UINT32_C(0xdf000000));
    CHECK(runtime[3] == 0);
}

static void test_invalid_inputs(void) {
    uint8_t input[17] = { 0 };
    uint8_t output[17] = { 0xa5 };

    CHECK(!pc_asset_decode(output, 3, input, 3, PC_ASSET_BE16));
    CHECK(!pc_asset_decode(output, 15, input, 15, PC_ASSET_VTX_BE));
    CHECK(!pc_asset_decode(output, 4, input, 4, PC_ASSET_GFX_BE));
    CHECK(!pc_asset_decode(output, 4, input, 3, PC_ASSET_RAW));
    CHECK(!pc_asset_decode(output, 4, input, 4, (pc_asset_format)99));
    CHECK(!pc_asset_decode(NULL, 4, input, 4, PC_ASSET_RAW));
    CHECK(!pc_asset_decode(output, 4, NULL, 4, PC_ASSET_RAW));
    CHECK(pc_asset_decode(NULL, 0, NULL, 0, PC_ASSET_RAW));
    CHECK(output[0] == 0xa5);
}

int main(void) {
    test_raw_texture();
    test_big_endian_scalars();
    test_vertex();
    test_display_list();
    test_invalid_inputs();
    return failures == 0 ? 0 : 1;
}
