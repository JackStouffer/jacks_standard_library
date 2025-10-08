#!/bin/bash

set -e  # Exit immediately if any command fails

echo "Checking for gcc..."
if ! command -v gcc &> /dev/null; then
    echo "ERROR: gcc is not installed or not in PATH"
    exit 1
fi

if ! command -v clang &> /dev/null; then
    echo "ERROR: clang is not installed or not in PATH"
    exit 1
fi

mkdir -p tests/bin

test_files=(tests/test_*.c)

if [ ${#test_files[@]} -eq 0 ] || [ ! -f "${test_files[0]}" ]; then
    echo "ERROR: No test files found matching tests/test_*.c"
    exit 1
fi

echo "Found ${#test_files[@]} test file(s):"
for file in "${test_files[@]}"; do
    echo "  - $(basename "$file")"
done
echo

# Test configurations: compiler, optimization flags, description
declare -a configs=(
    "gcc|-O0|GCC with O0"
    "gcc|-O3 -march=native|GCC with O3 and march=native"
    "clang|-O0|Clang with O0"
    "clang|-O3 -march=native|Clang with O3 and march=native"
)

# Run tests for each file with each configuration
for test_file in "${test_files[@]}"; do
    base_name=$(basename "$test_file" .c)
    echo "=== Testing $base_name ==="
    
    for config in "${configs[@]}"; do
        IFS='|' read -r compiler flags description <<< "$config"
        
        echo "[$description]"
        
        executable="tests/bin/${base_name}_${compiler}_$(echo "$flags" | tr ' -' '_' | tr ' =' '_')"
        echo "  Compiling: $compiler $flags $test_file -o $executable"
        
        if ! $compiler $flags "$test_file" -o "$executable" 2>&1; then
            echo "ERROR: Compilation failed for $test_file with $description"
            exit 1
        fi
        
        echo "  Running: $executable"
        if ! "$executable"; then
            echo "ERROR: Test execution failed for $test_file with $description"
            exit 1
        fi
        
        echo
    done
    
    echo
done

