#ifndef PC_PORTABILITY_H
#define PC_PORTABILITY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uintptr_t pc_host_addr_t;
typedef uint32_t gc_addr32_t;
typedef uint32_t n64_segaddr_t;
typedef uint32_t aram_addr_t;
typedef uint32_t dvd_offset_t;
typedef uint32_t pc_gbi_handle_t;

static inline bool pc_u32_from_host_addr(pc_host_addr_t address, uint32_t* result) {
    if (address > UINT32_MAX) {
        return false;
    }

    *result = (uint32_t)address;
    return true;
}

static inline uint16_t pc_load_be16(const void* source) {
    const uint8_t* bytes = (const uint8_t*)source;
    return (uint16_t)(((uint16_t)bytes[0] << 8) | (uint16_t)bytes[1]);
}

static inline uint32_t pc_load_be32(const void* source) {
    const uint8_t* bytes = (const uint8_t*)source;
    return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) | ((uint32_t)bytes[2] << 8) |
           (uint32_t)bytes[3];
}

static inline uint64_t pc_load_be64(const void* source) {
    const uint8_t* bytes = (const uint8_t*)source;
    return ((uint64_t)bytes[0] << 56) | ((uint64_t)bytes[1] << 48) | ((uint64_t)bytes[2] << 40) |
           ((uint64_t)bytes[3] << 32) | ((uint64_t)bytes[4] << 24) | ((uint64_t)bytes[5] << 16) |
           ((uint64_t)bytes[6] << 8) | (uint64_t)bytes[7];
}

static inline void pc_store_be16(void* destination, uint16_t value) {
    uint8_t* bytes = (uint8_t*)destination;
    bytes[0] = (uint8_t)(value >> 8);
    bytes[1] = (uint8_t)value;
}

static inline void pc_store_be32(void* destination, uint32_t value) {
    uint8_t* bytes = (uint8_t*)destination;
    bytes[0] = (uint8_t)(value >> 24);
    bytes[1] = (uint8_t)(value >> 16);
    bytes[2] = (uint8_t)(value >> 8);
    bytes[3] = (uint8_t)value;
}

static inline void pc_store_be64(void* destination, uint64_t value) {
    uint8_t* bytes = (uint8_t*)destination;
    bytes[0] = (uint8_t)(value >> 56);
    bytes[1] = (uint8_t)(value >> 48);
    bytes[2] = (uint8_t)(value >> 40);
    bytes[3] = (uint8_t)(value >> 32);
    bytes[4] = (uint8_t)(value >> 24);
    bytes[5] = (uint8_t)(value >> 16);
    bytes[6] = (uint8_t)(value >> 8);
    bytes[7] = (uint8_t)value;
}

#endif
