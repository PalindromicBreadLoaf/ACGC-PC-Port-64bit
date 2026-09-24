#include <stdint.h>
#include <string.h>

#include "pc_audio_bank.h"
#include "pc_portability.h"

static void build_fixture(uint8_t* data, size_t size) {
    size_t i;

    memset(data, 0, size);
    pc_store_be32(data, 0x20);
    pc_store_be32(data + 4, 0x28);
    pc_store_be32(data + 8, 0x40);
    pc_store_be32(data + 0x20, 0x60);
    pc_store_be32(data + 0x28, 0x80);
    pc_store_be32(data + 0x2c, UINT32_C(0x3fc00000));

    data[0x41] = 0;
    data[0x42] = 0x7f;
    data[0x43] = 9;
    pc_store_be32(data + 0x44, 0xe0);
    pc_store_be32(data + 0x50, 0x80);
    pc_store_be32(data + 0x54, UINT32_C(0x3f800000));

    data[0x60] = 4;
    data[0x61] = 64;
    pc_store_be32(data + 0x64, 0x80);
    pc_store_be32(data + 0x68, UINT32_C(0x3f000000));
    pc_store_be32(data + 0x6c, 0xe0);

    pc_store_be32(data + 0x80, ((uint32_t)MEDIUM_RAM << 26) | (UINT32_C(1) << 25) | 0x20);
    pc_store_be32(data + 0x84, 0x10);
    pc_store_be32(data + 0x88, 0x90);
    pc_store_be32(data + 0x8c, 0xc0);

    pc_store_be32(data + 0x90, 3);
    pc_store_be32(data + 0x94, 99);
    pc_store_be32(data + 0x98, 1);
    pc_store_be32(data + 0x9c, 120);
    for (i = 0; i < 16; ++i) {
        pc_store_be16(data + 0xa0 + i * 2, (uint16_t)(i + 1));
    }
    pc_store_be32(data + 0xc0, 1);
    pc_store_be32(data + 0xc4, 1);
    for (i = 0; i < 8; ++i) {
        pc_store_be16(data + 0xc8 + i * 2, (uint16_t)(0x100 + i));
    }
    pc_store_be16(data + 0xe0, 10);
    pc_store_be16(data + 0xe2, 0x1234);
    pc_store_be16(data + 0xe4, 0);
    pc_store_be16(data + 0xe6, 0);
}

int main(void) {
    uint8_t data[256];
    uint8_t original[256];
    uint8_t wave[64];
    pc_audio_bank_runtime runtime = {0};
    pc_audio_wave_media media = {wave, NULL, MEDIUM_CART, MEDIUM_DISK};
    smzwavetable* wavetable;

    build_fixture(data, sizeof(data));
    memcpy(original, data, sizeof(data));
    if (!pc_audio_bank_decode(&runtime, data, sizeof(data), 1, 1, 1, &media)) {
        return 1;
    }
    wavetable = runtime.instruments[0]->normal_pitch_tuned_sample.wavetable;
    if (runtime.percussion[0]->tuned_sample.wavetable != wavetable ||
        runtime.effects[0].tuned_sample.wavetable != wavetable || wavetable->sample != wave + 0x10 ||
        wavetable->loop->loop_end != 99 || wavetable->loop->predictor_state[15] != 16 ||
        wavetable->book->codebook[7] != 0x107 || runtime.instruments[0]->envelope[0].value != 0x1234 ||
        runtime.instruments[0]->envelope != runtime.percussion[0]->envelope || runtime.preload_wavetable_count != 1 ||
        runtime.preload_wavetables[0] != wavetable ||
        memcmp(data, original, sizeof(data)) != 0) {
        return 1;
    }
    pc_audio_bank_destroy(&runtime);
    if (runtime.private_state != NULL) {
        return 1;
    }

    build_fixture(data, sizeof(data));
    pc_store_be32(data + 0x88, 0xf0);
    if (pc_audio_bank_decode(&runtime, data, 0xf8, 1, 1, 1, &media)) {
        return 1;
    }
    pc_audio_bank_destroy(&runtime);
    return 0;
}
