#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pc_executable.h"
#include "pc_portability.h"

#define CHECK(condition)                                                                                              \
    do {                                                                                                              \
        if (!(condition)) {                                                                                           \
            fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__, __LINE__, #condition);                           \
            return 1;                                                                                                 \
        }                                                                                                             \
    } while (0)

static void make_dol(uint8_t* data, size_t size) {
    memset(data, 0, size);
    pc_store_be32(data, 0x100);
    pc_store_be32(data + 0x48, 0x80003100);
    pc_store_be32(data + 0x90, 0x20);
    pc_store_be32(data + 0x1c, 0x120);
    pc_store_be32(data + 0x64, 0x80300000);
    pc_store_be32(data + 0xac, 0x10);
    pc_store_be32(data + 0xd8, 0x80400000);
    pc_store_be32(data + 0xdc, 0x1000);
    pc_store_be32(data + 0xe0, 0x80003100);
}

static void make_rel(uint8_t* data, size_t size) {
    memset(data, 0, size);
    pc_store_be32(data, 7);
    pc_store_be32(data + 0x0c, 2);
    pc_store_be32(data + 0x10, 0x48);
    pc_store_be32(data + 0x14, 0x70);
    pc_store_be32(data + 0x18, 4);
    pc_store_be32(data + 0x1c, 2);
    pc_store_be32(data + 0x20, 0x80);
    pc_store_be32(data + 0x24, 0x60);
    pc_store_be32(data + 0x28, 0x58);
    pc_store_be32(data + 0x2c, 8);
    pc_store_be32(data + 0x40, 32);
    pc_store_be32(data + 0x44, 16);
    pc_store_be32(data + 0x50, 0x81);
    pc_store_be32(data + 0x54, 0x10);
    pc_store_be32(data + 0x58, 0);
    pc_store_be32(data + 0x5c, 0x60);
    data[0x62] = 203;
    memcpy(data + 0x70, "mod", 4);
}

static int test_dol(void) {
    uint8_t data[0x130];
    pc_dol_view view;

    make_dol(data, sizeof(data));
    CHECK(pc_dol_open(&view, data, sizeof(data)));
    CHECK(view.sections[0].file_offset == 0x100);
    CHECK(view.sections[0].address == UINT32_C(0x80003100));
    CHECK(view.sections[0].size == 0x20);
    CHECK(view.sections[0].executable);
    CHECK(view.sections[7].file_offset == 0x120);
    CHECK(!view.sections[7].executable);
    CHECK(view.bss_address == UINT32_C(0x80400000));
    CHECK(view.bss_size == 0x1000);
    CHECK(view.entry_point == UINT32_C(0x80003100));
    CHECK(!pc_dol_open(&view, data, sizeof(pc_dol_header_disk) - 1));

    pc_store_be32(data + 0x90, 0x31);
    CHECK(!pc_dol_open(&view, data, sizeof(data)));
    make_dol(data, sizeof(data));
    pc_store_be32(data + 0x48, UINT32_C(0xfffffff0));
    CHECK(!pc_dol_open(&view, data, sizeof(data)));
    make_dol(data, sizeof(data));
    pc_store_be32(data + 0xd8, UINT32_C(0xffffff80));
    pc_store_be32(data + 0xdc, 0x100);
    CHECK(!pc_dol_open(&view, data, sizeof(data)));
    return 0;
}

static int test_rel(void) {
    uint8_t data[0x90];
    pc_rel_view view;
    pc_executable_section section;
    pc_rel_import import_entry;

    make_rel(data, sizeof(data));
    CHECK(pc_rel_open(&view, data, sizeof(data)));
    CHECK(view.id == 7);
    CHECK(view.section_count == 2);
    CHECK(view.bss_size == 0x80);
    CHECK(pc_rel_get_section(&view, 0, &section));
    CHECK(!section.file_backed && section.size == 0);
    CHECK(pc_rel_get_section(&view, 1, &section));
    CHECK(section.file_backed && section.executable);
    CHECK(section.file_offset == 0x80 && section.size == 0x10);
    CHECK(!pc_rel_get_section(&view, 2, &section));
    CHECK(pc_rel_get_import(&view, 0, &import_entry));
    CHECK(import_entry.module_id == 0 && import_entry.relocation_offset == 0x60);
    CHECK(!pc_rel_get_import(&view, 1, &import_entry));
    CHECK(!pc_rel_open(&view, data, sizeof(pc_rel_header_v1_disk) - 1));

    make_rel(data, sizeof(data));
    pc_store_be32(data + 0x54, 0x11);
    CHECK(!pc_rel_open(&view, data, sizeof(data)));
    make_rel(data, sizeof(data));
    data[0x62] = 0;
    CHECK(!pc_rel_open(&view, data, sizeof(data)));
    make_rel(data, sizeof(data));
    pc_store_be32(data + 0x5c, 0x8c);
    CHECK(!pc_rel_open(&view, data, sizeof(data)));
    make_rel(data, sizeof(data));
    pc_store_be32(data + 0x10, UINT32_C(0xfffffff8));
    pc_store_be32(data + 0x0c, UINT32_C(0x20000001));
    CHECK(!pc_rel_open(&view, data, sizeof(data)));
    make_rel(data, sizeof(data));
    pc_store_be32(data + 0x40, 24);
    CHECK(!pc_rel_open(&view, data, sizeof(data)));
    make_rel(data, sizeof(data));
    pc_store_be32(data + 0x1c, 3);
    pc_store_be32(data + 0x10, 0x50);
    pc_store_be32(data + 0x24, 0x70);
    pc_store_be32(data + 0x28, 0x68);
    pc_store_be32(data + 0x48, 0x80);
    memset(data + 0x4c, 0, 0x24);
    pc_store_be32(data + 0x58, 0x81);
    pc_store_be32(data + 0x5c, 0x10);
    pc_store_be32(data + 0x6c, 0x70);
    data[0x72] = 203;
    CHECK(pc_rel_open(&view, data, sizeof(data)));
    CHECK(view.fix_size == 0x80);
    pc_store_be32(data + 0x48, 0x91);
    CHECK(!pc_rel_open(&view, data, sizeof(data)));
    return 0;
}

int main(void) {
    CHECK(test_dol() == 0);
    CHECK(test_rel() == 0);
    return 0;
}
