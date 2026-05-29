add_library(uc_iterator)
target_sources(
    uc_iterator
    PUBLIC FILE_SET uc_iterator_sources
    TYPE CXX_MODULES
    FILES
    basic_const_iterator.ixx
    iteration.ixx
    iterator.ixx
    misc.ixx
    reverse_iterator.ixx
    segmented_iterator.ixx
)
target_link_libraries(
    uc_iterator
    PRIVATE
    uc_tag_invoke
    uc_concepts
)