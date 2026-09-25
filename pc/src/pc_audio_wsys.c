#include "pc_audio_wsys.h"

#include <stdlib.h>
#include <string.h>

#include "pc_portability.h"

typedef enum pc_audio_wsys_kind {
    PC_AUDIO_WSYS_ARCHIVE_BANK,
    PC_AUDIO_WSYS_CONTROL_GROUP,
    PC_AUDIO_WSYS_ARCHIVE,
    PC_AUDIO_WSYS_SCENE,
    PC_AUDIO_WSYS_CONTROL,
    PC_AUDIO_WSYS_WAVE_ID,
    PC_AUDIO_WSYS_WAVE
} pc_audio_wsys_kind;

typedef struct pc_audio_wsys_object {
    uint32_t offset;
    pc_audio_wsys_kind kind;
    void* value;
} pc_audio_wsys_object;

typedef struct pc_audio_wsys_state {
    const uint8_t* data;
    size_t size;
    void** allocations;
    size_t allocation_count;
    size_t allocation_capacity;
    pc_audio_wsys_object* objects;
    size_t object_count;
    size_t object_capacity;
} pc_audio_wsys_state;

_Static_assert(sizeof(pc_audio_wsys_disk) == 24, "audio WSYS disk layout changed");
_Static_assert(sizeof(pc_audio_wsys_archive_bank_disk) == 8, "audio WINF disk layout changed");
_Static_assert(sizeof(pc_audio_wsys_control_group_disk) == 12, "audio WBCT disk layout changed");
_Static_assert(sizeof(pc_audio_wsys_scene_disk) == 24, "audio SCNE disk layout changed");
_Static_assert(sizeof(pc_audio_wsys_control_disk) == 8, "audio control disk layout changed");
_Static_assert(sizeof(pc_audio_wsys_wave_id_disk) == 56, "audio wave ID disk layout changed");
_Static_assert(sizeof(pc_audio_wsys_archive_disk) == 116, "audio archive disk layout changed");
_Static_assert(sizeof(pc_audio_wsys_wave_disk) == 40, "audio wave disk layout changed");

static bool pc_audio_wsys_range(const pc_audio_wsys_state* state, uint32_t offset, size_t length) {
    return offset <= state->size && length <= state->size - offset;
}

static bool pc_audio_wsys_array_range(const pc_audio_wsys_state* state, uint32_t offset, uint32_t count,
                                      size_t header_size, size_t entry_size) {
    size_t entries = count;

    if (entries > (SIZE_MAX - header_size) / entry_size) {
        return false;
    }
    return pc_audio_wsys_range(state, offset, header_size + entries * entry_size);
}

static bool pc_audio_wsys_allocation_size(size_t header_size, uint32_t count, size_t entry_size, size_t* size) {
    size_t entries_size = (size_t)count * entry_size;

    if ((count != 0 && entries_size / count != entry_size) || header_size > SIZE_MAX - entries_size) {
        return false;
    }
    *size = header_size + entries_size;
    return true;
}

static void* pc_audio_wsys_allocate(pc_audio_wsys_state* state, size_t size) {
    void* value;
    void** allocations;
    size_t capacity;

    if (size == 0) {
        size = 1;
    }
    value = calloc(1, size);
    if (value == NULL) {
        return NULL;
    }
    if (state->allocation_count == state->allocation_capacity) {
        capacity = state->allocation_capacity == 0 ? 32 : state->allocation_capacity * 2;
        if (capacity < state->allocation_capacity || capacity > SIZE_MAX / sizeof(*allocations)) {
            free(value);
            return NULL;
        }
        allocations = realloc(state->allocations, capacity * sizeof(*allocations));
        if (allocations == NULL) {
            free(value);
            return NULL;
        }
        state->allocations = allocations;
        state->allocation_capacity = capacity;
    }
    state->allocations[state->allocation_count++] = value;
    return value;
}

static void* pc_audio_wsys_find_object(const pc_audio_wsys_state* state, uint32_t offset, pc_audio_wsys_kind kind) {
    size_t i;

    for (i = 0; i < state->object_count; ++i) {
        if (state->objects[i].offset == offset && state->objects[i].kind == kind) {
            return state->objects[i].value;
        }
    }
    return NULL;
}

static bool pc_audio_wsys_add_object(pc_audio_wsys_state* state, uint32_t offset, pc_audio_wsys_kind kind,
                                     void* value) {
    pc_audio_wsys_object* objects;
    size_t capacity;

    if (state->object_count == state->object_capacity) {
        capacity = state->object_capacity == 0 ? 32 : state->object_capacity * 2;
        if (capacity < state->object_capacity || capacity > SIZE_MAX / sizeof(*objects)) {
            return false;
        }
        objects = realloc(state->objects, capacity * sizeof(*objects));
        if (objects == NULL) {
            return false;
        }
        state->objects = objects;
        state->object_capacity = capacity;
    }
    state->objects[state->object_count].offset = offset;
    state->objects[state->object_count].kind = kind;
    state->objects[state->object_count].value = value;
    state->object_count++;
    return true;
}

static float pc_audio_wsys_float(const uint8_t* disk) {
    uint32_t bits = pc_load_be32(disk);
    float value;

    memcpy(&value, &bits, sizeof(value));
    return value;
}

static Wave_* pc_audio_wsys_decode_wave(pc_audio_wsys_state* state, uint32_t offset) {
    const uint8_t* disk;
    Wave_* result;

    if (offset == 0 || !pc_audio_wsys_range(state, offset, sizeof(pc_audio_wsys_wave_disk))) {
        return NULL;
    }
    result = pc_audio_wsys_find_object(state, offset, PC_AUDIO_WSYS_WAVE);
    if (result != NULL) {
        return result;
    }
    result = pc_audio_wsys_allocate(state, sizeof(*result));
    if (result == NULL || !pc_audio_wsys_add_object(state, offset, PC_AUDIO_WSYS_WAVE, result)) {
        return NULL;
    }
    disk = state->data + offset;
    result->_00 = disk[0];
    result->compBlockIdx = disk[1];
    result->key = disk[2];
    result->_04 = pc_audio_wsys_float(disk + 4);
    result->srcAddress = (int32_t)pc_load_be32(disk + 8);
    result->length = (int32_t)pc_load_be32(disk + 12);
    result->isLooping = (int32_t)pc_load_be32(disk + 16);
    result->loopAddress = (int32_t)pc_load_be32(disk + 20);
    result->loopStartPosition = (int32_t)pc_load_be32(disk + 24);
    result->_1C = (int32_t)pc_load_be32(disk + 28);
    result->loopYN1 = (int16_t)pc_load_be16(disk + 32);
    result->loopYN2 = (int16_t)pc_load_be16(disk + 34);
    return result;
}

static WaveArchive_* pc_audio_wsys_decode_archive(pc_audio_wsys_state* state, uint32_t offset) {
    const uint8_t* disk;
    WaveArchive_* result;
    uint32_t count;
    size_t size;
    size_t i;

    if (offset == 0 || !pc_audio_wsys_range(state, offset, sizeof(pc_audio_wsys_archive_disk))) {
        return NULL;
    }
    result = pc_audio_wsys_find_object(state, offset, PC_AUDIO_WSYS_ARCHIVE);
    if (result != NULL) {
        return result;
    }
    disk = state->data + offset;
    count = pc_load_be32(disk + 112);
    if (!pc_audio_wsys_array_range(state, offset, count, sizeof(pc_audio_wsys_archive_disk), 4) ||
        !pc_audio_wsys_allocation_size(offsetof(WaveArchive_, waves), count, sizeof(*result->waves), &size)) {
        return NULL;
    }
    if (memchr(disk, '\0', sizeof(result->filePath)) == NULL) {
        return NULL;
    }
    result = pc_audio_wsys_allocate(state, size);
    if (result == NULL || !pc_audio_wsys_add_object(state, offset, PC_AUDIO_WSYS_ARCHIVE, result)) {
        return NULL;
    }
    memcpy(result->filePath, disk, sizeof(result->filePath));
    result->fileLoadStatus = pc_load_be32(disk + 108);
    result->waveCount = (int32_t)count;
    for (i = 0; i < count; ++i) {
        result->waves[i] = pc_audio_wsys_decode_wave(state, pc_load_be32(disk + 116 + i * 4));
        if (result->waves[i] == NULL) {
            return NULL;
        }
    }
    return result;
}

static WaveArchiveBank_* pc_audio_wsys_decode_archive_bank(pc_audio_wsys_state* state, uint32_t offset) {
    const uint8_t* disk;
    WaveArchiveBank_* result;
    uint32_t count;
    size_t size;
    size_t i;

    if (offset == 0 || !pc_audio_wsys_range(state, offset, sizeof(pc_audio_wsys_archive_bank_disk))) {
        return NULL;
    }
    result = pc_audio_wsys_find_object(state, offset, PC_AUDIO_WSYS_ARCHIVE_BANK);
    if (result != NULL) {
        return result;
    }
    disk = state->data + offset;
    if (pc_load_be32(disk) != UINT32_C(0x57494e46)) {
        return NULL;
    }
    count = pc_load_be32(disk + 4);
    if (!pc_audio_wsys_array_range(state, offset, count, sizeof(pc_audio_wsys_archive_bank_disk), 4) ||
        !pc_audio_wsys_allocation_size(offsetof(WaveArchiveBank_, waveGroups), count,
                                       sizeof(*result->waveGroups), &size)) {
        return NULL;
    }
    result = pc_audio_wsys_allocate(state, size);
    if (result == NULL || !pc_audio_wsys_add_object(state, offset, PC_AUDIO_WSYS_ARCHIVE_BANK, result)) {
        return NULL;
    }
    result->magic = (int32_t)UINT32_C(0x57494e46);
    result->count = (int32_t)count;
    for (i = 0; i < count; ++i) {
        result->waveGroups[i] = pc_audio_wsys_decode_archive(state, pc_load_be32(disk + 8 + i * 4));
        if (result->waveGroups[i] == NULL) {
            return NULL;
        }
    }
    return result;
}

static WaveID_* pc_audio_wsys_decode_wave_id(pc_audio_wsys_state* state, uint32_t offset) {
    const uint8_t* disk;
    WaveID_* result;
    uint32_t data_offset;

    if (offset == 0 || !pc_audio_wsys_range(state, offset, sizeof(pc_audio_wsys_wave_id_disk))) {
        return NULL;
    }
    result = pc_audio_wsys_find_object(state, offset, PC_AUDIO_WSYS_WAVE_ID);
    if (result != NULL) {
        return result;
    }
    result = pc_audio_wsys_allocate(state, sizeof(*result));
    if (result == NULL || !pc_audio_wsys_add_object(state, offset, PC_AUDIO_WSYS_WAVE_ID, result)) {
        return NULL;
    }
    disk = state->data + offset;
    result->id = pc_load_be32(disk);
    result->loadStatus = pc_load_be32(disk + 48);
    data_offset = pc_load_be32(disk + 52);
    if (data_offset == UINT32_MAX) {
        result->data = (Wave_*)(uintptr_t)UINT32_MAX;
    } else if (data_offset != 0) {
        result->data = pc_audio_wsys_decode_wave(state, data_offset);
        if (result->data == NULL) {
            return NULL;
        }
    }
    return result;
}

static Ctrl_* pc_audio_wsys_decode_control(pc_audio_wsys_state* state, uint32_t offset) {
    const uint8_t* disk;
    Ctrl_* result;
    uint32_t magic;
    uint32_t count;
    size_t size;
    size_t i;

    if (offset == 0) {
        return NULL;
    }
    result = pc_audio_wsys_find_object(state, offset, PC_AUDIO_WSYS_CONTROL);
    if (result != NULL) {
        return result;
    }
    if (!pc_audio_wsys_range(state, offset, sizeof(pc_audio_wsys_control_disk))) {
        return NULL;
    }
    disk = state->data + offset;
    magic = pc_load_be32(disk);
    if (magic != UINT32_C(0x432d4446) && magic != UINT32_C(0x432d4558) && magic != UINT32_C(0x432d5354)) {
        return NULL;
    }
    count = pc_load_be32(disk + 4);
    if (!pc_audio_wsys_array_range(state, offset, count, sizeof(pc_audio_wsys_control_disk), 4) ||
        !pc_audio_wsys_allocation_size(offsetof(Ctrl_, waveIDs), count, sizeof(*result->waveIDs), &size)) {
        return NULL;
    }
    result = pc_audio_wsys_allocate(state, size);
    if (result == NULL || !pc_audio_wsys_add_object(state, offset, PC_AUDIO_WSYS_CONTROL, result)) {
        return NULL;
    }
    result->magic = (int32_t)magic;
    result->count = (int32_t)count;
    for (i = 0; i < count; ++i) {
        result->waveIDs[i] = pc_audio_wsys_decode_wave_id(state, pc_load_be32(disk + 8 + i * 4));
        if (result->waveIDs[i] == NULL) {
            return NULL;
        }
    }
    return result;
}

static SCNE_* pc_audio_wsys_decode_scene(pc_audio_wsys_state* state, uint32_t offset) {
    const uint8_t* disk;
    SCNE_* result;
    uint32_t dependency_count;
    uint32_t control_offset;
    size_t size;
    size_t i;

    if (offset == 0 || !pc_audio_wsys_range(state, offset, sizeof(pc_audio_wsys_scene_disk))) {
        return NULL;
    }
    result = pc_audio_wsys_find_object(state, offset, PC_AUDIO_WSYS_SCENE);
    if (result != NULL) {
        return result;
    }
    disk = state->data + offset;
    if (pc_load_be32(disk) != UINT32_C(0x53434e45)) {
        return NULL;
    }
    dependency_count = pc_load_be32(disk + 8);
    if (!pc_audio_wsys_array_range(state, offset, dependency_count, sizeof(pc_audio_wsys_scene_disk), 4) ||
        !pc_audio_wsys_allocation_size(offsetof(SCNE_, _18), dependency_count, sizeof(*result->_18), &size)) {
        return NULL;
    }
    result = pc_audio_wsys_allocate(state, size);
    if (result == NULL || !pc_audio_wsys_add_object(state, offset, PC_AUDIO_WSYS_SCENE, result)) {
        return NULL;
    }
    result->magic = (int32_t)UINT32_C(0x53434e45);
    result->_04 = pc_load_be32(disk + 4);
    result->_08 = dependency_count;
    control_offset = pc_load_be32(disk + 12);
    result->cdf = control_offset == 0 ? NULL : pc_audio_wsys_decode_control(state, control_offset);
    if (control_offset != 0 && result->cdf == NULL) {
        return NULL;
    }
    control_offset = pc_load_be32(disk + 16);
    result->cex = control_offset == 0 ? NULL : pc_audio_wsys_decode_control(state, control_offset);
    if (control_offset != 0 && result->cex == NULL) {
        return NULL;
    }
    control_offset = pc_load_be32(disk + 20);
    result->cst = control_offset == 0 ? NULL : pc_audio_wsys_decode_control(state, control_offset);
    if (control_offset != 0 && result->cst == NULL) {
        return NULL;
    }
    for (i = 0; i < dependency_count; ++i) {
        result->_18[i] = (int32_t)pc_load_be32(disk + 24 + i * 4);
    }
    return result;
}

static CtrlGroup_* pc_audio_wsys_decode_control_group(pc_audio_wsys_state* state, uint32_t offset) {
    const uint8_t* disk;
    CtrlGroup_* result;
    uint32_t count;
    size_t size;
    size_t i;

    if (offset == 0 || !pc_audio_wsys_range(state, offset, sizeof(pc_audio_wsys_control_group_disk))) {
        return NULL;
    }
    result = pc_audio_wsys_find_object(state, offset, PC_AUDIO_WSYS_CONTROL_GROUP);
    if (result != NULL) {
        return result;
    }
    disk = state->data + offset;
    if (pc_load_be32(disk) != UINT32_C(0x57424354)) {
        return NULL;
    }
    count = pc_load_be32(disk + 8);
    if (!pc_audio_wsys_array_range(state, offset, count, sizeof(pc_audio_wsys_control_group_disk), 4) ||
        !pc_audio_wsys_allocation_size(offsetof(CtrlGroup_, scenes), count, sizeof(*result->scenes), &size)) {
        return NULL;
    }
    result = pc_audio_wsys_allocate(state, size);
    if (result == NULL || !pc_audio_wsys_add_object(state, offset, PC_AUDIO_WSYS_CONTROL_GROUP, result)) {
        return NULL;
    }
    result->magic = (int32_t)UINT32_C(0x57424354);
    result->_04 = pc_load_be32(disk + 4);
    result->count = (int32_t)count;
    for (i = 0; i < count; ++i) {
        result->scenes[i] = pc_audio_wsys_decode_scene(state, pc_load_be32(disk + 12 + i * 4));
        if (result->scenes[i] == NULL) {
            return NULL;
        }
    }
    return result;
}

bool pc_audio_wsys_decode(pc_audio_wsys_runtime* runtime, const void* data, size_t size) {
    pc_audio_wsys_state* state;
    const uint8_t* disk = data;
    uint32_t declared_size;

    if (runtime == NULL || data == NULL || size < sizeof(pc_audio_wsys_disk)) {
        return false;
    }
    memset(runtime, 0, sizeof(*runtime));
    if (pc_load_be32(disk) != UINT32_C(0x57535953)) {
        return false;
    }
    declared_size = pc_load_be32(disk + 4);
    if (declared_size < sizeof(pc_audio_wsys_disk) || declared_size > size) {
        return false;
    }
    state = calloc(1, sizeof(*state));
    if (state == NULL) {
        return false;
    }
    runtime->private_state = state;
    state->data = data;
    state->size = declared_size;
    runtime->wsys = pc_audio_wsys_allocate(state, sizeof(*runtime->wsys));
    if (runtime->wsys == NULL) {
        goto fail;
    }
    runtime->wsys->magic = (int32_t)UINT32_C(0x57535953);
    runtime->wsys->size = (int32_t)declared_size;
    runtime->wsys->globalID = (int32_t)pc_load_be32(disk + 8);
    runtime->wsys->_0C = (int32_t)pc_load_be32(disk + 12);
    runtime->wsys->waveArcBank = pc_audio_wsys_decode_archive_bank(state, pc_load_be32(disk + 16));
    runtime->wsys->ctrlGroup = pc_audio_wsys_decode_control_group(state, pc_load_be32(disk + 20));
    if (runtime->wsys->waveArcBank == NULL || runtime->wsys->ctrlGroup == NULL ||
        runtime->wsys->waveArcBank->count != runtime->wsys->ctrlGroup->count) {
        goto fail;
    }
    return true;

fail:
    pc_audio_wsys_destroy(runtime);
    return false;
}

void pc_audio_wsys_destroy(pc_audio_wsys_runtime* runtime) {
    pc_audio_wsys_state* state;
    size_t i;

    if (runtime == NULL) {
        return;
    }
    state = runtime->private_state;
    if (state != NULL) {
        for (i = 0; i < state->allocation_count; ++i) {
            free(state->allocations[i]);
        }
        free(state->allocations);
        free(state->objects);
        free(state);
    }
    memset(runtime, 0, sizeof(*runtime));
}
