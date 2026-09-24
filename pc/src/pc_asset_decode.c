#include "pc_asset_decode.h"

#include <stdint.h>
#include <string.h>

#include "pc_portability.h"

static int pc_asset_record_size(pc_asset_format format, size_t* record_size) {
    switch (format) {
        case PC_ASSET_RAW:
            *record_size = 1;
            return 1;
        case PC_ASSET_BE16:
            *record_size = 2;
            return 1;
        case PC_ASSET_VTX_BE:
            *record_size = 16;
            return 1;
        case PC_ASSET_BE32:
            *record_size = 4;
            return 1;
        case PC_ASSET_GFX_BE:
            *record_size = 8;
            return 1;
    }
    return 0;
}

int pc_asset_decode(void* destination, size_t destination_size,
                    const void* source, size_t source_size,
                    pc_asset_format format) {
    const uint8_t* input = (const uint8_t*)source;
    uint8_t* output = (uint8_t*)destination;
    size_t record_size;
    size_t offset;

    if ((destination == NULL && destination_size != 0) ||
        (source == NULL && source_size != 0) || destination_size != source_size ||
        !pc_asset_record_size(format, &record_size) || source_size % record_size != 0) {
        return 0;
    }

    if (source_size == 0) {
        return 1;
    }

    if (format == PC_ASSET_RAW) {
        memmove(output, input, source_size);
        return 1;
    }

    if (format == PC_ASSET_BE16) {
        for (offset = 0; offset < source_size; offset += 2) {
            uint16_t value = pc_load_be16(input + offset);
            memcpy(output + offset, &value, sizeof(value));
        }
        return 1;
    }

    if (format == PC_ASSET_BE32 || format == PC_ASSET_GFX_BE) {
        for (offset = 0; offset < source_size; offset += 4) {
            uint32_t value = pc_load_be32(input + offset);
            memcpy(output + offset, &value, sizeof(value));
        }
        return 1;
    }

    for (offset = 0; offset < source_size; offset += 16) {
        size_t field_offset;
        for (field_offset = 0; field_offset < 12; field_offset += 2) {
            uint16_t value = pc_load_be16(input + offset + field_offset);
            memcpy(output + offset + field_offset, &value, sizeof(value));
        }
        memcpy(output + offset + 12, input + offset + 12, 4);
    }
    return 1;
}
