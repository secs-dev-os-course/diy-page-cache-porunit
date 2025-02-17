get_filename_component(lab1_BENCH_SOURCE_PATH "./${PROJECT_NAME}" ABSOLUTE)

set(lab1_BENCH_TARGET_BASE "${PROJECT_NAME}-bench")

get_filename_component(lab1_BENCH_INCLUDE_PATH "." ABSOLUTE)
get_filename_component(lab1_BENCH_BENCH_SOURCE_PATH "./${PROJECT_NAME}" ABSOLUTE)

# lib
file(GLOB_RECURSE lab1_BENCH_SOURCES CONFIGURE_DEPENDS *.hpp *.cpp)
set(lab1_BENCH_LIB_SOURCES ${lab1_BENCH_SOURCES})
add_library(${PROJECT_NAME}-bench STATIC ${lab1_BENCH_LIB_SOURCES})
target_include_directories(${PROJECT_NAME}-bench PUBLIC ${lab1_BENCH_INCLUDE_PATH})
target_link_libraries(
    ${PROJECT_NAME}-bench
    lab2_cache_lib
    benchmark::benchmark
)

# app
set(lab1_BENCH_APP_SOURCES lab1/app/Main.cpp)
add_executable(${PROJECT_NAME}-bench-app ${lab1_BENCH_APP_SOURCES})
set_property(TARGET ${PROJECT_NAME}-bench-app PROPERTY OUTPUT_NAME ${PROJECT_NAME}-bench)
target_link_libraries(
    ${PROJECT_NAME}-bench-app PRIVATE
    ${PROJECT_NAME}-bench
    lab2_cache_lib
    benchmark::benchmark 
)
