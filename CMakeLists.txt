cmake_minimum_required(VERSION 3.16)
project(EcoNetCEIMPhoenixWater LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ECONET_BUILD_TESTS "Build EcoNet-CEIM tests" ON)

add_library(ceim_core
    src/ceim_core.cpp
    src/qpudata_csv.cpp
)

target_include_directories(ceim_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(phoenix_ceim
    src/main_cli.cpp
)

target_link_libraries(phoenix_ceim
    PRIVATE ceim_core
)

if(ECONET_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
