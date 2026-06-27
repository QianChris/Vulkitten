# Sample application using VulkittenRHI

# GLFW (reuse parent project's vendored GLFW)
if(EXISTS "${CMAKE_SOURCE_DIR}/../Vulkitten/vendor/glfw")
    # Already built by parent CMakeLists
endif()

set(SAMPLE_SOURCES main.cpp)

add_executable(Sample ${SAMPLE_SOURCES})

target_link_libraries(Sample PRIVATE VulkittenRHI glfw)

target_include_directories(Sample PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../Vulkitten/vendor/glfw/include
    ${CMAKE_SOURCE_DIR}/../Vulkitten/vendor/glm
)

# Windows: add opengl32
if(WIN32)
    target_link_libraries(Sample PRIVATE opengl32)
endif()

# Runtime output
set_target_properties(Sample PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Sample
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
