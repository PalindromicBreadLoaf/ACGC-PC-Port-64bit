#include "pc_rarc.h"

#include <string.h>

#include "pc_portability.h"

#define PC_RARC_MAGIC UINT32_C(0x52415243)
#define PC_RARC_DIRECTORY_FLAG UINT32_C(0x02000000)

static bool pc_rarc_range(size_t size, size_t offset, size_t count, size_t item_size) {
    return offset <= size && (item_size == 0 || count <= (size - offset) / item_size);
}

static bool pc_rarc_add(size_t left, size_t right, size_t* result) {
    if (right > SIZE_MAX - left) {
        return false;
    }
    *result = left + right;
    return true;
}

static bool pc_rarc_string_valid(const pc_rarc_view* view, uint32_t offset) {
    size_t string_offset;
    size_t remaining;

    if (offset >= view->info.string_table_length ||
        !pc_rarc_add(view->info_offset, view->info.string_table_offset, &string_offset) ||
        !pc_rarc_add(string_offset, offset, &string_offset)) {
        return false;
    }
    remaining = (size_t)view->info.string_table_length - offset;
    return memchr(view->bytes + string_offset, '\0', remaining) != NULL;
}

bool pc_rarc_get_directory(const pc_rarc_view* view, uint32_t index, pc_rarc_directory* directory) {
    size_t offset;
    const uint8_t* disk;

    if (view == NULL || directory == NULL || index >= view->info.node_count ||
        !pc_rarc_add(view->info_offset, view->info.node_offset, &offset) ||
        !pc_rarc_add(offset, (size_t)index * sizeof(pc_rarc_directory_disk), &offset)) {
        return false;
    }
    disk = view->bytes + offset;
    directory->type = pc_load_be32(disk);
    directory->name_offset = pc_load_be32(disk + 4);
    directory->name_hash = pc_load_be16(disk + 8);
    directory->file_count = pc_load_be16(disk + 10);
    directory->first_file_index = pc_load_be32(disk + 12);
    return true;
}

bool pc_rarc_get_file(const pc_rarc_view* view, uint32_t index, pc_rarc_file* file) {
    size_t offset;
    const uint8_t* disk;

    if (view == NULL || file == NULL || index >= view->info.file_count ||
        !pc_rarc_add(view->info_offset, view->info.file_offset, &offset) ||
        !pc_rarc_add(offset, (size_t)index * sizeof(pc_rarc_file_disk), &offset)) {
        return false;
    }
    disk = view->bytes + offset;
    file->file_id = pc_load_be16(disk);
    file->name_hash = pc_load_be16(disk + 2);
    file->flags_and_name_offset = pc_load_be32(disk + 4);
    file->data_offset = pc_load_be32(disk + 8);
    file->data_length = pc_load_be32(disk + 12);
    return true;
}

const char* pc_rarc_get_string(const pc_rarc_view* view, uint32_t offset) {
    size_t string_offset;

    if (view == NULL || !pc_rarc_string_valid(view, offset) ||
        !pc_rarc_add(view->info_offset, view->info.string_table_offset, &string_offset)) {
        return NULL;
    }
    return (const char*)(view->bytes + string_offset + offset);
}

const void* pc_rarc_get_string_table(const pc_rarc_view* view) {
    size_t string_offset;

    if (view == NULL || !pc_rarc_add(view->info_offset, view->info.string_table_offset, &string_offset)) {
        return NULL;
    }
    return view->bytes + string_offset;
}

const void* pc_rarc_get_file_data(const pc_rarc_view* view, const pc_rarc_file* file) {
    if (view == NULL || file == NULL || !view->has_file_data ||
        (file->flags_and_name_offset & PC_RARC_DIRECTORY_FLAG) != 0 ||
        file->data_offset > view->header.file_data_length ||
        file->data_length > view->header.file_data_length - file->data_offset) {
        return NULL;
    }
    return view->bytes + view->data_offset + file->data_offset;
}

bool pc_rarc_decode_header(pc_rarc_header* header, const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;

    if (header == NULL || data == NULL || size < sizeof(pc_rarc_header_disk) || pc_load_be32(bytes) != PC_RARC_MAGIC) {
        return false;
    }
    header->file_length = pc_load_be32(bytes + 4);
    header->header_length = pc_load_be32(bytes + 8);
    header->file_data_offset = pc_load_be32(bytes + 12);
    header->file_data_length = pc_load_be32(bytes + 16);
    header->mram_data_length = pc_load_be32(bytes + 20);
    header->aram_data_length = pc_load_be32(bytes + 24);
    if (header->header_length < sizeof(pc_rarc_header_disk) ||
        header->file_data_offset > UINT32_MAX - header->header_length ||
        header->file_data_length > UINT32_MAX - (header->header_length + header->file_data_offset) ||
        header->header_length + header->file_data_offset + header->file_data_length > header->file_length) {
        return false;
    }
    return true;
}

static bool pc_rarc_open_internal(pc_rarc_view* view, const void* data, size_t size, bool require_file_data) {
    const uint8_t* bytes = (const uint8_t*)data;
    pc_rarc_header header;
    size_t metadata_end;
    size_t archive_end;
    size_t nodes_offset;
    size_t files_offset;
    size_t strings_offset;
    uint32_t i;

    if (view == NULL || !pc_rarc_decode_header(&header, data, size)) {
        return false;
    }
    memset(view, 0, sizeof(*view));
    view->bytes = bytes;
    view->size = size;
    view->header = header;

    if (view->header.header_length < sizeof(pc_rarc_header_disk) ||
        !pc_rarc_range(size, view->header.header_length, 1, sizeof(pc_rarc_info_disk)) ||
        !pc_rarc_range(view->header.file_length, view->header.header_length, 1, sizeof(pc_rarc_info_disk))) {
        return false;
    }
    view->info_offset = view->header.header_length;
    bytes += view->info_offset;
    view->info.node_count = pc_load_be32(bytes);
    view->info.node_offset = pc_load_be32(bytes + 4);
    view->info.file_count = pc_load_be32(bytes + 8);
    view->info.file_offset = pc_load_be32(bytes + 12);
    view->info.string_table_length = pc_load_be32(bytes + 16);
    view->info.string_table_offset = pc_load_be32(bytes + 20);
    view->info.next_free_file_id = pc_load_be16(bytes + 24);
    view->info.sync_file_ids = bytes[26] != 0;

    if (!pc_rarc_add(view->info_offset, view->header.file_data_offset, &metadata_end) ||
        !pc_rarc_add(metadata_end, view->header.file_data_length, &archive_end) ||
        archive_end > view->header.file_length || metadata_end > size || (require_file_data && archive_end > size)) {
        return false;
    }
    view->data_offset = metadata_end;
    view->has_file_data = archive_end <= size;
    if (!pc_rarc_add(view->info_offset, view->info.node_offset, &nodes_offset) ||
        !pc_rarc_add(view->info_offset, view->info.file_offset, &files_offset) ||
        !pc_rarc_add(view->info_offset, view->info.string_table_offset, &strings_offset) ||
        !pc_rarc_range(metadata_end, nodes_offset, view->info.node_count, sizeof(pc_rarc_directory_disk)) ||
        !pc_rarc_range(metadata_end, files_offset, view->info.file_count, sizeof(pc_rarc_file_disk)) ||
        !pc_rarc_range(metadata_end, strings_offset, view->info.string_table_length, 1)) {
        return false;
    }

    for (i = 0; i < view->info.node_count; ++i) {
        pc_rarc_directory directory;
        if (!pc_rarc_get_directory(view, i, &directory) || !pc_rarc_string_valid(view, directory.name_offset) ||
            directory.first_file_index > view->info.file_count ||
            directory.file_count > view->info.file_count - directory.first_file_index) {
            return false;
        }
    }
    for (i = 0; i < view->info.file_count; ++i) {
        pc_rarc_file file;
        if (!pc_rarc_get_file(view, i, &file) ||
            !pc_rarc_string_valid(view, file.flags_and_name_offset & UINT32_C(0x00ffffff))) {
            return false;
        }
        if ((file.flags_and_name_offset & PC_RARC_DIRECTORY_FLAG) != 0) {
            if (file.data_offset >= view->info.node_count) {
                return false;
            }
        } else if (file.data_offset > view->header.file_data_length ||
                   file.data_length > view->header.file_data_length - file.data_offset) {
            return false;
        }
    }
    return true;
}

bool pc_rarc_open(pc_rarc_view* view, const void* data, size_t size) {
    return pc_rarc_open_internal(view, data, size, true);
}

bool pc_rarc_open_metadata(pc_rarc_view* view, const void* data, size_t size) {
    return pc_rarc_open_internal(view, data, size, false);
}
