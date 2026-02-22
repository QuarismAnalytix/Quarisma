# Cross-Platform Compatibility Analysis

**Document Version:** 1.0  
**Date:** 2025-10-28  
**Scope:** Quarisma Linter Infrastructure  

---

## Table of Contents
1. [Overview](#overview)
2. [Platform Support Matrix](#platform-support-matrix)
3. [Compatibility Patterns](#compatibility-patterns)
4. [Platform-Specific Issues](#platform-specific-issues)
5. [Best Practices](#best-practices)
6. [Testing Recommendations](#testing-recommendations)

---

## Overview

The Quarisma linting infrastructure demonstrates **good cross-platform awareness** with several adapters implementing Windows-specific handling. However, some components still rely on Unix-specific tools.

### Compatibility Score by Platform

| Platform | Score | Status | Notes |
|----------|-------|--------|-------|
| **Linux** | 10/10 | ✅ Fully Supported | Primary development platform |
| **macOS** | 10/10 | ✅ Fully Supported | Full compatibility |
| **Windows** | 7/10 | ⚠️ Mostly Supported | Requires Git for Windows for some linters |

---

## Platform Support Matrix

### Python Linters

| Linter | Linux | macOS | Windows | Notes |
|--------|-------|-------|---------|-------|
| FLAKE8 | ✅ | ✅ | ✅ | Pure Python, fully compatible |
| RUFF | ✅ | ✅ | ✅ | Rust-based, cross-platform binary |
| MYPY | ✅ | ✅ | ✅ | Pure Python, fully compatible |
| PYFMT | ✅ | ✅ | ✅ | Pure Python (isort+usort+ruff) |
| PYREFLY | ✅ | ✅ | ✅ | Pure Python, fully compatible |

### C++ Linters

| Linter | Linux | macOS | Windows | Notes |
|--------|-------|-------|---------|-------|
| CLANGFORMAT | ✅ | ✅ | ✅ | Has Windows detection (line 16) |
| CLANGTIDY | ✅ | ✅ | ✅ | Requires compile_commands.json |

### Build System Linters

| Linter | Linux | macOS | Windows | Notes |
|--------|-------|-------|---------|-------|
| CMAKE | ✅ | ✅ | ✅ | Pure Python wrapper |
| CMAKEFORMAT | ✅ | ✅ | ✅ | Pure Python (cmakelang) |
| BAZEL | ✅ | ✅ | ✅ | Bazel is cross-platform |

### Configuration Linters

| Linter | Linux | macOS | Windows | Notes |
|--------|-------|-------|---------|-------|
| ACTIONLINT | ✅ | ✅ | ✅ | Go binary, cross-platform |
| SHELLCHECK | ✅ | ✅ | ✅ | shellcheck-py package |
| CODESPELL | ✅ | ✅ | ✅ | Pure Python |

### Pattern-Based Linters

| Linter | Linux | macOS | Windows | Notes |
|--------|-------|-------|---------|-------|
| GREP | ✅ | ✅ | ⚠️ | Requires grep/sed (Git for Windows) |
| NEWLINES | ✅ | ✅ | ✅ | Pure Python |
| EXEC | ✅ | ✅ | ✅ | Pure Python |

### Infrastructure

| Component | Linux | macOS | Windows | Notes |
|-----------|-------|-------|---------|-------|
| pip_init.py | ✅ | ✅ | ✅ | Windows-aware (line 75) |
| s3_init.py | ✅ | ✅ | ✅ | Uses pathlib.Path |
| config_loader.py | ✅ | ✅ | ✅ | Uses pathlib.Path |

---

## Compatibility Patterns

### Pattern 1: Windows Detection

**Good Example:** `clangformat_linter.py`

```python
# Line 16: Detect Windows
IS_WINDOWS: bool = os.name == "nt"

# Line 38-39: Path conversion helper
def as_posix(name: str) -> str:
    return name.replace("\\", "/") if IS_WINDOWS else name

# Line 53: Shell execution for Windows
subprocess.run(
    args,
    capture_output=True,
    shell=IS_WINDOWS,  # Required for .bat files on Windows
    timeout=timeout,
    check=True,
)

# Line 209: Path normalization
binary = os.path.normpath(args.binary) if IS_WINDOWS else args.binary
```

**Files with Windows Detection:**
- ✅ `clangformat_linter.py` (line 16)
- ✅ `ruff_linter.py` (line 20)
- ✅ `grep_linter.py` (line 18)
- ✅ `pyfmt_linter.py` (line 20)
- ✅ `pip_init.py` (line 75)

**Files Missing Windows Detection:**
- ❌ `cmake_linter.py`
- ❌ `shellcheck_linter.py`
- ❌ `actionlint_linter.py`
- ❌ `codespell_linter.py`
- ❌ `mypy_linter.py`

### Pattern 2: Path Handling with pathlib

**Good Example:** `config_loader.py`

```python
from pathlib import Path

def get_config_path() -> Path:
    """Get the path to the Quarisma linter configuration file."""
    config_dir = Path(__file__).parent.absolute()
    config_file = config_dir / "quarisma_linter_config.yaml"
    return config_file

def get_repo_root() -> Path:
    """Get the repository root directory."""
    return Path(__file__).resolve().parents[3]
```

**Benefits:**
- ✅ Automatic path separator handling
- ✅ Cross-platform path operations
- ✅ Clean, readable code

**Files Using pathlib:**
- ✅ `config_loader.py` (throughout)
- ✅ `s3_init.py` (line 12, 29, 118, etc.)
- ✅ `codespell_linter.py` (line 11, 15, 88, 98)
- ✅ `pyfmt_linter.py` (line 12, 21, 106)
- ✅ `clangformat_linter.py` (line 12, 210)

### Pattern 3: Shell Command Execution

**Good Example:** `clangformat_linter.py`

```python
# Line 50-56: Proper subprocess handling
return subprocess.run(
    args,
    capture_output=True,
    shell=IS_WINDOWS,  # Enable shell on Windows for .bat files
    timeout=timeout,
    check=True,
)
```

**Bad Example:** Direct shell commands without Windows check

```python
# This will fail on Windows without Git Bash
subprocess.run(["grep", "-nEHI", pattern, filename])
subprocess.run(["sed", "-r", pattern, filename])
```

### Pattern 4: Binary Path Resolution

**Good Example:** `s3_init.py`

```python
# Line 118-124: Cross-platform binary path
binary_path = Path(output_dir, name)
if check(binary_path, reference_bin_hash):
    logging.info("Correct binary already exists at %s. Exiting.", binary_path)
    return True

# Create the output folder
binary_path.parent.mkdir(parents=True, exist_ok=True)
```

**Good Example:** `clangformat_linter.py`

```python
# Line 209-210: Windows path normalization
binary = os.path.normpath(args.binary) if IS_WINDOWS else args.binary
if not Path(binary).exists():
    # Error handling
```

### Pattern 5: Environment Variable Handling

**Good Example:** `pip_init.py`

```python
# Line 53-62: Cross-platform environment setup
env: dict[str, str] = {
    **os.environ,
    "UV_PYTHON": sys.executable,
    "UV_PYTHON_DOWNLOADS": "never",
    "FORCE_COLOR": "1",
    "CLICOLOR_FORCE": "1",
}

# Line 75: Windows path handling in environment
is_uv_managed_python = "uv/python" in sys.base_prefix.replace("\\", "/")
```

---

## Platform-Specific Issues

### Issue 1: Unix Tool Dependencies

**Affected Files:**
- `grep_linter.py` (lines 74, 113, 230-238)

**Problem:**
```python
# Line 74: grep command (Unix-specific)
proc = run_command(["grep", "-nEHI", allowlist_pattern, filename])

# Line 113: sed command (Unix-specific)
proc = run_command(["sed", "-r", replace_pattern, filename])
```

**Impact:**
- ❌ Fails on Windows without Git for Windows or WSL
- ❌ Requires additional setup for Windows users
- ❌ Not documented in Windows installation guide

**Solution:**
Implement pure Python fallback (see code-review-findings.md, issue H1)

### Issue 2: Case-Sensitive Path Issues

**Affected Files:**
- `codespell_linter.py` (line 17)

**Problem:**
```python
# Line 17: Lowercase "tools" but directory is "Tools"
DICTIONARY = REPO_ROOT / "tools" / "linter" / "dictionary.txt"
```

**Impact:**
- ❌ Fails on case-sensitive filesystems (Linux, macOS with APFS case-sensitive)
- ✅ Works on case-insensitive filesystems (Windows, macOS default)

**Solution:**
```python
# Use correct case
DICTIONARY = REPO_ROOT / "Tools" / "linter" / "dictionary.txt"
```

### Issue 3: Line Ending Differences

**Affected Files:**
- All text files

**Problem:**
- Windows uses CRLF (`\r\n`)
- Unix uses LF (`\n`)
- Can cause issues with pattern matching and file comparison

**Solution:**
- ✅ `newlines_linter.py` enforces POSIX line endings
- ✅ Git configuration: `core.autocrlf=input`
- ✅ `.gitattributes` file specifies line endings

### Issue 4: File Permissions

**Affected Files:**
- `s3_init.py` (lines 146-149)
- `exec_linter.py`

**Problem:**
```python
# Line 146-149: Unix-style file permissions
mode = os.stat(binary_path).st_mode
mode |= stat.S_IXUSR  # Add execute bit
os.chmod(binary_path, mode)
```

**Impact:**
- ⚠️ Execute bit not meaningful on Windows
- ✅ Code doesn't fail on Windows (no-op)
- ✅ Works correctly on Unix

**Solution:**
- Current implementation is acceptable (no-op on Windows)
- Could add Windows check to skip on Windows

### Issue 5: Shell Script Execution

**Affected Files:**
- `Scripts/*.sh` files

**Problem:**
- Shell scripts use bash syntax
- Windows CMD/PowerShell don't support bash

**Solution:**
- ✅ Use Git Bash on Windows
- ✅ Use WSL (Windows Subsystem for Linux)
- ⚠️ Consider PowerShell equivalents for critical scripts

---

## Best Practices

### ✅ DO: Use pathlib for Path Operations

```python
from pathlib import Path

# Good: Cross-platform path handling
config_path = Path(__file__).parent / "config.yaml"
repo_root = Path(__file__).resolve().parents[3]

# Bad: String concatenation
config_path = os.path.dirname(__file__) + "/config.yaml"
```

### ✅ DO: Detect Windows and Adjust Behavior

```python
import os

IS_WINDOWS: bool = os.name == "nt"

def as_posix(name: str) -> str:
    """Convert Windows paths to POSIX format."""
    return name.replace("\\", "/") if IS_WINDOWS else name

# Use in subprocess
subprocess.run(args, shell=IS_WINDOWS)
```

### ✅ DO: Use Cross-Platform Tools

```python
# Good: Pure Python or cross-platform tools
import re  # Pure Python regex
import shutil  # Cross-platform file operations

# Bad: Unix-specific tools without fallback
subprocess.run(["grep", pattern, file])  # Fails on Windows
subprocess.run(["sed", pattern, file])   # Fails on Windows
```

### ✅ DO: Handle Line Endings Properly

```python
# Good: Specify line ending handling
with open(file, 'r', newline='') as f:  # Preserve line endings
    content = f.read()

# Good: Normalize line endings
content = content.replace('\r\n', '\n')  # Convert CRLF to LF
```

### ✅ DO: Use Forward Slashes in Configuration

```toml
# Good: Forward slashes work on all platforms
include_patterns = ['Library/**/*.h', 'Library/**/*.cpp']

# Bad: Backslashes only work on Windows
include_patterns = ['Library\\**\\*.h']
```

### ❌ DON'T: Hardcode Absolute Paths

```python
# Bad: Hardcoded Unix path
config_path = "/usr/local/etc/config.yaml"

# Good: Relative to repo root
config_path = get_repo_root() / "config.yaml"
```

### ❌ DON'T: Assume Unix Tools Are Available

```python
# Bad: Assumes grep is available
subprocess.run(["grep", pattern, file])

# Good: Check availability or provide fallback
if shutil.which("grep"):
    subprocess.run(["grep", pattern, file])
else:
    # Pure Python fallback
    grep_with_python(pattern, file)
```

### ❌ DON'T: Use os.chdir() in Multi-threaded Code

```python
# Bad: Race condition in multi-threaded code
saved_cwd = os.getcwd()
os.chdir(build_dir)
# ... do work ...
os.chdir(saved_cwd)

# Good: Use absolute paths
abs_path = (build_dir / relative_path).resolve()
```

---

## Testing Recommendations

### Test Matrix

| Test Type | Linux | macOS | Windows | Priority |
|-----------|-------|-------|---------|----------|
| Unit Tests | ✅ | ✅ | ✅ | High |
| Integration Tests | ✅ | ✅ | ✅ | High |
| Path Handling | ✅ | ✅ | ✅ | High |
| Line Endings | ✅ | ✅ | ✅ | Medium |
| Binary Execution | ✅ | ✅ | ✅ | High |
| Shell Scripts | ✅ | ✅ | ⚠️ | Medium |

### Test Cases

#### 1. Path Handling Tests

```python
def test_path_handling():
    """Test cross-platform path handling."""
    # Test path construction
    path = Path("Tools") / "linter" / "config.yaml"
    assert path.exists()
    
    # Test path conversion
    posix_path = as_posix(str(path))
    assert "/" in posix_path
    assert "\\" not in posix_path

def test_repo_root():
    """Test repository root detection."""
    repo_root = get_repo_root()
    assert repo_root.exists()
    assert (repo_root / ".git").exists()
```

#### 2. Binary Execution Tests

```python
def test_binary_execution():
    """Test binary execution on all platforms."""
    # Test clang-format
    binary = ".lintbin/clang-format"
    if IS_WINDOWS:
        binary += ".exe"
    
    assert Path(binary).exists()
    
    result = subprocess.run(
        [binary, "--version"],
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0
```

#### 3. Line Ending Tests

```python
def test_line_endings():
    """Test line ending handling."""
    # Create test file with CRLF
    test_file = Path("test.txt")
    test_file.write_text("line1\r\nline2\r\n")
    
    # Read and normalize
    content = test_file.read_text()
    normalized = content.replace('\r\n', '\n')
    
    assert '\r' not in normalized
    assert normalized == "line1\nline2\n"
```

#### 4. Environment Tests

```python
def test_environment_setup():
    """Test environment variable handling."""
    env = {
        **os.environ,
        "TEST_VAR": "value",
    }
    
    # Test path handling in environment
    if IS_WINDOWS:
        env["PATH"] = env["PATH"].replace("\\", "/")
    
    assert "TEST_VAR" in env
```

### CI/CD Configuration

```yaml
# .github/workflows/lint.yml
name: Lint

on: [push, pull_request]

jobs:
  lint:
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest, windows-latest]
        python-version: ['3.9', '3.10', '3.11', '3.12']
    
    runs-on: ${{ matrix.os }}
    
    steps:
      - uses: actions/checkout@v4
      
      - name: Set up Python
        uses: actions/setup-python@v5
        with:
          python-version: ${{ matrix.python-version }}
      
      - name: Install dependencies
        run: |
          pip install lintrunner==0.12.7
          lintrunner init
      
      - name: Run linters
        run: lintrunner --take FLAKE8 --take RUFF --take MYPY
      
      - name: Test Windows-specific (Windows only)
        if: runner.os == 'Windows'
        run: |
          # Test with Git Bash
          bash -c "lintrunner --take GREP"
```

---

## Summary

### Current State

**Strengths:**
- ✅ Good foundation with pathlib usage
- ✅ Windows detection in key adapters
- ✅ Cross-platform binary downloads
- ✅ Consistent configuration format

**Weaknesses:**
- ⚠️ Some Unix tool dependencies (grep, sed)
- ⚠️ Missing Windows detection in some adapters
- ⚠️ Case-sensitive path issue in one file

### Recommendations

**Immediate:**
1. Fix case-sensitive path in `codespell_linter.py`
2. Add Windows detection to remaining adapters
3. Document Windows requirements clearly

**Short-term:**
4. Implement pure Python fallback for grep/sed
5. Add comprehensive cross-platform tests
6. Test on Windows CI/CD

**Long-term:**
7. Consider PowerShell equivalents for shell scripts
8. Enhance documentation with platform-specific notes
9. Create platform-specific troubleshooting guides

### Compatibility Score

| Platform | Current | Target | Gap |
|----------|---------|--------|-----|
| Linux | 10/10 | 10/10 | ✅ None |
| macOS | 10/10 | 10/10 | ✅ None |
| Windows | 7/10 | 9/10 | ⚠️ Minor improvements needed |

**Overall Cross-Platform Score: 9/10** (Excellent with minor improvements needed)

---

## References

- **Windows Enablement Guide:** `Docs/codereview/windows-enablement-guide.md`
- **Code Review Findings:** `Docs/codereview/code-review-findings.md`
- **Architecture Overview:** `Docs/codereview/linter-architecture-overview.md`
- **Python pathlib docs:** https://docs.python.org/3/library/pathlib.html
- **Cross-platform Python:** https://docs.python.org/3/library/os.html#os.name

