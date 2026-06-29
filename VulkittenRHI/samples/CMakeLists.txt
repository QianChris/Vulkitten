# RHISample - sample application using VulkittenRHI

set(SAMPLE_SOURCES main.cpp)

add_executable(RHISample ${SAMPLE_SOURCES})

target_link_libraries(RHISample PRIVATE VulkittenRHI glfw)

target_include_directories(RHISample PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/glm
)

# Absolute path to VulkittenRHI root, used to resolve asset paths at runtime
target_compile_definitions(RHISample PRIVATE
    VULKITTEN_RHI_DIR="${CMAKE_CURRENT_SOURCE_DIR}/..")

# Windows: add opengl32
if(WIN32)
    target_link_libraries(RHISample PRIVATE opengl32)
endif()

# Output to common bin/ directory so DLLs are found at runtime
# Working directory points to VulkittenRHI root for asset resolution
set_target_properties(RHISample PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
