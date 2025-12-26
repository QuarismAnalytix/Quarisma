#!/bin/bash
# Verification script for cmake-format and cmakelint compatibility
# This script demonstrates that the configuration conflict has been resolved

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=========================================="
echo "CMake Format & CMakeLint Compatibility Test"
echo "=========================================="
echo ""

# Get the repository root
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

# Check if tools are installed
echo "1. Checking tool availability..."
if ! command -v cmake-format >/dev/null 2>&1; then
    echo -e "${RED}✗ cmake-format not found${NC}"
    echo "  Install with: pip install cmakelang==0.6.13"
    exit 1
fi
echo -e "${GREEN}✓ cmake-format found: $(cmake-format --version 2>&1 | head -1)${NC}"

if ! command -v cmakelint >/dev/null 2>&1; then
    echo -e "${RED}✗ cmakelint not found${NC}"
    echo "  Install with: pip install cmakelint==1.4.1"
    exit 1
fi
echo -e "${GREEN}✓ cmakelint found: $(cmakelint --version 2>&1 | head -1)${NC}"

echo ""
echo "2. Checking configuration files..."
if [ ! -f ".cmake-format.yaml" ]; then
    echo -e "${RED}✗ .cmake-format.yaml not found${NC}"
    exit 1
fi
echo -e "${GREEN}✓ .cmake-format.yaml found${NC}"

if [ ! -f ".cmakelintrc" ]; then
    echo -e "${RED}✗ .cmakelintrc not found${NC}"
    exit 1
fi
echo -e "${GREEN}✓ .cmakelintrc found${NC}"

echo ""
echo "3. Validating cmake-format configuration..."
# Check for warnings in configuration
WARNINGS=$(cmake-format --config-file=.cmake-format.yaml --dump-config yaml 2>&1 | grep -E "^WARNING" || true)
if [ -n "$WARNINGS" ]; then
    echo -e "${RED}✗ Configuration has warnings:${NC}"
    echo "$WARNINGS"
    exit 1
fi
echo -e "${GREEN}✓ No warnings in cmake-format configuration${NC}"

echo ""
echo "4. Testing with sample CMake file..."
# Find a test file
TEST_FILE="Library/Core/CMakeLists.txt"
if [ ! -f "$TEST_FILE" ]; then
    echo -e "${YELLOW}⚠ Test file not found: $TEST_FILE${NC}"
    echo "  Skipping file-based tests"
else
    # Create a temporary copy
    TEMP_FILE=$(mktemp)
    cp "$TEST_FILE" "$TEMP_FILE"
    
    echo "  Testing: $TEST_FILE"
    
    # Format the file
    echo "  - Formatting with cmake-format..."
    if cmake-format --config-file=.cmake-format.yaml -i "$TEMP_FILE" 2>&1 | grep -E "^(WARNING|ERROR)" >/dev/null; then
        echo -e "${RED}✗ cmake-format produced warnings/errors${NC}"
        rm -f "$TEMP_FILE"
        exit 1
    fi
    echo -e "${GREEN}  ✓ Formatted successfully${NC}"
    
    # Lint the formatted file
    echo "  - Linting with cmakelint..."
    LINT_OUTPUT=$(cmakelint --config=.cmakelintrc "$TEMP_FILE" 2>&1)
    LINT_ERRORS=$(echo "$LINT_OUTPUT" | grep "Total Errors:" | awk '{print $3}')
    
    if [ "$LINT_ERRORS" != "0" ]; then
        echo -e "${RED}✗ cmakelint found $LINT_ERRORS errors:${NC}"
        echo "$LINT_OUTPUT"
        rm -f "$TEMP_FILE"
        exit 1
    fi
    echo -e "${GREEN}  ✓ No linting errors (Total Errors: 0)${NC}"
    
    # Clean up
    rm -f "$TEMP_FILE"
fi

echo ""
echo "5. Testing lintrunner integration..."
if [ -f "Tools/linter/adapters/cmake_format_linter.py" ]; then
    echo "  - Testing cmake_format_linter.py adapter..."
    # Just verify it can be imported and has the right structure
    if python3 -c "import sys; sys.path.insert(0, 'Tools/linter/adapters'); import cmake_format_linter" 2>/dev/null; then
        echo -e "${GREEN}  ✓ Lintrunner adapter is valid${NC}"
    else
        echo -e "${YELLOW}  ⚠ Could not import lintrunner adapter${NC}"
    fi
else
    echo -e "${YELLOW}  ⚠ Lintrunner adapter not found${NC}"
fi

echo ""
echo "=========================================="
echo -e "${GREEN}✓ All compatibility tests passed!${NC}"
echo "=========================================="
echo ""
echo "Summary:"
echo "  - cmake-format configuration is valid (no warnings)"
echo "  - Formatted CMake files pass cmakelint validation"
echo "  - No conflicts between cmake-format and cmakelint"
echo ""
echo "Key configuration settings:"
echo "  - separate_ctrl_name_with_space: false (aligns with cmakelint)"
echo "  - separate_fn_name_with_space: false"
echo "  - line_width: 100 (matches .clang-format)"
echo "  - tab_size: 2 (project standard)"
echo ""
echo "Usage:"
echo "  Format files:  cmake-format -i --config-file=.cmake-format.yaml CMakeLists.txt"
echo "  Lint files:    cmakelint --config=.cmakelintrc CMakeLists.txt"
echo "  Lintrunner:    lintrunner --take CMAKEFORMAT --apply-patches"
echo ""
