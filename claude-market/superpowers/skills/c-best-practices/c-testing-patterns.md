# C Testing Patterns with TDD

**Purpose**: Complete examples of applying test-driven-development RED-GREEN-REFACTOR cycle to C programming.

## Core TDD Workflow for C

### Testing Frameworks

#### Check Framework
```c
#include "check.h"
#include "buffer.h"

START_TEST(test_safe_copy_empty_string) {
    char dest[10];
    ck_assert_int_eq(SUCCESS, safe_copy(dest, "", sizeof(dest)));
    ck_assert_str_eq("", dest);
}
END_TEST

Suite *buffer_suite(void) {
    Suite *s = suite_create("Buffer");
    TCase *tc = tcase_create("Core");
    tcase_add_test(tc, test_safe_copy_empty_string);
    suite_add_tcase(s, tc);
    return s;
}
```

#### Unity Framework
```c
#include "unity.h"
#include "buffer.h"

void setUp(void) {
    // Setup before each test
}

void tearDown(void) {
    // Cleanup after each test
}

void test_safe_copy_should_handle_empty_string(void) {
    char dest[10];
    TEST_ASSERT_EQUAL(SUCCESS, safe_copy(dest, "", sizeof(dest)));
    TEST_ASSERT_EQUAL_STRING("", dest);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_safe_copy_should_handle_empty_string);
    return UNITY_END();
}
```

## Complete TDD Examples

### Example 1: Safe String Copy

#### RED Phase - Write Failing Tests
```c
// test_buffer.c
#include "unity.h"
#include "buffer.h"

void test_safe_copy_empty_string(void);
void test_safe_copy_null_destination(void);
void test_safe_copy_null_source(void);
void test_safe_copy_buffer_too_small(void);
void test_safe_copy_exact_fit(void);
void test_safe_copy_normal_case(void);

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_safe_copy_empty_string);
    RUN_TEST(test_safe_copy_null_destination);
    RUN_TEST(test_safe_copy_null_source);
    RUN_TEST(test_safe_copy_buffer_too_small);
    RUN_TEST(test_safe_copy_exact_fit);
    RUN_TEST(test_safe_copy_normal_case);
    return UNITY_END();
}

// All tests will fail because safe_copy doesn't exist yet
```

#### GREEN Phase - Minimal Implementation
```c
// buffer.h
#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>

typedef enum {
    SUCCESS = 0,
    FAILURE = 1
} buffer_result_t;

buffer_result_t safe_copy(char *dest, const char *src, size_t dest_size);

#endif
```

```c
// buffer.c
#include "buffer.h"

buffer_result_t safe_copy(char *dest, const char *src, size_t dest_size) {
    return FAILURE;  // Always fail - makes tests pass
}
```

#### REFACTOR Phase - Real Implementation
```c
// buffer.c - Complete implementation
#include "buffer.h"
#include <string.h>

buffer_result_t safe_copy(char *dest, const char *src, size_t dest_size) {
    // Handle NULL pointers
    if (!dest || !src) {
        return FAILURE;
    }

    // Handle zero-size buffer
    if (dest_size == 0) {
        return FAILURE;
    }

    size_t src_len = strlen(src);

    // Check if source fits in destination
    if (src_len >= dest_size) {
        return FAILURE;
    }

    // Safe copy
    strcpy(dest, src);

    return SUCCESS;
}
```

### Example 2: Memory Pool Allocator

#### RED Phase
```c
// test_memory_pool.c
#include "unity.h"
#include "memory_pool.h"

static memory_pool_t *pool;
static void *allocated[10];

void setUp(void) {
    pool = memory_pool_create(1024);
    TEST_ASSERT_NOT_NULL(pool);
}

void tearDown(void) {
    memory_pool_destroy(pool);
}

void test_memory_pool_allocate_basic(void) {
    void *ptr = memory_pool_allocate(pool, 100);
    TEST_ASSERT_NOT_NULL(ptr);
    TEST_ASSERT_TRUE(memory_pool_contains(pool, ptr));
}

void test_memory_pool_allocate_all_memory(void) {
    void *ptr = memory_pool_allocate(pool, 1024);
    TEST_ASSERT_NOT_NULL(ptr);

    // Should fail - no memory left
    void *ptr2 = memory_pool_allocate(pool, 1);
    TEST_ASSERT_NULL(ptr2);
}

void test_memory_pool_reset_frees_all(void) {
    // Allocate all memory
    void *ptr = memory_pool_allocate(pool, 1024);
    TEST_ASSERT_NOT_NULL(ptr);

    memory_pool_reset(pool);

    // Should be able to allocate again
    void *ptr2 = memory_pool_allocate(pool, 1024);
    TEST_ASSERT_NOT_NULL(ptr2);
}
```

#### GREEN Phase
```c
// memory_pool.c - Minimal implementation
#include "memory_pool.h"

memory_pool_t *memory_pool_create(size_t size) {
    return (memory_pool_t *)0x1;  // Fake non-NULL pointer
}

void *memory_pool_allocate(memory_pool_t *pool, size_t size) {
    return (void *)0x1000;  // Fake non-NULL pointer
}

bool memory_pool_contains(memory_pool_t *pool, void *ptr) {
    return true;  // Always true
}

void memory_pool_reset(memory_pool_t *pool) {
    // No-op
}

void memory_pool_destroy(memory_pool_t *pool) {
    // No-op
}
```

#### REFACTOR Phase
```c
// memory_pool.c - Real implementation
#include "memory_pool.h"
#include <stdlib.h>
#include <string.h>

struct memory_pool {
    uint8_t *buffer;
    size_t size;
    size_t used;
    size_t peak_used;
};

memory_pool_t *memory_pool_create(size_t size) {
    memory_pool_t *pool = malloc(sizeof(memory_pool_t));
    if (!pool) return NULL;

    pool->buffer = malloc(size);
    if (!pool->buffer) {
        free(pool);
        return NULL;
    }

    pool->size = size;
    pool->used = 0;
    pool->peak_used = 0;

    return pool;
}

void *memory_pool_allocate(memory_pool_t *pool, size_t size) {
    if (!pool) return NULL;

    // Align to 8-byte boundary
    size = (size + 7) & ~7;

    if (pool->used + size > pool->size) {
        return NULL;  // Not enough memory
    }

    void *ptr = pool->buffer + pool->used;
    pool->used += size;

    if (pool->used > pool->peak_used) {
        pool->peak_used = pool->used;
    }

    return ptr;
}

bool memory_pool_contains(memory_pool_t *pool, void *ptr) {
    if (!pool || !ptr) return false;

    uint8_t *addr = (uint8_t *)ptr;
    return addr >= pool->buffer && addr < pool->buffer + pool->size;
}

void memory_pool_reset(memory_pool_t *pool) {
    if (pool) {
        pool->used = 0;
    }
}

void memory_pool_destroy(memory_pool_t *pool) {
    if (pool) {
        free(pool->buffer);
        free(pool);
    }
}
```

## Integration with Static Analysis

### Running Tests with Valgrind
```bash
#!/bin/bash
# run_tests.sh

echo "Running unit tests..."
./test_runner

echo "Running tests under Valgrind..."
valgrind --leak-check=full --error-exitcode=1 ./test_runner

echo "Running static analysis..."
clang-format -i *.c *.h
clang-tidy *.c -warnings-as-errors=*
```

### CMake Integration
```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(CExample)

# Enable testing
enable_testing()

# Find required packages
find_package(Check REQUIRED)

# Add executable
add_executable(test_runner test_buffer.c buffer.c)

# Link with Check
target_link_libraries(test_runner check)

# Add test
add_test(NAME BufferTests COMMAND test_runner)

# Custom target for Valgrind
add_custom_target(valgrind
    COMMAND valgrind --leak-check=full --error-exitcode=1 ./test_runner
    DEPENDS test_runner
    COMMENT "Running tests under Valgrind"
)

# Custom target for static analysis
add_custom_target(clang-tidy
    COMMAND clang-tidy *.c -warnings-as-errors=*
    COMMENT "Running clang-tidy"
)
```

## Best Practices

### Test Organization
- **One test per file**: Keep test files focused
- **Descriptive names**: `test_function_name_expected_behavior`
- **Arrange-Act-Assert**: Structure tests clearly
- **Setup/Teardown**: Use Unity's setUp/tearDown for resource management

### Memory Testing
- **Always test**: malloc failure paths
- **Use Valgrind**: In CI pipeline
- **Track leaks**: Zero tolerance for memory leaks
- **Test boundary conditions**: Zero size, maximum size

### Integration with Superpowers 4.0.0

When writing C tests:
1. **Start with test-driven-development**: Write failing test first
2. **Apply systematic-debugging**: If tests fail unexpectedly
3. **Use verification-before-completion**: Before committing
4. **Follow c-best-practices**: Throughout implementation

This ensures robust, memory-safe C code with comprehensive test coverage.