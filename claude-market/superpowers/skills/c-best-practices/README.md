# C Language Best Practices Skill

This skill provides comprehensive guidance for advanced C developers working on system-level software, focusing on security, maintainability, and tool integration.

## Quick Start

1. **Install the skill**: Save to your Claude skills directory
2. **Setup environment**: Run `./scripts/setup-static-analysis.sh`
3. **Configure IDE**: Enable clang-format and clang-tidy integration
4. **Start coding**: Follow the patterns and examples provided

## Key Features

### 🔒 Memory Safety
- DMA-safe memory management
- Buffer overflow prevention
- Use-after-free detection
- Integer overflow handling

### 🔍 Static Analysis Integration
- clang-format for consistent code style
- clang-tidy for comprehensive static analysis
- cppcheck for additional checks
- CI/CD integration scripts

### 🏗️ Modular Architecture
- Clear interface design
- Version compatibility management
- Error handling patterns
- Thread safety guidelines

### 📦 Tool Integration
- Automated setup scripts
- Git hooks for quality checks
- Editor configuration files
- CI/CD pipeline examples

## Directory Structure

```
c-best-practices/
├── SKILL.md              # Main documentation
├── README.md             # This file
├── examples/             # Example implementations
│   ├── memory-safety.c   # Memory management patterns
│   ├── dma-example.c     # DMA programming example
│   ├── module-interface.h # Interface design
│   ├── key-patterns.c    # Essential C programming patterns
│   └── misra-compliant.c # MISRA C:2012 compliance example
├── configs/              # Tool configurations
│   ├── .clang-format     # Code formatting rules
│   └── clang-tidy.yaml   # Static analysis configuration
└── scripts/              # Utility scripts
    ├── setup-static-analysis.sh # Environment setup
    └── test-static-analysis.sh  # Quality checks
```

## Usage Examples

### Code Review Mode
1. Run static analysis: `./scripts/test-static-analysis.sh`
2. Check for memory issues: `cppcheck --enable=all your_code.c`
3. Format code: `clang-format -i your_code.c`

### During Development
1. Configure editor for real-time feedback
2. Use provided Makefile targets
3. Follow patterns from examples directory

#### MISRA C Mode
For safety-critical development requiring MISRA C compliance:

```bash
# Setup with MISRA C compliance mode
./scripts/setup-static-analysis.sh --misra

# This will:
# - Use stricter clang-tidy rules
# - Enable MISRA C:2012 specific checks
# - Apply additional safety constraints
```

See `examples/misra-compliant.c` for MISRA C compliant code examples.

## Integration Steps

1. **Clone or copy this skill to your skills directory**
2. **Run the setup script**:
   - Standard mode: `./scripts/setup-static-analysis.sh`
   - MISRA C mode: `./scripts/setup-static-analysis.sh --misra`
3. **Configure your IDE** with the provided configuration files
4. **Add the skill to your workflow** and reference it during development

### MISRA C Integration Checklist
- [ ] Safety-critical requirements identified
- [ ] MISRA C:2012 compliance mode enabled with `--misra` flag
- [ ] All team members trained on MISRA C constraints
- [ ] Code review includes MISRA C rule verification
- [ ] Static analysis configured with misra-c2012 checks
- [ ] Development environment enforces MISRA C rules

## Development Workflow

### Before Writing Code
- Check existing examples for similar patterns
- Ensure tools are installed and configured
- Review the relevant sections in SKILL.md

### During Development
- Use clang-format for consistent formatting
- Run clang-tidy frequently to catch issues early
- Follow memory safety patterns rigorously
- Maintain clear module boundaries

### Before Committing
- Run `./scripts/test-static-analysis.sh`
- Fix all reported warnings
- Ensure all error paths are tested
- Verify memory management is correct

## Best Practices Summary

### Memory Management
```c
// Always check allocations
void *ptr = kmalloc(size, GFP_KERNEL);
if (!ptr) {
    return -ENOMEM;
}

// Use bounded string operations
strlcpy(dest, src, sizeof(dest));

// Free in reverse order of allocation
kfree(ptr3);
kfree(ptr2);
kfree(ptr1);
```

### Error Handling
```c
// Use consistent error propagation
int result = function_call();
if (result != 0) {
    pr_err("Function failed: %d\n", result);
    return result;
}

// Cleanup on error path
int complex_function() {
    resource1 = alloc_resource1();
    if (!resource1) return -ENOMEM;

    resource2 = alloc_resource2();
    if (!resource2) {
        free_resource1(resource1);
        return -ENOMEM;
    }

    // ... use resources ...

    free_resource2(resource2);
    free_resource1(resource1);
    return 0;
}
```

### Thread Safety
```c
// Always lock shared data
mutex_lock(&device->lock);
device->shared_data++;
mutex_unlock(&device->lock);

// Use atomic operations for simple counters
atomic_inc(&device->packet_count);
```

## Tool Configuration

### VS Code Integration
Add to `.vscode/settings.json`:
```json
{
    "C_Cpp.default.cStandard": "c11",
    "C_Cpp.default.cppStandard": "c++17",
    "clang-format.executable": "clang-format",
    "clang-format.style": "file",
    "C_Cpp.codeAnalysis.clangTidy.enabled": true,
    "C_Cpp.codeAnalysis.clangTidy.config": "configs/clang-tidy.yaml"
}
```

### Vim Integration
Add to `.vimrc`:
```vim
" clang-format on save
autocmd BufWritePre *.c,*.h exec 'clang-format -i %'
" clang-tidy on syntax check
set makeprg=clang-tidy\ %\ --\ -I.
```

### Git Pre-commit Hook
The setup script automatically configures a pre-commit hook that:
- Checks code formatting with clang-format
- Runs cppcheck on changed files
- Prevents commits with formatting issues

## Common Issues and Solutions

### clang-format Conflicts
- Use `.clang-format` in project root
- Configure editor to use project settings
- Run `clang-format -i` before committing

### clang-tidy Warnings
- Review each warning carefully
- Add suppressions only for false positives
- Prefer fixing the code over suppressing warnings

### Memory Debugging
- Use Valgrind for userspace programs
- Enable KASAN for kernel debugging
- Use static analysis to prevent issues

## Contributing

This skill follows test-driven development. When adding new patterns:
1. Create failing test cases
2. Implement the pattern
3. Verify tools catch the issues
4. Update documentation

## License

This skill is provided as open source guidance for C developers.

## Related Resources

- [Linux Kernel Coding Style](https://www.kernel.org/doc/html/latest/process/coding-style.html)
- [CWE Classification](https://cwe.mitre.org/)
- [CERT C Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c/SEI+CERT+C+Coding+Standard)

## Support

For questions or issues:
1. Check the examples directory
2. Review SKILL.md for detailed patterns
3. Run the test script to verify your setup