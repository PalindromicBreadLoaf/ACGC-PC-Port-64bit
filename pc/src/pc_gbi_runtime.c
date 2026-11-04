#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pc_pointer_token.h"

static void pc_gbi_report_token_error(pc_pointer_token_result result, const char* expr, const char* file, int line) {
    fprintf(stderr, "[GBI] %s: %s at %s:%d\n", pc_pointer_token_result_name(result), expr, file, line);
}

/* Game code also spells segmented addresses as pointers, e.g. (Mtx*)0x0D000040. */
static int pc_gbi_is_segmented(uintptr_t addr) {
    return addr >= UINT32_C(0x01000000) && addr < UINT32_C(0x10000000);
}

unsigned int pc_gbi_pack_runtime_ptr(uintptr_t addr, int is_ptr, const char* expr, const char* file, int line) {
    uint32_t token;
    pc_pointer_token_result result;

    if (!is_ptr || pc_gbi_is_segmented(addr)) {
        if (addr > UINT32_MAX || pc_pointer_token_is_token((uint32_t)addr)) {
            pc_gbi_report_token_error(PC_POINTER_TOKEN_INVALID_DOMAIN, expr, file, line);
            return 0u;
        }
        return (unsigned int)addr;
    }

    result = pc_pointer_token_pack(PC_POINTER_TOKEN_DOMAIN_GBI, addr, &token);
    if (result != PC_POINTER_TOKEN_OK) {
        pc_gbi_report_token_error(result, expr, file, line);
        return 0u;
    }

    return token;
}

int pc_gbi_unpack_runtime_ptr(unsigned int packed, uintptr_t* addr_out) {
    return (int)pc_pointer_token_resolve(PC_POINTER_TOKEN_DOMAIN_GBI, packed, addr_out);
}

int pc_gbi_relocate_static_command(void* command) {
    uint32_t* words = command;
    uintptr_t value;
    uint32_t packed;
    pc_pointer_token_result result;

    if (command == NULL) {
        return (int)PC_POINTER_TOKEN_INVALID_DOMAIN;
    }
    if (words[1] != UINT32_C(0xfc000000) && words[1] != UINT32_C(0xfc000001)) {
        return (int)PC_POINTER_TOKEN_NOT_TOKEN;
    }

    memcpy(&value, words + 2, sizeof(value));
    if (words[1] == UINT32_C(0xfc000000) && !pc_gbi_is_segmented(value)) {
        result = pc_pointer_token_pack_permanent(PC_POINTER_TOKEN_DOMAIN_GBI, value, &packed);
    } else if (value > UINT32_MAX || pc_pointer_token_is_token((uint32_t)value)) {
        result = PC_POINTER_TOKEN_INVALID_DOMAIN;
    } else {
        packed = (uint32_t)value;
        result = PC_POINTER_TOKEN_OK;
    }

    if (result != PC_POINTER_TOKEN_OK) {
        return (int)result;
    }

    words[1] = packed;
    words[2] = 0u;
    words[3] = 0u;
    return (int)PC_POINTER_TOKEN_OK;
}
