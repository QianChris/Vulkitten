@echo off
REM Compile GLSL shaders to SPIR-V (OpenGL target)
REM Usage: compile_shader.bat [--include-dirs dir1 dir2 ...] [--output-dir dir] [--watch]
python tools/ShaderCompile/compile_shader.py --target opengl %*
