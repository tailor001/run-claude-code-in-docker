#!/bin/bash
# setup-static-analysis.sh
# Automated setup for static analysis tools in C projects

set -e

# Default mode
MISRA_MODE=false

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in a git repository
check_git_repo() {
    if ! git rev-parse --git-dir > /dev/null 2>&1; then
        print_warning "Not in a git repository. Git hooks will not be set up."
        return 1
    fi
    return 0
}

# Install tools based on package manager
install_tools() {
    print_status "Checking for required static analysis tools..."

    # Check which package manager is available
    if command -v apt-get &> /dev/null; then
        PKG_MANAGER="apt-get"
        INSTALL_CMD="sudo apt-get install -y"
    elif command -v yum &> /dev/null; then
        PKG_MANAGER="yum"
        INSTALL_CMD="sudo yum install -y"
    elif command -v brew &> /dev/null; then
        PKG_MANAGER="brew"
        INSTALL_CMD="brew install"
    else
        print_error "No supported package manager found. Please install manually:"
        echo "  - clang"
        echo "  - clang-tidy"
        echo "  - clang-format"
        echo "  - cppcheck"
        exit 1
    fi

    # Check if tools are installed
    MISSING_TOOLS=()

    for tool in clang clang-tidy clang-format cppcheck; do
        if ! command -v "$tool" &> /dev/null; then
            MISSING_TOOLS+=($tool)
        fi
    done

    if [ ${#MISSING_TOOLS[@]} -gt 0 ]; then
        print_status "Installing missing tools: ${MISSING_TOOLS[*]}"
        $INSTALL_CMD ${MISSING_TOOLS[*]}
    else
        print_status "All required tools are already installed!"
    fi
}

# Setup git pre-commit hook
setup_git_hooks() {
    if ! check_git_repo; then
        return 0
    fi

    print_status "Setting up git hooks..."

    # Create hooks directory if it doesn't exist
    mkdir -p .git/hooks

    # Create pre-commit hook
    cat > .git/hooks/pre-commit << 'EOF'
#!/bin/bash
# Pre-commit hook for C code quality

echo "Running pre-commit checks..."

# Find C/C++ files that are staged for commit
C_FILES=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(c|h|cpp|hpp|cc|cxx)$' || true)

if [ -z "$C_FILES" ]; then
    echo "No C/C++ files to check."
    exit 0
fi

# Check formatting with clang-format
echo "Checking code formatting..."
FORMAT_FAILED=0
for file in $C_FILES; do
    if [ -f "$file" ]; then
        if ! clang-format --dry-run --Werror "$file" 2>/dev/null; then
            echo "Formatting issues in $file"
            echo "Run: clang-format -i $file"
            FORMAT_FAILED=1
        fi
    fi
done

if [ $FORMAT_FAILED -ne 0 ]; then
    echo "Formatting issues found. Please fix them before committing."
    echo "Run 'clang-format -i' on all changed files."
    exit 1
fi

# Run static analysis on C files
echo "Running static analysis..."
C_SOURCE_FILES=$(echo "$C_FILES" | grep -E '\.c$' || true)

if [ -n "$C_SOURCE_FILES" ]; then
    # Run cppcheck
    if command -v cppcheck &> /dev/null; then
        echo "Running cppcheck..."
        cppcheck --error-exitcode=1 --quiet --enable=warning,style,performance,portability \
                 --suppress=missingIncludeSystem $C_SOURCE_FILES || {
            echo "cppcheck found issues. Please review and fix."
            exit 1
        }
    fi

    # Run clang-tidy if configuration exists
    if [ -f "clang-tidy.yaml" ] && command -v clang-tidy &> /dev/null; then
        echo "Running clang-tidy..."
        for file in $C_SOURCE_FILES; do
            if [ -f "$file" ]; then
                clang-tidy "$file" --config-file=clang-tidy.yaml --quiet || {
                    echo "clang-tidy found issues in $file. Please review and fix."
                    exit 1
                }
            fi
        done
    fi
fi

echo "All checks passed!"
exit 0
EOF

    chmod +x .git/hooks/pre-commit
    print_status "Git pre-commit hook installed successfully!"
}

# Copy configuration files
copy_configs() {
    print_status "Setting up configuration files..."

    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    CONFIG_DIR="$SCRIPT_DIR/../configs"

    # Copy .clang-format if it doesn't exist
    if [ ! -f ".clang-format" ]; then
        cp "$CONFIG_DIR/.clang-format" .
        print_status ".clang-format copied to project root"
    else
        print_warning ".clang-format already exists. Skipping."
    fi

    # Copy appropriate clang-tidy configuration
    if [ "$MISRA_MODE" = true ]; then
        if [ ! -f "clang-tidy.yaml" ]; then
            cp "$CONFIG_DIR/clang-tidy-misra.yaml" clang-tidy.yaml
            print_status "MISRA C clang-tidy configuration copied"
            print_warning "MISRA C mode enabled - strict compliance checks active"
        else
            print_warning "clang-tidy.yaml already exists. To use MISRA C:"
            echo "  1. Backup existing clang-tidy.yaml"
            echo "  2. Copy configs/clang-tidy-misra.yaml to clang-tidy.yaml"
        fi
    else
        if [ ! -f "clang-tidy.yaml" ]; then
            cp "$CONFIG_DIR/clang-tidy.yaml" .
            print_status "Standard clang-tidy.yaml copied to project root"
        else
            print_warning "clang-tidy.yaml already exists. Skipping."
        fi
    fi
}

# Create Makefile targets for static analysis
create_makefile_targets() {
    if [ -f "Makefile" ]; then
        print_warning "Makefile exists. Add these targets manually:"
        echo ""
        echo "# Static analysis targets"
        echo "format:"
        echo "	@find . -name '*.c' -o -name '*.h' | xargs clang-format -i"
        echo ""
        echo "check-format:"
        echo "	@find . -name '*.c' -o -name '*.h' | xargs clang-format --dry-run --Werror"
        echo ""
        echo "static-analysis:"
        echo "	@find . -name '*.c' | xargs clang-tidy"
        echo "	@find . -name '*.c' | xargs cppcheck --enable=all"
        echo ""
        echo ".PHONY: format check-format static-analysis"
    fi
}

# Verify installation
verify_installation() {
    print_status "Verifying installation..."

    # Check tools
    echo ""
    echo "Tool versions:"
    clang --version 2>/dev/null | head -1 || print_error "clang not found"
    clang-format --version 2>/dev/null || print_error "clang-format not found"
    clang-tidy --version 2>/dev/null || print_error "clang-tidy not found"
    cppcheck --version 2>/dev/null || print_error "cppcheck not found"

    # Check configurations
    echo ""
    echo "Configuration files:"
    [ -f ".clang-format" ] && echo "✓ .clang-format exists" || print_error ".clang-format missing"
    [ -f "clang-tidy.yaml" ] && echo "✓ clang-tidy.yaml exists" || print_error "clang-tidy.yaml missing"

    # Check git hook
    if check_git_repo; then
        [ -f ".git/hooks/pre-commit" ] && echo "✓ Pre-commit hook installed" || print_warning "Pre-commit hook not installed"
    fi
}

# Print usage instructions
print_usage() {
    echo ""
    print_status "Setup complete! Your development environment is now configured."
    echo ""
    echo "Next steps:"
    echo "1. Configure your editor for real-time feedback:"
    echo "   - VS Code: Install C/C++ extension"
    echo "   - Vim/Neovim: Add clangd integration"
    echo "   - Emacs: Use flycheck with clang-tidy"
    echo ""
    echo "2. Common commands:"
    echo "   - Format code: clang-format -i file.c"
    echo "   - Check formatting: clang-format --dry-run --Werror file.c"
    echo "   - Static analysis: clang-tidy file.c -- -Iinclude"
    echo "   - Additional checks: cppcheck --enable=all file.c"
    echo ""
    echo "3. Git hooks will automatically run checks before each commit."
    echo ""
}

# Main execution
main() {
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --misra)
                MISRA_MODE=true
                print_status "MISRA C mode enabled"
                shift
                ;;
            --help|-h)
                echo "Usage: $0 [--misra] [--help]"
                echo ""
                echo "Options:"
                echo "  --misra    Enable MISRA C:2012 compliance mode"
                echo "  --help     Show this help message"
                echo ""
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                echo "Use --help for usage information"
                exit 1
                ;;
        esac
    done

    print_status "Setting up static analysis tools for C development..."
    echo ""

    install_tools
    copy_configs
    setup_git_hooks
    create_makefile_targets
    verify_installation
    print_usage
}

# Run main function
main "$@"