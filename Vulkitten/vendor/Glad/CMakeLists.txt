# GLAD - OpenGL Function Loader
project(glad VERSION 1.0.0 LANGUAGES C)

add_library(glad STATIC
    src/glad.c
)

set_target_properties(glad PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(glad PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
