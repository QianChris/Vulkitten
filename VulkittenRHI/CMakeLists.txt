cmake_minimum_required(VERSION 3.16)
project(VulkittenRHI VERSION 1.0.0 LANGUAGES CXX)

# C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PLATFORM_NAME "x64")
else()
    set(PLATFORM_NAME "x86")
endif()

if(CMAKE_BUILD_TYPE)
    set(CONFIG_NAME ${CMAKE_BUILD_TYPE})
else()
    set(CONFIG_NAME "Debug")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CONFIG_NAME}-${PLATFORM_NAME})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CONFIG_NAME}-${PLATFORM_NAME})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CONFIG_NAME}-${PLATFORM_NAME})

# Vulkan SDK (optional)
find_package(Vulkan QUIET)
if(Vulkan_FOUND)
    add_definitions(-DRHI_HAS_VULKAN)
    message(STATUS "Vulkan SDK found: ${Vulkan_INCLUDE_DIRS}")
else()
    message(STATUS "Vulkan SDK not found — Vulkan backend disabled")
endif()

# ============================================================
# VulkittenRHI — static library
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

# GLFW — vendored from parent project
if(EXISTS "${CMAKE_SOURCE_DIR}/../Vulkitten/vendor/glfw")
    add_subdirectory(${CMAKE_SOURCE_DIR}/../Vulkitten/vendor/glfw ${CMAKE_BINARY_DIR}/glfw)
    target_link_libraries(VulkittenRHI PUBLIC glfw)
    target_include_directories(VulkittenRHI PRIVATE ${CMAKE_SOURCE_DIR}/../Vulkitten/vendor/glfw/include)
else()
    find_package(glfw3 QUIET)
    if(glfw3_FOUND)
        target_link_libraries(VulkittenRHI PUBLIC glfw)
    else()
        message(WARNING "GLFW not found — samples will not link")
    endif()
endif()

# GLAD — OpenGL function loader (vendored from parent)
if(EXISTS "${CMAKE_SOURCE_DIR}/../Vulkitten/vendor/Glad")
    add_subdirectory(${CMAKE_SOURCE_DIR}/../Vulkitten/vendor/Glad ${CMAKE_BINARY_DIR}/glad)
    target_link_libraries(VulkittenRHI PUBLIC glad)
else()
    message(WARNING "GLAD not found — OpenGL backend will not compile")
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
