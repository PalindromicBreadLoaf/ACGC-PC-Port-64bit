#include <stdint.h>
#include <string.h>

#include "pc_audio_wsys.h"
#include "pc_portability.h"

enum {
    FIXTURE_SIZE = 0x200,
    ARCHIVE_BANK_OFFSET = 0x20,
    CONTROL_GROUP_OFFSET = 0x40,
    ARCHIVE_OFFSET = 0x80,
    WAVE_OFFSET = 0x100,
    SCENE_OFFSET = 0x140,
    CONTROL_OFFSET = 0x160,
    WAVE_ID_OFFSET = 0x180
};

static void build_fixture(uint8_t* data) {
    memset(data, 0, FIXTURE_SIZE);
    pc_store_be32(data, UINT32_C(0x57535953));
    pc_store_be32(data + 4, FIXTURE_SIZE);
    pc_store_be32(data + 8, 7);
    pc_store_be32(data + 12, 9);
    pc_store_be32(data + 16, ARCHIVE_BANK_OFFSET);
    pc_store_be32(data + 20, CONTROL_GROUP_OFFSET);

    pc_store_be32(data + ARCHIVE_BANK_OFFSET, UINT32_C(0x57494e46));
    pc_store_be32(data + ARCHIVE_BANK_OFFSET + 4, 1);
    pc_store_be32(data + ARCHIVE_BANK_OFFSET + 8, ARCHIVE_OFFSET);

    pc_store_be32(data + CONTROL_GROUP_OFFSET, UINT32_C(0x57424354));
    pc_store_be32(data + CONTROL_GROUP_OFFSET + 4, UINT32_MAX);
    pc_store_be32(data + CONTROL_GROUP_OFFSET + 8, 1);
    pc_store_be32(data + CONTROL_GROUP_OFFSET + 12, SCENE_OFFSET);

    memcpy(data + ARCHIVE_OFFSET, "fixture.aw", sizeof("fixture.aw"));
    pc_store_be32(data + ARCHIVE_OFFSET + 108, 3);
    pc_store_be32(data + ARCHIVE_OFFSET + 112, 1);
    pc_store_be32(data + ARCHIVE_OFFSET + 116, WAVE_OFFSET);

    data[WAVE_OFFSET] = 0x80;
    data[WAVE_OFFSET + 1] = 2;
    data[WAVE_OFFSET + 2] = 60;
    pc_store_be32(data + WAVE_OFFSET + 4, UINT32_C(0x3fc00000));
    pc_store_be32(data + WAVE_OFFSET + 8, 0x1234);
    pc_store_be32(data + WAVE_OFFSET + 12, 0x5678);
    pc_store_be32(data + WAVE_OFFSET + 16, 1);
    pc_store_be32(data + WAVE_OFFSET + 20, 0x20);
    pc_store_be32(data + WAVE_OFFSET + 24, 0x30);
    pc_store_be32(data + WAVE_OFFSET + 28, 0x40);
    pc_store_be16(data + WAVE_OFFSET + 32, 0x1111);
    pc_store_be16(data + WAVE_OFFSET + 34, 0x2222);

    pc_store_be32(data + SCENE_OFFSET, UINT32_C(0x53434e45));
    pc_store_be32(data + SCENE_OFFSET + 4, 5);
    pc_store_be32(data + SCENE_OFFSET + 8, 1);
    pc_store_be32(data + SCENE_OFFSET + 12, CONTROL_OFFSET);
    pc_store_be32(data + SCENE_OFFSET + 24, 0);

    pc_store_be32(data + CONTROL_OFFSET, UINT32_C(0x432d4446));
    pc_store_be32(data + CONTROL_OFFSET + 4, 1);
    pc_store_be32(data + CONTROL_OFFSET + 8, WAVE_ID_OFFSET);

    pc_store_be32(data + WAVE_ID_OFFSET, UINT32_C(0x12345678));
    pc_store_be32(data + WAVE_ID_OFFSET + 48, 4);
    pc_store_be32(data + WAVE_ID_OFFSET + 52, UINT32_MAX);
}

static int expect_rejected(uint8_t* data, size_t size) {
    pc_audio_wsys_runtime runtime = {0};

    if (pc_audio_wsys_decode(&runtime, data, size)) {
        pc_audio_wsys_destroy(&runtime);
        return 1;
    }
    pc_audio_wsys_destroy(&runtime);
    return 0;
}

int main(void) {
    uint8_t data[FIXTURE_SIZE];
    uint8_t original[FIXTURE_SIZE];
    pc_audio_wsys_runtime runtime = {0};
    WaveArchive_* archive;
    Wave_* wave;
    SCNE_* scene;
    WaveID_* wave_id;

    build_fixture(data);
    memcpy(original, data, sizeof(data));
    if (!pc_audio_wsys_decode(&runtime, data, sizeof(data))) {
        return 1;
    }
    archive = runtime.wsys->waveArcBank->waveGroups[0];
    wave = archive->waves[0];
    scene = runtime.wsys->ctrlGroup->scenes[0];
    wave_id = scene->cdf->waveIDs[0];
    if (runtime.wsys->globalID != 7 || runtime.wsys->_0C != 9 || strcmp(archive->filePath, "fixture.aw") != 0 ||
        archive->fileLoadStatus != 3 || archive->waveCount != 1 || wave->compBlockIdx != 2 || wave->key != 60 ||
        wave->_04 != 1.5f || wave->srcAddress != 0x1234 || wave->length != 0x5678 || wave->loopYN1 != 0x1111 ||
        wave->loopYN2 != 0x2222 || scene->_04 != 5 || scene->_08 != 1 || scene->_18[0] != 0 || scene->cex != NULL ||
        scene->cst != NULL || wave_id->id != UINT32_C(0x12345678) || wave_id->loadStatus != 4 ||
        (uintptr_t)wave_id->data != UINT32_MAX || memcmp(data, original, sizeof(data)) != 0) {
        return 1;
    }
    pc_audio_wsys_destroy(&runtime);
    if (runtime.wsys != NULL || runtime.private_state != NULL) {
        return 1;
    }

    build_fixture(data);
    pc_store_be32(data + 4, FIXTURE_SIZE + 1);
    if (expect_rejected(data, sizeof(data))) {
        return 1;
    }
    build_fixture(data);
    pc_store_be32(data + 16, FIXTURE_SIZE - 4);
    if (expect_rejected(data, sizeof(data))) {
        return 1;
    }
    build_fixture(data);
    memset(data + ARCHIVE_OFFSET, 'x', 64);
    if (expect_rejected(data, sizeof(data))) {
        return 1;
    }
    build_fixture(data);
    pc_store_be32(data + CONTROL_OFFSET, UINT32_C(0x42414421));
    if (expect_rejected(data, sizeof(data))) {
        return 1;
    }
    build_fixture(data);
    pc_store_be32(data + CONTROL_GROUP_OFFSET + 8, 2);
    if (expect_rejected(data, sizeof(data))) {
        return 1;
    }
    build_fixture(data);
    pc_store_be32(data + ARCHIVE_OFFSET + 116, FIXTURE_SIZE - 20);
    if (expect_rejected(data, sizeof(data))) {
        return 1;
    }
    return 0;
}
