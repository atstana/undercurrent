add_executable(test1 test1.cpp)
target_link_libraries(
    test1
    PRIVATE
        undercurrent
        Catch2::Catch2WithMain
)