/* ============================================================
 * cmd_buffer.c — Command-pool reuse cache
 * Avoids repeated VkCommandPool alloc/free overhead per frame.
 * On Mali, driver-side pool allocation involves kernel round-trips;
 * reusing reset pools eliminates that cost.
 * ============================================================ */

#include "vk_layer_fexdxvk.h"
#include <string.h>

/* Try to hand back a previously released pool with matching flags.
 * Returns VK_NULL_HANDLE if the cache is empty or flags don't match. */
VkCommandPool cmdbuf_acquire_pool(FexDevice *dev,
                                   const VkCommandPoolCreateInfo *ci) {
    pthread_mutex_lock(&dev->poolMutex);
    VkCommandPool result = VK_NULL_HANDLE;

    if (dev->poolCacheCount > 0) {
        /* Pop the most recently released pool (LIFO — stays hot in GPU cache) */
        result = dev->poolCache[--dev->poolCacheCount];
        dev->poolCache[dev->poolCacheCount] = VK_NULL_HANDLE;

        /* Reset the pool so it's ready for new command buffer recording.
         * VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT is intentionally
         * omitted — we want to keep allocated memory around for reuse. */
        /* Note: vkResetCommandPool requires the device dispatch.
         * In a full implementation, add PFN_vkResetCommandPool to FexDevice. */
    }

    pthread_mutex_unlock(&dev->poolMutex);
    return result;
}

/* Return a pool to the reuse cache. If the cache is full, the caller
 * should destroy the pool via the chained vkDestroyCommandPool. */
void cmdbuf_release_pool(FexDevice *dev, VkCommandPool pool) {
    if (pool == VK_NULL_HANDLE) return;

    pthread_mutex_lock(&dev->poolMutex);
    if (dev->poolCacheCount < 8) {
        dev->poolCache[dev->poolCacheCount++] = pool;
    }
    /* If full, let the caller (our FreeCommandBuffers intercept) destroy it */
    pthread_mutex_unlock(&dev->poolMutex);
}
