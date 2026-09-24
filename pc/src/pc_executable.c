#include "pc_executable.h"

#include <string.h>

#include "pc_portability.h"

enum {
    PC_REL_RELOCATION_SECTION = 202,
    PC_REL_RELOCATION_END = 203
};

static bool pc_executable_range(size_t size, size_t offset, size_t length) {
    return offset <= size && length <= size - offset;
}

static bool pc_executable_u32_span(uint32_t start, uint32_t length) {
    return length <= UINT32_MAX - start;
}

static bool pc_executable_power_of_two(uint32_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

static bool pc_rel_relocation_stream_valid(const pc_rel_view* view, uint32_t offset) {
    size_t cursor = offset;

    while (pc_executable_range(view->size, cursor, sizeof(pc_rel_relocation_disk))) {
        const uint8_t* relocation = view->bytes + cursor;
        uint8_t type = relocation[2];

        if (type == PC_REL_RELOCATION_END) {
            return true;
        }
        if (type == PC_REL_RELOCATION_SECTION && relocation[3] >= view->section_count) {
            return false;
        }
        cursor += sizeof(pc_rel_relocation_disk);
    }
    return false;
}

bool pc_dol_open(pc_dol_view* view, const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t i;

    if (view == NULL || data == NULL || size < sizeof(pc_dol_header_disk)) {
        return false;
    }
    memset(view, 0, sizeof(*view));
    view->bytes = bytes;
    view->size = size;

    for (i = 0; i < PC_DOL_SECTION_COUNT; ++i) {
        uint32_t offset_position = i < PC_DOL_TEXT_SECTION_COUNT ? i * 4 : 0x1c + (i - 7) * 4;
        uint32_t address_position = i < PC_DOL_TEXT_SECTION_COUNT ? 0x48 + i * 4 : 0x64 + (i - 7) * 4;
        uint32_t size_position = i < PC_DOL_TEXT_SECTION_COUNT ? 0x90 + i * 4 : 0xac + (i - 7) * 4;
        pc_executable_section* section = &view->sections[i];

        section->file_offset = pc_load_be32(bytes + offset_position);
        section->address = pc_load_be32(bytes + address_position);
        section->size = pc_load_be32(bytes + size_position);
        section->executable = i < PC_DOL_TEXT_SECTION_COUNT;
        section->file_backed = section->size != 0;
        if (section->size != 0 &&
            (!pc_executable_range(size, section->file_offset, section->size) ||
             !pc_executable_u32_span(section->address, section->size))) {
            return false;
        }
    }

    view->bss_address = pc_load_be32(bytes + 0xd8);
    view->bss_size = pc_load_be32(bytes + 0xdc);
    view->entry_point = pc_load_be32(bytes + 0xe0);
    return pc_executable_u32_span(view->bss_address, view->bss_size);
}

bool pc_rel_get_section(const pc_rel_view* view, uint32_t index, pc_executable_section* section) {
    size_t offset;
    const uint8_t* disk;
    uint32_t encoded_offset;

    if (view == NULL || section == NULL || index >= view->section_count) {
        return false;
    }
    offset = (size_t)view->section_table_offset + (size_t)index * sizeof(pc_rel_section_disk);
    disk = view->bytes + offset;
    encoded_offset = pc_load_be32(disk);
    section->file_offset = encoded_offset & ~UINT32_C(1);
    section->address = 0;
    section->size = pc_load_be32(disk + 4);
    section->executable = (encoded_offset & UINT32_C(1)) != 0;
    section->file_backed = section->file_offset != 0;
    return true;
}

bool pc_rel_get_import(const pc_rel_view* view, uint32_t index, pc_rel_import* import_entry) {
    size_t import_count;
    size_t offset;
    const uint8_t* disk;

    if (view == NULL || import_entry == NULL) {
        return false;
    }
    import_count = view->import_size / sizeof(pc_rel_import_disk);
    if ((size_t)index >= import_count) {
        return false;
    }
    offset = (size_t)view->import_offset + (size_t)index * sizeof(pc_rel_import_disk);
    disk = view->bytes + offset;
    import_entry->module_id = pc_load_be32(disk);
    import_entry->relocation_offset = pc_load_be32(disk + 4);
    return true;
}

bool pc_rel_open(pc_rel_view* view, const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    size_t header_size;
    uint32_t i;

    if (view == NULL || data == NULL || size < sizeof(pc_rel_header_v1_disk)) {
        return false;
    }
    memset(view, 0, sizeof(*view));
    view->bytes = bytes;
    view->size = size;
    view->id = pc_load_be32(bytes);
    view->section_count = pc_load_be32(bytes + 0x0c);
    view->section_table_offset = pc_load_be32(bytes + 0x10);
    view->name_offset = pc_load_be32(bytes + 0x14);
    view->name_size = pc_load_be32(bytes + 0x18);
    view->version = pc_load_be32(bytes + 0x1c);
    view->bss_size = pc_load_be32(bytes + 0x20);
    view->relocation_offset = pc_load_be32(bytes + 0x24);
    view->import_offset = pc_load_be32(bytes + 0x28);
    view->import_size = pc_load_be32(bytes + 0x2c);
    if (view->version >= 2) {
        if (size < sizeof(pc_rel_header_v2_disk)) {
            return false;
        }
        view->alignment = pc_load_be32(bytes + 0x40);
        view->bss_alignment = pc_load_be32(bytes + 0x44);
    } else {
        view->alignment = 32;
        view->bss_alignment = 32;
    }
    header_size = view->version >= 2 ? sizeof(pc_rel_header_v2_disk) : sizeof(pc_rel_header_v1_disk);
    if (view->version >= 3) {
        if (size < sizeof(pc_rel_header_v3_disk)) {
            return false;
        }
        view->fix_size = pc_load_be32(bytes + 0x48);
        if (view->fix_size > size) {
            return false;
        }
        header_size = sizeof(pc_rel_header_v3_disk);
    }

    if (view->version < 1 || view->version > 3 || view->section_count == 0 ||
        view->section_table_offset < header_size ||
        !pc_executable_range(size, view->section_table_offset,
                             (size_t)view->section_count * sizeof(pc_rel_section_disk)) ||
        !pc_executable_range(size, view->name_offset, view->name_size) ||
        !pc_executable_range(size, view->import_offset, view->import_size) ||
        view->import_size % sizeof(pc_rel_import_disk) != 0 || view->relocation_offset > size ||
        (view->version >= 2 &&
         (!pc_executable_power_of_two(view->alignment) || !pc_executable_power_of_two(view->bss_alignment)))) {
        return false;
    }

    for (i = 0; i < view->section_count; ++i) {
        pc_executable_section section;
        if (!pc_rel_get_section(view, i, &section) ||
            (section.file_backed && !pc_executable_range(size, section.file_offset, section.size))) {
            return false;
        }
    }
    for (i = 0; i < view->import_size / sizeof(pc_rel_import_disk); ++i) {
        pc_rel_import import_entry;
        if (!pc_rel_get_import(view, i, &import_entry) ||
            !pc_rel_relocation_stream_valid(view, import_entry.relocation_offset)) {
            return false;
        }
    }
    return true;
}
