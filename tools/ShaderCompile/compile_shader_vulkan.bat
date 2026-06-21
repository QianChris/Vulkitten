@echo off
REM Compile GLSL shaders to SPIR-V (Vulkan target)
REM Usage: compile_shader_vulkan.bat [--include-dirs dir1 dir2 ...] [--output-dir dir] [--watch]
python tools/ShaderCompile/compile_shader.py --target vulkan %*
