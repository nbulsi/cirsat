#!/bin/bash

# Get the project root directory
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Format all C++ files in the project using .clang-format from root directory
find "$PROJECT_ROOT" \
    -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.cc" -o -name "*.h" \) \
    -not -path "*/build/*" \
    -not -path "*/lib/*" \
    -not -path "*/test/catch2/*" \
    -exec clang-format -i -style=file {} \;

echo "Code formatting completed!"
