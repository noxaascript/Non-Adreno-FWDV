/* ============================================================
 * queue_optimizer.c — Vulkan Queue Optimizer
 * Detects separate transfer queues on Mali-G610/G720/PowerVR
 * and stores queue family indices for render queue balancing.
 * ============================================================ */

#include "vk_layer_fexdxvk.h"
#include <stdlib.h>
#include <string.h>

void queue_optimizer_init(FexDevice *dev, VkPhysicalDevice phys) {
    /* We need instance dispatch to enumerate queue families.
     * FexInstance holds the GPA; look it up via a chained call.
     * For simplicity here we use a static function pointer obtained
     * at layer init time. In production, store it in FexInstance. */

    /* Walk queue family properties looking for:
     *  1. Graphics queue (required, becomes primaryGraphicsQueue)
     *  2. Transfer-only queue (optional, becomes transferQueue) */

    uint32_t famCount = 0;
    /* vkGetPhysicalDeviceQueueFamilyProperties requires instance dispatch.
     * In the full layer it's obtained from the chained GPA; placeholder: */
    typedef void (VKAPI_PTR *PFN_GetQFP)(VkPhysicalDevice, uint32_t*,
                                          VkQueueFamilyProperties*);
    /* Retrieved via stored instance GPA at CreateDevice time: */
    extern PFN_vkGetPhysicalDeviceQueueFamilyProperties g_getQFP;
    if (!g_getQFP) return;

    g_getQFP(phys, &famCount, NULL);
    if (!famCount) return;

    VkQueueFamilyProperties *fams = calloc(famCount, sizeof(*fams));
    g_getQFP(phys, &famCount, fams);

    dev->graphicsQueueFamily = UINT32_MAX;
    dev->transferQueueFamily = UINT32_MAX;

    for (uint32_t i = 0; i < famCount; i++) {
        VkQueueFlags f = fams[i].queueFlags;
        if ((f & VK_QUEUE_GRAPHICS_BIT) && dev->graphicsQueueFamily == UINT32_MAX)
            dev->graphicsQueueFamily = i;

        /* Prefer a queue that is transfer-only (no graphics/compute) */
        if ((f & VK_QUEUE_TRANSFER_BIT) &&
            !(f & VK_QUEUE_GRAPHICS_BIT) &&
            !(f & VK_QUEUE_COMPUTE_BIT)  &&
            dev->transferQueueFamily == UINT32_MAX)
            dev->transferQueueFamily = i;
    }

    dev->hasTransferQueue = (dev->transferQueueFamily != UINT32_MAX);
    free(fams);
}

/* Declare the function pointer (defined in vk_layer_fexdxvk.c or layer init) */
PFN_vkGetPhysicalDeviceQueueFamilyProperties g_getQFP = NULL;
