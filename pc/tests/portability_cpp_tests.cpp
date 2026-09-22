#include <cstdint>
#include <type_traits>

#include "types.h"
#include "PR/ultratypes.h"
#include "pc_portability.h"

static_assert(std::is_same_v<u8, std::uint8_t>);
static_assert(std::is_same_v<s8, std::int8_t>);
static_assert(std::is_same_v<u16, std::uint16_t>);
static_assert(std::is_same_v<s16, std::int16_t>);
static_assert(std::is_same_v<u32, std::uint32_t>);
static_assert(std::is_same_v<s32, std::int32_t>);
static_assert(std::is_same_v<u64, std::uint64_t>);
static_assert(std::is_same_v<s64, std::int64_t>);
static_assert(std::is_same_v<pc_host_addr_t, std::uintptr_t>);

int main() {
    return 0;
}
