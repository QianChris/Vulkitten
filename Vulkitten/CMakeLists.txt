# Vulkitten - 引擎核心DLL
project(Vulkitten VERSION 1.0.0 LANGUAGES CXX)

# 添加 spdlog 子目录
add_subdirectory(vendor/spdlog)
add_subdirectory(vendor/glm)
add_subdirectory(vendor/glfw)
add_subdirectory(vendor/glad)
add_subdirectory(vendor/entt)
# add_subdirectory(vendor/imgui)

# 宏开关
add_definitions(-DVULKITTEN_ENABLE_ASSERTS=1)

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
target_include_directories(Vulkitten PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/spdlog/include")

# glm
target_link_libraries(Vulkitten PUBLIC glm::glm)
target_include_directories(Vulkitten PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm")

# glfw
target_link_libraries(Vulkitten PUBLIC glfw)
target_include_directories(Vulkitten PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glfw/include")

# glad (OpenGL function loader)
target_link_libraries(Vulkitten PUBLIC glad)

# EnTT (ECS library) - include without linking to bypass C++20 requirement
target_include_directories(Vulkitten PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor/entt/single_include/")

# imgui (GUI library)
# target_link_libraries(Vulkitten PUBLIC imgui)
target_include_directories(Vulkitten PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui")

# stb_image
target_include_directories(Vulkitten PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb_image")

# nlohmann json
target_include_directories(Vulkitten PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor/nlohmann/include")

# OpenGL (Windows system library)
target_link_libraries(Vulkitten PUBLIC opengl32)

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