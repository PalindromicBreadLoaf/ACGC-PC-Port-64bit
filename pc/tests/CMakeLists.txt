add_executable(pc_portability_tests
    portability_tests.c
    ../../src/TwoHeadArena.c
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
