# CMake Linting & Formatting Integration Analysis for Quarisma

## Executive Summary

This document provides a comprehensive analysis of the current CMake linting configuration in Quarisma and detailed recommendations for integrating `cmake-format` as a formatter alongside the existing `cmakelint` linter.

---

## 1. CURRENT STATE ANALYSIS

### 1.1 Current CMake Linting Tools

**Active Tool: `cmakelint` (v1.4.1)**
- **Location**: Configured in `.lintrunner.toml` (lines 751-777)
- **Adapter**: `Tools/linter/adapters/cmake_linter.py`
- **Configuration File**: `.cmakelintrc`
- **Purpose**: Static analysis and style checking for CMake files

**Existing Formatter: `cmake-format`**
- **Status**: Already in `requirements.txt` and has a shell script (`Scripts/all-cmake-format.sh`)
- **Current Integration**: Minimal - only a basic shell script for git-based formatting
- **Issue**: Not integrated into the lintrunner framework or CI/CD pipeline

### 1.2 Configuration Details

#### `.cmakelintrc` Configuration
```
filter=-convention/filename,-linelength,-package/consistency,-readability/logic,-readability/mixedcase,-readability/wonkycase,-syntax,-whitespace/eol,+whitespace/extra,-whitespace/indent,-whitespace/mismatch,-whitespace/newline,-whitespace/tabs
```

**Disabled Checks** (with `-` prefix):
- `convention/filename` - File naming conventions
- `linelength` - Line length limits
- `package/consistency` - Package consistency
- `readability/logic` - Logic readability
- `readability/mixedcase` - Mixed case naming
- `readability/wonkycase` - Wonky case naming
- `syntax` - Syntax errors
- `whitespace/eol` - End-of-line whitespace
- `whitespace/indent` - Indentation
- `whitespace/mismatch` - Whitespace mismatches
- `whitespace/newline` - Newline handling
- `whitespace/tabs` - Tab usage

**Enabled Checks** (with `+` prefix):
- `whitespace/extra` - Extra whitespace

**Analysis**: The configuration is very permissive, disabling most checks. This suggests the project prioritizes flexibility over strict linting.

### 1.3 Integration Points

**Lintrunner Integration**:
- Code: `CMAKE`
- Include patterns: `**/*.cmake`, `**/*.cmake.in`, `**/CMakeLists.txt`
- Exclude patterns: `ThirdParty/**`, `Tools/**`, `.vscode/**`, `.git/**`, `.augment/**`
- Init command: Installs `cmakelint==1.4.1` via pip

**CI/CD Pipeline**:
- No explicit CMake linting step in `.github/workflows/ci.yml`
- Linting is run via lintrunner locally but not enforced in CI

**Developer Workflow**:
- Manual execution: `python Tools/linter/adapters/cmake_linter.py --config=.cmakelintrc <files>`
- Git hook: `Scripts/all-cmake-format.sh` (basic, not integrated with lintrunner)

### 1.4 Existing cmake-format Usage

**Current State**:
- `cmake-format` is in `requirements.txt`
- `Scripts/all-cmake-format.sh` exists but is incomplete and not integrated
- `ThirdParty/magic_enum/.cmake-format` shows a reference configuration

**Issues**:
- No `.cmake-format` configuration file in project root
- Shell script doesn't handle all CMake file patterns
- Not integrated into lintrunner framework
- Not enforced in CI/CD

---

## 2. TOOL SELECTION & BENEFITS

### 2.1 Why `cmake-format` (from `cmakelang` package)?

**Advantages**:
1. **Complementary to cmakelint**: Handles formatting while cmakelint handles linting
2. **Consistent with Quarisma's approach**: Similar to how clang-format complements clang-tidy
3. **Cross-platform**: Works on Windows, Linux, macOS
4. **Configurable**: YAML-based configuration for fine-grained control
5. **Active maintenance**: Part of the cmakelang project
6. **Integration-ready**: Can be wrapped in a lintrunner adapter like clang-format

**Comparison with Alternatives**:
- **cmake-lint**: Linting only, no formatting
- **cmake-format**: Formatting only (complementary)
- **cmakelang**: Provides both cmake-format and cmake-lint

### 2.2 Benefits for Quarisma

1. **Consistency**: Enforces uniform CMake code style across the project
2. **Maintainability**: Reduces manual formatting effort
3. **CI/CD Integration**: Can be checked in pull requests
4. **Developer Experience**: Automatic formatting on save (IDE integration)
5. **Alignment**: Mirrors the C++ workflow (clang-format + clang-tidy)

---

## 3. RECOMMENDED CONFIGURATION

### 3.1 `.cmake-format.yaml` Configuration

Create a new file at the project root with settings aligned to Quarisma's C++ standards:

```yaml
# cmake-format configuration for Quarisma
# Aligns with .clang-format and project coding standards

format:
  # Line width (matches clang-format ColumnLimit)
  line_width: 100

  # Indentation
  indent_width: 2
  tab_size: 2
  use_tabchars: false

  # Formatting behavior
  dangle_parens: true
  separate_ctrl_name_with_space: true
  separate_fn_name_with_space: false

  # Comment handling
  comment_prefix: '  #'
  enable_markup: true

  # Wrapping
  max_subgroups_hwrap: 3
  max_paren_depth: 6

  # Sorting
  enable_sort: true
  autosort: false

# Markup configuration
markup:
  bullet_char: '*'
  enum_char: '.'
  first_comment_is_literal: false
  literal_comment_pattern: null
  fence_pattern: '^\\s*([`~]{3}[`~]*)(.*)$'
  ruler_pattern: '^\\s*[*]{10,}$'
  explicit_start: null
  explicit_end: null
  implicit_block_start: null
  implicit_block_end: null
  implicit_paragraph_start: null

# Lint configuration (for reference, not used by cmake-format)
lint:
  disabled_codes: []
  function_pattern: '[0-9a-z_]+'
  macro_pattern: '[0-9A-Z_]+'
  global_var_pattern: '[A-Z][0-9A-Z_]*'
  internal_var_pattern: '_[A-Z][0-9A-Z_]*'
  local_var_pattern: '[a-z][a-z0-9_]*'
  private_var_pattern: '_[a-z][a-z0-9_]*'
  public_var_pattern: '[A-Z][A-Z0-9_]*'
  argument_var_pattern: '[a-z][a-z0-9_]*'
  keyword_pattern: '[A-Z][A-Z0-9_]*'
```

**Key Decisions**:
- `line_width: 100` - Matches `.clang-format` ColumnLimit
- `indent_width: 2` - Matches `.clang-format` IndentWidth
- `dangle_parens: true` - Improves readability for long commands
- `enable_sort: false` - Preserves developer intent for command ordering

---

## 4. INTEGRATION STRATEGY

### 4.1 Create cmake-format Linter Adapter

Create `Tools/linter/adapters/cmake_format_linter.py` following the pattern of `clangformat_linter.py`:

**Key Features**:
- Runs `cmake-format` in check mode by default
- Supports `--apply` flag for automatic formatting
- Handles cross-platform paths (Windows/Unix)
- Concurrent file processing
- Proper error handling and reporting

### 4.2 Update `.lintrunner.toml`

Add a new formatter entry after the CMAKE linter:

```toml
[[linter]]
code = 'CMAKEFORMAT'
include_patterns = [
    "**/*.cmake",
    "**/*.cmake.in",
    "**/CMakeLists.txt",
]
exclude_patterns = [
    'ThirdParty/**',
    'Tools/**',
    '.vscode/**',
    '.git/**',
    '.augment/**',
]
command = [
    'python3',
    'Tools/linter/adapters/cmake_format_linter.py',
    '--config=.cmake-format.yaml',
    '--',
    '@{{PATHSFILE}}',
]
init_command = [
    'python3',
    'Tools/linter/adapters/pip_init.py',
    '--dry-run={{DRYRUN}}',
    'cmakelang==0.6.13',
]
is_formatter = true
```

### 4.3 Update `Scripts/all-cmake-format.sh`

Enhance the shell script to be more robust and cross-platform:

```bash
#!/bin/bash
set -e

cd "$(dirname "$0")/.."

# Find cmake-format
FMT=""
for fmt_cmd in cmake-format cmake-format-py; do
  if command -v "$fmt_cmd" >/dev/null 2>&1; then
    FMT="$fmt_cmd"
    break
  fi
done

if [ -z "$FMT" ]; then
  echo "Error: cmake-format not found in PATH" >&2
  exit 1
fi

echo "Using $FMT"

# Find all CMake files
echo "Scanning for CMake files..."
find . \
  -type d \( -name .git -o -name .vscode -o -name .augment -o -name ThirdParty -o -name build -o -name 'build_*' \) -prune -false -o \
  -type f \( -name "CMakeLists.txt" -o -name "*.cmake" -o -name "*.cmake.in" \) -print0 \
  | xargs -0 -I{} "$FMT" -i {}

echo "cmake-format complete."
```

### 4.4 CI/CD Integration

Add to `.github/workflows/ci.yml` (in a code quality job):

```yaml
- name: Check CMake Formatting
  run: |
    python3 -m pip install cmakelang==0.6.13
    cmake-format --check --config-file=.cmake-format.yaml \
      $(find . -name CMakeLists.txt -o -name "*.cmake" | grep -v ThirdParty | grep -v build)
```

---

## 5. DEVELOPER WORKFLOW

### 5.1 Local Development

**Check formatting**:
```bash
lintrunner --only=CMAKEFORMAT
```

**Apply formatting**:
```bash
lintrunner --take CMAKEFORMAT --apply-patches
```

**Format all CMake files**:
```bash
bash Scripts/all-cmake-format.sh
```

### 5.2 IDE Integration

**VS Code** (`.vscode/settings.json`):
```json
{
  "[cmake]": {
    "editor.formatOnSave": true,
    "editor.defaultFormatter": "ms-vscode.cmake-tools"
  }
}
```

**CLion/IntelliJ**:
- Settings → Editor → Code Style → CMake
- Enable "Reformat Code" on save

---

## 6. ENFORCEMENT STRATEGY

### 6.1 Recommended Approach

**Phase 1 (Immediate)**:
- Create `.cmake-format.yaml` configuration
- Create `cmake_format_linter.py` adapter
- Add to `.lintrunner.toml` as formatter
- Update `Scripts/all-cmake-format.sh`
- Document in `Docs/readme/linter.md`

**Phase 2 (Short-term)**:
- Run formatter on entire codebase
- Add to CI/CD in check mode (warning, not blocking)
- Update developer documentation

**Phase 3 (Long-term)**:
- Make CI check blocking for new PRs
- Integrate into pre-commit hooks
- Monitor and refine configuration

### 6.2 Enforcement Modes

**Check Mode** (Default):
- Verifies formatting without modifying files
- Used in CI/CD and pre-commit hooks
- Developers must fix manually or use `--apply`

**Apply Mode**:
- Automatically formats files
- Used locally before commits
- Can be integrated into pre-commit hooks

---

## 7. CROSS-PLATFORM COMPATIBILITY

### 7.1 Considerations

- **Path handling**: Use `pathlib` in Python adapters
- **Line endings**: cmake-format handles CRLF/LF automatically
- **Executable discovery**: Check multiple possible names
- **Shell scripts**: Use POSIX-compatible syntax

### 7.2 Testing

Test on:
- Windows (PowerShell, Git Bash)
- Linux (Ubuntu, various shells)
- macOS (Zsh, Bash)

---

## 8. COMPATIBILITY WITH EXISTING TOOLS

### 8.1 Interaction with cmakelint

- **Complementary**: cmakelint checks style, cmake-format enforces it
- **No conflicts**: Different tools, different purposes
- **Recommended order**: Run cmake-format first, then cmakelint

### 8.2 Interaction with clang-format

- **Independent**: Different file types (CMake vs C++)
- **Consistent philosophy**: Both enforce project standards
- **Parallel execution**: Can run simultaneously

---

## 9. IMPLEMENTATION CHECKLIST

- [ ] Create `.cmake-format.yaml` configuration file
- [ ] Create `Tools/linter/adapters/cmake_format_linter.py`
- [ ] Update `.lintrunner.toml` with CMAKEFORMAT entry
- [ ] Update `Scripts/all-cmake-format.sh`
- [ ] Update `Docs/readme/linter.md` with cmake-format documentation
- [ ] Run formatter on entire codebase
- [ ] Add CI/CD check (warning mode initially)
- [ ] Update README.md with cmake-format usage
- [ ] Test on Windows, Linux, macOS
- [ ] Update pre-commit hooks (if applicable)
- [ ] Document in developer guide

---

## 10. REFERENCES

- [cmake-format Documentation](https://cmake-format.readthedocs.io/)
- [cmakelang GitHub](https://github.com/cheshirekow/cmake_format)
- [Quarisma .clang-format](../.clang-format)
- [Quarisma Linter Documentation](readme/linter.md)
