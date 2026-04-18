# SandBox - 编辑器/示例程序
project(SandBox VERSION 1.0.0 LANGUAGES CXX)

# 收集源文件
file(GLOB_RECURSE SANDBOX_SOURCES 
    src/*.cpp 
    src/*.h 
    src/*.hpp
)

# 创建可执行文件
add_executable(SandBox ${SANDBOX_SOURCES})

# 确保Vulkitten先构建完成
add_dependencies(SandBox Vulkitten)

# 链接Vulkitten DLL
target_link_libraries(SandBox PRIVATE Vulkitten)

# 设置输出目录为项目名子文件夹
set_target_properties(SandBox PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/SandBox
)

# 为每种配置设置输出目录
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set_target_properties(SandBox PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER}}/SandBox
    )
endforeach()

# 中间文件目录
set_target_properties(SandBox PROPERTIES
    CMAKE_INTERMEDIATE_DIRECTORY ${CMAKE_INTERMEDIATE_DIRECTORY}/SandBox
    PDB_OUTPUT_DIRECTORY ${CMAKE_INTERMEDIATE_DIRECTORY}/SandBox
    COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_INTERMEDIATE_DIRECTORY}/SandBox
)

# Windows特定: 设置PDB名称
if(MSVC)
    set_target_properties(SandBox PROPERTIES
        COMPILE_PDB_NAME SandBox
    )
endif()

# 添加构建后事件: 复制Vulkitten.dll到SandBox输出目录 (解决DLL找不到问题)
# 这样SandBox.exe和Vulkitten.dll在同一目录，运行时自动加载
# 同时强制重新链接当DLL文件时间戳改变时
if(WIN32)
    add_custom_command(TARGET SandBox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Vulkitten>
            $<TARGET_FILE_DIR:SandBox>
        DEPENDS $<TARGET_FILE:Vulkitten>
        COMMENT "Copying Vulkitten.dll to SandBox output directory"
    )
endif()

# 安装规则 (可选)
install(TARGETS SandBox
    RUNTIME DESTINATION bin
)