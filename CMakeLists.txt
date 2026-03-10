cmake_minimum_required(VERSION 3.10)
project(poly_ops C ASM)

# ML-DSA constants
set(MLDSA_N 256)
set(MLDSA_Q 8380417)

# Platform detection - always enable SVE since we build both variants
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PLATFORM_FLAGS "-march=armv8-a+sve")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(PLATFORM_FLAGS "-arch arm64")
endif()

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PLATFORM_FLAGS} -Wall -Wextra -O2")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${PLATFORM_FLAGS} -Wall")

# Include paths
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src/include)

# Source files - always include all assembly sources for both NEON and SVE
set(ASM_SOURCES
    src/poly_c.c
    src/poly_caddq_asm.S
    src/poly_caddq_asm_sve.S
    src/poly_chknorm_asm.S
    src/poly_chknorm_asm_sve.S
)

# Test executable - single binary containing both NEON and SVE implementations
set(TARGET_NAME "test_poly_ops")

add_executable(${TARGET_NAME} test.c ${ASM_SOURCES})
target_compile_definitions(${TARGET_NAME} PRIVATE -DHAVE_SVE)

# Default target
add_custom_target(run
    COMMAND ${TARGET_NAME}
    DEPENDS ${TARGET_NAME}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running ${TARGET_NAME}"
)
