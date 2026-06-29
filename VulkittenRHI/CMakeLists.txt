cmake_minimum_required(VERSION 3.16)
project(VulkittenRHI VERSION 1.0.0 LANGUAGES CXX)

# C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Vulkan SDK (optional)
find_package(Vulkan QUIET)
if(Vulkan_FOUND)
    add_definitions(-DRHI_HAS_VULKAN)
    message(STATUS "Vulkan SDK found: ${Vulkan_INCLUDE_DIRS}")
else()
    message(STATUS "Vulkan SDK not found - Vulkan backend disabled")
endif()

# ============================================================
# VulkittenRHI - dynamic library (DLL)
# ============================================================
set(RHI_HEADERS
    include/rhi/Core/Handle.hpp
    include/rhi/Core/Format.hpp
    include/rhi/Core/Types.hpp
    include/rhi/Core/Export.hpp
    include/rhi/ResourceDescs.hpp
    include/rhi/ResourceManager.hpp
    include/rhi/ISurface.hpp
    include/rhi/FrameContext.hpp
    include/rhi/IBuffer.hpp
    include/rhi/ITexture.hpp
    include/rhi/IShader.hpp
    include/rhi/IPipeline.hpp
    include/rhi/IGeometry.hpp
    include/rhi/ISampler.hpp
    include/rhi/ICommandBuffer.hpp
    include/rhi/IRenderDevice.hpp
    include/rhi/RenderCommandList.hpp
    include/rhi/Renderer.hpp
)

set(RHI_SOURCES
    src/Renderer.cpp
    src/RenderCommandList.cpp
    src/ResourceManager.cpp
    src/gl/GLDevice.cpp
    src/gl/GLCommandBuffer.cpp
    src/gl/GLResources.cpp
)

# Vulkan backend (conditional)
if(Vulkan_FOUND)
    list(APPEND RHI_SOURCES
        src/vk/VKDevice.cpp
        src/vk/VKCommandBuffer.cpp
        src/vk/VKSwapchain.cpp
        src/vk/VKResources.cpp
    )
endif()

add_library(VulkittenRHI SHARED ${RHI_HEADERS} ${RHI_SOURCES})

# DLL export macro
target_compile_definitions(VulkittenRHI PRIVATE RHI_BUILD_DLL)

target_include_directories(VulkittenRHI
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# ---- Output Directory Properties ----
# DLL goes to common bin/ directory so executables can find it at runtime.
# Intermediate files (obj, pdb) stay in per-project subdirectory.
set_target_properties(VulkittenRHI PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
)

# Per-config output directories
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set_target_properties(VulkittenRHI PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER}}
        LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER}}
        ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER}}
    )
endforeach()

# Intermediate file directory (per-project, avoids clashes)
set_target_properties(VulkittenRHI PROPERTIES
    CMAKE_INTERMEDIATE_DIRECTORY ${CMAKE_INTERMEDIATE_DIRECTORY}/VulkittenRHI
    PDB_OUTPUT_DIRECTORY ${CMAKE_INTERMEDIATE_DIRECTORY}/VulkittenRHI
    COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_INTERMEDIATE_DIRECTORY}/VulkittenRHI
)

# Windows: set PDB name to match DLL
if(MSVC)
    set_target_properties(VulkittenRHI PROPERTIES
        COMPILE_PDB_NAME VulkittenRHI
    )
endif()

# Install rules
install(TARGETS VulkittenRHI
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# GLFW - vendored from parent project
if(NOT TARGET glfw)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/glfw")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/glfw ${CMAKE_BINARY_DIR}/glfw_rhi)
        target_include_directories(VulkittenRHI PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/glfw/include)
    else()
        find_package(glfw3 QUIET)
        if(NOT glfw3_FOUND)
            message(WARNING "GLFW not found - samples will not link")
        endif()
    endif()
endif()
if(TARGET glfw)
    target_link_libraries(VulkittenRHI PUBLIC glfw)
endif()

# GLAD - OpenGL function loader (vendored from parent)
if(NOT TARGET glad)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/Glad")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/Glad ${CMAKE_BINARY_DIR}/glad_rhi)
    else()
        message(WARNING "GLAD not found - OpenGL backend will not compile")
    endif()
endif()
if(TARGET glad)
    target_link_libraries(VulkittenRHI PUBLIC glad)
endif()

# OpenGL (Windows system library)
if(WIN32)
    target_link_libraries(VulkittenRHI PUBLIC opengl32)
endif()

# Vulkan
if(Vulkan_FOUND)
    target_link_libraries(VulkittenRHI PUBLIC Vulkan::Vulkan)
endif()

# ============================================================
# Samples
# ============================================================
add_subdirectory(samples)

# ============================================================
# Tests
# ============================================================
option(VULKITTEN_BUILD_TESTS "Build VulkittenRHI unit tests" ON)
if(VULKITTEN_BUILD_TESTS)
    enable_testing()
    # Prevent googletest from overriding our output directories
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    # Add googletest from vendored location
    if(NOT TARGET gtest_main)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/googletest
                         ${CMAKE_BINARY_DIR}/googletest)
    endif()
    add_subdirectory(tests)
endif()

# ============================================================
# Solution Folder Organization
# ============================================================

# 3rdparty targets built by RHI (only if not already built by Vulkitten)
if(TARGET glfw)
    set_target_properties(glfw PROPERTIES FOLDER "3rdparty")
endif()
if(TARGET glad)
    set_target_properties(glad PROPERTIES FOLDER "3rdparty")
endif()
set_target_properties(gtest PROPERTIES FOLDER "3rdparty")
set_target_properties(gtest_main PROPERTIES FOLDER "3rdparty")
if(TARGET gmock)
    set_target_properties(gmock PROPERTIES FOLDER "3rdparty")
endif()
if(TARGET gmock_main)
    set_target_properties(gmock_main PROPERTIES FOLDER "3rdparty")
endif()

# Test targets
set_target_properties(VulkittenRHIUnitTests PROPERTIES FOLDER "Tests")
set_target_properties(VulkittenRHIGLTests PROPERTIES FOLDER "Tests")
if(TARGET VulkittenRHIVKTests)
    set_target_properties(VulkittenRHIVKTests PROPERTIES FOLDER "Tests")
endif()
if(TARGET CompileTestShaders)
    set_target_properties(CompileTestShaders PROPERTIES FOLDER "Tests")
endif()
