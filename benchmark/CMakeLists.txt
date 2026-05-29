add_executable(benchmark1 benchmark1.cpp)
target_link_libraries(
    benchmark1
    PRIVATE
        undercurrent
        uc_dev
)
add_executable(seg_stack_benchmark1 seg_stack_benchmark1.cpp)
target_link_libraries(
    seg_stack_benchmark1
    PRIVATE
        undercurrent
        uc_dev
)
