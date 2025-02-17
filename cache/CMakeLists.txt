get_filename_component(lab2_INCLUDE_PATH "." ABSOLUTE)
get_filename_component(lab2_SOURCE_PATH "./lab2" ABSOLUTE)

file(GLOB_RECURSE lab2_SOURCES CONFIGURE_DEPENDS *.hpp *.cpp)

set(lab2_LIB_SOURCES ${lab2_SOURCES})

add_library(lab2_cache_lib SHARED ${lab2_LIB_SOURCES})
target_include_directories(lab2_cache_lib PUBLIC ${lab2_INCLUDE_PATH})
