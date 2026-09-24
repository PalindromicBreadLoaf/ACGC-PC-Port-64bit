#include "pc_audio_bank.h"

#include <stdlib.h>
#include <string.h>

#include "pc_portability.h"

typedef enum pc_audio_object_kind {
    PC_AUDIO_WAVETABLE,
    PC_AUDIO_LOOP,
    PC_AUDIO_BOOK,
    PC_AUDIO_ENVELOPE
} pc_audio_object_kind;

typedef struct pc_audio_object {
    uint32_t offset;
    pc_audio_object_kind kind;
    void* value;
} pc_audio_object;

typedef struct pc_audio_bank_state {
    const uint8_t* data;
    size_t size;
    pc_audio_wave_media media;
    void** allocations;
    size_t allocation_count;
    size_t allocation_capacity;
    pc_audio_object* objects;
    size_t object_count;
    size_t object_capacity;
} pc_audio_bank_state;

_Static_assert(sizeof(pc_audio_wtstr_disk) == 8, "audio wtstr disk layout changed");
_Static_assert(sizeof(pc_audio_voicetable_disk) == 32, "audio voice disk layout changed");
_Static_assert(sizeof(pc_audio_perctable_disk) == 16, "audio percussion disk layout changed");
_Static_assert(sizeof(pc_audio_wavetable_disk) == 16, "audio wavetable disk layout changed");
_Static_assert(sizeof(pc_audio_loop_disk) == 16, "audio loop disk layout changed");
_Static_assert(sizeof(pc_audio_book_disk) == 8, "audio book disk layout changed");
_Static_assert(sizeof(pc_audio_env_disk) == 4, "audio envelope disk layout changed");

static bool pc_audio_range(const pc_audio_bank_state* state, uint32_t offset, size_t length) {
    return offset <= state->size && length <= state->size - offset;
}

static void* pc_audio_allocate(pc_audio_bank_state* state, size_t size) {
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

static void* pc_audio_find_object(const pc_audio_bank_state* state, uint32_t offset, pc_audio_object_kind kind) {
    size_t i;

    for (i = 0; i < state->object_count; ++i) {
        if (state->objects[i].offset == offset && state->objects[i].kind == kind) {
            return state->objects[i].value;
        }
    }
    return NULL;
}

static bool pc_audio_add_object(pc_audio_bank_state* state, uint32_t offset, pc_audio_object_kind kind, void* value) {
    pc_audio_object* objects;
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

static float pc_audio_float(const uint8_t* disk) {
    uint32_t bits = pc_load_be32(disk);
    float value;

    memcpy(&value, &bits, sizeof(value));
    return value;
}

static envdat* pc_audio_decode_envelope(pc_audio_bank_state* state, uint32_t offset) {
    envdat* result;
    size_t count;
    size_t i;
    bool terminated = false;

    if (offset == 0) {
        return NULL;
    }
    result = pc_audio_find_object(state, offset, PC_AUDIO_ENVELOPE);
    if (result != NULL) {
        return result;
    }
    for (count = 0; count < 67; ++count) {
        int16_t delay;
        if (!pc_audio_range(state, offset, (count + 1) * sizeof(pc_audio_env_disk))) {
            return NULL;
        }
        delay = (int16_t)pc_load_be16(state->data + offset + count * sizeof(pc_audio_env_disk));
        if (delay >= -3 && delay <= 0) {
            count++;
            terminated = true;
            break;
        }
    }
    if (!terminated) {
        return NULL;
    }
    result = pc_audio_allocate(state, count * sizeof(*result));
    if (result == NULL || !pc_audio_add_object(state, offset, PC_AUDIO_ENVELOPE, result)) {
        return NULL;
    }
    for (i = 0; i < count; ++i) {
        const uint8_t* disk = state->data + offset + i * sizeof(pc_audio_env_disk);
        result[i].delay = (int16_t)pc_load_be16(disk);
        result[i].value = (int16_t)pc_load_be16(disk + 2);
    }
    return result;
}

static adpcmloop* pc_audio_decode_loop(pc_audio_bank_state* state, uint32_t offset) {
    const uint8_t* disk;
    adpcmloop* result;
    uint32_t count;
    size_t size;
    size_t i;

    if (offset == 0) {
        return NULL;
    }
    result = pc_audio_find_object(state, offset, PC_AUDIO_LOOP);
    if (result != NULL) {
        return result;
    }
    if (!pc_audio_range(state, offset, sizeof(pc_audio_loop_disk))) {
        return NULL;
    }
    disk = state->data + offset;
    count = pc_load_be32(disk + 8);
    size = count == 0 ? offsetof(adpcmloop, predictor_state) : sizeof(adpcmloop);
    if (!pc_audio_range(state, offset, size)) {
        return NULL;
    }
    result = pc_audio_allocate(state, sizeof(*result));
    if (result == NULL || !pc_audio_add_object(state, offset, PC_AUDIO_LOOP, result)) {
        return NULL;
    }
    result->loop_start = pc_load_be32(disk);
    result->loop_end = pc_load_be32(disk + 4);
    result->count = count;
    result->sample_end = pc_load_be32(disk + 12);
    if (count != 0) {
        for (i = 0; i < 16; ++i) {
            result->predictor_state[i] = (int16_t)pc_load_be16(disk + 16 + i * 2);
        }
    }
    return result;
}

static adpcmbook* pc_audio_decode_book(pc_audio_bank_state* state, uint32_t offset) {
    const uint8_t* disk;
    adpcmbook* result;
    uint32_t order;
    uint32_t predictors;
    size_t entry_count;
    size_t size;
    size_t i;

    if (offset == 0 || !pc_audio_range(state, offset, sizeof(pc_audio_book_disk))) {
        return NULL;
    }
    result = pc_audio_find_object(state, offset, PC_AUDIO_BOOK);
    if (result != NULL) {
        return result;
    }
    disk = state->data + offset;
    order = pc_load_be32(disk);
    predictors = pc_load_be32(disk + 4);
    if (order == 0 || predictors == 0 || order > INT32_MAX || predictors > INT32_MAX ||
        (size_t)predictors > SIZE_MAX / ((size_t)order * 8)) {
        return NULL;
    }
    entry_count = (size_t)order * predictors * 8;
    if (entry_count > (SIZE_MAX - sizeof(*result)) / sizeof(int16_t)) {
        return NULL;
    }
    size = sizeof(*result) + entry_count * sizeof(int16_t);
    if (!pc_audio_range(state, offset, size)) {
        return NULL;
    }
    result = pc_audio_allocate(state, size);
    if (result == NULL || !pc_audio_add_object(state, offset, PC_AUDIO_BOOK, result)) {
        return NULL;
    }
    result->order = (int32_t)order;
    result->n_predictors = (int32_t)predictors;
    for (i = 0; i < entry_count; ++i) {
        result->codebook[i] = (int16_t)pc_load_be16(disk + 8 + i * 2);
    }
    return result;
}

static smzwavetable* pc_audio_decode_wavetable(pc_audio_bank_state* state, uint32_t offset) {
    const uint8_t* disk;
    smzwavetable* result;
    uint32_t flags;
    uint32_t sample_offset;
    uintptr_t sample_base = 0;

    if (offset == 0 || !pc_audio_range(state, offset, sizeof(pc_audio_wavetable_disk))) {
        return NULL;
    }
    result = pc_audio_find_object(state, offset, PC_AUDIO_WAVETABLE);
    if (result != NULL) {
        return result;
    }
    result = pc_audio_allocate(state, sizeof(*result));
    if (result == NULL || !pc_audio_add_object(state, offset, PC_AUDIO_WAVETABLE, result)) {
        return NULL;
    }
    disk = state->data + offset;
    flags = pc_load_be32(disk);
    result->bit31 = (flags & UINT32_C(0x80000000)) != 0;
    result->codec = (flags >> 28) & 7;
    result->medium = (flags >> 26) & 3;
    result->bit26 = (flags >> 25) & 1;
    result->size = flags & UINT32_C(0x00ffffff);
    sample_offset = pc_load_be32(disk + 4);
    if (result->size != 0) {
        result->loop = pc_audio_decode_loop(state, pc_load_be32(disk + 8));
        result->book = pc_audio_decode_book(state, pc_load_be32(disk + 12));
        if (result->loop == NULL || result->book == NULL) {
            return NULL;
        }
        if (result->medium == MEDIUM_RAM) {
            sample_base = (uintptr_t)state->media.wave0_base;
            result->medium = (uint8_t)(state->media.wave0_medium & 3);
        } else if (result->medium == MEDIUM_DISK) {
            sample_base = (uintptr_t)state->media.wave1_base;
            result->medium = (uint8_t)(state->media.wave1_medium & 3);
        }
    }
    result->sample = (uint8_t*)(sample_base + sample_offset);
    result->is_relocated = TRUE;
    return result;
}

static bool pc_audio_decode_wtstr(pc_audio_bank_state* state, wtstr* result, const uint8_t* disk) {
    uint32_t offset = pc_load_be32(disk);

    result->wavetable = offset == 0 ? NULL : pc_audio_decode_wavetable(state, offset);
    result->tuning = pc_audio_float(disk + 4);
    return offset == 0 || result->wavetable != NULL;
}

static voicetable* pc_audio_decode_voice(pc_audio_bank_state* state, uint32_t offset) {
    const uint8_t* disk;
    voicetable* result;

    if (offset == 0 || !pc_audio_range(state, offset, sizeof(pc_audio_voicetable_disk))) {
        return NULL;
    }
    disk = state->data + offset;
    result = pc_audio_allocate(state, sizeof(*result));
    if (result == NULL) {
        return NULL;
    }
    result->is_relocated = TRUE;
    result->normal_range_low = disk[1];
    result->normal_range_high = disk[2];
    result->adsr_decay_idx = disk[3];
    result->envelope = pc_audio_decode_envelope(state, pc_load_be32(disk + 4));
    if (result->envelope == NULL || !pc_audio_decode_wtstr(state, &result->low_pitch_tuned_sample, disk + 8) ||
        !pc_audio_decode_wtstr(state, &result->normal_pitch_tuned_sample, disk + 16) ||
        !pc_audio_decode_wtstr(state, &result->high_pitch_tuned_sample, disk + 24)) {
        return NULL;
    }
    return result;
}

static perctable* pc_audio_decode_percussion(pc_audio_bank_state* state, uint32_t offset) {
    const uint8_t* disk;
    perctable* result;

    if (offset == 0 || !pc_audio_range(state, offset, sizeof(pc_audio_perctable_disk))) {
        return NULL;
    }
    disk = state->data + offset;
    result = pc_audio_allocate(state, sizeof(*result));
    if (result == NULL) {
        return NULL;
    }
    result->adsr_decay_idx = disk[0];
    result->pan = disk[1];
    result->is_relocated = TRUE;
    result->envelope = pc_audio_decode_envelope(state, pc_load_be32(disk + 12));
    if (result->envelope == NULL || !pc_audio_decode_wtstr(state, &result->tuned_sample, disk + 4)) {
        return NULL;
    }
    return result;
}

static bool pc_audio_collect_preloads(pc_audio_bank_runtime* runtime, pc_audio_bank_state* state) {
    size_t i;
    size_t count = 0;

    for (i = 0; i < state->object_count; ++i) {
        smzwavetable* wavetable;
        if (state->objects[i].kind != PC_AUDIO_WAVETABLE) {
            continue;
        }
        wavetable = state->objects[i].value;
        if (wavetable->bit26 && wavetable->medium != MEDIUM_RAM) {
            count++;
        }
    }
    if (count == 0) {
        return true;
    }
    runtime->preload_wavetables = pc_audio_allocate(state, count * sizeof(*runtime->preload_wavetables));
    if (runtime->preload_wavetables == NULL) {
        return false;
    }
    for (i = 0; i < state->object_count; ++i) {
        smzwavetable* wavetable;
        if (state->objects[i].kind != PC_AUDIO_WAVETABLE) {
            continue;
        }
        wavetable = state->objects[i].value;
        if (wavetable->bit26 && wavetable->medium != MEDIUM_RAM) {
            runtime->preload_wavetables[runtime->preload_wavetable_count++] = wavetable;
        }
    }
    return true;
}

bool pc_audio_bank_decode(pc_audio_bank_runtime* runtime, const void* data, size_t size, uint32_t instrument_count,
                          uint32_t percussion_count, uint32_t effect_count, const pc_audio_wave_media* media) {
    pc_audio_bank_state* state;
    uint32_t offset;
    size_t control_count;
    size_t i;

    if (runtime == NULL || data == NULL || media == NULL || instrument_count > 126) {
        return false;
    }
    memset(runtime, 0, sizeof(*runtime));
    state = calloc(1, sizeof(*state));
    if (state == NULL) {
        return false;
    }
    runtime->private_state = state;
    state->data = data;
    state->size = size;
    state->media = *media;
    control_count = 2 + instrument_count;
    if (control_count > size / 4) {
        goto fail;
    }
    runtime->instruments = pc_audio_allocate(state, instrument_count * sizeof(*runtime->instruments));
    runtime->percussion = pc_audio_allocate(state, percussion_count * sizeof(*runtime->percussion));
    runtime->effects = pc_audio_allocate(state, effect_count * sizeof(*runtime->effects));
    if ((instrument_count != 0 && runtime->instruments == NULL) ||
        (percussion_count != 0 && runtime->percussion == NULL) || (effect_count != 0 && runtime->effects == NULL)) {
        goto fail;
    }
    offset = pc_load_be32(state->data);
    if (percussion_count != 0) {
        if (offset == 0 || !pc_audio_range(state, offset, percussion_count * 4)) {
            goto fail;
        }
        for (i = 0; i < percussion_count; ++i) {
            uint32_t entry_offset = pc_load_be32(state->data + offset + i * 4);
            if (entry_offset != 0 && (runtime->percussion[i] = pc_audio_decode_percussion(state, entry_offset)) == NULL) {
                goto fail;
            }
        }
    }
    offset = pc_load_be32(state->data + 4);
    if (effect_count != 0) {
        if (offset == 0 || !pc_audio_range(state, offset, effect_count * sizeof(pc_audio_wtstr_disk))) {
            goto fail;
        }
        for (i = 0; i < effect_count; ++i) {
            if (!pc_audio_decode_wtstr(state, &runtime->effects[i].tuned_sample,
                                      state->data + offset + i * sizeof(pc_audio_wtstr_disk))) {
                goto fail;
            }
        }
    }
    for (i = 0; i < instrument_count; ++i) {
        offset = pc_load_be32(state->data + 8 + i * 4);
        if (offset != 0 && (runtime->instruments[i] = pc_audio_decode_voice(state, offset)) == NULL) {
            goto fail;
        }
    }
    if (!pc_audio_collect_preloads(runtime, state)) {
        goto fail;
    }
    return true;

fail:
    pc_audio_bank_destroy(runtime);
    return false;
}

void pc_audio_bank_destroy(pc_audio_bank_runtime* runtime) {
    pc_audio_bank_state* state;
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
