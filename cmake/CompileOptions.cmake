set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

if(lab1_DEVELOPER)
    add_compile_options(-Werror)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

if(lab1_OPT)
    add_compile_options(-O3 -march=native)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()