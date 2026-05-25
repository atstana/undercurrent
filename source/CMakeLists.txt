add_library(undercurrent)
target_sources(
    undercurrent
    PUBLIC FILE_SET uc_sources
    TYPE CXX_MODULES
    FILES
    undercurrent.ixx
)

add_subdirectory(adaptors)
add_subdirectory(algorithm)
add_subdirectory(concepts)
add_subdirectory(dev)
add_subdirectory(iterator)
add_subdirectory(tag_invoke)

target_link_libraries(
    undercurrent
    PRIVATE
    uc_adaptors
    uc_algorithm
    uc_concepts
    uc_iterator
    uc_tag_invoke
)