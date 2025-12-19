// examples/memory-safety.c
// Example demonstrating safe memory management patterns for kernel drivers
// This file shows proper DMA allocation, bounds checking, and resource cleanup

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/atomic.h>
#include <linux/jiffies.h>
#include <linux/time.h>

#define MAX_DEVICES 64
#define BUFFER_SIZE PAGE_SIZE
#define MAX_TRANSFER_SIZE (64 * 1024)  // 64KB max transfer

struct device_context {
    struct mutex lock;
    void *dma_buffer;
    dma_addr_t dma_addr;
    size_t buffer_size;
    bool active;
    atomic_t ref_count;  // Reference count for safe cleanup

    /* Statistics */
    atomic_t total_allocations;
    atomic_t failed_allocations;
    atomic_t total_transfers;
    atomic_t error_count;
};

/**
 * Safely allocate DMA-coherent memory with comprehensive error checking
 * Returns 0 on success, negative error on failure
 */
int device_init_dma(struct device_context *ctx, size_t size) {
    // Validate input parameters
    if (!ctx) {
        pr_err("%s: NULL context provided\n", __func__);
        return -EINVAL;
    }

    if (size == 0 || size > BUFFER_SIZE) {
        pr_err("%s: Invalid size %zu (max: %d)\n", __func__, size, BUFFER_SIZE);
        return -EINVAL;
    }

    // Check for size overflow (this check is redundant since size is size_t
    // and we're not adding it to anything, but kept for defensive programming)
    if (size == SIZE_MAX) {
        pr_err("%s: Invalid size (SIZE_MAX)\n", __func__);
        return -EINVAL;
    }

    atomic_inc(&ctx->total_allocations);

    // Check if already allocated
    mutex_lock(&ctx->lock);
    if (ctx->dma_buffer) {
        mutex_unlock(&ctx->lock);
        pr_warn("%s: DMA already allocated\n", __func__);
        return -EALREADY;
    }

    // Allocate DMA-coherent memory
    ctx->dma_buffer = dma_alloc_coherent(NULL, size, &ctx->dma_addr, GFP_KERNEL);
    if (!ctx->dma_buffer) {
        atomic_inc(&ctx->failed_allocations);
        mutex_unlock(&ctx->lock);
        pr_err("%s: Failed to allocate %zu bytes of DMA memory\n", __func__, size);
        return -ENOMEM;
    }

    // Initialize allocated memory for security
    memset(ctx->dma_buffer, 0, size);

    ctx->buffer_size = size;
    ctx->active = true;
    atomic_set(&ctx->ref_count, 1);

    mutex_unlock(&ctx->lock);

    pr_info("%s: DMA allocated successfully: size=%zu, dma_addr=%pad\n",
            __func__, size, &ctx->dma_addr);
    return 0;
}

/**
 * Safely free DMA memory with proper synchronization
 * Ensures no ongoing operations reference the memory
 */
void device_cleanup_dma(struct device_context *ctx) {
    if (!ctx) {
        pr_warn("%s: NULL context\n", __func__);
        return;
    }

    mutex_lock(&ctx->lock);

    // Wait for all references to be released with timeout
    // Timeout after 5 seconds to prevent infinite loop
    unsigned long timeout = jiffies + msecs_to_jiffies(5000);
    while (atomic_read(&ctx->ref_count) > 0) {
        mutex_unlock(&ctx->lock);

        // Check if we've timed out
        if (time_after(jiffies, timeout)) {
            pr_warn("%s: Timeout waiting for reference count to reach 0 (count=%d)\n",
                    __func__, atomic_read(&ctx->ref_count));
            // Force cleanup even if references remain
            atomic_set(&ctx->ref_count, 0);
            mutex_lock(&ctx->lock);
            break;
        }

        msleep(1);  // Brief pause before checking again
        mutex_lock(&ctx->lock);
    }

    if (ctx->dma_buffer) {
        // Clear sensitive data before freeing
        memset(ctx->dma_buffer, 0, ctx->buffer_size);

        dma_free_coherent(NULL, ctx->buffer_size, ctx->dma_buffer, ctx->dma_addr);
        ctx->dma_buffer = NULL;
        ctx->buffer_size = 0;
        ctx->dma_addr = 0;

        pr_info("%s: DMA memory freed\n", __func__);
    }

    ctx->active = false;
    mutex_unlock(&ctx->lock);
}

/**
 * Get reference to DMA buffer for safe concurrent access
 * Must be paired with put_dma_buffer()
 */
void *get_dma_buffer(struct device_context *ctx, dma_addr_t *dma_addr, size_t *size) {
    void *buffer = NULL;

    if (!ctx) {
        pr_err("%s: NULL context\n", __func__);
        return NULL;
    }

    mutex_lock(&ctx->lock);

    if (!ctx->active || !ctx->dma_buffer) {
        atomic_inc(&ctx->error_count);
        mutex_unlock(&ctx->lock);
        pr_err("%s: Device not active or DMA not allocated\n", __func__);
        return NULL;
    }

    // Increment reference count
    atomic_inc(&ctx->ref_count);

    buffer = ctx->dma_buffer;
    if (dma_addr) *dma_addr = ctx->dma_addr;
    if (size) *size = ctx->buffer_size;

    mutex_unlock(&ctx->lock);

    return buffer;
}

/**
 * Release reference to DMA buffer
 */
void put_dma_buffer(struct device_context *ctx) {
    if (!ctx) {
        pr_warn("%s: NULL context\n", __func__);
        return;
    }

    atomic_dec(&ctx->ref_count);
}

/**
 * Safe DMA buffer write with bounds checking and user space validation
 * Returns bytes written on success, negative error on failure
 */
int device_write_dma(struct device_context *ctx, const void __user *data, size_t count) {
    int ret;
    void *buffer;

    // Validate parameters
    if (!ctx || !data) {
        pr_err("%s: Invalid parameters\n", __func__);
        return -EINVAL;
    }

    // Check for unreasonable transfer size
    if (count > MAX_TRANSFER_SIZE) {
        pr_err("%s: Transfer size %zu exceeds maximum %d\n",
               __func__, count, MAX_TRANSFER_SIZE);
        return -EINVAL;
    }

    // Get DMA buffer reference
    buffer = get_dma_buffer(ctx, NULL, NULL);
    if (!buffer) {
        return -ENODEV;
    }

    atomic_inc(&ctx->total_transfers);

    // Check bounds
    if (count > ctx->buffer_size) {
        atomic_inc(&ctx->error_count);
        ret = -EOVERFLOW;
        pr_err("%s: Transfer size %zu exceeds buffer size %zu\n",
               __func__, count, ctx->buffer_size);
        goto out;
    }

    // Safe copy from user space with error checking
    if (copy_from_user(buffer, data, count)) {
        atomic_inc(&ctx->error_count);
        ret = -EFAULT;
        pr_err("%s: Failed to copy %zu bytes from user space\n", __func__, count);
        goto out;
    }

    // Success
    ret = count;
    pr_debug("%s: Successfully wrote %zu bytes to DMA buffer\n", __func__, count);

out:
    put_dma_buffer(ctx);
    return ret;
}

/**
 * Safe DMA buffer read with bounds checking and user space validation
 * Returns bytes read on success, negative error on failure
 */
int device_read_dma(struct device_context *ctx, void __user *data, size_t count) {
    int ret;
    void *buffer;

    // Validate parameters
    if (!ctx || !data) {
        pr_err("%s: Invalid parameters\n", __func__);
        return -EINVAL;
    }

    if (count > MAX_TRANSFER_SIZE) {
        pr_err("%s: Transfer size %zu exceeds maximum %d\n",
               __func__, count, MAX_TRANSFER_SIZE);
        return -EINVAL;
    }

    // Get DMA buffer reference
    buffer = get_dma_buffer(ctx, NULL, NULL);
    if (!buffer) {
        return -ENODEV;
    }

    atomic_inc(&ctx->total_transfers);

    // Check bounds
    if (count > ctx->buffer_size) {
        atomic_inc(&ctx->error_count);
        ret = -EOVERFLOW;
        pr_err("%s: Transfer size %zu exceeds buffer size %zu\n",
               __func__, count, ctx->buffer_size);
        goto out;
    }

    // Safe copy to user space with error checking
    if (copy_to_user(data, buffer, count)) {
        atomic_inc(&ctx->error_count);
        ret = -EFAULT;
        pr_err("%s: Failed to copy %zu bytes to user space\n", __func__, count);
        goto out;
    }

    // Success
    ret = count;
    pr_debug("%s: Successfully read %zu bytes from DMA buffer\n", __func__, count);

out:
    put_dma_buffer(ctx);
    return ret;
}

/**
 * Initialize device context with safe defaults
 */
int device_context_init(struct device_context *ctx) {
    if (!ctx) {
        pr_err("%s: NULL context\n", __func__);
        return -EINVAL;
    }

    // Zero-initialize entire structure
    memset(ctx, 0, sizeof(*ctx));

    // Initialize synchronization primitives
    mutex_init(&ctx->lock);

    // Initialize atomic counters
    atomic_set(&ctx->ref_count, 0);
    atomic_set(&ctx->total_allocations, 0);
    atomic_set(&ctx->failed_allocations, 0);
    atomic_set(&ctx->total_transfers, 0);
    atomic_set(&ctx->error_count, 0);

    pr_debug("%s: Device context initialized\n", __func__);
    return 0;
}

/**
 * Cleanup device context and print statistics
 */
void device_context_cleanup(struct device_context *ctx) {
    if (!ctx) {
        pr_warn("%s: NULL context\n", __func__);
        return;
    }

    // Print statistics before cleanup
    pr_info("%s: Statistics:\n", __func__);
    pr_info("  Total allocations: %d\n", atomic_read(&ctx->total_allocations));
    pr_info("  Failed allocations: %d\n", atomic_read(&ctx->failed_allocations));
    pr_info("  Total transfers: %d\n", atomic_read(&ctx->total_transfers));
    pr_info("  Error count: %d\n", atomic_read(&ctx->error_count));

    // Clean up DMA if allocated
    device_cleanup_dma(ctx);

    // Destroy synchronization primitives
    mutex_destroy(&ctx->lock);

    pr_info("%s: Device context cleaned up\n", __func__);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("C Best Practices Skill");
MODULE_DESCRIPTION("Memory safety patterns example for kernel drivers");