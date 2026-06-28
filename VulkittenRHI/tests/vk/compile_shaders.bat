@echo off
setlocal
set "GLSLANG=%VULKAN_SDK%\Bin\glslangValidator.exe"
if not exist "%GLSLANG%" (
    echo [VKTest] glslangValidator not found at %GLSLANG%
    echo [VKTest] Install Vulkan SDK or set VULKAN_SDK environment variable.
    exit /b 1
)

set "SRC_DIR=%~dp0..\gl\shaders"
set "DST_DIR=%~dp0shaders"

if not exist "%DST_DIR%" mkdir "%DST_DIR%"

echo [VKTest] Compiling compute shaders to SPIR-V...

for %%f in (fill_constant add_one copy_buffer) do (
    if exist "%SRC_DIR%\%%f.comp" (
        echo   %SRC_DIR%\%%f.comp -^> %DST_DIR%\%%f.comp.spv
        "%GLSLANG%" -V "%SRC_DIR%\%%f.comp" -o "%DST_DIR%\%%f.comp.spv"
        if errorlevel 1 (
            echo   ERROR: Failed to compile %%f.comp
            exit /b 1
        )
    )
)

echo [VKTest] SPIR-V shaders compiled successfully.
endlocal
