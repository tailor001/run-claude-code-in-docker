// examples/dma-example.c
// Example demonstrating DMA ring buffer management with interrupt-safe operations
// Shows proper synchronization between interrupts and normal context

#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/delay.h>

#define RING_SIZE 256
#define DESCRIPTOR_SIZE sizeof(struct dma_descriptor)
#define RING_BUFFER_SIZE (RING_SIZE * DESCRIPTOR_SIZE)
#define MAX_FRAME_SIZE 1518  // Standard Ethernet frame size
#define MAX_TRANSFER_SIZE (64 * 1024)  // 64KB max transfer
#define MEMORY_BARRIER() wmb()  // Write memory barrier for consistency

/* Hardware register offsets */
#define TX_RING_PTR    0x00
#define RX_RING_PTR    0x04
#define TX_STATUS      0x08
#define RX_STATUS      0x0C
#define INT_ENABLE     0x10
#define INT_STATUS     0x14
#define MAC_CONTROL    0x18

/* Descriptor flags */
#define DESC_FLAG_OWNER_HW     0x80000000  // Descriptor owned by hardware
#define DESC_FLAG_OWNER_SW     0x00000000  // Descriptor owned by software
#define DESC_FLAG_INTR_ENABLE  0x40000000  // Enable interrupt on completion
#define DESC_STATUS_DONE       0x00000001  // Descriptor completed by hardware

/* Interrupt flags */
#define INT_STATUS_TX   0x00000001  // Transmit interrupt
#define INT_STATUS_RX   0x00000002  // Receive interrupt
#define INT_ENABLE_TX   0x00000001  // Enable transmit interrupts
#define INT_ENABLE_RX   0x00000002  // Enable receive interrupts

/* MAC control flags */
#define MAC_CONTROL_ENABLE    0x00000001  // Enable MAC interface

/**
 * DMA descriptor structure
 * Must match hardware expected layout
 */
struct dma_descriptor {
    __le32 buffer_addr;   // Physical address of data buffer
    __le32 length;        // Buffer length
    __le32 status;        // Descriptor status bits
    __le32 control;       // Control flags
} __packed;

/**
 * DMA ring buffer with proper synchronization
 */
struct ring_buffer {
    struct dma_descriptor *desc;  // Descriptor array (kernel virtual)
    dma_addr_t dma_addr;          // Physical address for hardware
    void **data_buffers;          // Data buffer pointers
    dma_addr_t *data_addrs;       // Physical addresses of data buffers

    size_t size;                  // Number of descriptors
    unsigned int head;            // Software head pointer
    unsigned int tail;            // Hardware tail pointer
    unsigned int count;           // Number of pending descriptors

    spinlock_t lock;              // For protecting ring access
    bool initialized;             // Initialization flag

    /* Statistics */
    atomic_t total_desc;
    atomic_t overflow_count;
    atomic_t underrun_count;
};

/**
 * PCI device structure
 */
struct pci_device {
    struct pci_dev *pdev;
    void __iomem *mmio_base;
    int irq;

    struct ring_buffer *tx_ring;
    struct ring_buffer *rx_ring;
    dma_addr_t tx_dma;
    dma_addr_t rx_dma;

    /* Synchronization */
    spinlock_t dev_lock;
    struct mutex reg_lock;

    /* State */
    bool started;
    bool int_enabled;

    /* Wait queues for blocking operations */
    wait_queue_head_t tx_wait;
    wait_queue_head_t rx_wait;

    /* Statistics */
    atomic_t tx_packets;
    atomic_t rx_packets;
    atomic_t tx_errors;
    atomic_t rx_errors;
    atomic_t interrupts;
};

/**
 * Allocate DMA-coherent memory for ring buffer
 * Returns 0 on success, negative error on failure
 */
static int alloc_ring_buffer(struct pci_dev *pdev, struct ring_buffer *ring) {
    size_t i;
    int ret = 0;

    // Allocate descriptor ring
    ring->desc = dma_alloc_coherent(&pdev->dev, RING_BUFFER_SIZE,
                                   &ring->dma_addr, GFP_KERNEL);
    if (!ring->desc) {
        dev_err(&pdev->dev, "Failed to allocate ring descriptors\n");
        return -ENOMEM;
    }

    // Clear descriptor memory
    memset(ring->desc, 0, RING_BUFFER_SIZE);

    // Allocate array for data buffer pointers
    ring->data_buffers = kzalloc(sizeof(void *) * RING_SIZE, GFP_KERNEL);
    if (!ring->data_buffers) {
        ret = -ENOMEM;
        goto err_free_desc;
    }

    // Allocate array for data buffer physical addresses
    ring->data_addrs = kzalloc(sizeof(dma_addr_t) * RING_SIZE, GFP_KERNEL);
    if (!ring->data_addrs) {
        ret = -ENOMEM;
        goto err_free_buffers;
    }

    // Allocate individual data buffers
    for (i = 0; i < RING_SIZE; i++) {
        ring->data_buffers[i] = dma_alloc_coherent(&pdev->dev,
                                                  MAX_FRAME_SIZE,
                                                  &ring->data_addrs[i],
                                                  GFP_KERNEL);
        if (!ring->data_buffers[i]) {
            dev_err(&pdev->dev, "Failed to allocate data buffer %zu\n", i);
            ret = -ENOMEM;
            goto err_free_data;
        }

        // Initialize descriptor
        ring->desc[i].buffer_addr = cpu_to_le32(ring->data_addrs[i]);
        ring->desc[i].length = cpu_to_le32(MAX_FRAME_SIZE);
        ring->desc[i].status = 0;
        ring->desc[i].control = cpu_to_le32(DESC_FLAG_OWNER_HW);
    }

    ring->size = RING_SIZE;
    ring->head = 0;
    ring->tail = 0;
    ring->count = 0;
    ring->initialized = true;

    atomic_set(&ring->total_desc, RING_SIZE);
    atomic_set(&ring->overflow_count, 0);
    atomic_set(&ring->underrun_count, 0);

    dev_info(&pdev->dev, "Ring buffer allocated: %d descriptors\n", RING_SIZE);
    return 0;

err_free_data:
    // Free already allocated data buffers
    while (i-- > 0) {
        dma_free_coherent(&pdev->dev, MAX_FRAME_SIZE,
                         ring->data_buffers[i], ring->data_addrs[i]);
    }
    kfree(ring->data_addrs);

err_free_buffers:
    kfree(ring->data_buffers);

err_free_desc:
    dma_free_coherent(&pdev->dev, RING_BUFFER_SIZE,
                     ring->desc, ring->dma_addr);

    return ret;
}

/**
 * Free ring buffer and all associated resources
 */
static void free_ring_buffer(struct pci_dev *pdev, struct ring_buffer *ring) {
    size_t i;

    if (!ring || !ring->initialized) {
        return;
    }

    // Free data buffers
    for (i = 0; i < ring->size; i++) {
        if (ring->data_buffers[i]) {
            dma_free_coherent(&pdev->dev, MAX_FRAME_SIZE,
                             ring->data_buffers[i], ring->data_addrs[i]);
        }
    }

    // Free pointer arrays
    kfree(ring->data_addrs);
    kfree(ring->data_buffers);

    // Free descriptor ring
    dma_free_coherent(&pdev->dev, RING_BUFFER_SIZE,
                     ring->desc, ring->dma_addr);

    ring->initialized = false;

    dev_info(&pdev->dev, "Ring buffer freed\n");
}

/**
 * Setup DMA rings for device
 */
static int setup_dma_rings(struct pci_device *dev) {
    int ret;

    // Allocate TX ring
    dev->tx_ring = kzalloc(sizeof(*dev->tx_ring), GFP_KERNEL);
    if (!dev->tx_ring) {
        return -ENOMEM;
    }

    spin_lock_init(&dev->tx_ring->lock);
    ret = alloc_ring_buffer(dev->pdev, dev->tx_ring);
    if (ret) {
        goto err_free_tx;
    }

    // Allocate RX ring
    dev->rx_ring = kzalloc(sizeof(*dev->rx_ring), GFP_KERNEL);
    if (!dev->rx_ring) {
        ret = -ENOMEM;
        goto err_free_tx_ring;
    }

    spin_lock_init(&dev->rx_ring->lock);
    ret = alloc_ring_buffer(dev->pdev, dev->rx_ring);
    if (ret) {
        goto err_free_rx;
    }

    // Write ring addresses to hardware
    writel(dev->tx_ring->dma_addr, dev->mmio_base + TX_RING_PTR);
    writel(dev->rx_ring->dma_addr, dev->mmio_base + RX_RING_PTR);

    // Enable interrupts
    dev->int_enabled = true;
    writel(INT_ENABLE_TX | INT_ENABLE_RX, dev->mmio_base + INT_ENABLE);

    dev_info(&dev->pdev->dev, "DMA rings setup complete\n");
    return 0;

err_free_rx:
    kfree(dev->rx_ring);

err_free_tx_ring:
    free_ring_buffer(dev->pdev, dev->tx_ring);

err_free_tx:
    kfree(dev->tx_ring);
    return ret;
}

/**
 * Interrupt-safe descriptor update for TX ring
 * This function is called from both interrupt and process context
 */
static void update_tx_descriptor(struct pci_device *dev, unsigned int idx,
                                dma_addr_t addr, size_t len) {
    unsigned long flags;
    struct ring_buffer *ring = dev->tx_ring;

    // Critical section: modify shared ring buffer
    spin_lock_irqsave(&ring->lock, flags);

    // Bounds check
    if (idx < ring->size) {
        // Update descriptor
        ring->desc[idx].buffer_addr = cpu_to_le32(addr);
        ring->desc[idx].length = cpu_to_le32(len);
        ring->desc[idx].status = 0;
        ring->desc[idx].control = cpu_to_le32(DESC_FLAG_OWNER_HW |
                                              DESC_FLAG_INTR_ENABLE);

        // Ensure descriptor updates are visible to hardware
        MEMORY_BARRIER();

        // Update hardware pointer
        writel(idx, dev->mmio_base + TX_RING_PTR);

        // Update ring state
        ring->head = (idx + 1) % ring->size;
        ring->count++;

        // Wake up any waiting writers
        wake_up_interruptible(&dev->tx_wait);
    } else {
        atomic_inc(&ring->overflow_count);
        dev_warn(&dev->pdev->dev, "Invalid TX descriptor index: %u\n", idx);
    }

    spin_unlock_irqrestore(&ring->lock, flags);
}

/**
 * Transmit packet using DMA ring buffer
 * Returns 0 on success, negative error on failure
 */
static int transmit_packet(struct pci_device *dev, const void *data, size_t len) {
    unsigned long flags;
    unsigned int idx;
    struct ring_buffer *ring = dev->tx_ring;
    void *buffer;

    if (!dev->started || len > MAX_FRAME_SIZE) {
        return -EINVAL;
    }

    // Get next available descriptor
    spin_lock_irqsave(&ring->lock, flags);

    if (ring->count >= ring->size) {
        // Ring is full
        atomic_inc(&ring->overflow_count);
        spin_unlock_irqrestore(&ring->lock, flags);
        return -ENOBUFS;
    }

    idx = ring->head;
    buffer = ring->data_buffers[idx];

    spin_unlock_irqrestore(&ring->lock, flags);

    // Copy data to DMA buffer
    memcpy(buffer, data, len);

    // Update descriptor and inform hardware
    update_tx_descriptor(dev, idx, ring->data_addrs[idx], len);

    atomic_inc(&dev->tx_packets);
    return 0;
}

/**
 * Interrupt handler for device
 * Must be fast and minimal - do all heavy work in bottom half
 */
static irqreturn_t device_interrupt(int irq, void *dev_id) {
    struct pci_device *dev = dev_id;
    u32 status;
    unsigned long flags;
    bool tx_complete = false;
    bool rx_complete = false;

    // Read and clear interrupt status
    status = readl(dev->mmio_base + INT_STATUS);
    if (!status) {
        return IRQ_NONE;  // Not our interrupt
    }

    // Acknowledge interrupts
    writel(status, dev->mmio_base + INT_STATUS);

    atomic_inc(&dev->interrupts);

    // Handle TX completion
    if (status & INT_STATUS_TX) {
        spin_lock_irqsave(&dev->tx_ring->lock, flags);
        // Process completed descriptors
        while (dev->tx_ring->count > 0) {
            unsigned int tail = dev->tx_ring->tail;
            if (le32_to_cpu(dev->tx_ring->desc[tail].status) & DESC_STATUS_DONE) {
                dev->tx_ring->tail = (tail + 1) % dev->tx_ring->size;
                dev->tx_ring->count--;
                tx_complete = true;
            } else {
                break;  // No more completed descriptors
            }
        }
        spin_unlock_irqrestore(&dev->tx_ring->lock, flags);
    }

    // Handle RX completion
    if (status & INT_STATUS_RX) {
        spin_lock_irqsave(&dev->rx_ring->lock, flags);
        // Process received packets
        while (true) {
            unsigned int head = dev->rx_ring->head;
            if (le32_to_cpu(dev->rx_ring->desc[head].status) & DESC_STATUS_DONE) {
                // Process received packet
                dev->rx_ring->head = (head + 1) % dev->rx_ring->size;
                rx_complete = true;
                atomic_inc(&dev->rx_packets);
            } else {
                break;  // No more packets
            }
        }
        spin_unlock_irqrestore(&dev->rx_ring->lock, flags);
    }

    // Wake up waiting threads
    if (tx_complete) {
        wake_up_interruptible(&dev->tx_wait);
    }
    if (rx_complete) {
        wake_up_interruptible(&dev->rx_wait);
    }

    return IRQ_HANDLED;
}

/**
 * Initialize DMA device
 */
static int init_dma_device(struct pci_device *dev) {
    int ret;

    // Initialize synchronization primitives
    spin_lock_init(&dev->dev_lock);
    mutex_init(&dev->reg_lock);

    // Initialize wait queues
    init_waitqueue_head(&dev->tx_wait);
    init_waitqueue_head(&dev->rx_wait);

    // Setup DMA rings
    ret = setup_dma_rings(dev);
    if (ret) {
        return ret;
    }

    // Request interrupt
    ret = request_irq(dev->irq, device_interrupt, IRQF_SHARED,
                      "dma-example", dev);
    if (ret) {
        dev_err(&dev->pdev->dev, "Failed to request IRQ %d\n", dev->irq);
        goto err_cleanup_rings;
    }

    // Enable device
    dev->started = true;
    writel(MAC_CONTROL_ENABLE, dev->mmio_base + MAC_CONTROL);

    dev_info(&dev->pdev->dev, "DMA device initialized\n");
    return 0;

err_cleanup_rings:
    free_ring_buffer(dev->pdev, dev->tx_ring);
    free_ring_buffer(dev->pdev, dev->rx_ring);
    kfree(dev->tx_ring);
    kfree(dev->rx_ring);
    return ret;
}

/**
 * Cleanup DMA device
 */
static void cleanup_dma_device(struct pci_device *dev) {
    if (!dev) {
        return;
    }

    // Disable device
    if (dev->started) {
        dev->started = false;
        writel(0, dev->mmio_base + MAC_CONTROL);
    }

    // Disable interrupts
    writel(0, dev->mmio_base + INT_ENABLE);
    dev->int_enabled = false;

    // Free interrupt
    if (dev->irq) {
        free_irq(dev->irq, dev);
    }

    // Cleanup rings
    if (dev->tx_ring) {
        free_ring_buffer(dev->pdev, dev->tx_ring);
        kfree(dev->tx_ring);
    }

    if (dev->rx_ring) {
        free_ring_buffer(dev->pdev, dev->rx_ring);
        kfree(dev->rx_ring);
    }

    // Print statistics
    dev_info(&dev->pdev->dev, "Final statistics:\n");
    dev_info("  TX packets: %d\n", atomic_read(&dev->tx_packets));
    dev_info("  RX packets: %d\n", atomic_read(&dev->rx_packets));
    dev_info("  TX errors: %d\n", atomic_read(&dev->tx_errors));
    dev_info("  RX errors: %d\n", atomic_read(&dev->rx_errors));
    dev_info("  Interrupts: %d\n", atomic_read(&dev->interrupts));

    dev_info(&dev->pdev->dev, "DMA device cleaned up\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("C Best Practices Skill");
MODULE_DESCRIPTION("DMA ring buffer example with interrupt-safe operations");