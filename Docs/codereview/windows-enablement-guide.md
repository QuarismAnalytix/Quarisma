# Windows Enablement Guide for Quarisma Linter

**Document Version:** 1.0  
**Date:** 2025-10-28  
**Status:** Implementation Guide  

---

## Table of Contents
1. [Executive Summary](#executive-summary)
2. [Current Windows Compatibility Status](#current-windows-compatibility-status)
3. [Prerequisites](#prerequisites)
4. [Installation Steps](#installation-steps)
5. [Platform-Specific Considerations](#platform-specific-considerations)
6. [Troubleshooting](#troubleshooting)
7. [Known Limitations](#known-limitations)
8. [Recommended Fixes](#recommended-fixes)

---

## Executive Summary

The Quarisma linting infrastructure has **good but incomplete** Windows support. Most linters work out-of-the-box on Windows, but some require Unix tools (grep, sed) that need special setup.

### Compatibility Summary

| Category | Status | Notes |
|----------|--------|-------|
| Python Linters | ✅ **Fully Compatible** | flake8, ruff, mypy, pyfmt all work |
| C++ Linters | ✅ **Fully Compatible** | clang-format, clang-tidy work with minor path fixes |
| CMake Linters | ✅ **Fully Compatible** | cmake-format, cmakelint work |
| Config Linters | ✅ **Fully Compatible** | actionlint, shellcheck work |
| Pattern Linters | ⚠️ **Needs Unix Tools** | grep_linter requires grep/sed |
| Infrastructure | ✅ **Fully Compatible** | pip_init, s3_init work |

### Quick Start (TL;DR)

```powershell
# 1. Install Python 3.9+
winget install Python.Python.3.12

# 2. Install Git for Windows (includes bash, grep, sed)
winget install Git.Git

# 3. Install lintrunner
pip install lintrunner==0.12.7

# 4. Initialize linters
lintrunner init

# 5. Run linters
lintrunner
```

---

## Current Windows Compatibility Status

### ✅ Fully Working Linters (No Changes Needed)

#### Python Linters
- **FLAKE8** (`flake8_linter.py`) - ✅ Works perfectly
- **RUFF** (`ruff_linter.py`) - ✅ Works perfectly
- **MYPY** (`mypy_linter.py`) - ✅ Works perfectly
- **PYFMT** (`pyfmt_linter.py`) - ✅ Works perfectly
- **PYREFLY** (`pyrefly_linter.py`) - ✅ Works perfectly

#### C++ Linters
- **CLANGFORMAT** (`clangformat_linter.py`) - ✅ Works (has Windows detection)
  - Line 16: `IS_WINDOWS: bool = os.name == "nt"`
  - Line 53: `shell=IS_WINDOWS` for subprocess
  - Line 209: Path normalization for Windows
  
- **CLANGTIDY** (`clangtidy_linter.py`) - ✅ Works (needs compile_commands.json)

#### Build System Linters
- **CMAKE** (`cmake_linter.py`) - ✅ Works perfectly
- **CMAKEFORMAT** (`cmake_format_linter.py`) - ✅ Works perfectly
- **BAZEL** (`bazel_linter.py`) - ✅ Works perfectly

#### Configuration Linters
- **ACTIONLINT** (`actionlint_linter.py`) - ✅ Works perfectly
- **SHELLCHECK** (`shellcheck_linter.py`) - ✅ Works (shellcheck-py package)
- **CODESPELL** (`codespell_linter.py`) - ✅ Works perfectly

#### Utility Linters
- **NEWLINES** (`newlines_linter.py`) - ✅ Works perfectly
- **EXEC** (`exec_linter.py`) - ✅ Works perfectly
- **PYPROJECT** (`pyproject_linter.py`) - ✅ Works perfectly

### ⚠️ Linters Requiring Unix Tools

#### Pattern-Based Linters
- **GREP** (`grep_linter.py`) - ⚠️ Requires grep and sed
  - Line 74: Uses `grep -nEHI`
  - Line 113: Uses `sed -r`
  - **Solution:** Install Git for Windows or use WSL

### 🔧 Infrastructure Scripts

- **pip_init.py** - ✅ Works perfectly
  - Line 75: Handles Windows paths: `"uv/python" in sys.base_prefix.replace("\\", "/")`
  
- **s3_init.py** - ✅ Works perfectly
  - Cross-platform path handling with `pathlib.Path`

- **config_loader.py** - ✅ Works perfectly
  - Uses `pathlib.Path` for cross-platform compatibility

---

## Prerequisites

### Required Software

#### 1. Python 3.9 or Higher
```powershell
# Using winget (Windows Package Manager)
winget install Python.Python.3.12

# Or download from python.org
# https://www.python.org/downloads/windows/

# Verify installation
python --version
# Should output: Python 3.12.x
```

#### 2. Git for Windows (Includes Unix Tools)
```powershell
# Using winget
winget install Git.Git

# Or download from git-scm.com
# https://git-scm.com/download/win

# This includes:
# - git
# - bash
# - grep
# - sed
# - awk
# - find

# Verify installation
git --version
grep --version
sed --version
```

#### 3. Visual Studio Build Tools (For C++ Linters)
```powershell
# Using winget
winget install Microsoft.VisualStudio.2022.BuildTools

# Or download from Microsoft
# https://visualstudio.microsoft.com/downloads/

# Required components:
# - MSVC v143 - VS 2022 C++ x64/x86 build tools
# - Windows 10/11 SDK
# - CMake tools for Windows
```

#### 4. LLVM/Clang (For C++ Linters)
```powershell
# Using winget
winget install LLVM.LLVM

# Or download from llvm.org
# https://github.com/llvm/llvm-project/releases

# Verify installation
clang-format --version
clang-tidy --version
```

### Optional Software

#### CMake (For CMake Linters)
```powershell
winget install Kitware.CMake
cmake --version
```

#### Node.js (For Some Linters)
```powershell
winget install OpenJS.NodeJS
node --version
```

---

## Installation Steps

### Step 1: Install Python Dependencies

```powershell
# Navigate to repository root
cd C:\path\to\Quarisma

# Install lintrunner
pip install lintrunner==0.12.7

# Verify installation
lintrunner --version
# Should output: lintrunner 0.12.7
```

### Step 2: Initialize Linters

```powershell
# This will install all Python packages and download binaries
lintrunner init

# Expected output:
# Installing Python packages...
# Downloading clang-format...
# Downloading actionlint...
# ...
# Initialization complete!
```

### Step 3: Verify Installation

```powershell
# Check that binaries were downloaded
dir .lintbin

# Should show:
# clang-format.exe
# actionlint.exe
# etc.

# Test a simple linter
lintrunner --take FLAKE8 -- Scripts/setup.py
```

### Step 4: Configure PATH (If Needed)

If `grep` or `sed` are not found, add Git's bin directory to PATH:

```powershell
# Add to PATH (PowerShell)
$env:Path += ";C:\Program Files\Git\usr\bin"

# Or permanently via System Properties:
# 1. Open System Properties → Environment Variables
# 2. Edit PATH
# 3. Add: C:\Program Files\Git\usr\bin
# 4. Restart terminal
```

### Step 5: Test All Linters

```powershell
# Run all linters on a test file
lintrunner -- Scripts/setup.py

# Run specific linters
lintrunner --take FLAKE8 -- Scripts/**/*.py
lintrunner --take CLANGFORMAT -- Library/**/*.h
lintrunner --take CMAKE -- CMakeLists.txt
```

---

## Platform-Specific Considerations

### 1. Path Separators

**Issue:** Windows uses backslashes (`\`), Unix uses forward slashes (`/`)

**Solution:** Most adapters already handle this:

```python
# Good: Already implemented in many adapters
IS_WINDOWS: bool = os.name == "nt"

def as_posix(name: str) -> str:
    return name.replace("\\", "/") if IS_WINDOWS else name
```

**Files with Windows support:**
- `clangformat_linter.py` (line 16, 38-39)
- `ruff_linter.py` (line 20, 54-55)
- `grep_linter.py` (line 18, 40-41)
- `pyfmt_linter.py` (line 20, 43-44)

### 2. Shell Execution

**Issue:** Windows doesn't have bash by default

**Solution:** Use `shell=IS_WINDOWS` in subprocess calls:

```python
# Good: Already implemented in clangformat_linter.py
subprocess.run(
    args,
    capture_output=True,
    shell=IS_WINDOWS,  # Allows .bat files to run
    timeout=timeout,
    check=True,
)
```

### 3. Binary Extensions

**Issue:** Windows executables need `.exe` extension

**Solution:** Use `shutil.which()` or check both variants:

```python
# Good: Cross-platform binary detection
binary = shutil.which("clang-format")
if binary is None:
    binary = shutil.which("clang-format.exe")
```

### 4. Line Endings

**Issue:** Windows uses CRLF (`\r\n`), Unix uses LF (`\n`)

**Solution:** The `newlines_linter.py` enforces POSIX line endings:

```python
# Configure Git to handle line endings
git config --global core.autocrlf input
```

### 5. File Permissions

**Issue:** Windows doesn't have Unix-style execute bits

**Solution:** The `exec_linter.py` is less relevant on Windows, but still works:

```python
# Windows: Checks file extension instead
if sys.platform == "win32":
    # Check for .exe, .bat, .cmd extensions
    pass
```

---

## Troubleshooting

### Issue 1: "grep: command not found"

**Symptom:**
```
Error: grep: command not found
Linter: GREP
```

**Solution:**
```powershell
# Option 1: Install Git for Windows
winget install Git.Git

# Option 2: Use WSL (Windows Subsystem for Linux)
wsl --install

# Option 3: Install GnuWin32
# Download from: http://gnuwin32.sourceforge.net/packages/grep.htm

# Add to PATH
$env:Path += ";C:\Program Files\Git\usr\bin"
```

### Issue 2: "sed: command not found"

**Symptom:**
```
Error: sed: command not found
Linter: GREP (with replace pattern)
```

**Solution:** Same as Issue 1 (sed comes with Git for Windows)

### Issue 3: "clang-format: No such file or directory"

**Symptom:**
```
Error: Could not find clang-format binary at .lintbin/clang-format
```

**Solution:**
```powershell
# Reinitialize linters
lintrunner init

# Or manually download
python Tools/linter/adapters/s3_init.py `
  --config-json=Tools/linter/adapters/s3_init_config.json `
  --linter=clang-format `
  --output-dir=.lintbin `
  --output-name=clang-format.exe
```

### Issue 4: "Permission denied" errors

**Symptom:**
```
PermissionError: [WinError 5] Access is denied
```

**Solution:**
```powershell
# Run PowerShell as Administrator
# Or check antivirus settings (may block .exe downloads)

# Disable real-time protection temporarily
# Windows Security → Virus & threat protection → Manage settings
```

### Issue 5: "Python module not found"

**Symptom:**
```
ModuleNotFoundError: No module named 'flake8'
```

**Solution:**
```powershell
# Reinstall Python dependencies
python Tools/linter/adapters/pip_init.py `
  flake8==7.3.0 `
  ruff==0.13.1 `
  mypy

# Or use lintrunner init
lintrunner init
```

### Issue 6: Path too long errors

**Symptom:**
```
OSError: [WinError 206] The filename or extension is too long
```

**Solution:**
```powershell
# Enable long path support (Windows 10+)
# Run as Administrator:
New-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem" `
  -Name "LongPathsEnabled" -Value 1 -PropertyType DWORD -Force

# Or use shorter paths
# Move repository closer to root: C:\Quarisma
```

### Issue 7: Line ending issues

**Symptom:**
```
Error: File has CRLF line endings
Linter: NEWLINE
```

**Solution:**
```powershell
# Configure Git to use LF line endings
git config --global core.autocrlf input
git config --global core.eol lf

# Re-checkout files
git rm --cached -r .
git reset --hard
```

---

## Known Limitations

### 1. grep_linter.py Requires Unix Tools

**Impact:** Medium  
**Affected Linters:** GREP-based pattern matchers  
**Workaround:** Install Git for Windows or use WSL  

**Technical Details:**
- Line 74: `grep -nEHI` command
- Line 113: `sed -r` command
- These are Unix-specific tools

**Recommended Fix:** See "Recommended Fixes" section below

### 2. Shell Scripts May Not Run Natively

**Impact:** Low  
**Affected Files:** `Scripts/*.sh`  
**Workaround:** Use Git Bash or WSL  

**Technical Details:**
- Shell scripts use bash syntax
- Windows CMD/PowerShell don't support bash
- Git Bash provides bash environment

### 3. Symbolic Links May Not Work

**Impact:** Low  
**Affected:** Some test files  
**Workaround:** Enable Developer Mode in Windows  

**Technical Details:**
- Windows requires admin rights or Developer Mode for symlinks
- Most linters don't rely on symlinks

---

## Recommended Fixes

### Fix 1: Make grep_linter.py Windows-Compatible

**File:** `Tools/linter/adapters/grep_linter.py`

**Current Issue:**
```python
# Line 74: Unix-specific grep command
proc = run_command(["grep", "-nEHI", allowlist_pattern, filename])

# Line 113: Unix-specific sed command
proc = run_command(["sed", "-r", replace_pattern, filename])
```

**Recommended Solution:**
```python
import re
import platform

IS_WINDOWS = platform.system() == "Windows"

def grep_file(pattern: str, filename: str) -> list[str]:
    """Cross-platform grep implementation."""
    if not IS_WINDOWS and shutil.which("grep"):
        # Use native grep on Unix
        proc = subprocess.run(
            ["grep", "-nEHI", pattern, filename],
            capture_output=True,
            text=True
        )
        return proc.stdout.splitlines()
    else:
        # Pure Python implementation for Windows
        matches = []
        with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
            for line_num, line in enumerate(f, 1):
                if re.search(pattern, line):
                    matches.append(f"{filename}:{line_num}:{line.rstrip()}")
        return matches

def sed_replace(pattern: str, filename: str) -> str:
    """Cross-platform sed implementation."""
    if not IS_WINDOWS and shutil.which("sed"):
        # Use native sed on Unix
        proc = subprocess.run(
            ["sed", "-r", pattern, filename],
            capture_output=True,
            text=True
        )
        return proc.stdout
    else:
        # Pure Python implementation for Windows
        with open(filename, 'r', encoding='utf-8') as f:
            content = f.read()
        # Parse sed pattern and apply with re.sub()
        # This is simplified; full sed compatibility is complex
        return re.sub(pattern, '', content)
```

### Fix 2: Add Windows Detection to All Adapters

**Pattern to Follow:**
```python
import os
import platform

IS_WINDOWS: bool = os.name == "nt"

def as_posix(name: str) -> str:
    """Convert Windows paths to POSIX format."""
    return name.replace("\\", "/") if IS_WINDOWS else name

# Use in subprocess calls
subprocess.run(
    args,
    shell=IS_WINDOWS,  # Required for .bat files on Windows
    ...
)
```

**Files to Update:**
- `cmake_linter.py` (add IS_WINDOWS check)
- `shellcheck_linter.py` (add IS_WINDOWS check)
- `actionlint_linter.py` (add IS_WINDOWS check)
- `codespell_linter.py` (add IS_WINDOWS check)

### Fix 3: Update Documentation

**Add Windows-specific sections to:**
- `Docs/readme/linter.md` (add Windows installation section)
- `Tools/linter/adapters/README.md` (add Windows notes)
- `.lintrunner.toml` (add comments about Windows compatibility)

---

## Testing on Windows

### Test Suite

```powershell
# Test Python linters
lintrunner --take FLAKE8 -- Scripts/**/*.py
lintrunner --take RUFF -- Scripts/**/*.py
lintrunner --take MYPY -- Scripts/**/*.py

# Test C++ linters
lintrunner --take CLANGFORMAT -- Library/**/*.h
lintrunner --take CLANGTIDY -- Library/**/*.cxx

# Test CMake linters
lintrunner --take CMAKE -- CMakeLists.txt
lintrunner --take CMAKEFORMAT -- CMakeLists.txt

# Test config linters
lintrunner --take ACTIONLINT -- .github/workflows/*.yml
lintrunner --take SHELLCHECK -- Scripts/*.sh

# Test all linters
lintrunner --all-files
```

### Expected Results

| Linter | Expected Status | Notes |
|--------|----------------|-------|
| FLAKE8 | ✅ Pass | Should work perfectly |
| RUFF | ✅ Pass | Should work perfectly |
| MYPY | ✅ Pass | May need dmypy daemon |
| CLANGFORMAT | ✅ Pass | Requires clang-format.exe |
| CLANGTIDY | ✅ Pass | Requires compile_commands.json |
| CMAKE | ✅ Pass | Should work perfectly |
| CMAKEFORMAT | ✅ Pass | Should work perfectly |
| ACTIONLINT | ✅ Pass | Should work perfectly |
| SHELLCHECK | ✅ Pass | Should work perfectly |
| GREP | ⚠️ Conditional | Requires Git for Windows |

---

## Summary

### What Works Out-of-the-Box
✅ All Python linters (flake8, ruff, mypy, pyfmt)  
✅ All C++ linters (clang-format, clang-tidy)  
✅ All CMake linters (cmake, cmake-format)  
✅ All config linters (actionlint, shellcheck, codespell)  
✅ Infrastructure scripts (pip_init, s3_init)  

### What Needs Setup
⚠️ Install Git for Windows (for grep/sed)  
⚠️ Install LLVM (for clang-format/clang-tidy)  
⚠️ Configure PATH environment variable  

### What Needs Code Changes
🔧 grep_linter.py (pure Python fallback)  
🔧 Add IS_WINDOWS checks to remaining adapters  
🔧 Update documentation with Windows instructions  

---

## Next Steps

1. **For Users:** Follow installation steps above
2. **For Developers:** Implement recommended fixes
3. **For Maintainers:** Test on Windows CI/CD
4. **For Documentation:** Add Windows sections to all docs

---

## References

- **Main Documentation:** `Docs/readme/linter.md`
- **Architecture Overview:** `Docs/codereview/linter-architecture-overview.md`
- **Code Review:** `Docs/codereview/code-review-findings.md`
- **Git for Windows:** https://git-scm.com/download/win
- **LLVM for Windows:** https://github.com/llvm/llvm-project/releases

