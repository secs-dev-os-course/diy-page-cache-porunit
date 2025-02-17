cmake_minimum_required(VERSION 3.28)

project(lab1 VERSION 0.1.0)

option(lab1_DEVELOPER "Enable ${PROJECT_NAME} developer mode" ON)
option(lab1_BENCHMARK "Enable ${PROJECT_NAME} benchmark module" ON)
option(lab2_CACHE "Enable cache module" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(cmake/CompileOptions.cmake)

if(lab1_BENCHMARK)
    find_package(GBenchmark REQUIRED)
    add_subdirectory(benchmark)
endif()


if(lab2_CACHE)
    add_subdirectory(cache)
    find_package(GTest REQUIRED)
    add_subdirectory(test_cache)
endif()
