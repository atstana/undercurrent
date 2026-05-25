add_library(uc_adaptors)
target_sources(
    uc_adaptors
    PUBLIC FILE_SET uc_adaptors_sources
    TYPE CXX_MODULES
    FILES
    adaptors.ixx
    drop_while_view.ixx
    filter_view.ixx
    join_view.ixx
    misc.ixx
    movable_box.ixx
    non_propagating_cache.ixx
    reverse_view.ixx
    take_while_view.ixx
    transform_view.ixx
)
target_link_libraries(
    uc_adaptors
    PRIVATE
    uc_algorithm
    uc_iterator
    uc_tag_invoke
)