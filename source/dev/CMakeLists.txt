add_library(uc_dev)
target_sources(
    uc_dev
    PUBLIC FILE_SET uc_dev_sources
    TYPE CXX_MODULES
    FILES
    chunked_iterator.ixx
    dev.ixx
    seg_stack.ixx
)
target_link_libraries(
    uc_dev
    PRIVATE
    uc_iterator
)
