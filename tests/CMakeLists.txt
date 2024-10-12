# Find GTest package
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# RIIF Ultrasonic Test
add_executable(test_riif_ultrasonic
    test_ultrasonic_core.cpp
    ${COMMON_TEST_SOURCES}
)

target_include_directories(test_riif_ultrasonic
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/encryption
        ${CMAKE_SOURCE_DIR}/src/pos_protocol
)

target_link_libraries(test_riif_ultrasonic
    PRIVATE
        GTest::GTest
        GTest::Main
        riif_ultrasonic_lib
)

# POS Protocol Test
# add_executable(test_pos_protocol
#     test_pos_protocol.cpp
#     ${COMMON_TEST_SOURCES}
# )

# target_include_directories(test_pos_protocol
#     PRIVATE
#         ${CMAKE_SOURCE_DIR}/include
#         ${CMAKE_SOURCE_DIR}/src
#         ${CMAKE_SOURCE_DIR}/src/encryption
#         ${CMAKE_SOURCE_DIR}/src/pos_protocol
# )

# target_link_libraries(test_pos_protocol
#     PRIVATE
#         GTest::GTest
#         GTest::Main
#         riif_ultrasonic_lib
# )

include(GoogleTest)
gtest_discover_tests(test_riif_ultrasonic)
# gtest_discover_tests(test_pos_protocol)

message(STATUS "RIIF Ultrasonic include dirs: ${riif_ultrasonic_INCLUDE_DIRS}")
message(STATUS "RIIF Ultrasonic libraries: ${riif_ultrasonic_LIBRARIES}")