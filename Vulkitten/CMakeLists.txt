# Vulkitten - 引擎核心DLL
project(Vulkitten VERSION 1.0.0 LANGUAGES CXX)

# 添加 spdlog 子目录 (3rdparty)
add_subdirectory(../3rdparty/spdlog ${CMAKE_BINARY_DIR}/spdlog)
add_subdirectory(../3rdparty/glm ${CMAKE_BINARY_DIR}/glm)
add_subdirectory(../3rdparty/glfw ${CMAKE_BINARY_DIR}/glfw)
add_subdirectory(../3rdparty/Glad ${CMAKE_BINARY_DIR}/glad)
add_subdirectory(../3rdparty/entt ${CMAKE_BINARY_DIR}/entt)
add_subdirectory(../3rdparty/yaml-cpp ${CMAKE_BINARY_DIR}/yaml-cpp)
# add_subdirectory(../3rdparty/imgui ${CMAKE_BINARY_DIR}/imgui)

# 宏开关
add_definitions(-DVULKITTEN_ENABLE_ASSERTS=1)

# Vulkan SDK (optional - gates VKT_HASs_VULKAN)
find_package(Vulkan QUIET)
if(Vulkan_FOUND)
    add_definitions(-DVKT_HAS_VULKAN)
    message(STATUS "Vulkan SDK found: ${Vulkan_INCLUDE_DIRS}")
else()
    message(STATUS "Vulkan SDK not found - Vulkan backend disabled")
endif()

# 收集源文件
file(GLOB_RECURSE VULKITTEN_SOURCES 
    src/*.cpp 
    src/*.h 
    src/*.hpp
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${VULKITTEN_SOURCES})

# 创建DLL
add_library(Vulkitten SHARED ${VULKITTEN_SOURCES})

# 预编译头
target_precompile_headers(Vulkitten PRIVATE src/vktpch.h)

# 定义导出宏
target_compile_definitions(Vulkitten PRIVATE 
    VULKITTEN_BUILD_DLL
    VULKITTEN_EXPORTS
)
target_include_directories(Vulkitten PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

# spdlog
target_link_libraries(Vulkitten PUBLIC spdlog::spdlog)
target_include_directories(Vulkitten PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/spdlog/include")

# glm
target_link_libraries(Vulkitten PUBLIC glm::glm)
target_include_directories(Vulkitten PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/glm")

# glfw
target_link_libraries(Vulkitten PUBLIC glfw)
target_include_directories(Vulkitten PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/glfw/include")

# glad (OpenGL function loader)
target_link_libraries(Vulkitten PUBLIC glad)

# EnTT (ECS library) - include without linking to bypass C++20 requirement
target_include_directories(Vulkitten PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/entt/single_include/")

# imgui (GUI library)
# target_link_libraries(Vulkitten PUBLIC imgui)
target_include_directories(Vulkitten PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/imgui")

# Create imguizmo SHARED library and link to Vulkitten
target_include_directories(Vulkitten PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/imguizmo/src")

# stb_image
target_include_directories(Vulkitten PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/stb_image")

# nlohmann json
target_include_directories(Vulkitten PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/nlohmann/include")

# OpenGL (Windows system library)
target_link_libraries(Vulkitten PUBLIC opengl32)

# Vulkan (optional)
if(Vulkan_FOUND)
    target_link_libraries(Vulkitten PUBLIC Vulkan::Vulkan)
endif()

# VulkittenRHI — rendering hardware interface DLL
target_link_libraries(Vulkitten PUBLIC VulkittenRHI)
target_include_directories(Vulkitten PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../VulkittenRHI/include")

# yaml-cpp
target_link_libraries(Vulkitten PUBLIC yaml-cpp::yaml-cpp)

# tinygltf (glTF 2.0 loader) - header-only to avoid stb_image conflicts
set(TINYGLTF_BUILD_LOADER_EXAMPLE OFF CACHE BOOL "" FORCE)
set(TINYGLTF_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE BOOL "" FORCE)
set(TINYGLTF_HEADER_ONLY ON CACHE BOOL "" FORCE)
add_subdirectory(../3rdparty/tinygltf ${CMAKE_BINARY_DIR}/tinygltf)
target_link_libraries(Vulkitten PUBLIC tinygltf)

# 公开头文件目录 (供SandBox使用)
target_include_directories(Vulkitten 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

# 设置输出目录为项目名子文件夹
set_target_properties(Vulkitten PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Vulkitten
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Vulkitten
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/Vulkitten
)

# 为每种配置设置输出目录
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set_target_properties(Vulkitten PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER}}/Vulkitten
        LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER}}/Vulkitten
        ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER}}/Vulkitten
    )
endforeach()

# 中间文件目录
set_target_properties(Vulkitten PROPERTIES
    CMAKE_INTERMEDIATE_DIRECTORY ${CMAKE_INTERMEDIATE_DIRECTORY}/Vulkitten
    PDB_OUTPUT_DIRECTORY ${CMAKE_INTERMEDIATE_DIRECTORY}/Vulkitten
    COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_INTERMEDIATE_DIRECTORY}/Vulkitten
)

# Windows特定: 设置PDB名称与DLL一致
if(MSVC)
    set_target_properties(Vulkitten PROPERTIES
        COMPILE_PDB_NAME Vulkitten
    )
endif()

# 安装规则 (可选)
install(TARGETS Vulkitten
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)