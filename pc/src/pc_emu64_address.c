#include "pc_emu64_address.h"

#include <stddef.h>

pc_pointer_token_result pc_emu64_resolve_address(uint32_t address,
                                                 const uintptr_t segments[PC_EMU64_SEGMENT_COUNT],
                                                 uintptr_t* resolved_out) {
    pc_pointer_token_result result;
    uintptr_t resolved;
    uint32_t segment;

    if (resolved_out == NULL) {
        return PC_POINTER_TOKEN_INVALID_DOMAIN;
    }

    result = pc_pointer_token_resolve(PC_POINTER_TOKEN_DOMAIN_GBI, address, &resolved);
    if (result == PC_POINTER_TOKEN_OK) {
        *resolved_out = resolved;
        return PC_POINTER_TOKEN_OK;
    }
    if (result != PC_POINTER_TOKEN_NOT_TOKEN) {
        return result;
    }

    segment = address >> 24;
    if (segments != NULL && segment >= PC_EMU64_FIRST_SEGMENT && segment < PC_EMU64_SEGMENT_COUNT &&
        segments[segment] != 0u) {
        uintptr_t offset = (uintptr_t)(address & UINT32_C(0x00ffffff));

        if (segments[segment] > UINTPTR_MAX - offset) {
            return PC_POINTER_TOKEN_INVALID_DOMAIN;
        }
        *resolved_out = segments[segment] + offset;
    } else {
        *resolved_out = (uintptr_t)address;
    }
    return PC_POINTER_TOKEN_OK;
}
