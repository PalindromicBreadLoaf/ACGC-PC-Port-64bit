#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pc_rarc.h"

static int failures;

#define CHECK(condition)                                                                                               \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition);                              \
            ++failures;                                                                                                \
        }                                                                                                              \
    } while (0)

static void store_be16(uint8_t* out, uint16_t value) {
    out[0] = (uint8_t)(value >> 8);
    out[1] = (uint8_t)value;
}

static void store_be32(uint8_t* out, uint32_t value) {
    out[0] = (uint8_t)(value >> 24);
    out[1] = (uint8_t)(value >> 16);
    out[2] = (uint8_t)(value >> 8);
    out[3] = (uint8_t)value;
}

static void make_archive(uint8_t archive[128]) {
    memset(archive, 0, 128);
    memcpy(archive, "RARC", 4);
    store_be32(archive + 4, 128);
    store_be32(archive + 8, 32);
    store_be32(archive + 12, 80);
    store_be32(archive + 16, 16);

    store_be32(archive + 32, 1);
    store_be32(archive + 36, 32);
    store_be32(archive + 40, 1);
    store_be32(archive + 44, 48);
    store_be32(archive + 48, 12);
    store_be32(archive + 52, 68);
    store_be16(archive + 56, 1);
    archive[58] = 1;

    memcpy(archive + 64, "ROOT", 4);
    store_be32(archive + 68, 0);
    store_be16(archive + 72, 0x1234);
    store_be16(archive + 74, 1);
    store_be32(archive + 76, 0);

    store_be16(archive + 80, 7);
    store_be16(archive + 82, 0x4567);
    store_be32(archive + 84, UINT32_C(0x11000005));
    store_be32(archive + 88, 3);
    store_be32(archive + 92, 4);
    store_be32(archive + 96, UINT32_C(0xdeadbeef));
    memcpy(archive + 100, "root\0file\0\0", 11);
    memcpy(archive + 115, "DATA", 4);
}

static void test_valid_archive(void) {
    uint8_t archive[128];
    pc_rarc_view view;
    pc_rarc_directory directory;
    pc_rarc_file file;

    make_archive(archive);
    CHECK(pc_rarc_open(&view, archive, sizeof(archive)));
    CHECK(view.header.file_length == 128);
    CHECK(view.info.node_count == 1);
    CHECK(view.info.file_count == 1);
    CHECK(view.info.next_free_file_id == 1);
    CHECK(view.info.sync_file_ids);
    CHECK(pc_rarc_get_directory(&view, 0, &directory));
    CHECK(directory.type == UINT32_C(0x524f4f54));
    CHECK(directory.name_hash == UINT16_C(0x1234));
    CHECK(directory.file_count == 1);
    CHECK(pc_rarc_get_file(&view, 0, &file));
    CHECK(file.file_id == 7);
    CHECK(file.name_hash == UINT16_C(0x4567));
    CHECK(file.data_offset == 3);
    CHECK(file.data_length == 4);
    CHECK(strcmp(pc_rarc_get_string(&view, directory.name_offset), "root") == 0);
    CHECK(strcmp(pc_rarc_get_string(&view, file.flags_and_name_offset & UINT32_C(0x00ffffff)), "file") == 0);
    CHECK(memcmp(pc_rarc_get_file_data(&view, &file), "DATA", 4) == 0);
    CHECK(!pc_rarc_get_file(&view, 1, &file));
}

static void test_rejected_archives(void) {
    uint8_t archive[128];
    pc_rarc_view view;

    make_archive(archive);
    CHECK(!pc_rarc_open(&view, archive, 31));
    archive[0] = 'X';
    CHECK(!pc_rarc_open(&view, archive, sizeof(archive)));

    make_archive(archive);
    store_be32(archive + 44, UINT32_MAX);
    CHECK(!pc_rarc_open(&view, archive, sizeof(archive)));

    make_archive(archive);
    store_be32(archive + 92, 14);
    CHECK(!pc_rarc_open(&view, archive, sizeof(archive)));

    make_archive(archive);
    archive[109] = 'x';
    archive[110] = 'x';
    archive[111] = 'x';
    CHECK(!pc_rarc_open(&view, archive, sizeof(archive)));
}

int main(void) {
    test_valid_archive();
    test_rejected_archives();
    return failures == 0 ? 0 : 1;
}
