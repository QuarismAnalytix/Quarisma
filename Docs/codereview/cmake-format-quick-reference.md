# CMake Format - Quick Reference Card

## Installation

```bash
# Install cmake-format
pip install cmakelang==0.6.13

# Or via requirements.txt
pip install -r requirements.txt
```

## Common Commands

### Check Formatting

```bash
# Check all CMake files
lintrunner --only=CMAKEFORMAT

# Check specific file
cmake-format --check --config-file=.cmake-format.yaml CMakeLists.txt

# Check with verbose output
cmake-format --check --config-file=.cmake-format.yaml -v CMakeLists.txt
```

### Apply Formatting

```bash
# Format all CMake files (via lintrunner)
lintrunner --take CMAKEFORMAT --apply-patches

# Format all CMake files (via shell script)
bash Scripts/all-cmake-format.sh

# Format specific file
cmake-format -i --config-file=.cmake-format.yaml CMakeLists.txt

# Format multiple files
cmake-format -i --config-file=.cmake-format.yaml *.cmake CMakeLists.txt
```

### View Differences

```bash
# Show what would change
cmake-format --check --config-file=.cmake-format.yaml CMakeLists.txt

# Show diff
cmake-format --check --config-file=.cmake-format.yaml --diff CMakeLists.txt
```

## Configuration

### File Location
- **Path**: `.cmake-format.yaml` (project root)
- **Format**: YAML
- **Key Settings**:
  - `line_width: 100` - Max line length
  - `indent_width: 2` - Spaces per indent
  - `dangle_parens: true` - Allow dangling parentheses

### Customization

Edit `.cmake-format.yaml` to adjust:
- Line width
- Indentation
- Comment handling
- Sorting behavior

## IDE Integration

### VS Code
1. Install "CMake Tools" extension
2. Add to `.vscode/settings.json`:
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
2. Set indentation to 2 spaces
3. Enable "Reformat Code" on save

### Vim/Neovim
Add to config:
```vim
autocmd FileType cmake setlocal formatprg=cmake-format\ -
```

## Workflow

### Before Committing

```bash
# 1. Format your changes
lintrunner --take CMAKEFORMAT --apply-patches

# 2. Verify formatting
lintrunner --only=CMAKEFORMAT

# 3. Commit
git add .
git commit -m "Format CMake files"
```

### In Pull Request

```bash
# CI will automatically check formatting
# If formatting issues found:
# 1. Pull latest changes
# 2. Run formatter locally
# 3. Commit and push
```

## Troubleshooting

### cmake-format not found
```bash
# Install it
pip install cmakelang==0.6.13

# Verify installation
cmake-format --version
```

### Configuration file not found
```bash
# Check file exists
ls -la .cmake-format.yaml

# Verify path in command
cmake-format --check --config-file=.cmake-format.yaml CMakeLists.txt
```

### Formatting takes too long
```bash
# Check file size
wc -l CMakeLists.txt

# Try with timeout
cmake-format --check --config-file=.cmake-format.yaml --timeout=180 CMakeLists.txt
```

### Conflicts with cmakelint
```bash
# Run formatter first, then linter
lintrunner --take CMAKEFORMAT --apply-patches
lintrunner --only=CMAKE
```

## File Patterns

### Included Files
- `CMakeLists.txt`
- `*.cmake`
- `*.cmake.in`

### Excluded Directories
- `ThirdParty/**`
- `Tools/**`
- `.vscode/**`
- `.git/**`
- `.augment/**`
- `build/**`
- `build_*/**`

## Configuration Reference

| Setting | Value | Purpose |
|---------|-------|---------|
| `line_width` | 100 | Max line length |
| `indent_width` | 2 | Spaces per indent |
| `tab_size` | 2 | Tab width |
| `use_tabchars` | false | Use spaces |
| `dangle_parens` | true | Allow dangling parens |
| `enable_sort` | true | Enable sorting |
| `autosort` | false | Manual sorting |

## Performance Tips

- **Parallel Processing**: Automatically uses all CPU cores
- **Concurrent Files**: Processes multiple files simultaneously
- **Timeout**: Default 90 seconds per file
- **Large Projects**: Scales well with concurrent processing

## Common Issues & Solutions

| Issue | Solution |
|-------|----------|
| Slow formatting | Check file size, increase timeout |
| Formatting conflicts | Run formatter first, then linter |
| Cross-platform issues | Use Git Bash on Windows |
| IDE not formatting | Verify extension installed and configured |

## Integration with Other Tools

### With clang-format
- Independent (different file types)
- Can run in parallel
- No conflicts

### With cmakelint
- Complementary (formatting vs linting)
- Run formatter first
- Then run linter

### With lintrunner
- Fully integrated
- Use `--take CMAKEFORMAT` to apply
- Use `--only=CMAKEFORMAT` to check

## Documentation

- **Full Guide**: `Docs/cmake-format-implementation-guide.md`
- **Technical Specs**: `Docs/cmake-format-technical-specs.md`
- **Analysis**: `Docs/cmake-format-integration-analysis.md`
- **cmake-format Docs**: https://cmake-format.readthedocs.io/

## Quick Links

- **Configuration**: `.cmake-format.yaml`
- **Adapter**: `Tools/linter/adapters/cmake_format_linter.py`
- **Script**: `Scripts/all-cmake-format.sh`
- **Lintrunner Config**: `.lintrunner.toml` (search for CMAKEFORMAT)

## Support

For issues:
1. Check this quick reference
2. Review implementation guide
3. Check cmake-format documentation
4. Open an issue in repository

---

**Last Updated**: 2025-10-27
**Version**: 1.0
**Status**: Production Ready
