cmake_minimum_required(VERSION 4.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(undercurrent VERSION 0.1.0)
add_subdirectory(source)

if (PROJECT_IS_TOP_LEVEL)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.14.0
    )
    FetchContent_MakeAvailable(Catch2)

    add_subdirectory(test)
    add_subdirectory(benchmark)
endif()