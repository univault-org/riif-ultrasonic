cmake_minimum_required(VERSION 3.10)
project(RiifUltrasonic)

# Find GTest
find_package(GTest REQUIRED)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Enable testing
enable_testing()

set(CMAKE_CXX_STANDARD 17)

# Define source files for the library
set(RIIF_ULTRASONIC_SOURCES
    include/fft_impl.hpp
    src/core/riif_ultrasonic.cpp
    src/encryption/encryption.cpp
    src/pos_protocol/pos_protocol.cpp
)

# Create the library
add_library(riif_ultrasonic_lib STATIC ${RIIF_ULTRASONIC_SOURCES})

# Set include directories for the library
target_include_directories(riif_ultrasonic_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Tests
add_subdirectory(tests)

