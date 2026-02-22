# Quarisma Linter Architecture Overview

**Document Version:** 1.0  
**Date:** 2025-10-28  
**Author:** Code Review Analysis  

---

## Table of Contents
1. [Executive Summary](#executive-summary)
2. [System Architecture](#system-architecture)
3. [Component Overview](#component-overview)
4. [Data Flow](#data-flow)
5. [Integration Points](#integration-points)
6. [Technology Stack](#technology-stack)

---

## Executive Summary

The Quarisma linting infrastructure is a comprehensive, modular system built around the `lintrunner` framework. It provides:

- **30+ specialized linters** covering Python, C++, CMake, YAML, and shell scripts
- **Cross-platform support** for Linux, macOS, and Windows
- **Centralized configuration** via `.lintrunner.toml` and YAML config files
- **Automated dependency management** through `pip_init.py` and `s3_init.py`
- **Parallel execution** using Python's `concurrent.futures`
- **Consistent JSON protocol** for all linter outputs

### Key Strengths
✅ Modular adapter-based architecture  
✅ Comprehensive test coverage requirements (98%)  
✅ Well-documented configuration system  
✅ Strong separation of concerns  
✅ Extensive error handling  

### Areas for Improvement
⚠️ Windows compatibility needs enhancement (grep/sed dependencies)  
⚠️ Some hardcoded paths in older adapters  
⚠️ Documentation could be more comprehensive for Windows users  

---

## System Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      lintrunner CLI                          │
│                  (Orchestration Layer)                       │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ├─── Reads: .lintrunner.toml
                     │
                     ├─── Executes: init_command (setup)
                     │
                     └─── Executes: command (linting)
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
┌──────────────┐      ┌──────────────┐     ┌──────────────┐
│   Python     │      │     C++      │     │   Config     │
│   Linters    │      │   Linters    │     │   Linters    │
├──────────────┤      ├──────────────┤     ├──────────────┤
│ • flake8     │      │ • clang-fmt  │     │ • cmake      │
│ • ruff       │      │ • clang-tidy │     │ • actionlint │
│ • mypy       │      │              │     │ • shellcheck │
│ • pyfmt      │      │              │     │              │
└──────────────┘      └──────────────┘     └──────────────┘
        │                     │                     │
        └─────────────────────┴─────────────────────┘
                              │
                              ▼
                    ┌──────────────────┐
                    │  JSON Protocol   │
                    │  (LintMessage)   │
                    └──────────────────┘
```

### Directory Structure

```
Tools/linter/
├── __init__.py                    # Package marker
├── dictionary.txt                 # Codespell dictionary
├── adapters/                      # Linter adapters
│   ├── README.md                  # Adapter guidelines
│   ├── _linter/                   # Shared framework (Python-focused)
│   ├── pip_init.py                # Python dependency installer
│   ├── s3_init.py                 # Binary downloader
│   ├── flake8_linter.py           # Python style checker
│   ├── ruff_linter.py             # Fast Python linter
│   ├── mypy_linter.py             # Python type checker
│   ├── clangformat_linter.py      # C++ formatter
│   ├── clangtidy_linter.py        # C++ static analyzer
│   ├── cmake_linter.py            # CMake linter
│   ├── shellcheck_linter.py       # Shell script linter
│   ├── codespell_linter.py        # Spell checker
│   ├── grep_linter.py             # Generic pattern matcher
│   └── [30+ other adapters]       # Specialized linters
├── clang_tidy/                    # Clang-tidy support
│   ├── __init__.py
│   └── generate_build_files.py    # Compile DB generator
└── config/                        # Configuration management
    ├── __init__.py
    ├── config_loader.py           # YAML config loader
    └── quarisma_linter_config.yaml  # Centralized config
```

---

## Component Overview

### 1. Core Infrastructure

#### lintrunner (External Dependency)
- **Purpose:** Orchestrates all linters
- **Version:** 0.12.7
- **Configuration:** `.lintrunner.toml`
- **Responsibilities:**
  - Parse configuration
  - Execute init commands
  - Run linter commands
  - Aggregate results
  - Apply patches

#### Configuration System
- **Primary Config:** `.lintrunner.toml` (TOML format)
- **Centralized Config:** `Tools/linter/config/quarisma_linter_config.yaml`
- **Purpose:** Avoid hardcoded paths, enable cross-platform compatibility

**Key Configuration Files:**
```
.lintrunner.toml          # Main linter orchestration
.flake8                   # Python linting rules
mypy.ini / mypy-strict.ini # Type checking config
pyproject.toml            # Python project metadata
.clang-format             # C++ formatting rules
.cmakelintrc              # CMake linting rules
.cmake-format.yaml        # CMake formatting rules
```

### 2. Adapter Categories

#### A. Python Linters
| Adapter | Tool | Purpose | Windows Compatible |
|---------|------|---------|-------------------|
| `flake8_linter.py` | flake8 | Style & error checking | ✅ Yes |
| `ruff_linter.py` | ruff | Fast linting & fixing | ✅ Yes |
| `mypy_linter.py` | mypy/dmypy | Type checking | ✅ Yes |
| `pyfmt_linter.py` | isort+usort+ruff | Code formatting | ✅ Yes |
| `pyrefly_linter.py` | pyrefly | Advanced analysis | ✅ Yes |

#### B. C++ Linters
| Adapter | Tool | Purpose | Windows Compatible |
|---------|------|---------|-------------------|
| `clangformat_linter.py` | clang-format | Code formatting | ✅ Yes (with fixes) |
| `clangtidy_linter.py` | clang-tidy | Static analysis | ✅ Yes (with fixes) |

#### C. Build System Linters
| Adapter | Tool | Purpose | Windows Compatible |
|---------|------|---------|-------------------|
| `cmake_linter.py` | cmakelint | CMake linting | ✅ Yes |
| `cmake_format_linter.py` | cmake-format | CMake formatting | ✅ Yes |
| `bazel_linter.py` | bazel | Bazel validation | ✅ Yes |

#### D. Configuration Linters
| Adapter | Tool | Purpose | Windows Compatible |
|---------|------|---------|-------------------|
| `actionlint_linter.py` | actionlint | GitHub Actions | ✅ Yes |
| `shellcheck_linter.py` | shellcheck | Shell scripts | ✅ Yes |
| `codespell_linter.py` | codespell | Spell checking | ✅ Yes |

#### E. Pattern-Based Linters
| Adapter | Tool | Purpose | Windows Compatible |
|---------|------|---------|-------------------|
| `grep_linter.py` | grep+sed | Pattern matching | ⚠️ Needs WSL/Git Bash |
| `newlines_linter.py` | Python | Line ending check | ✅ Yes |
| `exec_linter.py` | Python | Executable check | ✅ Yes |

### 3. Support Infrastructure

#### Dependency Management
- **`pip_init.py`**: Installs Python packages with version pinning
- **`s3_init.py`**: Downloads prebuilt binaries (clang-format, etc.)
- **`update_s3.py`**: Uploads updated binaries

#### Configuration Management
- **`config_loader.py`**: Loads YAML configuration
- **`quarisma_linter_config.yaml`**: Centralized path/pattern definitions

---

## Data Flow

### 1. Initialization Flow

```
User runs: lintrunner init
    │
    ├─→ Parse .lintrunner.toml
    │
    ├─→ For each linter with init_command:
    │   │
    │   ├─→ Execute pip_init.py (Python packages)
    │   │   └─→ Install: flake8, ruff, mypy, etc.
    │   │
    │   └─→ Execute s3_init.py (Binaries)
    │       └─→ Download: clang-format, clang-tidy, actionlint
    │
    └─→ Create .lintbin/ directory with binaries
```

### 2. Linting Flow

```
User runs: lintrunner [files]
    │
    ├─→ Parse .lintrunner.toml
    │
    ├─→ Match files against include/exclude patterns
    │
    ├─→ For each applicable linter:
    │   │
    │   ├─→ Execute adapter script
    │   │   │
    │   │   ├─→ Read file(s)
    │   │   ├─→ Run linter tool
    │   │   ├─→ Parse output
    │   │   └─→ Emit JSON (LintMessage)
    │   │
    │   └─→ Collect results
    │
    ├─→ Aggregate all LintMessages
    │
    └─→ Display results / Apply patches
```

### 3. LintMessage Protocol

All adapters emit JSON conforming to this schema:

```json
{
  "path": "path/to/file.py",
  "line": 42,
  "char": 10,
  "code": "FLAKE8",
  "severity": "error",
  "name": "E501",
  "original": "original code",
  "replacement": "fixed code",
  "description": "Line too long (120 > 88 characters)"
}
```

**Fields:**
- `path`: File path (relative to repo root)
- `line`: Line number (1-indexed, nullable)
- `char`: Column number (1-indexed, nullable)
- `code`: Linter code (e.g., "FLAKE8", "CLANGFORMAT")
- `severity`: "error", "warning", "advice", or "disabled"
- `name`: Specific error code (e.g., "E501", "format")
- `original`: Original content (for formatters)
- `replacement`: Fixed content (for formatters)
- `description`: Human-readable message

---

## Integration Points

### 1. Version Control Integration
- **Pre-commit hooks**: Can run linters before commit
- **CI/CD**: GitHub Actions workflows run linters
- **Git hooks**: `Scripts/setup_git_hooks.py`

### 2. IDE Integration
- **VS Code**: Can use lintrunner extension
- **PyCharm/CLion**: External tools configuration
- **Vim/Neovim**: ALE or similar plugins

### 3. Build System Integration
- **CMake**: Clang-tidy integration via compile_commands.json
- **Python**: Uses pyproject.toml for configuration
- **Bazel**: Validates BUILD files

---

## Technology Stack

### Languages
- **Python 3.9+**: All adapter scripts
- **TOML**: Configuration format
- **YAML**: Centralized config
- **JSON**: Inter-process communication

### Key Dependencies
- **lintrunner**: 0.12.7 (orchestration)
- **flake8**: 7.3.0 (Python linting)
- **ruff**: 0.13.1 (fast Python linting)
- **mypy**: Latest (type checking)
- **clang-format**: 18.x (C++ formatting)
- **clang-tidy**: 18.x (C++ analysis)
- **shellcheck**: 0.7.2.1 (shell linting)
- **actionlint**: Latest (GitHub Actions)

### Platform Support
- **Linux**: Full support (primary platform)
- **macOS**: Full support
- **Windows**: Partial support (see Windows enablement guide)

---

## Performance Characteristics

### Parallel Execution
- Most adapters use `concurrent.futures.ThreadPoolExecutor` or `ProcessPoolExecutor`
- Worker count: `os.cpu_count()` (typically 4-16 workers)
- Significant speedup on multi-core systems

### Typical Performance
- **Single file**: 10-500ms per linter
- **100 files**: 5-30 seconds (all linters)
- **1000 files**: 30-300 seconds (all linters)

### Optimization Strategies
- File-level parallelism
- Incremental linting (changed files only)
- Caching (dmypy for mypy)
- Binary downloads (avoid compilation)

---

## Security Considerations

### Binary Verification
- All downloaded binaries verified via SHA256 hash
- Hashes stored in `s3_init_config.json`
- Automatic deletion of mismatched binaries

### Dependency Pinning
- All Python packages pinned to specific versions
- Ensures reproducible linting results
- Prevents supply chain attacks

### Sandboxing
- Linters run in subprocess isolation
- No direct file system access beyond specified files
- Timeout mechanisms prevent hanging

---

## Future Enhancements

### Planned Improvements
1. **Full Windows support** for grep-based linters
2. **Incremental linting** with file change detection
3. **Distributed caching** for large codebases
4. **Language server protocol** integration
5. **Auto-fix suggestions** with AI assistance

### Extensibility
- Easy to add new adapters (follow existing patterns)
- Configuration-driven (minimal code changes)
- Plugin architecture for custom linters

---

## References

- **Main Documentation**: `Docs/readme/linter.md`
- **Adapter Guidelines**: `Tools/linter/adapters/README.md`
- **Configuration**: `.lintrunner.toml`
- **Centralized Config**: `Tools/linter/config/quarisma_linter_config.yaml`

