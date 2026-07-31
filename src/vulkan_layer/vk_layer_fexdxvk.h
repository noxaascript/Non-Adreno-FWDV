#pragma once
/* ============================================================
 * vk_layer_fexdxvk.h
 * FexDXVK NonAdreno — Vulkan Implicit Layer
 * Builds to: libVkLayer_fexdxvk.so
 * ============================================================ */

#include <vulkan/vulkan.h>
#include <vulkan/vk_layer.h>
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Per-instance dispatch table ---- */
typedef struct FexInstance {
    VkInstance                  instance;
    PFN_vkGetInstanceProcAddr   gpa;
    /* Chained dispatch */
    PFN_vkDestroyInstance       DestroyInstance;
    PFN_vkEnumeratePhysicalDevices EnumeratePhysicalDevices;
} FexInstance;

/* ---- Per-device dispatch table ---- */
typedef struct FexDevice {
    VkDevice                    device;
    VkPhysicalDevice            physicalDevice;
    PFN_vkGetDeviceProcAddr     gdpa;

    /* Chained dispatch — graphics */
    PFN_vkDestroyDevice         DestroyDevice;
    PFN_vkCreateCommandPool     CreateCommandPool;
    PFN_vkAllocateCommandBuffers AllocateCommandBuffers;
    PFN_vkFreeCommandBuffers    FreeCommandBuffers;
    PFN_vkBeginCommandBuffer    BeginCommandBuffer;
    PFN_vkEndCommandBuffer      EndCommandBuffer;
    PFN_vkQueueSubmit           QueueSubmit;
    PFN_vkQueueSubmit2          QueueSubmit2;
    PFN_vkQueuePresentKHR       QueuePresentKHR;
    PFN_vkCreateGraphicsPipelines CreateGraphicsPipelines;
    PFN_vkCreateComputePipelines  CreateComputePipelines;
    PFN_vkCreatePipelineCache   CreatePipelineCache;
    PFN_vkDestroyPipelineCache  DestroyPipelineCache;
    PFN_vkGetPipelineCacheData  GetPipelineCacheData;
    PFN_vkMergePipelineCaches   MergePipelineCaches;

    /* Frame pacing */
    uint64_t    lastPresentNs;
    uint64_t    targetFrameNs;   /* ns per frame, e.g. 16666666 for 60 Hz */
    uint32_t    frameIndex;

    /* Command-buffer pool (reuse) */
    VkCommandPool   poolCache[8];
    uint32_t        poolCacheCount;
    pthread_mutex_t poolMutex;

    /* Pipeline cache persistence */
    VkPipelineCache persistentCache;
    char            cacheFilePath[256];
    pthread_mutex_t cacheMutex;

    /* Queue optimizer */
    VkQueue     primaryGraphicsQueue;
    VkQueue     transferQueue;          /* separate transfer if available */
    uint32_t    graphicsQueueFamily;
    uint32_t    transferQueueFamily;
    bool        hasTransferQueue;
    pthread_mutex_t queueMutex;

    /* Async submission */
    bool        asyncEnabled;
    pthread_t   asyncThread;
    volatile int asyncStop;
} FexDevice;

/* ---- Config (read from wrapper.json at startup) ---- */
typedef struct FexLayerConfig {
    bool    vulkanQueueOptimizer;
    bool    pipelineCachePersistent;
    bool    shaderCachePersistent;
    bool    commandBufferOptimizer;
    bool    framePacing;
    bool    renderQueueBalancer;
    bool    asyncCommandSubmission;
    int     targetFps;
    char    cacheDir[256];
} FexLayerConfig;

extern FexLayerConfig g_fexConfig;

/* ---- Public init ---- */
void fexlayer_load_config(const char *wrapperJsonPath);

/* ---- Exported Vulkan entry points ---- */
__attribute__((visibility("default"))) VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
fexdxvk_GetInstanceProcAddr(VkInstance instance, const char *pName);

__attribute__((visibility("default"))) VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
fexdxvk_GetDeviceProcAddr(VkDevice device, const char *pName);

/* ---- Pipeline cache helpers ---- */
void pipeline_cache_load(FexDevice *dev);
void pipeline_cache_save(FexDevice *dev);

/* ---- Frame pacing helpers ---- */
void frame_pacing_present(FexDevice *dev);

/* ---- Queue optimizer helpers ---- */
void queue_optimizer_init(FexDevice *dev, VkPhysicalDevice phys);

/* ---- Command buffer pool helpers ---- */
VkCommandPool cmdbuf_acquire_pool(FexDevice *dev, const VkCommandPoolCreateInfo *ci);
void          cmdbuf_release_pool(FexDevice *dev, VkCommandPool pool);

#ifdef __cplusplus
}
#endif
