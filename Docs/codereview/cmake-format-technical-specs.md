# CMake Format Integration - Technical Specifications

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    Developer Workflow                        │
├─────────────────────────────────────────────────────────────┤
│  Local: lintrunner --take CMAKEFORMAT --apply-patches       │
│  Local: bash Scripts/all-cmake-format.sh                    │
│  IDE: Format on save (VS Code, CLion, etc.)                 │
└────────────────────┬────────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────────┐
│              Lintrunner Framework                            │
├─────────────────────────────────────────────────────────────┤
│  .lintrunner.toml (CMAKEFORMAT entry)                       │
│  ├─ include_patterns: **/*.cmake, CMakeLists.txt            │
│  ├─ exclude_patterns: ThirdParty/**, build/**               │
│  └─ is_formatter: true                                      │
└────────────────────┬────────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────────┐
│         cmake_format_linter.py Adapter                       │
├─────────────────────────────────────────────────────────────┤
│  • Concurrent file processing (ThreadPoolExecutor)          │
│  • Timeout handling (90s default)                           │
│  • Cross-platform path handling                             │
│  • Error reporting and logging                              │
└────────────────────┬────────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────────┐
│            cmake-format Tool                                 │
├─────────────────────────────────────────────────────────────┤
│  • Reads .cmake-format.yaml configuration                   │
│  • Formats CMake files according to rules                   │
│  • Outputs formatted content to stdout                      │
└────────────────────┬────────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────────┐
│              CI/CD Pipeline                                  │
├─────────────────────────────────────────────────────────────┤
│  GitHub Actions: lintrunner --only=CMAKEFORMAT              │
│  Status: Check mode (warning, not blocking)                 │
└─────────────────────────────────────────────────────────────┘
```

## Component Specifications

### 1. Configuration File: `.cmake-format.yaml`

**Location**: Project root

**Format**: YAML

**Key Sections**:

#### Format Section
```yaml
format:
  line_width: 100              # Max line length
  indent_width: 2              # Spaces per indent level
  tab_size: 2                  # Tab width
  use_tabchars: false          # Use spaces, not tabs
  dangle_parens: true          # Allow parens to dangle
  separate_ctrl_name_with_space: true
  separate_fn_name_with_space: false
  comment_prefix: '  #'        # Comment indentation
  enable_markup: true          # Enable markup processing
  max_subgroups_hwrap: 3       # Max subgroups before wrap
  max_paren_depth: 6           # Max nesting depth
  enable_sort: true            # Enable sorting
  autosort: false              # Don't auto-sort
```

#### Markup Section
- Handles documentation comments
- Supports bullet points and rulers
- Configurable fence patterns

#### Lint Section
- Naming patterns for functions, macros, variables
- Used for reference (not enforced by formatter)

### 2. Lintrunner Adapter: `cmake_format_linter.py`

**Location**: `Tools/linter/adapters/cmake_format_linter.py`

**Language**: Python 3.9+

**Dependencies**:
- `cmakelang==0.6.13` (provides cmake-format)
- Standard library only (subprocess, concurrent.futures, etc.)

**Key Functions**:

```python
def check_file(filename, config, retries, timeout) -> list[LintMessage]
    # Runs cmake-format on a single file
    # Returns list of LintMessage objects
    # Handles timeouts and errors gracefully

def run_command(args, retries, timeout) -> CompletedProcess
    # Executes command with retry logic
    # Handles subprocess timeouts

def main()
    # Entry point for lintrunner
    # Processes multiple files concurrently
    # Outputs JSON-formatted lint messages
```

**Concurrency Model**:
- ThreadPoolExecutor with `os.cpu_count()` workers
- Parallel file processing
- Thread-safe JSON output

**Error Handling**:
- Timeout: Retries up to 3 times (configurable)
- Command failure: Reports error with exit code
- Missing config: Reports init error
- File I/O: Graceful error reporting

### 3. Lintrunner Configuration: `.lintrunner.toml`

**Entry**: CMAKEFORMAT

**Configuration**:
```toml
[[linter]]
code = 'CMAKEFORMAT'
include_patterns = ["**/*.cmake", "**/*.cmake.in", "**/CMakeLists.txt"]
exclude_patterns = ['ThirdParty/**', 'Tools/**', '.vscode/**', '.git/**', '.augment/**']
command = ['python3', 'Tools/linter/adapters/cmake_format_linter.py', '--config=.cmake-format.yaml', '--', '@{{PATHSFILE}}']
init_command = ['python3', 'Tools/linter/adapters/pip_init.py', '--dry-run={{DRYRUN}}', 'cmakelang==0.6.13']
is_formatter = true
```

**Key Properties**:
- `is_formatter = true`: Marks as formatter (can apply patches)
- Parallel with CMAKE linter (different tools)
- Same include/exclude patterns as CMAKE

### 4. Shell Script: `Scripts/all-cmake-format.sh`

**Purpose**: Format all CMake files in the project

**Features**:
- Finds cmake-format in PATH
- Discovers all CMake files (excluding ThirdParty, build)
- Applies formatting in-place
- Cross-platform compatible

**Usage**:
```bash
bash Scripts/all-cmake-format.sh
```

## Data Flow

### Check Mode (Default)

```
User Input (files)
    ↓
cmake_format_linter.py
    ├─ Read original file
    ├─ Run cmake-format
    ├─ Compare original vs formatted
    └─ Output LintMessage (if different)
    ↓
Lintrunner
    ├─ Collect messages
    ├─ Display to user
    └─ Exit with status
```

### Apply Mode

```
User Input (--apply-patches)
    ↓
Lintrunner
    ├─ Collect LintMessages
    ├─ Apply replacements
    └─ Write files
    ↓
Files Updated
```

## Performance Characteristics

### Throughput
- **Single file**: 100-500ms (depends on size)
- **100 files**: ~10-50s (parallel processing)
- **1000 files**: ~100-500s (parallel processing)

### Resource Usage
- **Memory**: ~50MB base + ~1MB per concurrent file
- **CPU**: Scales with thread count
- **Disk I/O**: Sequential reads, parallel processing

### Optimization Strategies
- Concurrent processing (ThreadPoolExecutor)
- Timeout handling (90s default)
- Early exit on errors
- Efficient path handling

## Integration Points

### With cmakelint
- **Relationship**: Complementary
- **Order**: Format first, then lint
- **Conflicts**: Minimal (different concerns)

### With clang-format
- **Relationship**: Independent
- **File types**: Different (CMake vs C++)
- **Execution**: Can run in parallel

### With CI/CD
- **Trigger**: On PR, push to main/develop
- **Mode**: Check (non-blocking initially)
- **Reporting**: Via lintrunner output

## Configuration Tuning

### For Stricter Formatting
```yaml
format:
  line_width: 80          # Shorter lines
  max_paren_depth: 4      # Less nesting
  dangle_parens: false    # No dangling parens
```

### For More Flexible Formatting
```yaml
format:
  line_width: 120         # Longer lines
  max_paren_depth: 8      # More nesting
  dangle_parens: true     # Allow dangling
```

### For Specific Projects
```yaml
format:
  enable_sort: false      # Preserve order
  autosort: false         # Manual sorting
```

## Troubleshooting Guide

### Issue: cmake-format not found
**Cause**: Package not installed
**Solution**: `pip install cmakelang==0.6.13`

### Issue: Configuration file not found
**Cause**: `.cmake-format.yaml` missing
**Solution**: Ensure file exists in project root

### Issue: Timeout errors
**Cause**: Large files or slow system
**Solution**: Increase timeout: `--timeout=180`

### Issue: Formatting conflicts
**Cause**: cmakelint and cmake-format disagree
**Solution**: Run formatter first, adjust linter config

## Version Compatibility

| Component | Version | Notes |
|-----------|---------|-------|
| cmakelang | 0.6.13 | Latest stable |
| cmake-format | 0.6.13 | Part of cmakelang |
| Python | 3.9+ | Type hints required |
| CMake | 3.8+ | Supported by cmake-format |

## Security Considerations

- **Input validation**: File paths validated
- **Command injection**: Arguments properly escaped
- **Temporary files**: Not used (stdin/stdout)
- **Permissions**: Respects file permissions

## Maintenance

### Regular Tasks
- Monitor cmake-format updates
- Test with new CMake versions
- Gather developer feedback
- Refine configuration as needed

### Update Procedure
1. Update `cmakelang` version in `.lintrunner.toml`
2. Test locally
3. Update documentation
4. Commit changes
5. Monitor CI/CD

## References

- [cmake-format Docs](https://cmake-format.readthedocs.io/)
- [cmakelang GitHub](https://github.com/cheshirekow/cmake_format)
- [Python subprocess](https://docs.python.org/3/library/subprocess.html)
- [concurrent.futures](https://docs.python.org/3/library/concurrent.futures.html)
