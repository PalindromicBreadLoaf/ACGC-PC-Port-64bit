#ifndef PC_RARC_H
#define PC_RARC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pc_rarc_header_disk {
    uint8_t bytes[32];
} pc_rarc_header_disk;

typedef struct pc_rarc_info_disk {
    uint8_t bytes[32];
} pc_rarc_info_disk;

typedef struct pc_rarc_directory_disk {
    uint8_t bytes[16];
} pc_rarc_directory_disk;

typedef struct pc_rarc_file_disk {
    uint8_t bytes[20];
} pc_rarc_file_disk;

typedef struct pc_rarc_header {
    uint32_t file_length;
    uint32_t header_length;
    uint32_t file_data_offset;
    uint32_t file_data_length;
    uint32_t mram_data_length;
    uint32_t aram_data_length;
} pc_rarc_header;

typedef struct pc_rarc_info {
    uint32_t node_count;
    uint32_t node_offset;
    uint32_t file_count;
    uint32_t file_offset;
    uint32_t string_table_length;
    uint32_t string_table_offset;
    uint16_t next_free_file_id;
    bool sync_file_ids;
} pc_rarc_info;

typedef struct pc_rarc_directory {
    uint32_t type;
    uint32_t name_offset;
    uint16_t name_hash;
    uint16_t file_count;
    uint32_t first_file_index;
} pc_rarc_directory;

typedef struct pc_rarc_file {
    uint16_t file_id;
    uint16_t name_hash;
    uint32_t flags_and_name_offset;
    uint32_t data_offset;
    uint32_t data_length;
} pc_rarc_file;

typedef struct pc_rarc_view {
    const uint8_t* bytes;
    size_t size;
    size_t info_offset;
    size_t data_offset;
    pc_rarc_header header;
    pc_rarc_info info;
    bool has_file_data;
} pc_rarc_view;

bool pc_rarc_decode_header(pc_rarc_header* header, const void* data, size_t size);
bool pc_rarc_open(pc_rarc_view* view, const void* data, size_t size);
bool pc_rarc_open_metadata(pc_rarc_view* view, const void* data, size_t size);
bool pc_rarc_get_directory(const pc_rarc_view* view, uint32_t index, pc_rarc_directory* directory);
bool pc_rarc_get_file(const pc_rarc_view* view, uint32_t index, pc_rarc_file* file);
const char* pc_rarc_get_string(const pc_rarc_view* view, uint32_t offset);
const void* pc_rarc_get_string_table(const pc_rarc_view* view);
const void* pc_rarc_get_file_data(const pc_rarc_view* view, const pc_rarc_file* file);

#ifdef __cplusplus
}

static_assert(sizeof(pc_rarc_header_disk) == 32);
static_assert(sizeof(pc_rarc_info_disk) == 32);
static_assert(sizeof(pc_rarc_directory_disk) == 16);
static_assert(sizeof(pc_rarc_file_disk) == 20);
#else
_Static_assert(sizeof(pc_rarc_header_disk) == 32, "RARC header layout changed");
_Static_assert(sizeof(pc_rarc_info_disk) == 32, "RARC info layout changed");
_Static_assert(sizeof(pc_rarc_directory_disk) == 16, "RARC directory layout changed");
_Static_assert(sizeof(pc_rarc_file_disk) == 20, "RARC file layout changed");
#endif

#endif
