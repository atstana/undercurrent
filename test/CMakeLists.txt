add_executable(test1 test1.cpp)
target_link_libraries(
    test1
    PRIVATE
        Catch2::Catch2WithMain
        undercurrent
)
add_executable(segmented_iterator_test1 segmented_iterator_test1.cpp)
target_link_libraries(
    segmented_iterator_test1
    PRIVATE
        Catch2::Catch2WithMain
        uc_dev
        undercurrent
)
add_executable(seg_stack_test1 seg_stack_test1.cpp)
target_link_libraries(
    seg_stack_test1
    PRIVATE
        Catch2::Catch2WithMain
        uc_dev
        undercurrent
)
