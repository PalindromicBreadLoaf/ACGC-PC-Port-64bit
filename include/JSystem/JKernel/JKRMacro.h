#ifndef JKRMARCO_H
#define JKRMACRO_H

#ifdef __cplusplus
#ifdef TARGET_PC
#define JKR_ISALIGNED(addr, alignment) ((((uintptr_t)(addr)) & (((uintptr_t)(alignment)) - 1)) == 0)
#define JKR_ALIGN(addr, alignment) (((uintptr_t)(addr)) & (~(((uintptr_t)(alignment)) - 1)))
#else
#define JKR_ISALIGNED(addr, alignment) ((((u32)addr) & (((u32)alignment) - 1)) == 0)
#define JKR_ALIGN(addr, alignment) (((u32)addr) & (~(((u32)alignment) - 1)))
#endif
#define JKR_ISALIGNED32(addr) (JKR_ISALIGNED(addr, 32))

#define JKR_ISNOTALIGNED(addr, alignment) (!JKR_ISALIGNED(addr, alignment))
#define JKR_ISNOTALIGNED32(addr) (JKR_ISNOTALIGNED(addr, 32))

#define JKR_ALIGN32(addr) (JKR_ALIGN(addr, 32))
#endif

#endif
