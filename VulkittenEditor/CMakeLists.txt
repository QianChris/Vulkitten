# VulkittenEditor - 编辑器/示例程序
project(VulkittenEditor VERSION 1.0.0 LANGUAGES CXX)

# 收集源文件
file(GLOB_RECURSE VulkittenEditor_SOURCES 
    src/*.cpp 
    src/*.h 
    src/*.hpp
)

# 创建可执行文件
add_executable(VulkittenEditor ${VulkittenEditor_SOURCES})

# 确保Vulkitten先构建完成
add_dependencies(VulkittenEditor Vulkitten)

# 链接Vulkitten DLL
target_link_libraries(VulkittenEditor PRIVATE Vulkitten)

# 设置输出目录为项目名子文件夹
set_target_properties(VulkittenEditor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/VulkittenEditor
)

# 为每种配置设置输出目录
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set_target_properties(VulkittenEditor PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER}}/VulkittenEditor
    )
endforeach()

# 中间文件目录
set_target_properties(VulkittenEditor PROPERTIES
    CMAKE_INTERMEDIATE_DIRECTORY ${CMAKE_INTERMEDIATE_DIRECTORY}/VulkittenEditor
    PDB_OUTPUT_DIRECTORY ${CMAKE_INTERMEDIATE_DIRECTORY}/VulkittenEditor
    COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_INTERMEDIATE_DIRECTORY}/VulkittenEditor
)

# Windows特定: 设置PDB名称
if(MSVC)
    set_target_properties(VulkittenEditor PROPERTIES
        COMPILE_PDB_NAME VulkittenEditor
    )
endif()

# 添加构建后事件: 复制Vulkitten.dll到VulkittenEditor输出目录 (解决DLL找不到问题)
# 这样VulkittenEditor.exe和Vulkitten.dll在同一目录，运行时自动加载
# 同时强制重新链接当DLL文件时间戳改变时
# if(WIN32)
#     add_custom_command(TARGET VulkittenEditor POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             $<TARGET_FILE:Vulkitten>
#             $<TARGET_FILE_DIR:VulkittenEditor>
#         DEPENDS $<TARGET_FILE:Vulkitten>
#         COMMENT "Copying Vulkitten.dll to VulkittenEditor output directory"
#     )
# endif()

if(WIN32)
    # 使用与 set_target_properties 中一致的目录变量
    set(VulkittenEditor_DLL_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/VulkittenEditor)

    # 删除原来的 POST_BUILD 事件，改用独立的自定义目标
    add_custom_target(CopyVulkittenDLLEditor ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${VulkittenEditor_DLL_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Vulkitten>
            ${VulkittenEditor_DLL_DIR}
        COMMENT "Copying Vulkitten.dll to VulkittenEditor output directory"
    )
    
    # 确保 Vulkitten 先构建完，再执行复制
    add_dependencies(CopyVulkittenDLLEditor Vulkitten)
    
    # 确保 VulkittenEditor 在复制完成后才构建/运行
    add_dependencies(VulkittenEditor CopyVulkittenDLLEditor)
endif()

# 安装规则 (可选)
install(TARGETS VulkittenEditor
    RUNTIME DESTINATION bin
)