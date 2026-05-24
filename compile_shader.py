#!/usr/bin/env python3
"""
批量编译 GLSL 着色器为 SPIR-V (.spv)，目标环境 OpenGL 4.5。
遍历当前目录及所有子目录，处理 .vert, .frag, .geom, .comp 文件。
输出文件: 原文件名 + .spv

支持 BLACKLIST_DIR：列出的目录名及其所有子目录都会被跳过。
支持 INCLUDE_DIR：编译时额外提供的头文件搜索路径。
"""

import subprocess
import sys
from pathlib import Path

# ==================== 配置区 ====================

# 支持的着色器扩展名（glslangValidator 自动识别 stage）
SHADER_EXTS = {'.vert', '.frag', '.geom', '.comp'}

# 黑名单目录名：这些目录及其所有子目录下的 shader 都会被跳过
# 例如：['.git', 'node_modules', 'build', 'third_party']
BLACKLIST_DIRS = {
    'vendor',
}

# 额外的头文件搜索路径（对应 glslangValidator 的 -I 参数）
# 例如：['./shaders/include', './common']
# 留空则不添加额外 include 路径
INCLUDE_DIRS = [
    './Vulkitten/src',
    './Sandbox/assets/computeshaders/common'
]

# ==================== 逻辑区 ====================

def is_blacklisted(path: Path) -> bool:
    """检查路径是否位于任何黑名单目录下。"""
    return any(part in BLACKLIST_DIRS for part in path.parts)

def compile_shader(src_path: Path) -> bool:
    """编译单个着色器文件，返回是否成功。"""
    out_path = src_path.with_suffix(src_path.suffix + '.spv')

    # 增量编译：如果 spv 存在且比源文件新，则跳过
    if out_path.exists() and out_path.stat().st_mtime >= src_path.stat().st_mtime:
        print(f"  [SKIP] {src_path} (up-to-date)")
        return True

    cmd = [
        'glslangValidator',
        '--target-env', 'opengl',
    ]

    # 添加 include 路径
    for inc in INCLUDE_DIRS:
        cmd.extend(['-I', inc])

    cmd.extend([
        '-o', str(out_path),
        str(src_path)
    ])

    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            check=False
        )
        if result.returncode == 0:
            print(f"  [OK]   {src_path} -> {out_path.name}")
            return True
        else:
            print(f"  [FAIL] {src_path}")
            print(result.stdout)
            print(result.stderr, file=sys.stderr)
            return False
    except FileNotFoundError:
        print("  [ERR]  glslangValidator 未找到，请确保 Vulkan SDK / glslang 已安装并在 PATH 中", file=sys.stderr)
        return False
    except Exception as e:
        print(f"  [ERR]  {src_path}: {e}", file=sys.stderr)
        return False

def main():
    root = Path('.')

    # 收集所有 shader 文件，同时过滤黑名单目录
    shaders = sorted(
        f for f in root.rglob('*')
        if f.is_file()
        and f.suffix.lower() in SHADER_EXTS
        and not is_blacklisted(f)
    )

    if not shaders:
        print("未找到任何需要编译的着色器文件（或被黑名单过滤）。")
        return

    print(f"发现 {len(shaders)} 个着色器文件，开始编译...")
    if INCLUDE_DIRS:
        print(f"Include 路径: {', '.join(INCLUDE_DIRS)}")
    success = 0
    failed = 0

    for shader in shaders:
        if compile_shader(shader):
            success += 1
        else:
            failed += 1

    print(f"\n完成: {success} 成功, {failed} 失败")
    if failed > 0:
        sys.exit(1)

if __name__ == '__main__':
    main()