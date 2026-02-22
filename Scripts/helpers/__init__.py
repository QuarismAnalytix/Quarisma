"""
Quarisma Build System Helper Modules

This package contains modular helper modules for the Quarisma build system,
extracted from setup.py for better organization and maintainability.

Modules:
    - cppcheck: Static analysis with cppcheck
    - coverage: Code coverage analysis
    - build: Build operations
    - config: Configuration generation
    - test: Test execution
    - sanitizer: Sanitizer operations
"""

__version__ = "1.0.0"
__all__ = [
    "cppcheck",
    "coverage",
    "build",
    "config",
    "test",
    "sanitizer",
]
