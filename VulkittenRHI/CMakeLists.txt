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
# VulkittenRHI - static library
# ============================================================
set(RHI_HEADERS
    include/rhi/Core/Handle.hpp
    include/rhi/Core/Format.hpp
    include/rhi/Core/Types.hpp
    include/rhi/ResourceDescs.hpp
    include/rhi/ResourceManager.hpp
    include/rhi/ISurface.hpp
    include/rhi/FrameContext.hpp
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

add_library(VulkittenRHI STATIC ${RHI_HEADERS} ${RHI_SOURCES})

target_include_directories(VulkittenRHI
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# GLFW - vendored from parent project
if(NOT TARGET glfw)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../Vulkitten/vendor/glfw")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../Vulkitten/vendor/glfw ${CMAKE_BINARY_DIR}/glfw)
        target_include_directories(VulkittenRHI PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../Vulkitten/vendor/glfw/include)
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
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../Vulkitten/vendor/Glad")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../Vulkitten/vendor/Glad ${CMAKE_BINARY_DIR}/glad)
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
