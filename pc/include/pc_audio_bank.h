#ifndef PC_AUDIO_BANK_H
#define PC_AUDIO_BANK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "jaudio_NES/audiostruct.h"

typedef struct pc_audio_wtstr_disk {
    uint8_t wavetable_offset[4];
    uint8_t tuning[4];
} pc_audio_wtstr_disk;

typedef struct pc_audio_voicetable_disk {
    uint8_t is_relocated;
    uint8_t normal_range_low;
    uint8_t normal_range_high;
    uint8_t adsr_decay_idx;
    uint8_t envelope_offset[4];
    pc_audio_wtstr_disk low_pitch;
    pc_audio_wtstr_disk normal_pitch;
    pc_audio_wtstr_disk high_pitch;
} pc_audio_voicetable_disk;

typedef struct pc_audio_perctable_disk {
    uint8_t adsr_decay_idx;
    uint8_t pan;
    uint8_t is_relocated;
    uint8_t padding;
    pc_audio_wtstr_disk tuned_sample;
    uint8_t envelope_offset[4];
} pc_audio_perctable_disk;

typedef struct pc_audio_wavetable_disk {
    uint8_t flags_and_size[4];
    uint8_t sample_offset[4];
    uint8_t loop_offset[4];
    uint8_t book_offset[4];
} pc_audio_wavetable_disk;

typedef struct pc_audio_loop_disk {
    uint8_t loop_start[4];
    uint8_t loop_end[4];
    uint8_t count[4];
    uint8_t sample_end[4];
} pc_audio_loop_disk;

typedef struct pc_audio_book_disk {
    uint8_t order[4];
    uint8_t predictor_count[4];
} pc_audio_book_disk;

typedef struct pc_audio_env_disk {
    uint8_t delay[2];
    uint8_t value[2];
} pc_audio_env_disk;

typedef struct pc_audio_bank_runtime {
    voicetable** instruments;
    perctable** percussion;
    percvoicetable* effects;
    smzwavetable** preload_wavetables;
    size_t preload_wavetable_count;
    void* private_state;
} pc_audio_bank_runtime;

typedef struct pc_audio_wave_media {
    const void* wave0_base;
    const void* wave1_base;
    uint32_t wave0_medium;
    uint32_t wave1_medium;
} pc_audio_wave_media;

bool pc_audio_bank_decode(pc_audio_bank_runtime* runtime, const void* data, size_t size, uint32_t instrument_count,
                          uint32_t percussion_count, uint32_t effect_count, const pc_audio_wave_media* media);
void pc_audio_bank_destroy(pc_audio_bank_runtime* runtime);

#endif
