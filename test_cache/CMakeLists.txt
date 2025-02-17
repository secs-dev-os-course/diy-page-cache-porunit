get_filename_component(lab2_TEST_INCLUDE_PATH "." ABSOLUTE)
get_filename_component(lab2_TEST_SOURCE_PATH "./lab2" ABSOLUTE)

set(lab2_TEST_TARGET "lab2-test")

file(GLOB_RECURSE lab2_TEST_SOURCES CONFIGURE_DEPENDS *.hpp *.cpp)

add_executable(${lab2_TEST_TARGET} ${lab2_TEST_SOURCES})

target_include_directories(${lab2_TEST_TARGET} PUBLIC ${lab2_TEST_INCLUDE_PATH})

target_link_libraries(
    ${lab2_TEST_TARGET} PRIVATE
    lab2_cache_lib
    GTest::gtest
    GTest::gmock
)

gtest_discover_tests(${lab2_TEST_TARGET})
