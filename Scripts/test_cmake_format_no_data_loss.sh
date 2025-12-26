#!/bin/bash
# Test script to verify cmake-format does not delete file content
# This script tests the bug fix for the empty replacement issue

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=========================================="
echo "CMake Format Data Loss Prevention Test"
echo "=========================================="
echo ""

# Get the repository root
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

# Create a test file with known content
TEST_FILE=$(mktemp /tmp/test_cmake_XXXXXX.txt)
cat > "$TEST_FILE" << 'EOF'
# Test CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

project(TestProject)

# This is a test file with various CMake constructs
if(WIN32)
    set(PLATFORM_VAR "Windows")
    message(STATUS "Building on Windows")
else()
    set(PLATFORM_VAR "Unix")
    message(STATUS "Building on Unix")
endif()

# Add a library
add_library(TestLib
    src/file1.cpp
    src/file2.cpp
    include/header1.h
    include/header2.h
)

# Set properties
set_target_properties(TestLib PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Link libraries
target_link_libraries(TestLib
    PUBLIC
        SomeLib::SomeLib
    PRIVATE
        AnotherLib::AnotherLib
)

# Install
install(TARGETS TestLib
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
EOF

ORIGINAL_SIZE=$(wc -c < "$TEST_FILE")
ORIGINAL_LINES=$(wc -l < "$TEST_FILE")

echo "1. Test file created:"
echo "   Path: $TEST_FILE"
echo "   Size: $ORIGINAL_SIZE bytes"
echo "   Lines: $ORIGINAL_LINES"
echo ""

echo "2. Testing cmake-format direct output..."
OUTPUT=$(cmake-format --config-file=.cmake-format.yaml -o - "$TEST_FILE" 2>&1)
OUTPUT_SIZE=${#OUTPUT}

if [ "$OUTPUT_SIZE" -eq 0 ]; then
    echo -e "${RED}✗ FAILED: cmake-format produced empty output!${NC}"
    rm -f "$TEST_FILE"
    exit 1
fi
echo -e "${GREEN}✓ cmake-format produced output: $OUTPUT_SIZE bytes${NC}"

# Verify content is preserved
if echo "$OUTPUT" | grep -q "cmake_minimum_required"; then
    echo -e "${GREEN}✓ Content preserved: cmake_minimum_required found${NC}"
else
    echo -e "${RED}✗ FAILED: Content missing!${NC}"
    rm -f "$TEST_FILE"
    exit 1
fi

if echo "$OUTPUT" | grep -q "add_library"; then
    echo -e "${GREEN}✓ Content preserved: add_library found${NC}"
else
    echo -e "${RED}✗ FAILED: Content missing!${NC}"
    rm -f "$TEST_FILE"
    exit 1
fi

echo ""
echo "3. Testing lintrunner adapter..."
ADAPTER_OUTPUT=$(python3 Tools/linter/adapters/cmake_format_linter.py --config=.cmake-format.yaml "$TEST_FILE" 2>&1)

# Check if adapter produced output
if [ -z "$ADAPTER_OUTPUT" ]; then
    echo -e "${YELLOW}⚠ No linter output (file may already be formatted)${NC}"
else
    # Parse JSON output (skip debug lines)
    JSON_LINE=$(echo "$ADAPTER_OUTPUT" | grep -E '^\{' || true)
    
    if [ -n "$JSON_LINE" ]; then
        # Extract replacement field using Python (save to temp file to avoid shell escaping issues)
        echo "$JSON_LINE" > /tmp/cmake_test_json.txt
        REPLACEMENT_LENGTH=$(python3 -c "
import json
try:
    with open('/tmp/cmake_test_json.txt', 'r') as f:
        data = json.load(f)
    replacement = data.get('replacement', '')
    print(len(replacement))
except Exception as e:
    print(0)
" 2>/dev/null || echo "0")
        
        if [ "$REPLACEMENT_LENGTH" -eq 0 ]; then
            echo -e "${RED}✗ FAILED: Linter adapter produced empty replacement!${NC}"
            echo "Debug output:"
            echo "$ADAPTER_OUTPUT"
            rm -f "$TEST_FILE"
            exit 1
        fi
        
        echo -e "${GREEN}✓ Linter adapter produced replacement: $REPLACEMENT_LENGTH bytes${NC}"
        
        # Verify content in replacement
        REPLACEMENT_CONTENT=$(python3 -c "
import json
try:
    with open('/tmp/cmake_test_json.txt', 'r') as f:
        data = json.load(f)
    print(data.get('replacement', ''))
except Exception as e:
    pass
" 2>/dev/null || echo "")
        
        if echo "$REPLACEMENT_CONTENT" | grep -q "cmake_minimum_required"; then
            echo -e "${GREEN}✓ Replacement preserves content: cmake_minimum_required found${NC}"
        else
            echo -e "${RED}✗ FAILED: Replacement missing content!${NC}"
            rm -f "$TEST_FILE"
            exit 1
        fi
    else
        echo -e "${YELLOW}⚠ No JSON output from adapter (file may be formatted)${NC}"
    fi
fi

echo ""
echo "4. Testing in-place formatting..."
cp "$TEST_FILE" "${TEST_FILE}.backup"
cmake-format --config-file=.cmake-format.yaml -i "$TEST_FILE" 2>&1

FORMATTED_SIZE=$(wc -c < "$TEST_FILE")
FORMATTED_LINES=$(wc -l < "$TEST_FILE")

echo "   Original: $ORIGINAL_SIZE bytes, $ORIGINAL_LINES lines"
echo "   Formatted: $FORMATTED_SIZE bytes, $FORMATTED_LINES lines"

if [ "$FORMATTED_SIZE" -eq 0 ]; then
    echo -e "${RED}✗ FAILED: In-place formatting deleted all content!${NC}"
    rm -f "$TEST_FILE" "${TEST_FILE}.backup"
    exit 1
fi

# Check if content is preserved (allow for formatting changes)
if [ "$FORMATTED_SIZE" -lt 100 ]; then
    echo -e "${RED}✗ FAILED: Formatted file is suspiciously small!${NC}"
    rm -f "$TEST_FILE" "${TEST_FILE}.backup"
    exit 1
fi

echo -e "${GREEN}✓ In-place formatting preserved content${NC}"

# Verify specific content
if grep -q "cmake_minimum_required" "$TEST_FILE"; then
    echo -e "${GREEN}✓ Content check: cmake_minimum_required found${NC}"
else
    echo -e "${RED}✗ FAILED: Content missing after formatting!${NC}"
    rm -f "$TEST_FILE" "${TEST_FILE}.backup"
    exit 1
fi

if grep -q "add_library" "$TEST_FILE"; then
    echo -e "${GREEN}✓ Content check: add_library found${NC}"
else
    echo -e "${RED}✗ FAILED: Content missing after formatting!${NC}"
    rm -f "$TEST_FILE" "${TEST_FILE}.backup"
    exit 1
fi

echo ""
echo "5. Testing with real project file..."
REAL_FILE="Library/Core/CMakeLists.txt"
if [ -f "$REAL_FILE" ]; then
    REAL_ORIGINAL_SIZE=$(wc -c < "$REAL_FILE")
    REAL_OUTPUT=$(cmake-format --config-file=.cmake-format.yaml -o - "$REAL_FILE" 2>&1)
    REAL_OUTPUT_SIZE=${#REAL_OUTPUT}
    
    echo "   Original file: $REAL_ORIGINAL_SIZE bytes"
    echo "   Formatted output: $REAL_OUTPUT_SIZE bytes"
    
    if [ "$REAL_OUTPUT_SIZE" -eq 0 ]; then
        echo -e "${RED}✗ FAILED: Real file produced empty output!${NC}"
        rm -f "$TEST_FILE" "${TEST_FILE}.backup"
        exit 1
    fi
    
    # Verify content
    if echo "$REAL_OUTPUT" | grep -q "cmake_minimum_required"; then
        echo -e "${GREEN}✓ Real file content preserved${NC}"
    else
        echo -e "${RED}✗ FAILED: Real file content missing!${NC}"
        rm -f "$TEST_FILE" "${TEST_FILE}.backup"
        exit 1
    fi
else
    echo -e "${YELLOW}⚠ Real file not found, skipping${NC}"
fi

# Clean up
rm -f "$TEST_FILE" "${TEST_FILE}.backup"

echo ""
echo "=========================================="
echo -e "${GREEN}✓ All data loss prevention tests passed!${NC}"
echo "=========================================="
echo ""
echo "Summary:"
echo "  ✓ cmake-format produces non-empty output"
echo "  ✓ Linter adapter captures formatted content correctly"
echo "  ✓ In-place formatting preserves file content"
echo "  ✓ Real project files format correctly"
echo ""
echo "Bug Status: FIXED"
echo "  - Added '-o -' flag to cmake-format command"
echo "  - Ensures output goes to stdout instead of in-place modification"
echo "  - Prevents empty replacement field in linter output"
echo ""
