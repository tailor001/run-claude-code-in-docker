/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Module Interface Example
 *
 * This header demonstrates best practices for designing modular C interfaces.
 * Shows version compatibility, opaque pointers, and clean API design.
 */

#ifndef MODULE_INTERFACE_H
#define MODULE_INTERFACE_H

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>
#include <linux/wait.h>
#include <linux/list.h>

/* Module version information - allows for backward compatibility */
#define STORAGE_MODULE_VERSION        2
#define STORAGE_MODULE_MIN_VERSION    1
#define STORAGE_MODULE_MAGIC         0x53544F52  /* "STOR" */

/* Feature flags - modules can advertise capabilities */
#define STORAGE_FEATURE_ASYNC       (1 << 0)   /* Asynchronous operations */
#define STORAGE_FEATURE_ENCRYPTION  (1 << 1)   /* Built-in encryption */
#define STORAGE_FEATURE_COMPRESSION (1 << 2)   /* Data compression */
#define STORAGE_FEATURE_SNAPSHOTS   (1 << 3)   /* Point-in-time snapshots */

/* Operation flags for fine-grained control */
#define STORAGE_OP_SYNC        (1 << 0)   /* Synchronous operation */
#define STORAGE_OP_NOCACHE     (1 << 1)   /* Bypass cache */
#define STORAGE_OP_FUA         (1 << 2)   /* Force Unit Access */
#define STORAGE_OP_ZERO        (1 << 3)   /* Zero-fill on error */

/* Forward declarations - opaque handles for API users */
struct storage_device;
struct storage_context;
struct storage_request;

/**
 * Storage statistics structure
 * Tracks usage metrics for monitoring and debugging
 */
struct storage_stats {
    /* I/O counters */
    u64 reads_completed;
    u64 writes_completed;
    u64 bytes_read;
    u64 bytes_written;

    /* Error tracking */
    u64 read_errors;
    u64 write_errors;
    u64 timeout_errors;

    /* Performance metrics */
    u64 avg_read_latency_us;
    u64 avg_write_latency_us;
    u64 max_queue_depth;

    /* Cache statistics */
    u64 cache_hits;
    u64 cache_misses;
    u64 cache_evictions;

    /* Last update timestamp */
    u64 last_update_ns;
};

/**
 * Storage capabilities structure
 * Describes what the storage backend can do
 */
struct storage_caps {
    u32 version;           /* Interface version */
    u32 features;          /* Feature flags */

    /* Size limits */
    u64 max_device_size;   /* Maximum device size in bytes */
    u64 max_transfer_size; /* Maximum single transfer */
    u32 min_io_size;       /* Minimum I/O size */
    u32 optimal_io_size;   /* Optimal I/O size */

    /* Alignment requirements */
    u32 dma_alignment;     /* DMA alignment requirement */
    u32 sector_size;       /* Physical sector size */

    /* Performance characteristics */
    u32 max_queue_depth;   /* Maximum concurrent operations */
    bool supports_trim;    /* Supports TRIM/UNMAP */
    bool supports_discard; /* Supports discard */

    /* Reliability features */
    bool has_power_loss_protection;
    bool has_end_to_end_protection;
    u32 max_retries;       /* Maximum retry attempts */
};

/**
 * Storage operations interface
 * Function table implementing storage backend operations
 */
struct storage_ops {
    /* Mandatory operations - must be implemented */
    int (*probe)(struct storage_device *dev);
    int (*remove)(struct storage_device *dev);

    /* Core I/O operations */
    int (*read)(struct storage_context *ctx, u64 offset,
               void *buf, size_t len, u32 flags);
    int (*write)(struct storage_context *ctx, u64 offset,
                const void *buf, size_t len, u32 flags);
    int (*flush)(struct storage_context *ctx, u32 flags);

    /* Optional operations - can be NULL if not supported */
    int (*erase)(struct storage_context *ctx, u64 offset, size_t len);
    int (*trim)(struct storage_context *ctx, u64 offset, size_t len);
    int (*sync)(struct storage_context *ctx);

    /* Asynchronous operations */
    int (*read_async)(struct storage_context *ctx, u64 offset,
                     void *buf, size_t len, u32 flags,
                     struct storage_request *req);
    int (*write_async)(struct storage_context *ctx, u64 offset,
                      const void *buf, size_t len, u32 flags,
                      struct storage_request *req);

    /* Management operations */
    int (*get_stats)(struct storage_context *ctx,
                    struct storage_stats *stats);
    int (*get_caps)(struct storage_context *ctx,
                   struct storage_caps *caps);
    int (*set_power_state)(struct storage_context *ctx, u32 state);

    /* Notification callbacks */
    void (*error_notify)(struct storage_context *ctx, int error_code);
    void (*completion_notify)(struct storage_context *ctx,
                             struct storage_request *req);

    /* Module information */
    u32 version;
    const char *name;
    const char *description;
    const char *author;
    const char *license;

    /* Internal data - opaque to users */
    void *private_data;
};

/**
 * Storage device structure
 * Represents a physical or virtual storage device
 */
struct storage_device {
    /* Device identification */
    u32 id;
    char name[32];
    char model[64];
    char serial[32];
    char firmware[32];

    /* Device state */
    atomic_t state;  /* 0=offline, 1=online, 2=error */
    struct mutex state_lock;

    /* Operations interface */
    const struct storage_ops *ops;

    /* Device-specific data */
    void *private_data;

    /* Child contexts */
    struct list_head contexts;
    struct mutex contexts_lock;

    /* Statistics */
    struct storage_stats global_stats;

    /* Power management */
    u32 current_power_state;
    struct mutex power_lock;

    /* Reference counting */
    atomic_t refcount;

    /* Magic number for corruption detection */
    u32 magic;
};

/**
 * Storage context structure
 * Represents an open instance/channel to a storage device
 */
struct storage_context {
    /* Parent device */
    struct storage_device *device;

    /* Context state */
    u32 flags;
    atomic_t active_requests;

    /* Configuration */
    u32 queue_depth;
    u32 timeout_ms;
    bool read_only;

    /* Request queue */
    struct list_head pending_requests;
    spinlock_t queue_lock;
    wait_queue_head_t queue_wait;

    /* Error tracking */
    int last_error;
    u64 error_count;
    struct error_info last_error_info;

    /* Private data for backend */
    void *private;

    /* List node for device's context list */
    struct list_head list;

    /* Synchronization for context-wide operations */
    struct mutex lock;

    /* Magic number for validation */
    u32 magic;
};

/**
 * Storage request structure
 * Represents an asynchronous I/O request
 */
struct storage_request {
    /* Request parameters */
    u64 offset;
    size_t length;
    void *buffer;
    u32 flags;

    /* Request type */
    enum {
        STORAGE_REQ_READ,
        STORAGE_REQ_WRITE,
        STORAGE_REQ_FLUSH,
        STORAGE_REQ_TRIM
    } type;

    /* Completion status */
    int result;
    size_t bytes_transferred;

    /* Completion callback */
    void (*completion)(struct storage_request *req);
    void *callback_data;

    /* Timing information */
    u64 start_time_ns;
    u64 completion_time_ns;

    /* List management */
    struct list_head list;

    /* Reference count for async operations */
    atomic_t refcount;

    /* Request identifier */
    u64 req_id;
};

/**
 * Error information structure
 * Detailed error context for debugging
 */
struct error_info {
    int error_code;        /* Primary error code */
    int sub_error;         /* Sub-error for more detail */
    u64 timestamp;         /* When error occurred */

    /* Operation context */
    const char *operation;  /* What was being done */
    u64 offset;           /* Relevant offset */
    size_t length;        /* Relevant length */

    /* Additional context */
    const char *details;   /* Human-readable details */
    void *backend_data;    /* Backend-specific error data */

    /* Recovery suggestions */
    const char *recovery_hint;
    bool is_recoverable;
};

/* Public API functions */

/**
 * Register a storage backend with the system
 * @ops: Pointer to operations structure
 * Returns: 0 on success, negative error on failure
 */
int storage_register_backend(const struct storage_ops *ops);

/**
 * Unregister a storage backend
 * @ops: Pointer to operations structure to unregister
 */
void storage_unregister_backend(const struct storage_ops *ops);

/**
 * Create and initialize a storage device
 * @name: Human-readable device name
 * @ops: Operations interface for the device
 * Returns: Device pointer on success, NULL on failure
 */
struct storage_device *storage_create_device(const char *name,
                                            const struct storage_ops *ops);

/**
 * Destroy a storage device and clean up resources
 * @dev: Device to destroy
 */
void storage_destroy_device(struct storage_device *dev);

/**
 * Open a storage context for I/O operations
 * @dev: Storage device
 * Returns: Context pointer on success, NULL on failure
 */
struct storage_context *storage_open_context(struct storage_device *dev);

/**
 * Close a storage context
 * @ctx: Context to close
 */
void storage_close_context(struct storage_context *ctx);

/**
 * Synchronous read operation
 * @ctx: Storage context
 * @offset: Byte offset to read from
 * @buf: Buffer to read into
 * @len: Number of bytes to read
 * @flags: Operation flags
 * Returns: Number of bytes read on success, negative error on failure
 */
int storage_read(struct storage_context *ctx, u64 offset,
                void *buf, size_t len, u32 flags);

/**
 * Synchronous write operation
 * @ctx: Storage context
 * @offset: Byte offset to write to
 * @buf: Buffer containing data to write
 * @len: Number of bytes to write
 * @flags: Operation flags
 * Returns: Number of bytes written on success, negative error on failure
 */
int storage_write(struct storage_context *ctx, u64 offset,
                 const void *buf, size_t len, u32 flags);

/**
 * Asynchronous read operation
 * @ctx: Storage context
 * @offset: Byte offset to read from
 * @buf: Buffer to read into
 * @len: Number of bytes to read
 * @flags: Operation flags
 * @req: Request structure for async completion
 * Returns: 0 on success (async), negative error on failure
 */
int storage_read_async(struct storage_context *ctx, u64 offset,
                      void *buf, size_t len, u32 flags,
                      struct storage_request *req);

/**
 * Asynchronous write operation
 * @ctx: Storage context
 * @offset: Byte offset to write to
 * @buf: Buffer containing data to write
 * @len: Number of bytes to write
 * @flags: Operation flags
 * @req: Request structure for async completion
 * Returns: 0 on success (async), negative error on failure
 */
int storage_write_async(struct storage_context *ctx, u64 offset,
                       const void *buf, size_t len, u32 flags,
                       struct storage_request *req);

/**
 * Flush pending writes to stable storage
 * @ctx: Storage context
 * @flags: Flush options
 * Returns: 0 on success, negative error on failure
 */
int storage_flush(struct storage_context *ctx, u32 flags);

/**
 * Get storage device statistics
 * @ctx: Storage context
 * @stats: Statistics structure to fill
 * Returns: 0 on success, negative error on failure
 */
int storage_get_stats(struct storage_context *ctx,
                     struct storage_stats *stats);

/**
 * Get storage device capabilities
 * @ctx: Storage context
 * @caps: Capabilities structure to fill
 * Returns: 0 on success, negative error on failure
 */
int storage_get_caps(struct storage_context *ctx,
                    struct storage_caps *caps);

/**
 * Check interface compatibility
 * @version: Version to check
 * Returns: 0 if compatible, negative error if not
 */
int storage_check_compatibility(u32 version);

/**
 * Set power state for device
 * @ctx: Storage context
 * @state: Power state to set
 * Returns: 0 on success, negative error on failure
 */
int storage_set_power_state(struct storage_context *ctx, u32 state);

/**
 * Get last error information
 * @ctx: Storage context
 * @err: Error structure to fill
 * Returns: 0 if error available, -ENOENT if no recent error
 */
int storage_get_last_error(struct storage_context *ctx,
                          struct error_info *err);

/* Macro definitions for common patterns */

/**
 * Declare a storage backend
 * Usage: DECLARE_STORAGE_BACKEND(my_storage, "My Storage Backend")
 */
#define DECLARE_STORAGE_BACKEND(name, desc)               \
    extern const struct storage_ops __storage_backend_##name; \
    const char *__storage_backend_##name##_desc = desc;

/**
 * Helper for validating storage context
 * Returns true if valid, false if corrupted
 */
static inline bool storage_context_is_valid(struct storage_context *ctx) {
    return ctx && ctx->magic == STORAGE_MODULE_MAGIC &&
           ctx->device && ctx->device->magic == STORAGE_MODULE_MAGIC;
}

/**
 * Helper for getting device from context
 */
static inline struct storage_device *storage_get_device(struct storage_context *ctx) {
    return storage_context_is_valid(ctx) ? ctx->device : NULL;
}

/**
 * Helper for checking if operation is supported
 */
static inline bool storage_op_supported(struct storage_context *ctx,
                                       const char *op_name) {
    struct storage_device *dev = storage_get_device(ctx);
    return dev && dev->ops && dev->ops->version >= STORAGE_MODULE_MIN_VERSION;
}

#endif /* MODULE_INTERFACE_H */