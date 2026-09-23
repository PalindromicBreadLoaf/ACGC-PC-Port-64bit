#include <stdint.h>
#include <stdio.h>

#include "pc_pointer_token.h"

static void pc_gbi_report_token_error(pc_pointer_token_result result, const char* expr, const char* file, int line) {
    fprintf(stderr, "[GBI] %s: %s at %s:%d\n", pc_pointer_token_result_name(result), expr, file, line);
}

unsigned int pc_gbi_pack_runtime_ptr(uintptr_t addr, int is_ptr, const char* expr, const char* file, int line) {
    uint32_t token;
    pc_pointer_token_result result;

    if (!is_ptr) {
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
