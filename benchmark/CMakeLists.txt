add_executable(benchmark1 benchmark1.cpp)
target_link_libraries(
    benchmark1
    PRIVATE
        undercurrent
        uc_dev
)