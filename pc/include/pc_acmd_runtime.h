#ifndef PC_ACMD_RUNTIME_H
#define PC_ACMD_RUNTIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

unsigned int pc_acmd_pack_runtime_ptr(uintptr_t addr, const char* expr, const char* file, int line);
int pc_acmd_unpack_runtime_ptr(unsigned int packed, uintptr_t* addr_out);

#ifdef __cplusplus
}
#endif

#endif
