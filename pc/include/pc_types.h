#ifndef PC_TYPES_H
#define PC_TYPES_H

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef float              f32;
typedef double             f64;
typedef int                BOOL;

#ifdef __cplusplus
#define PC_TYPES_STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
#define PC_TYPES_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#endif

PC_TYPES_STATIC_ASSERT(sizeof(u8) == 1, "u8 must be 8 bits");
PC_TYPES_STATIC_ASSERT(sizeof(s8) == 1, "s8 must be 8 bits");
PC_TYPES_STATIC_ASSERT(sizeof(u16) == 2, "u16 must be 16 bits");
PC_TYPES_STATIC_ASSERT(sizeof(s16) == 2, "s16 must be 16 bits");
PC_TYPES_STATIC_ASSERT(sizeof(u32) == 4, "u32 must be 32 bits");
PC_TYPES_STATIC_ASSERT(sizeof(s32) == 4, "s32 must be 32 bits");
PC_TYPES_STATIC_ASSERT(sizeof(u64) == 8, "u64 must be 64 bits");
PC_TYPES_STATIC_ASSERT(sizeof(s64) == 8, "s64 must be 64 bits");
PC_TYPES_STATIC_ASSERT(sizeof(f32) == 4, "f32 must be 32 bits");
PC_TYPES_STATIC_ASSERT(sizeof(f64) == 8, "f64 must be 64 bits");

#undef PC_TYPES_STATIC_ASSERT

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif /* PC_TYPES_H */
