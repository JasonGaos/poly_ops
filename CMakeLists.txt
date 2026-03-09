cmake_minimum_required(VERSION 3.10)
project(poly_ops C ASM)

# ML-DSA constants
set(MLDSA_N 256)
set(MLDSA_Q 8380417)

# Option to build SVE variant instead of NEON
option(BUILD_SVE "Build SVE variant instead of NEON" OFF)

# Platform detection
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PLATFORM_FLAGS "-march=armv8-a")
    if(BUILD_SVE)
        set(PLATFORM_FLAGS "${PLATFORM_FLAGS}+sve")
        add_definitions(-DHAVE_SVE)
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(PLATFORM_FLAGS "-arch arm64")
    if(BUILD_SVE)
        add_definitions(-DHAVE_SVE)
    endif()
endif()

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PLATFORM_FLAGS} -Wall -Wextra -O2")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${PLATFORM_FLAGS} -Wall")

# Include paths
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src/include)

# Source files
set(ASM_SOURCES
    src/poly_c.c
)

# Select assembly sources based on variant
if(BUILD_SVE)
    set(ASM_SOURCES ${ASM_SOURCES}
        src/poly_caddq_asm_sve.S
        src/poly_chknorm_asm_sve.S
    )
    set(TARGET_NAME "test_poly_ops_sve")
else()
    set(ASM_SOURCES ${ASM_SOURCES}
        src/poly_caddq_asm.S
        src/poly_chknorm_asm.S
    )
    set(TARGET_NAME "test_poly_ops_neon")
endif()

# Test executable
add_executable(${TARGET_NAME} test.c ${ASM_SOURCES})

# Default target
add_custom_target(run
    COMMAND ${TARGET_NAME}
    DEPENDS ${TARGET_NAME}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running ${TARGET_NAME}"
)

# Build both variants
add_custom_target(build_both
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target test_poly_ops_neon
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target test_poly_ops_sve
    COMMENT "Building both NEON and SVE variants"
)
