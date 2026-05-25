add_library(uc_tag_invoke)
target_sources(
    uc_tag_invoke
    PUBLIC FILE_SET uc_tag_invoke_sources
    TYPE CXX_MODULES
    FILES
    tag_invoke.ixx
)