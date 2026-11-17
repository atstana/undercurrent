function(uc_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(
        ${name}
        PRIVATE
            Catch2::Catch2WithMain
            ${ARGN}
            undercurrent
    )
    add_test(NAME ${name} COMMAND ${name})
endfunction()

uc_add_test(test1)
uc_add_test(iterator_test1)
uc_add_test(adaptors_test1)
uc_add_test(segmented_iterator_test1 uc_dev)
uc_add_test(seg_stack_test1 uc_dev)
