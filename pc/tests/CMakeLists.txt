add_executable(pc_portability_tests
    portability_tests.c
    ../src/pc_gbi_runtime.c
    ../src/pc_acmd_runtime.c
    ../src/pc_pointer_token.c
    ../src/pc_emu64_address.c
    ../../src/TwoHeadArena.c
    ../../src/gamealloc.c
)

target_include_directories(pc_portability_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_definitions(pc_portability_tests PRIVATE TARGET_PC _LANGUAGE_C F3DEX_GBI_2 PC_GBI_LAYOUT_TEST)
target_compile_options(pc_portability_tests PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wno-comment
    -Wpointer-to-int-cast
    -Wint-to-pointer-cast
)

add_test(NAME pc_portability COMMAND pc_portability_tests)

add_executable(pc_portability_cpp_tests portability_cpp_tests.cpp)

target_include_directories(pc_portability_cpp_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_definitions(pc_portability_cpp_tests PRIVATE TARGET_PC _LANGUAGE_C_PLUS_PLUS)
target_compile_options(pc_portability_cpp_tests PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wno-comment
)

add_test(NAME pc_portability_cpp COMMAND pc_portability_cpp_tests)

add_executable(pc_allocator_cpp_tests
    allocator_cpp_tests.cpp
    allocator_test_stubs.cpp
    ../../src/static/JSystem/JKernel/JKRHeap.cpp
    ../../src/static/JSystem/JKernel/JKRExpHeap.cpp
    ../../src/static/JSystem/JKernel/JKRDisposer.cpp
    ../../src/static/JSystem/JSupport/JSUList.cpp
)

target_include_directories(pc_allocator_cpp_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_definitions(pc_allocator_cpp_tests PRIVATE TARGET_PC _LANGUAGE_C_PLUS_PLUS PC_GBI_LAYOUT_TEST)
target_compile_options(pc_allocator_cpp_tests PRIVATE
    -Wall
    -Wextra
    -Wconversion
    -Wno-comment
    -Wno-multichar
    -Wno-unknown-pragmas
    -Wno-unused-function
    -Wno-unused-parameter
    -ffunction-sections
    -fdata-sections
)
target_link_options(pc_allocator_cpp_tests PRIVATE -Wl,--gc-sections)

add_test(NAME pc_allocator_cpp COMMAND pc_allocator_cpp_tests)

add_executable(pc_libc64_allocator_tests
    libc64_allocator_tests.c
    libc64_allocator_test_stubs.c
    ../../src/static/libc64/__osMalloc.c
)

target_include_directories(pc_libc64_allocator_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_definitions(pc_libc64_allocator_tests PRIVATE
    TARGET_PC
    _LANGUAGE_C
    F3DEX_GBI_2
    PC_GBI_LAYOUT_TEST
)
target_compile_options(pc_libc64_allocator_tests PRIVATE
    -Wall
    -Wextra
    -Wconversion
    -Wno-comment
    -Wno-multichar
    -Wno-unknown-pragmas
    -Wno-unused-function
    -Wno-unused-parameter
    -Werror=pointer-to-int-cast
    -Werror=int-to-pointer-cast
)

add_test(NAME pc_libc64_allocator COMMAND pc_libc64_allocator_tests)

add_executable(pc_serialized_layout_tests
    serialized_layout_tests.c
    serialized_layout_cpp_tests.cpp
)

target_include_directories(pc_serialized_layout_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_definitions(pc_serialized_layout_tests PRIVATE
    TARGET_PC
    F3DEX_GBI_2
    PC_GBI_LAYOUT_TEST
    $<$<COMPILE_LANGUAGE:C>:_LANGUAGE_C>
    $<$<COMPILE_LANGUAGE:CXX>:_LANGUAGE_C_PLUS_PLUS>
)
target_compile_options(pc_serialized_layout_tests PRIVATE
    -Wall
    -Wextra
    -Wno-comment
    $<$<COMPILE_LANGUAGE:C>:-Werror=pointer-to-int-cast>
    $<$<COMPILE_LANGUAGE:C>:-Werror=int-to-pointer-cast>
)

add_test(NAME pc_serialized_layout COMMAND pc_serialized_layout_tests)

add_executable(pc_aram_tests
    aram_tests.c
    ../src/pc_aram.c
)

target_include_directories(pc_aram_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_definitions(pc_aram_tests PRIVATE TARGET_PC _LANGUAGE_C)
target_compile_options(pc_aram_tests PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wno-comment
    -Wpointer-to-int-cast
    -Wint-to-pointer-cast
)

add_test(NAME pc_aram COMMAND pc_aram_tests)

add_executable(pc_rarc_tests
    rarc_tests.c
    ../src/pc_rarc.c
)

target_include_directories(pc_rarc_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_options(pc_rarc_tests PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
)

add_test(NAME pc_rarc COMMAND pc_rarc_tests)

add_executable(pc_executable_tests
    executable_tests.c
    ../src/pc_executable.c
)

target_include_directories(pc_executable_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_options(pc_executable_tests PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
)

add_test(NAME pc_executable COMMAND pc_executable_tests)

add_executable(pc_audio_bank_tests
    audio_bank_tests.c
    ../src/pc_audio_bank.c
)

target_include_directories(pc_audio_bank_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_definitions(pc_audio_bank_tests PRIVATE TARGET_PC _LANGUAGE_C F3DEX_GBI_2 PC_GBI_LAYOUT_TEST)
target_compile_options(pc_audio_bank_tests PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wno-comment
    -Wno-multichar
)

add_test(NAME pc_audio_bank COMMAND pc_audio_bank_tests)

add_executable(pc_asset_decode_tests
    asset_decode_tests.c
    ../src/pc_asset_decode.c
)

target_include_directories(pc_asset_decode_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_options(pc_asset_decode_tests PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
)

add_test(NAME pc_asset_decode COMMAND pc_asset_decode_tests)

add_executable(pc_save_roundtrip_tests
    save_roundtrip_tests.c
    ../src/pc_save_bswap.c
)

target_include_directories(pc_save_roundtrip_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_definitions(pc_save_roundtrip_tests PRIVATE TARGET_PC _LANGUAGE_C F3DEX_GBI_2 PC_GBI_LAYOUT_TEST)
target_compile_options(pc_save_roundtrip_tests PRIVATE
    -Wall
    -Wextra
    -Wno-comment
    -Wno-multichar
    -Wno-unknown-pragmas
    -Wno-unused-function
    -Werror=pointer-to-int-cast
    -Werror=int-to-pointer-cast
)

add_test(NAME pc_save_roundtrip COMMAND pc_save_roundtrip_tests)
