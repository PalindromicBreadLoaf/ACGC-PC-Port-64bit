#ifndef PC_ASSET_DECODE_H
#define PC_ASSET_DECODE_H

#include <stddef.h>

typedef enum pc_asset_format {
    PC_ASSET_RAW = 0,
    PC_ASSET_BE16 = 1,
    PC_ASSET_VTX_BE = 2,
    PC_ASSET_BE32 = 3,
    PC_ASSET_GFX_BE = 4
} pc_asset_format;

int pc_asset_decode(void* destination, size_t destination_size,
                    const void* source, size_t source_size,
                    pc_asset_format format);

#endif
