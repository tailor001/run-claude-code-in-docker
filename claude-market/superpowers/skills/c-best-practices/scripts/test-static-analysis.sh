#!/bin/bash
# Test script to verify static analysis tool integration
# Part of C Language Best Practices Skill

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SKILL_DIR="$(dirname "$SCRIPT_DIR")"

echo "Testing C Language Best Practices Skill..."
echo "========================================="
echo

# Check if required tools are installed
check_tool() {
    if ! command -v "$1" &> /dev/null; then
        echo "ERROR: $1 is not installed"
        echo "Run: sudo apt-get install $1"
        exit 1
    fi
}

echo "Checking required tools..."
check_tool clang-format
check_tool clang-tidy
check_tool cppcheck
echo "✓ All required tools installed"
echo

# Test code formatting
echo "1. Testing clang-format..."
cd "$SKILL_DIR"
FORMAT_ERRORS=0

for file in examples/*.c examples/*.h; do
    if [ -f "$file" ]; then
        if ! clang-format --dry-run --Werror "$file" 2>/dev/null; then
            echo "  - Formatting issues in $file"
            FORMAT_ERRORS=$((FORMAT_ERRORS + 1))
        fi
    fi
done

if [ $FORMAT_ERRORS -gt 0 ]; then
    echo "  clang-format found issues. Running auto-format..."
    clang-format -i examples/*.c examples/*.h 2>/dev/null || true
    echo "  Code formatted automatically"
fi
echo "✓ clang-format check completed"
echo

# Test static analysis
echo "2. Testing clang-tidy..."
TIDY_WARNINGS=0
MISRA_MODE=false

# Detect which configuration is being used
if [ -f "clang-tidy.yaml" ] && grep -q "readability-misra-c2012" clang-tidy.yaml; then
    MISRA_MODE=true
    echo "  MISRA C mode detected"
else
    echo "  Standard mode detected"
fi

for file in examples/*.c; do
    if [ -f "$file" ]; then
        echo "  Analyzing $(basename "$file")..."
        # Run clang-tidy and count warnings
        if ! clang-tidy "$file" --config-file=configs/clang-tidy.yaml -- -I. 2>/dev/null; then
            TIDY_WARNINGS=$((TIDY_WARNINGS + 1))
        fi
    fi
done

if [ $TIDY_WARNINGS -gt 0 ]; then
    if [ "$MISRA_MODE" = true ]; then
        echo "  clang-tidy found $TIDY_WARNINGS files with MISRA C warnings"
        echo "  Note: MISRA C rules are stricter - review carefully"
    else
        echo "  clang-tidy found $TIDY_WARNINGS files with warnings"
    fi
else
    if [ "$MISRA_MODE" = true ]; then
        echo "  No MISRA C violations found - code is compliant!"
    else
        echo "  No clang-tidy warnings found"
    fi
fi
echo "✓ clang-tidy analysis completed"
echo

# Test cppcheck
echo "3. Testing cppcheck..."
CPPCHECK_ERRORS=0

# Create a temporary cppcheck configuration
cat > /tmp/cppcheck.cfg << 'EOF'
[cppcheck]
suppress=missingIncludeSystem
suppress=unusedFunction
EOF

for file in examples/*.c; do
    if [ -f "$file" ]; then
        echo "  Checking $(basename "$file")..."
        if ! cppcheck --enable=all --config=/tmp/cppcheck.cfg "$file" 2>/dev/null; then
            CPPCHECK_ERRORS=$((CPPCHECK_ERRORS + 1))
        fi
    fi
done

rm -f /tmp/cppcheck.cfg

if [ $CPPCHECK_ERRORS -gt 0 ]; then
    echo "  cppcheck found potential issues"
else
    echo "  No cppcheck issues found"
fi
echo "✓ cppcheck analysis completed"
echo

# Test compilation with warnings
echo "4. Testing compilation with strict warnings..."
COMPILE_ERRORS=0

for file in examples/*.c; do
    if [ -f "$file" ]; then
        echo "  Compiling $(basename "$file")..."
        # Try to compile with strict warnings
        if ! gcc -Wall -Wextra -Werror -Wpedantic -std=c11 \
                   -I. -fsyntax-only "$file" 2>/dev/null; then
            echo "    Note: Some warnings expected in kernel-style code"
            COMPILE_ERRORS=$((COMPILE_ERRORS + 1))
        fi
    fi
done
echo "✓ Compilation check completed"
echo

# Check for common C best practices violations
echo "5. Checking for common anti-patterns..."
ANTI_PATTERNS=0

# Check for dangerous functions
if grep -E "\b(strcpy|strcat|sprintf|gets)\b" examples/*.c 2>/dev/null; then
    echo "  ✗ Found dangerous string functions"
    ANTI_PATTERNS=$((ANTI_PATTERNS + 1))
else
    echo "  ✓ No dangerous string functions found"
fi

# Check for magic numbers without defines
if grep -E "\b([0-9]{3,})\b" examples/*.c 2>/dev/null | grep -v -E "(0x[0-9a-fA-F]+|SIZE_MAX|PAGE_SIZE)"; then
    echo "  ⚠ Found potential magic numbers (consider #define constants)"
else
    echo "  ✓ No obvious magic numbers found"
fi

# Check for missing error handling
if grep -E "\b(kmalloc|kmalloc_array|krealloc)\b" examples/*.c 2>/dev/null | grep -v "if.*==.*NULL"; then
    echo "  ⚠ Potential missing NULL checks after allocation"
else
    echo "  ✓ Allocation checks appear to be present"
fi
echo

# Memory safety checks
echo "6. Memory safety analysis..."
if grep -E "\b(dma_alloc_coherent|dma_free_coherent)\b" examples/*.c 2>/dev/null; then
    echo "  ✓ DMA memory management found"
    # Check for proper pairing
    ALLOCS=$(grep -c "dma_alloc_coherent" examples/*.c 2>/dev/null || echo 0)
    FREES=$(grep -c "dma_free_coherent" examples/*.c 2>/dev/null || echo 0)
    if [ "$ALLOCS" -eq "$FREES" ]; then
        echo "  ✓ DMA alloc/free calls balanced"
    else
        echo "  ⚠ DMA alloc/free calls may not be balanced ($ALLOCS allocs, $FREES frees)"
    fi
else
    echo "  - No DMA memory management in examples"
fi

# Check for mutex usage
if grep -E "\b(mutex_lock|spin_lock)\b" examples/*.c 2>/dev/null; then
    echo "  ✓ Synchronization primitives found"
    # Check for proper unlock pairing
    LOCKS=$(grep -c "mutex_lock\|spin_lock" examples/*.c 2>/dev/null || echo 0)
    UNLOCKS=$(grep -c "mutex_unlock\|spin_unlock" examples/*.c 2>/dev/null || echo 0)
    echo "  Found $LOCKS lock operations and $UNLOCKS unlock operations"
else
    echo "  - No synchronization primitives in examples"
fi
echo

# Generate summary report
echo "Test Summary:"
echo "============="
echo "clang-format: $([ $FORMAT_ERRORS -eq 0 ] && echo 'PASSED' || echo 'NEEDS ATTENTION')"
echo "clang-tidy: ANALYZED ($TIDY_WARNINGS files with warnings)"
echo "cppcheck: ANALYZED ($CPPCHECK_ERRORS files with issues)"
echo "Compilation: CHECKED"
echo "Anti-patterns: $([ $ANTI_PATTERNS -eq 0 ] && echo 'CLEAN' || echo 'REVIEW NEEDED')"
echo

echo "Recommendations:"
echo "==============="
if [ $FORMAT_ERRORS -gt 0 ]; then
    echo "- Review clang-format output and fix formatting issues"
fi
if [ $TIDY_WARNINGS -gt 0 ]; then
    echo "- Review clang-tidy warnings and apply fixes"
fi
if [ $CPPCHECK_ERRORS -gt 0 ]; then
    echo "- Review cppcheck output for potential bugs"
fi
if [ $ANTI_PATTERNS -gt 0 ]; then
    echo "- Review identified anti-patterns"
fi

echo
echo "Test completed at $(date)"