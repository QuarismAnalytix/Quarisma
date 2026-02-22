# CMake Format Integration - Implementation Guide

## Overview

This guide provides step-by-step instructions for implementing and using the `cmake-format` integration in Quarisma.

## Files Created/Modified

### New Files
1. **`.cmake-format.yaml`** - Configuration file for cmake-format
2. **`Tools/linter/adapters/cmake_format_linter.py`** - Lintrunner adapter for cmake-format
3. **`Docs/cmake-format-integration-analysis.md`** - Detailed analysis and recommendations

### Modified Files
1. **`.lintrunner.toml`** - Added CMAKEFORMAT linter entry
2. **`Scripts/all-cmake-format.sh`** - Enhanced shell script for formatting

## Quick Start

### 1. Install Dependencies

```bash
# Install cmake-format (cmakelang package)
pip install cmakelang==0.6.13

# Or via requirements.txt (already included)
pip install -r requirements.txt
```

### 2. Check CMake Formatting

```bash
# Check all CMake files
lintrunner --only=CMAKEFORMAT

# Check specific files
python Tools/linter/adapters/cmake_format_linter.py \
  --config=.cmake-format.yaml \
  CMakeLists.txt
```

### 3. Apply Formatting

```bash
# Apply formatting to all CMake files
lintrunner --take CMAKEFORMAT --apply-patches

# Or use the shell script
bash Scripts/all-cmake-format.sh
```

## Configuration Details

### `.cmake-format.yaml` Settings

| Setting | Value | Rationale |
|---------|-------|-----------|
| `line_width` | 100 | Matches `.clang-format` ColumnLimit |
| `indent_width` | 2 | Matches `.clang-format` IndentWidth |
| `use_tabchars` | false | Consistent with project standards |
| `dangle_parens` | true | Improves readability for long commands |
| `enable_sort` | true | Enables sorting of commands |
| `autosort` | false | Preserves developer intent |

### Key Features

- **Line Width**: 100 characters (consistent with C++ formatting)
- **Indentation**: 2 spaces (consistent with project standards)
- **Dangling Parentheses**: Enabled for better readability
- **Comment Handling**: Preserves and formats comments appropriately

## Integration Points

### Lintrunner Integration

The CMAKEFORMAT linter is configured in `.lintrunner.toml`:

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

### Developer Workflow

**Before Committing**:
```bash
# Format all CMake files
lintrunner --take CMAKEFORMAT --apply-patches

# Or format specific files
bash Scripts/all-cmake-format.sh
```

**In CI/CD** (Recommended):
```bash
# Check formatting (non-blocking initially)
lintrunner --only=CMAKEFORMAT
```

## IDE Integration

### VS Code

Add to `.vscode/settings.json`:
```json
{
  "[cmake]": {
    "editor.formatOnSave": true,
    "editor.defaultFormatter": "ms-vscode.cmake-tools"
  }
}
```

### CLion/IntelliJ

1. Settings → Editor → Code Style → CMake
2. Enable "Reformat Code" on save
3. Configure indentation to 2 spaces

### Vim/Neovim

Add to your config:
```vim
autocmd FileType cmake setlocal formatprg=cmake-format\ -
```

## Troubleshooting

### cmake-format not found

**Error**: `cmake-format: command not found`

**Solution**:
```bash
pip install cmakelang==0.6.13
```

### Configuration file not found

**Error**: `Could not find cmake-format config at .cmake-format.yaml`

**Solution**: Ensure `.cmake-format.yaml` exists in the project root:
```bash
ls -la .cmake-format.yaml
```

### Formatting conflicts

**Issue**: cmake-format and cmakelint disagree on formatting

**Solution**:
1. Run cmake-format first (formatter)
2. Then run cmakelint (linter)
3. Adjust `.cmakelintrc` if needed

### Cross-platform issues

**Windows**: Use Git Bash or PowerShell with proper path handling
**Linux/macOS**: Ensure `cmake-format` is in PATH

## Performance Considerations

- **Concurrent Processing**: The adapter uses thread pool for parallel formatting
- **Large Files**: cmake-format handles large CMake files efficiently
- **Timeout**: Default 90 seconds per file (configurable)

## Enforcement Strategy

### Phase 1: Immediate (Current)
- ✅ Configuration created
- ✅ Adapter implemented
- ✅ Lintrunner integration added
- ✅ Shell script enhanced
- [ ] Run formatter on entire codebase
- [ ] Document in README

### Phase 2: Short-term
- [ ] Add to CI/CD (warning mode)
- [ ] Update developer documentation
- [ ] Integrate with pre-commit hooks

### Phase 3: Long-term
- [ ] Make CI check blocking
- [ ] Monitor and refine configuration
- [ ] Gather developer feedback

## Testing

### Local Testing

```bash
# Test on a single file
cmake-format --check --config-file=.cmake-format.yaml CMakeLists.txt

# Test with lintrunner
lintrunner --only=CMAKEFORMAT

# Test formatting application
lintrunner --take CMAKEFORMAT --apply-patches
```

### Cross-platform Testing

Test on:
- [ ] Windows (PowerShell, Git Bash)
- [ ] Linux (Ubuntu, various shells)
- [ ] macOS (Zsh, Bash)

## Compatibility

### With Existing Tools

- **cmakelint**: Complementary (linting vs formatting)
- **clang-format**: Independent (different file types)
- **clang-tidy**: Independent (different file types)

### CMake Versions

- Supports CMake 3.8+
- Tested with CMake 3.20+

## References

- [cmake-format Documentation](https://cmake-format.readthedocs.io/)
- [cmakelang GitHub](https://github.com/cheshirekow/cmake_format)
- [Quarisma Linter Documentation](readme/linter.md)
- [Quarisma Coding Standards](.augment/rules/coding.md)

## Support

For issues or questions:
1. Check the troubleshooting section above
2. Review cmake-format documentation
3. Open an issue in the Quarisma repository

## Next Steps

1. **Run formatter on codebase**: `bash Scripts/all-cmake-format.sh`
2. **Verify formatting**: `lintrunner --only=CMAKEFORMAT`
3. **Commit changes**: Include formatting changes in PR
4. **Update CI/CD**: Add cmake-format check to pipeline
5. **Document**: Update README and developer guides
