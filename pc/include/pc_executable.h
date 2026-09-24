#ifndef PC_EXECUTABLE_H
#define PC_EXECUTABLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    PC_DOL_TEXT_SECTION_COUNT = 7,
    PC_DOL_DATA_SECTION_COUNT = 11,
    PC_DOL_SECTION_COUNT = PC_DOL_TEXT_SECTION_COUNT + PC_DOL_DATA_SECTION_COUNT
};

typedef struct pc_dol_header_disk {
    uint8_t bytes[256];
} pc_dol_header_disk;

typedef struct pc_rel_header_v1_disk {
    uint8_t bytes[64];
} pc_rel_header_v1_disk;

typedef struct pc_rel_header_v2_disk {
    uint8_t bytes[72];
} pc_rel_header_v2_disk;

typedef struct pc_rel_header_v3_disk {
    uint8_t bytes[76];
} pc_rel_header_v3_disk;

typedef struct pc_rel_section_disk {
    uint8_t bytes[8];
} pc_rel_section_disk;

typedef struct pc_rel_import_disk {
    uint8_t bytes[8];
} pc_rel_import_disk;

typedef struct pc_rel_relocation_disk {
    uint8_t bytes[8];
} pc_rel_relocation_disk;

typedef struct pc_executable_section {
    uint32_t file_offset;
    uint32_t address;
    uint32_t size;
    bool executable;
    bool file_backed;
} pc_executable_section;

typedef struct pc_dol_view {
    const uint8_t* bytes;
    size_t size;
    pc_executable_section sections[PC_DOL_SECTION_COUNT];
    uint32_t bss_address;
    uint32_t bss_size;
    uint32_t entry_point;
} pc_dol_view;

typedef struct pc_rel_view {
    const uint8_t* bytes;
    size_t size;
    uint32_t id;
    uint32_t section_count;
    uint32_t section_table_offset;
    uint32_t name_offset;
    uint32_t name_size;
    uint32_t version;
    uint32_t bss_size;
    uint32_t relocation_offset;
    uint32_t import_offset;
    uint32_t import_size;
    uint32_t alignment;
    uint32_t bss_alignment;
    uint32_t fix_size;
} pc_rel_view;

typedef struct pc_rel_import {
    uint32_t module_id;
    uint32_t relocation_offset;
} pc_rel_import;

bool pc_dol_open(pc_dol_view* view, const void* data, size_t size);
bool pc_rel_open(pc_rel_view* view, const void* data, size_t size);
bool pc_rel_get_section(const pc_rel_view* view, uint32_t index, pc_executable_section* section);
bool pc_rel_get_import(const pc_rel_view* view, uint32_t index, pc_rel_import* import_entry);

#ifdef __cplusplus
}

static_assert(sizeof(pc_dol_header_disk) == 256);
static_assert(sizeof(pc_rel_header_v1_disk) == 64);
static_assert(sizeof(pc_rel_header_v2_disk) == 72);
static_assert(sizeof(pc_rel_header_v3_disk) == 76);
static_assert(sizeof(pc_rel_section_disk) == 8);
static_assert(sizeof(pc_rel_import_disk) == 8);
static_assert(sizeof(pc_rel_relocation_disk) == 8);
#else
_Static_assert(sizeof(pc_dol_header_disk) == 256, "DOL header layout changed");
_Static_assert(sizeof(pc_rel_header_v1_disk) == 64, "REL v1 header layout changed");
_Static_assert(sizeof(pc_rel_header_v2_disk) == 72, "REL v2 header layout changed");
_Static_assert(sizeof(pc_rel_header_v3_disk) == 76, "REL v3 header layout changed");
_Static_assert(sizeof(pc_rel_section_disk) == 8, "REL section layout changed");
_Static_assert(sizeof(pc_rel_import_disk) == 8, "REL import layout changed");
_Static_assert(sizeof(pc_rel_relocation_disk) == 8, "REL relocation layout changed");
#endif

#endif
