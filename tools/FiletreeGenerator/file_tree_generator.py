#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
目录文件树生成器
遍历指定目录，生成文件树并输出为 Markdown 文件。
支持 BlacklistPath 功能，可屏蔽指定子目录。
"""

import os
import sys
import argparse
from pathlib import Path
from typing import List, Set


class FileTreeGenerator:
    def __init__(
        self,
        root_dir: str,
        output_file: str,
        blacklist_paths: List[str] = None,
        include_files: bool = True,
        max_depth: int = None,
        verbose: bool = True,
    ):
        """
        初始化文件树生成器

        Args:
            root_dir: 输入目录路径
            output_file: 输出的 .md 文件路径
            blacklist_paths: 黑名单路径列表（相对或绝对路径）
            include_files: 是否包含文件（False 则只输出目录）
            max_depth: 最大遍历深度，None 表示不限制
            verbose: 是否打印过程输出
        """
        self.root_dir = Path(root_dir).resolve()
        self.output_file = Path(output_file)
        self.include_files = include_files
        self.max_depth = max_depth
        self.verbose = verbose

        # 统计信息
        self.scanned_dirs = 0
        self.scanned_files = 0
        self.skipped_blacklist = 0
        self.skipped_permission = 0

        # 处理黑名单路径：统一转换为绝对路径字符串集合
        self.blacklist: Set[str] = set()
        if blacklist_paths:
            for bp in blacklist_paths:
                bp_path = Path(bp).resolve()
                # 如果提供的是相对路径，尝试相对于 root_dir 解析
                if not bp_path.exists() and not bp_path.is_absolute():
                    bp_path = (self.root_dir / bp).resolve()
                self.blacklist.add(str(bp_path))
                # 也添加带尾部斜杠的版本以兼容比较
                self.blacklist.add(str(bp_path) + os.sep)

    def _log(self, message: str, end: str = "\n"):
        """打印过程信息"""
        if self.verbose:
            print(message, end=end, flush=True)

    def _is_blacklisted(self, path: Path) -> bool:
        """检查路径是否在黑名单中"""
        path_str = str(path.resolve())
        for black in self.blacklist:
            if path_str == black.rstrip(os.sep) or path_str.startswith(black):
                return True
        return False

    def _build_tree(
        self,
        current_dir: Path,
        prefix: str = "",
        depth: int = 0,
    ) -> List[str]:
        """递归构建文件树列表"""
        if self.max_depth is not None and depth > self.max_depth:
            return []

        self.scanned_dirs += 1
        if self.scanned_dirs % 100 == 0:
            self._log(f"  [进度] 已扫描 {self.scanned_dirs} 个目录, {self.scanned_files} 个文件...", end="\r")

        lines = []
        try:
            entries = sorted(
                current_dir.iterdir(),
                key=lambda e: (not e.is_dir(), e.name.lower()),
            )
        except PermissionError:
            self.skipped_permission += 1
            self._log(f"  ⚠️  权限拒绝: {current_dir}")
            return [f"{prefix}[权限拒绝]"]
        except OSError as e:
            self._log(f"  ⚠️  错误访问 {current_dir}: {e}")
            return [f"{prefix}[错误: {e}]"]

        filtered_entries = []
        for entry in entries:
            if self._is_blacklisted(entry):
                self.skipped_blacklist += 1
                if self.skipped_blacklist <= 10 or self.skipped_blacklist % 100 == 0:
                    self._log(f"  ⛔ 跳过黑名单: {entry.relative_to(self.root_dir)}")
                continue
            filtered_entries.append(entry)
            if entry.is_file():
                self.scanned_files += 1

        count = len(filtered_entries)

        for idx, entry in enumerate(filtered_entries):
            is_last = idx == count - 1
            connector = "└── " if is_last else "├── "
            lines.append(f"{prefix}{connector}{entry.name}")

            if entry.is_dir():
                extension = "    " if is_last else "│   "
                lines.extend(
                    self._build_tree(
                        entry,
                        prefix + extension,
                        depth + 1,
                    )
                )

        return lines

    def generate(self) -> None:
        """生成 Markdown 文件"""
        if not self.root_dir.exists():
            raise FileNotFoundError(f"输入目录不存在: {self.root_dir}")
        if not self.root_dir.is_dir():
            raise NotADirectoryError(f"输入路径不是目录: {self.root_dir}")

        self._log("=" * 50)
        self._log(f"📂 开始扫描目录: {self.root_dir}")
        self._log(f"📝 输出文件: {self.output_file.resolve()}")

        if self.blacklist:
            self._log(f"⛔ 黑名单路径 ({len(self.blacklist) // 2} 条):")
            for bp in sorted(set(b.rstrip(os.sep) for b in self.blacklist)):
                self._log(f"   - {bp}")
        else:
            self._log("⛔ 黑名单: 无")

        if self.max_depth is not None:
            self._log(f"📏 最大深度: {self.max_depth}")
        if not self.include_files:
            self._log("📁 仅输出目录结构")

        self._log("-" * 50)
        self._log("🔍 正在扫描...")

        # 构建 Markdown 内容
        md_lines = [
            f"# 目录结构: `{self.root_dir.name}`",
            "",
            f"**完整路径**: `{self.root_dir}`",
            f"**生成时间**: {os.popen('date +"%Y-%m-%d %H:%M:%S"').read().strip()}",
            "",
            "```",
            self.root_dir.name,
        ]

        tree_lines = self._build_tree(self.root_dir)
        md_lines.extend(tree_lines)
        md_lines.append("```")

        # 如果存在黑名单，追加说明
        if self.blacklist:
            md_lines.extend([
                "",
                "---",
                "",
                "## 黑名单路径",
                "",
            ])
            for bp in sorted(set(b.rstrip(os.sep) for b in self.blacklist)):
                md_lines.append(f"- `{bp}`")

        # 写入文件
        self.output_file.parent.mkdir(parents=True, exist_ok=True)
        self.output_file.write_text("\n".join(md_lines), encoding="utf-8")

        self._log("-" * 50)
        self._log("✅ 扫描完成!")
        self._log(f"   📂 扫描目录数: {self.scanned_dirs}")
        self._log(f"   📄 扫描文件数: {self.scanned_files}")
        self._log(f"   ⛔ 跳过黑名单: {self.skipped_blacklist}")
        self._log(f"   ⚠️  权限拒绝: {self.skipped_permission}")
        self._log(f"   📝 输出总行数: {len(tree_lines)}")
        self._log(f"   💾 输出文件: {self.output_file.resolve()}")
        self._log("=" * 50)


def main():
    parser = argparse.ArgumentParser(
        description="生成目录文件树并输出为 Markdown 文件",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
使用示例:
  python file_tree.py -i ./my_project -o tree.md
  python file_tree.py -i ./my_project -o tree.md -b node_modules -b .git
  python file_tree.py -i ./my_project -o tree.md -b build -b dist -b "*.pyc"
        """,
    )
    parser.add_argument(
        "-i", "--input",
        required=True,
        help="输入目录路径",
    )
    parser.add_argument(
        "-o", "--output",
        required=True,
        help="输出 Markdown 文件路径（如 tree.md）",
    )
    parser.add_argument(
        "-b", "--blacklist",
        action="append",
        default=[],
        help="黑名单路径（可多次使用，支持相对/绝对路径）",
    )
    parser.add_argument(
        "--dirs-only",
        action="store_true",
        help="仅输出目录，不包含文件",
    )
    parser.add_argument(
        "--max-depth",
        type=int,
        default=None,
        help="最大遍历深度",
    )
    parser.add_argument(
        "--quiet",
        action="store_true",
        help="静默模式，不输出过程信息",
    )

    args = parser.parse_args()

    generator = FileTreeGenerator(
        root_dir=args.input,
        output_file=args.output,
        blacklist_paths=args.blacklist,
        include_files=not args.dirs_only,
        max_depth=args.max_depth,
        verbose=not args.quiet,
    )
    generator.generate()


if __name__ == "__main__":
    main()