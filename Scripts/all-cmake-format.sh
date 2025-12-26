#!/bin/bash
# cmake-format script for XSigma
# Formats all CMake files in the project according to .cmake-format.yaml
# Usage: bash Scripts/all-cmake-format.sh

set -e

# Get the repository root
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

# Find cmake-format executable
FMT=""
for fmt_cmd in cmake-format cmake-format-py; do
  if command -v "$fmt_cmd" >/dev/null 2>&1; then
    FMT="$fmt_cmd"
    break
  fi
done

if [ -z "$FMT" ]; then
  echo "Error: cmake-format not found in PATH" >&2
  echo "Install it with: pip install cmakelang" >&2
  exit 1
fi

echo "Using $FMT"

# Find all CMake files (excluding ThirdParty and build directories)
echo "Scanning for CMake files..."
find "$REPO_ROOT" \
  -type d \( -name .git -o -name .vscode -o -name .augment -o -name ThirdParty -o -name build -o -name 'build_*' -o -name dist \) -prune -false -o \
  -type f \( -name "CMakeLists.txt" -o -name "*.cmake" -o -name "*.cmake.in" \) -print0 \
  | xargs -0 -I{} "$FMT" -i --config-file=.cmake-format.yaml {}

echo "cmake-format complete."