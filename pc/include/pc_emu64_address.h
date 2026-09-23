#ifndef PC_EMU64_ADDRESS_H
#define PC_EMU64_ADDRESS_H

#include <stdint.h>

#include "pc_pointer_token.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PC_EMU64_SEGMENT_COUNT 16u
#define PC_EMU64_FIRST_SEGMENT 3u

pc_pointer_token_result pc_emu64_resolve_address(uint32_t address,
                                                 const uintptr_t segments[PC_EMU64_SEGMENT_COUNT],
                                                 uintptr_t* resolved_out);

#ifdef __cplusplus
}
#endif

#endif
