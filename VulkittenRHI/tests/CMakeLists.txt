# ============================================================
# VulkittenRHI Unit Tests
#
# googletest setup is in parent VulkittenRHI/CMakeLists.txt
# ============================================================

# ============================================================
# Test source files
# ============================================================

# ---- Mock-based unit tests (no GPU required) ----
set(UNIT_TEST_SOURCES
    main.cpp
    mocks/MockCommandBuffer.hpp
    unit/RenderCommandListTest.cpp
    unit/HandleTest.cpp
    unit/TypesTest.cpp
    unit/FormatTest.cpp
)

# ---- GL backend tests (require GPU + OpenGL context) ----
set(GL_TEST_SOURCES
    gl/GLTestFixture.hpp
    gl/GLTestFixture.cpp
    gl/GLDrawTest.cpp
    gl/GLDispatchTest.cpp
    gl/GLDebugLabelTest.cpp
)

# ============================================================
# Unit test executable (no GPU needed, always builds)
# ============================================================
add_executable(VulkittenRHIUnitTests ${UNIT_TEST_SOURCES})
target_link_libraries(VulkittenRHIUnitTests PRIVATE VulkittenRHI gtest_main)
target_include_directories(VulkittenRHIUnitTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
# Tests need GLAD headers for types, even if not linking GL functions
if(TARGET glad)
    target_link_libraries(VulkittenRHIUnitTests PRIVATE glad)
endif()
add_test(NAME VulkittenRHI.UnitTests COMMAND VulkittenRHIUnitTests)

# ============================================================
# GL backend test executable (requires GPU)
#
# Needs access to private GL backend headers in src/gl/
# ============================================================
add_executable(VulkittenRHIGLTests ${GL_TEST_SOURCES})
target_link_libraries(VulkittenRHIGLTests PRIVATE VulkittenRHI gtest_main glfw opengl32)
target_include_directories(VulkittenRHIGLTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    # Private GL backend headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
if(TARGET glad)
    target_link_libraries(VulkittenRHIGLTests PRIVATE glad)
endif()
# Absolute path to VulkittenRHI root, for loading test shaders
target_compile_definitions(VulkittenRHIGLTests PRIVATE
    VULKITTEN_RHI_DIR="${CMAKE_CURRENT_SOURCE_DIR}/..")
add_test(NAME VulkittenRHI.GLTests COMMAND VulkittenRHIGLTests)

# ============================================================
# VK backend test executable (requires Vulkan SDK + GPU)
#
# Needs access to private VK backend headers in src/vk/
# ============================================================
if(Vulkan_FOUND)
    set(VK_TEST_SOURCES
        vk/VKTestFixture.hpp
        vk/VKTestFixture.cpp
        vk/VKDrawTest.cpp
        vk/VKDispatchTest.cpp
        vk/VKDebugLabelTest.cpp
    )
    add_executable(VulkittenRHIVKTests ${VK_TEST_SOURCES})
    target_link_libraries(VulkittenRHIVKTests PRIVATE VulkittenRHI gtest_main glfw)
    target_include_directories(VulkittenRHIVKTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        # Private VK backend headers
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )
    target_compile_definitions(VulkittenRHIVKTests PRIVATE
        VULKITTEN_RHI_DIR="${CMAKE_CURRENT_SOURCE_DIR}/..")
    add_test(NAME VulkittenRHI.VKTests COMMAND VulkittenRHIVKTests)
    message(STATUS "Vulkan tests enabled")
else()
    message(STATUS "Vulkan SDK not found - Vulkan tests disabled")
endif()
