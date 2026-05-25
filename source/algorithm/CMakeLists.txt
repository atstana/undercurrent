add_library(uc_algorithm)
target_sources(
    uc_algorithm
    PUBLIC FILE_SET uc_algorithm_sources
    TYPE CXX_MODULES
    FILES
    algorithm.ixx
    find.ixx
    for_each.ixx
)
target_link_libraries(
    uc_algorithm
    PRIVATE
    uc_iterator
)