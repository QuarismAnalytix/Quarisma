"""
Sanitizer Operations Helper Module

This module handles sanitizer-related operations and configuration.
Extracted from setup.py for better modularity and maintainability.
"""

import os
from typing import Optional


def get_sanitizer_options(sanitizer_type: str, source_path: str) -> Optional[str]:
    """
    Get sanitizer options string for the specified sanitizer type.

    Args:
        sanitizer_type: Type of sanitizer (address, undefined, thread, memory, leak)
        source_path: Path to source directory

    Returns:
        Sanitizer options string or None if not found
    """
    suppressions_dir = os.path.join(source_path, "Scripts", "suppressions")

    sanitizer_options = {
        "address": {
            "env_var": "ASAN_OPTIONS",
            "base_options": "print_stacktrace=1:check_initialization_order=1:strict_init_order=1",
            "suppression_file": os.path.join(suppressions_dir, "asan_suppressions.txt"),
        },
        "leak": {
            "env_var": "LSAN_OPTIONS",
            "base_options": "print_suppressions=1",
            "suppression_file": os.path.join(suppressions_dir, "lsan_suppressions.txt"),
        },
        "thread": {
            "env_var": "TSAN_OPTIONS",
            "base_options": "print_stacktrace=1:halt_on_error=1",
            "suppression_file": os.path.join(suppressions_dir, "tsan_suppressions.txt"),
        },
        "memory": {
            "env_var": "MSAN_OPTIONS",
            "base_options": "print_stats=1:halt_on_error=1",
            "suppression_file": os.path.join(suppressions_dir, "msan_suppressions.txt"),
        },
        "undefined": {
            "env_var": "UBSAN_OPTIONS",
            "base_options": "print_stacktrace=1:halt_on_error=1",
            "suppression_file": os.path.join(
                suppressions_dir, "ubsan_suppressions.txt"
            ),
        },
    }

    if sanitizer_type not in sanitizer_options:
        return None

    config = sanitizer_options[sanitizer_type]
    options = config["base_options"]

    # Add suppression file if it exists
    if os.path.exists(config["suppression_file"]):
        options += f":suppressions={config['suppression_file']}"

    return options


def build_sanitizer_environment(
    sanitizer_type: str, source_path: str
) -> dict[str, str]:
    """
    Build environment variables for the specified sanitizer.

    Args:
        sanitizer_type: Type of sanitizer
        source_path: Path to source directory

    Returns:
        Dictionary of environment variables to set
    """
    env_vars = {}

    sanitizer_options = {
        "address": "ASAN_OPTIONS",
        "leak": "LSAN_OPTIONS",
        "thread": "TSAN_OPTIONS",
        "memory": "MSAN_OPTIONS",
        "undefined": "UBSAN_OPTIONS",
    }

    if sanitizer_type in sanitizer_options:
        options = get_sanitizer_options(sanitizer_type, source_path)
        if options:
            env_vars[sanitizer_options[sanitizer_type]] = options

    return env_vars
