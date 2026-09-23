#ifndef PC_POINTER_TOKEN_H
#define PC_POINTER_TOKEN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum pc_pointer_token_domain {
    PC_POINTER_TOKEN_DOMAIN_GBI = 1,
    PC_POINTER_TOKEN_DOMAIN_ACMD = 2
} pc_pointer_token_domain;

typedef enum pc_pointer_token_result {
    PC_POINTER_TOKEN_OK = 0,
    PC_POINTER_TOKEN_NOT_TOKEN,
    PC_POINTER_TOKEN_INVALID_DOMAIN,
    PC_POINTER_TOKEN_WRONG_DOMAIN,
    PC_POINTER_TOKEN_STALE,
    PC_POINTER_TOKEN_EXHAUSTED
} pc_pointer_token_result;

#define PC_POINTER_TOKEN_CAPACITY 8192u
#define PC_POINTER_TOKEN_NAMESPACE_MASK UINT32_C(0xF0000000)
#define PC_POINTER_TOKEN_NAMESPACE UINT32_C(0xF0000000)

int pc_pointer_token_is_token(uint32_t value);
const char* pc_pointer_token_result_name(pc_pointer_token_result result);
pc_pointer_token_result pc_pointer_token_pack(pc_pointer_token_domain domain, uintptr_t pointer,
                                              uint32_t* token_out);
pc_pointer_token_result pc_pointer_token_resolve(pc_pointer_token_domain domain, uint32_t token,
                                                 uintptr_t* pointer_out);
pc_pointer_token_result pc_pointer_token_release(pc_pointer_token_domain domain, uint32_t token);
void pc_pointer_token_reset(pc_pointer_token_domain domain);
uint32_t pc_pointer_token_active_count(pc_pointer_token_domain domain);

#ifdef __cplusplus
}
#endif

#endif
