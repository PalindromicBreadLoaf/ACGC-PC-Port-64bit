#include <stdint.h>
#include <stdio.h>

#include "pc_acmd_runtime.h"
#include "pc_pointer_token.h"

unsigned int pc_acmd_pack_runtime_ptr(uintptr_t addr, const char* expr, const char* file, int line) {
    uint32_t token;
    pc_pointer_token_result result = pc_pointer_token_pack(PC_POINTER_TOKEN_DOMAIN_ACMD, addr, &token);

    if (result != PC_POINTER_TOKEN_OK) {
        fprintf(stderr, "[ACMD] %s: %s at %s:%d\n", pc_pointer_token_result_name(result), expr, file, line);
        return 0u;
    }
    return token;
}

int pc_acmd_unpack_runtime_ptr(unsigned int packed, uintptr_t* addr_out) {
    pc_pointer_token_result result = pc_pointer_token_resolve(PC_POINTER_TOKEN_DOMAIN_ACMD, packed, addr_out);

    if (result != PC_POINTER_TOKEN_OK) {
        fprintf(stderr, "[ACMD] %s: 0x%08x\n", pc_pointer_token_result_name(result), packed);
    }
    return (int)result;
}
