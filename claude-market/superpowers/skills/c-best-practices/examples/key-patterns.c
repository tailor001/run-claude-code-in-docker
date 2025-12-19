/**
 * Key C Programming Patterns Example
 *
 * This file demonstrates the most important patterns from the C best practices skill:
 * - Memory safety with bounds checking
 * - Structured error handling with cleanup
 * - Thread-safe operations with proper synchronization
 * - Modular design with clear interfaces
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/atomic.h>
#include <linux/errno.h>

/* Configuration constants with bounds */
#define MAX_DEVICES   64
#define BUFFER_SIZE   PAGE_SIZE
#define TIMEOUT_MS    5000

/* Example 1: Memory Safety Pattern */
struct safe_buffer {
    size_t size;           /* Current size */
    size_t capacity;       /* Maximum capacity */
    void *data;           /* Actual data */
    dma_addr_t dma_addr;  /* DMA address */
};

/**
 * Allocate buffer with overflow protection
 */
int buffer_alloc(struct safe_buffer *buf, size_t requested_size) {
    /* Check for overflow */
    if (requested_size > MAX_DEVICES * BUFFER_SIZE) {
        pr_err("Requested size too large: %zu\n", requested_size);
        return -EINVAL;
    }

    /* Zero-allocate for safety */
    buf->data = dma_alloc_coherent(NULL, requested_size,
                                   &buf->dma_addr, GFP_KERNEL);
    if (!buf->data) {
        return -ENOMEM;
    }

    buf->size = 0;
    buf->capacity = requested_size;
    return 0;
}

/**
 * Safe buffer write with bounds checking
 */
int buffer_write(struct safe_buffer *buf, const void *data, size_t len) {
    /* Validate parameters */
    if (!buf || !data || !buf->data) {
        return -EINVAL;
    }

    /* Check bounds */
    if (buf->size + len > buf->capacity) {
        pr_err("Buffer overflow: %zu + %zu > %zu\n",
               buf->size, len, buf->capacity);
        return -EOVERFLOW;
    }

    /* Safe memory copy */
    memcpy(buf->data + buf->size, data, len);
    buf->size += len;
    return 0;
}

/* Example 2: Structured Error Handling Pattern */
struct device_context {
    struct mutex lock;
    void *resource1;
    void *resource2;
    atomic_t ref_count;
    bool initialized;
};

/**
 * Complex operation with proper cleanup
 */
int device_operation(struct device_context *ctx, void *input, size_t len) {
    int ret = 0;
    void *temp_buffer = NULL;

    /* Pre-condition validation */
    if (!ctx || !input || len == 0) {
        return -EINVAL;
    }

    /* Acquire lock with timeout */
    mutex_lock(&ctx->lock);

    /* Check device state */
    if (!ctx->initialized) {
        ret = -ENODEV;
        goto unlock;
    }

    /* Allocate temporary resource */
    temp_buffer = kmalloc(len, GFP_KERNEL);
    if (!temp_buffer) {
        ret = -ENOMEM;
        goto unlock;
    }

    /* Perform main operation */
    ret = process_data(input, len, temp_buffer);
    if (ret != 0) {
        pr_err("Data processing failed: %d\n", ret);
        goto cleanup;
    }

    /* Success path */
    atomic_inc(&ctx->ref_count);

cleanup:
    kfree(temp_buffer);
unlock:
    mutex_unlock(&ctx->lock);
    return ret;
}

/* Example 3: Thread-Safe Pattern */
struct counter {
    atomic_t value;
    spinlock_t lock;
    struct list_head waiters;
};

/**
 * Thread-safe increment with notification
 */
int counter_increment(struct counter *ctr, int delta) {
    unsigned long flags;
    int old_value, new_value;

    /* Fast path: atomic operation */
    old_value = atomic_read(&ctr->value);
    new_value = old_value + delta;

    /* Check for overflow */
    if (delta > 0 && new_value < old_value) {
        return -EOVERFLOW;
    }
    if (delta < 0 && new_value > old_value) {
        return -EOVERFLOW;
    }

    /* Atomic update */
    atomic_set(&ctr->value, new_value);

    /* Notify waiters if needed (requires lock) */
    if (list_empty(&ctr->waiters)) {
        return 0;
    }

    spin_lock_irqsave(&ctr->lock, flags);
    wake_up_all_locked(&ctr->waiters);
    spin_unlock_irqrestore(&ctr->lock, flags);

    return 0;
}

/* Example 4: Modular Interface Pattern */
struct module_ops {
    int (*init)(void *context);
    int (*process)(void *context, void *data, size_t len);
    void (*cleanup)(void *context);
    const char *name;
    u32 version;
};

struct module_instance {
    const struct module_ops *ops;
    void *private_data;
    atomic_t use_count;
};

/**
 * Safe module operation with validation
 */
int module_call_process(struct module_instance *inst,
                       void *data, size_t len) {
    int ret;

    /* Validate instance */
    if (!inst || !inst->ops) {
        return -EINVAL;
    }

    /* Check if operation is supported */
    if (!inst->ops->process) {
        return -ENOTSUPP;
    }

    /* Increment use count for thread safety */
    atomic_inc(&inst->use_count);

    /* Call through interface */
    ret = inst->ops->process(inst->private_data, data, len);

    /* Always decrement on exit */
    atomic_dec(&inst->use_count);

    return ret;
}

/* Module lifecycle */
static int __init example_init(void) {
    pr_info("C Best Practices Example loaded\n");
    return 0;
}

static void __exit example_exit(void) {
    pr_info("C Best Practices Example unloaded\n");
}

module_init(example_init);
module_exit(example_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Claude Code");
MODULE_DESCRIPTION("Key C Programming Patterns Example");

/*
 * Key Patterns Demonstrated:
 *
 * 1. Memory Safety:
 *    - Bounds checking before allocation/access
 *    - Overflow detection in arithmetic
 *    - DMA-safe memory management
 *
 * 2. Error Handling:
 *    - Consistent error codes (negative)
 *    - Structured cleanup with goto
 *    - Resource acquisition with failure handling
 *
 * 3. Thread Safety:
 *    - Atomic operations for simple counters
 *    - Proper locking hierarchy
 *    - Interrupt-safe operations
 *
 * 4. Modular Design:
 *    - Clear interface boundaries
 *    - Version compatibility
 *    - Reference counting for lifecycle
 */