#!/usr/bin/env python
"""
Quarisma Git Hooks Setup Script
==============================

This script installs and configures git hooks for the Quarisma project.

Hooks installed:
1. pre-commit: Automatically formats C++ files with clang-format
2. commit-msg: Validates commit messages for quality and spelling

Cross-platform compatible: Works on Windows, Linux, and macOS.

Usage:
    python setup_git_hooks.py [--install|--uninstall|--status]

Author: Quarisma Development Team
"""

import os
import shutil
import stat
import subprocess
import sys
from pathlib import Path
from typing import Optional


# ============================================================================
# Configuration
# ============================================================================

HOOKS_TO_INSTALL = {
    "pre-commit": "Formats C++ files with clang-format",
    "commit-msg": "Validates commit messages",
}


# ============================================================================
# Utility Functions
# ============================================================================


def print_error(message: str) -> None:
    """Print error message in red."""
    print(f"\033[91m✗ ERROR: {message}\033[0m", file=sys.stderr)


def print_warning(message: str) -> None:
    """Print warning message in yellow."""
    print(f"\033[93m⚠ WARNING: {message}\033[0m")


def print_success(message: str) -> None:
    """Print success message in green."""
    print(f"\033[92m✓ {message}\033[0m")


def print_info(message: str) -> None:
    """Print info message in blue."""
    print(f"\033[94mℹ {message}\033[0m")


def print_header(message: str) -> None:
    """Print header message."""
    print()
    print("=" * 70)
    print(f"  {message}")
    print("=" * 70)
    print()


def run_command(cmd: list[str], cwd: Optional[str] = None) -> tuple[int, str, str]:
    """
    Run a command and return exit code, stdout, and stderr.

    Args:
        cmd: Command and arguments as a list
        cwd: Working directory (optional)

    Returns:
        Tuple of (exit_code, stdout, stderr)
    """
    try:
        result = subprocess.run(
            cmd,
            cwd=cwd,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        return result.returncode, result.stdout, result.stderr
    except Exception as e:
        return 1, "", str(e)


def get_repo_root() -> Optional[Path]:
    """
    Get the root directory of the git repository.

    Returns:
        Path to repository root or None if not in a git repository
    """
    exit_code, stdout, stderr = run_command(["git", "rev-parse", "--show-toplevel"])

    if exit_code != 0:
        print_error(f"Not in a git repository: {stderr}")
        return None

    return Path(stdout.strip())


def make_executable(filepath: Path) -> bool:
    """
    Make a file executable on Unix-like systems.
    On Windows, this is a no-op as .py files are executable via Python.

    Args:
        filepath: Path to the file

    Returns:
        True if successful
    """
    try:
        if sys.platform != "win32":
            # Add execute permission for owner, group, and others
            current_permissions = os.stat(filepath).st_mode
            os.chmod(
                filepath,
                current_permissions | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH,
            )
        return True
    except Exception as e:
        print_error(f"Failed to make {filepath} executable: {e}")
        return False


def check_clang_format() -> tuple[bool, Optional[str]]:
    """
    Check if clang-format is available in PATH.

    Returns:
        Tuple of (is_available, version_string)
    """
    check_cmd = "where" if sys.platform == "win32" else "which"
    exit_code, stdout, _ = run_command([check_cmd, "clang-format"])

    if exit_code != 0:
        return False, None

    # Get version
    exit_code, version_output, _ = run_command(["clang-format", "--version"])
    if exit_code == 0:
        return True, version_output.strip()

    return True, "unknown version"


# ============================================================================
# Hook Installation
# ============================================================================


def install_hooks(repo_root: Path) -> bool:
    """
    Install git hooks.

    Args:
        repo_root: Path to repository root

    Returns:
        True if successful
    """
    hooks_dir = repo_root / ".git" / "hooks"

    if not hooks_dir.exists():
        print_error(f"Git hooks directory not found: {hooks_dir}")
        return False

    print_info(f"Installing hooks to: {hooks_dir}")
    print()

    success = True

    for hook_name, description in HOOKS_TO_INSTALL.items():
        hook_path = hooks_dir / hook_name

        # Check if hook already exists
        if hook_path.exists():
            print_warning(f"Hook '{hook_name}' already exists")
            response = input("  Overwrite? [y/N]: ").strip().lower()
            if response not in ["y", "yes"]:
                print_info(f"  Skipping '{hook_name}'")
                continue

            # Backup existing hook
            backup_path = hook_path.with_suffix(".backup")
            try:
                shutil.copy2(hook_path, backup_path)
                print_info(f"  Backed up existing hook to: {backup_path.name}")
            except Exception as e:
                print_warning(f"  Failed to backup existing hook: {e}")

        # Hook already exists in .git/hooks (we created it earlier)
        if hook_path.exists():
            # Make executable
            if make_executable(hook_path):
                print_success(f"Installed '{hook_name}': {description}")
            else:
                print_error(f"Failed to make '{hook_name}' executable")
                success = False
        else:
            print_error(f"Hook file not found: {hook_path}")
            success = False

    return success


def uninstall_hooks(repo_root: Path) -> bool:
    """
    Uninstall git hooks.

    Args:
        repo_root: Path to repository root

    Returns:
        True if successful
    """
    hooks_dir = repo_root / ".git" / "hooks"

    if not hooks_dir.exists():
        print_error(f"Git hooks directory not found: {hooks_dir}")
        return False

    print_info(f"Uninstalling hooks from: {hooks_dir}")
    print()

    for hook_name in HOOKS_TO_INSTALL.keys():
        hook_path = hooks_dir / hook_name

        if not hook_path.exists():
            print_info(f"Hook '{hook_name}' not installed")
            continue

        try:
            # Check if there's a backup
            backup_path = hook_path.with_suffix(".backup")
            if backup_path.exists():
                # Restore backup
                shutil.copy2(backup_path, hook_path)
                backup_path.unlink()
                print_success(f"Restored backup for '{hook_name}'")
            else:
                # Remove hook
                hook_path.unlink()
                print_success(f"Removed '{hook_name}'")
        except Exception as e:
            print_error(f"Failed to uninstall '{hook_name}': {e}")
            return False

    return True


def show_status(repo_root: Path) -> None:
    """
    Show status of git hooks.

    Args:
        repo_root: Path to repository root
    """
    hooks_dir = repo_root / ".git" / "hooks"

    if not hooks_dir.exists():
        print_error(f"Git hooks directory not found: {hooks_dir}")
        return

    print_info(f"Git hooks directory: {hooks_dir}")
    print()

    for hook_name, description in HOOKS_TO_INSTALL.items():
        hook_path = hooks_dir / hook_name

        if hook_path.exists():
            # Check if executable
            is_executable = os.access(hook_path, os.X_OK) or sys.platform == "win32"
            status = "✓ Installed" if is_executable else "⚠ Installed (not executable)"
            color = "\033[92m" if is_executable else "\033[93m"
            print(f"{color}{status}\033[0m - {hook_name}: {description}")
        else:
            print(f"\033[91m✗ Not installed\033[0m - {hook_name}: {description}")

    print()

    # Check clang-format availability
    has_clang_format, version = check_clang_format()
    if has_clang_format:
        print_success(f"clang-format is available: {version}")
    else:
        print_warning("clang-format is not available in PATH")
        print_info("  Install instructions:")
        print_info("    - Windows: choco install llvm")
        print_info("    - macOS: brew install clang-format")
        print_info("    - Linux: apt-get install clang-format")


# ============================================================================
# Main
# ============================================================================


def main() -> int:
    """
    Main function.

    Returns:
        0 if successful, 1 if failed
    """
    print_header("Quarisma Git Hooks Setup")

    # Get repository root
    repo_root = get_repo_root()
    if not repo_root:
        return 1

    # Parse command line arguments
    if len(sys.argv) > 1:
        action = sys.argv[1].lower()
    else:
        action = "--install"

    if action in ["--install", "-i"]:
        print_info("Installing git hooks...")
        print()
        if install_hooks(repo_root):
            print()
            print_success("Git hooks installed successfully!")
            print()
            print_info("The following hooks are now active:")
            for hook_name, description in HOOKS_TO_INSTALL.items():
                print(f"  • {hook_name}: {description}")
            print()
            print_info("To bypass hooks, use: git commit --no-verify")
            return 0
        else:
            print()
            print_error("Failed to install git hooks")
            return 1

    elif action in ["--uninstall", "-u"]:
        print_info("Uninstalling git hooks...")
        print()
        if uninstall_hooks(repo_root):
            print()
            print_success("Git hooks uninstalled successfully!")
            return 0
        else:
            print()
            print_error("Failed to uninstall git hooks")
            return 1

    elif action in ["--status", "-s"]:
        show_status(repo_root)
        return 0

    else:
        print_error(f"Unknown action: {action}")
        print()
        print_info("Usage: python setup_git_hooks.py [--install|--uninstall|--status]")
        print_info("  --install, -i    Install git hooks (default)")
        print_info("  --uninstall, -u  Uninstall git hooks")
        print_info("  --status, -s     Show status of git hooks")
        return 1


if __name__ == "__main__":
    sys.exit(main())
