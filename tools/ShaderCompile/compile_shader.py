#!/usr/bin/env python3
"""
Batch compile GLSL shaders to SPIR-V (.spv).

Usage:
    python compile_shader.py [--target opengl|vulkan] [--include-dirs DIRS...]
                             [--output-dir DIR] [--watch]

Traverses current directory and subdirectories, compiling .vert, .frag,
.geom, .comp files to .spv using glslangValidator.

Arguments:
    --target       Target API: 'opengl' (default) or 'vulkan'
    --include-dirs Additional GLSL #include search paths
    --output-dir   Output directory (default: same as source file)
    --watch        Watch mode: recompile on file changes
    --force        Force recompile all shaders, ignoring timestamps
"""

import argparse
import subprocess
import sys
import time
from pathlib import Path

# Default supported extensions
SHADER_EXTS = {'.vert', '.frag', '.geom', '.comp'}

# Default blacklist
BLACKLIST_DIRS = {'vendor'}

# Default include dirs
DEFAULT_INCLUDE_DIRS = [
    'Vulkitten/assets/computeshaders/common'
]


def is_blacklisted(path: Path) -> bool:
    return any(part in BLACKLIST_DIRS for part in path.parts)


def compile_shader(src_path: Path, target_env: str, include_dirs: list, output_dir: str = None, force: bool = False) -> bool:
    """Compile a single shader file. Returns True on success."""
    if output_dir:
        out_path = Path(output_dir) / (src_path.name + '.spv')
    else:
        out_path = src_path.with_suffix(src_path.suffix + '.spv')

    # Incremental: skip if .spv exists and is newer (unless --force)
    if not force and out_path.exists() and out_path.stat().st_mtime >= src_path.stat().st_mtime:
        print(f"  [SKIP] {src_path} (up-to-date)")
        return True

    # Select target-env for glslangValidator
    if target_env == 'vulkan':
        target_flag = 'vulkan1.3'
    else:
        target_flag = 'opengl'

    cmd = [
        'glslangValidator',
        '--target-env', target_flag,
    ]

    for inc in include_dirs:
        cmd.extend(['-I' + inc])

    # Ensure output directory exists
    out_path.parent.mkdir(parents=True, exist_ok=True)

    cmd.extend([
        '-o', str(out_path),
        str(src_path)
    ])

    try:
        # print(cmd)
        result = subprocess.run(cmd, capture_output=True, text=True, check=False)
        if result.returncode == 0:
            print(f"  [OK]   {src_path} -> {out_path.name}")
            return True
        else:
            print(f"  [FAIL] {src_path}")
            print(result.stdout)
            print(result.stderr, file=sys.stderr)
            return False
    except FileNotFoundError:
        print("  [ERR]  glslangValidator not found. Install Vulkan SDK.", file=sys.stderr)
        return False
    except Exception as e:
        print(f"  [ERR]  {src_path}: {e}", file=sys.stderr)
        return False


def compile_all(root: Path, target_env: str, include_dirs: list, output_dir: str = None, force: bool = False):
    """Find and compile all shader files."""
    shaders = sorted(
        f for f in root.rglob('*')
        if f.is_file()
        and f.suffix.lower() in SHADER_EXTS
        and not is_blacklisted(f)
    )

    if not shaders:
        print("No shader files found (or all blacklisted).")
        return 0, 0

    print(f"Found {len(shaders)} shader files, target={target_env}...")
    if force:
        print("Force mode: recompiling all shaders")
    if include_dirs:
        print(f"Include dirs: {', '.join(include_dirs)}")

    success, failed = 0, 0
    for shader in shaders:
        if compile_shader(shader, target_env, include_dirs, output_dir, force):
            success += 1
        else:
            failed += 1

    return success, failed


def main():
    parser = argparse.ArgumentParser(description='Compile GLSL shaders to SPIR-V')
    parser.add_argument('--target', choices=['opengl', 'vulkan'], default='opengl',
                        help='Target API (default: opengl)')
    parser.add_argument('--include-dirs', nargs='*', default=None,
                        help='Additional GLSL #include search paths')
    parser.add_argument('--output-dir', default=None,
                        help='Output directory for .spv files')
    parser.add_argument('--watch', action='store_true',
                        help='Watch mode: recompile on file changes')
    parser.add_argument('--force', action='store_true',
                        help='Force recompile all shaders, ignoring timestamps')

    args = parser.parse_args()

    include_dirs = args.include_dirs if args.include_dirs is not None else DEFAULT_INCLUDE_DIRS

    root = Path('.')

    if args.watch:
        print(f"Watch mode: compiling shaders on change (target={args.target})...")
        compile_all(root, args.target, include_dirs, args.output_dir, args.force)

        # Track file modification times
        mtimes = {}
        for f in root.rglob('*'):
            if f.is_file() and f.suffix.lower() in SHADER_EXTS and not is_blacklisted(f):
                mtimes[f] = f.stat().st_mtime

        try:
            while True:
                # time.sleep(1)
                for f in root.rglob('*'):
                    if f.is_file() and f.suffix.lower() in SHADER_EXTS and not is_blacklisted(f):
                        old_mtime = mtimes.get(f, 0)
                        new_mtime = f.stat().st_mtime
                        if new_mtime > old_mtime:
                            print(f"\n[CHANGED] {f}")
                            compile_shader(f, args.target, include_dirs, args.output_dir)
                            mtimes[f] = new_mtime
        except KeyboardInterrupt:
            print("\nWatch mode stopped.")
    else:
        success, failed = compile_all(root, args.target, include_dirs, args.output_dir, args.force)
        print(f"\nDone: {success} succeeded, {failed} failed")
        if failed > 0:
            sys.exit(1)


if __name__ == '__main__':
    main()
