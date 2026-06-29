# Sample application using VulkittenRHI

set(SAMPLE_SOURCES main.cpp)

add_executable(Sample ${SAMPLE_SOURCES})

target_link_libraries(Sample PRIVATE VulkittenRHI glfw)

target_include_directories(Sample PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/glm
)

# Absolute path to VulkittenRHI root, used to resolve asset paths at runtime
target_compile_definitions(Sample PRIVATE
    VULKITTEN_RHI_DIR="${CMAKE_CURRENT_SOURCE_DIR}/..")

# Windows: add opengl32
if(WIN32)
    target_link_libraries(Sample PRIVATE opengl32)
endif()

# Runtime output - debugger working dir points to VulkittenRHI root
set_target_properties(Sample PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Sample
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
