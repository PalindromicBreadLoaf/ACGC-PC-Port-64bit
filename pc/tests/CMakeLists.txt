add_executable(pc_portability_tests portability_tests.c)

target_include_directories(pc_portability_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_definitions(pc_portability_tests PRIVATE TARGET_PC _LANGUAGE_C)
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
