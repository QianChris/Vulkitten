@echo off
setlocal enabledelayedexpansion
set "GLSLANG=%VULKAN_SDK%\Bin\glslangValidator.exe"
if not exist "%GLSLANG%" (
    echo [ShaderCompile] glslangValidator not found. Set VULKAN_SDK.
    exit /b 0
)

set "SRC_DIR=%~dp0gl\shaders"
set "VK_OUT=%~dp0vk\shaders"

if not exist "%VK_OUT%" mkdir "%VK_OUT%"

echo [ShaderCompile] Compiling shaders to SPIR-V...

rem --- Compute shaders (used by VK tests) ---
for %%f in (fill_constant fill_linear add_one copy_buffer) do (
    if exist "%SRC_DIR%\%%f.comp" (
        echo   %SRC_DIR%\%%f.comp
        "%GLSLANG%" -V "%SRC_DIR%\%%f.comp" -o "%VK_OUT%\%%f.comp.spv" 2>&1
        if errorlevel 1 exit /b 1
    )
)

rem --- Graphics shaders (future VK draw tests) ---
if exist "%SRC_DIR%\simple.vert" (
    echo   %SRC_DIR%\simple.vert
    "%GLSLANG%" -V "%SRC_DIR%\simple.vert" -o "%VK_OUT%\simple.vert.spv" 2>&1
    if errorlevel 1 exit /b 1
)
if exist "%SRC_DIR%\solid_red.frag" (
    echo   %SRC_DIR%\solid_red.frag
    "%GLSLANG%" -V "%SRC_DIR%\solid_red.frag" -o "%VK_OUT%\solid_red.frag.spv" 2>&1
    if errorlevel 1 exit /b 1
)
if exist "%SRC_DIR%\solid_green.frag" (
    echo   %SRC_DIR%\solid_green.frag
    "%GLSLANG%" -V "%SRC_DIR%\solid_green.frag" -o "%VK_OUT%\solid_green.frag.spv" 2>&1
    if errorlevel 1 exit /b 1
)
if exist "%SRC_DIR%\color.frag" (
    echo   %SRC_DIR%\color.frag
    "%GLSLANG%" -V "%SRC_DIR%\color.frag" -o "%VK_OUT%\color.frag.spv" 2>&1
    if errorlevel 1 exit /b 1
)

echo [ShaderCompile] Done.
endlocal
exit /b 0
