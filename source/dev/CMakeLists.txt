add_library(uc_dev)
target_sources(
    uc_dev
    PUBLIC FILE_SET uc_dev_sources
    TYPE CXX_MODULES
    FILES
    dev.ixx
)