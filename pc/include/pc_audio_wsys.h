#ifndef PC_AUDIO_WSYS_H
#define PC_AUDIO_WSYS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "jaudio_NES/bx.h"

typedef struct pc_audio_wsys_disk {
    uint8_t magic[4];
    uint8_t size[4];
    uint8_t global_id[4];
    uint8_t unknown[4];
    uint8_t wave_archive_bank_offset[4];
    uint8_t control_group_offset[4];
} pc_audio_wsys_disk;

typedef struct pc_audio_wsys_archive_bank_disk {
    uint8_t magic[4];
    uint8_t count[4];
} pc_audio_wsys_archive_bank_disk;

typedef struct pc_audio_wsys_control_group_disk {
    uint8_t magic[4];
    uint8_t current_scene[4];
    uint8_t count[4];
} pc_audio_wsys_control_group_disk;

typedef struct pc_audio_wsys_scene_disk {
    uint8_t magic[4];
    uint8_t flags[4];
    uint8_t dependency_count[4];
    uint8_t cdf_offset[4];
    uint8_t cex_offset[4];
    uint8_t cst_offset[4];
} pc_audio_wsys_scene_disk;

typedef struct pc_audio_wsys_control_disk {
    uint8_t magic[4];
    uint8_t count[4];
} pc_audio_wsys_control_disk;

typedef struct pc_audio_wsys_wave_id_disk {
    uint8_t id[4];
    uint8_t heap[44];
    uint8_t load_status[4];
    uint8_t wave_offset[4];
} pc_audio_wsys_wave_id_disk;

typedef struct pc_audio_wsys_archive_disk {
    uint8_t file_path[64];
    uint8_t heap[44];
    uint8_t file_load_status[4];
    uint8_t wave_count[4];
} pc_audio_wsys_archive_disk;

typedef struct pc_audio_wsys_wave_disk {
    uint8_t flags[4];
    uint8_t frequency[4];
    uint8_t source_address[4];
    uint8_t length[4];
    uint8_t is_looping[4];
    uint8_t loop_address[4];
    uint8_t loop_start_position[4];
    uint8_t unknown[4];
    uint8_t loop_yn1[2];
    uint8_t loop_yn2[2];
    uint8_t file_load_status_offset[4];
} pc_audio_wsys_wave_disk;

typedef struct pc_audio_wsys_runtime {
    Wsys_* wsys;
    void* private_state;
} pc_audio_wsys_runtime;

bool pc_audio_wsys_decode(pc_audio_wsys_runtime* runtime, const void* data, size_t size);
void pc_audio_wsys_destroy(pc_audio_wsys_runtime* runtime);

#endif
