get_filename_component(lab1_TEST_INCLUDE_PATH "." ABSOLUTE)
get_filename_component(lab1_TEST_SOURCE_PATH "./${PROJECT_NAME}" ABSOLUTE)

set(lab1_TEST_TARGET "${PROJECT_NAME}-test")

file(GLOB_RECURSE lab1_TEST_SOURCES CONFIGURE_DEPENDS *.hpp *.cpp)

add_executable(${lab1_TEST_TARGET} ${lab1_TEST_SOURCES})

target_include_directories(${lab1_TEST_TARGET} PUBLIC ${lab1_TEST_INCLUDE_PATH})

target_link_libraries(
    ${lab1_TEST_TARGET} PRIVATE
    ${PROJECT_NAME}
    GTest::gtest
    GTest::gmock
)

gtest_discover_tests(${lab1_TEST_TARGET})
