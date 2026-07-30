/* ============================================================
 * pipeline_cache.c — Persistent Vulkan pipeline cache
 * Saves/loads VkPipelineCache blob to disk so pipelines
 * compiled in one session are reused in the next, eliminating
 * first-run compilation stalls on Mali/PowerVR.
 * ============================================================ */

#include "vk_layer_fexdxvk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

static void get_cache_path(FexDevice *dev, char *out, size_t sz) {
    /* Key the cache file by the physical device's Vulkan driver UUID */
    VkPhysicalDeviceProperties props;
    /* Note: we'd need the instance dispatch to call this; approximating
     * with a fixed name per device slot for portability. */
    snprintf(out, sz, "%s/pipeline.vkpc", g_fexConfig.cacheDir);
}

/* Load cache blob from disk and create VkPipelineCache pre-populated */
void pipeline_cache_load(FexDevice *dev) {
    pthread_mutex_lock(&dev->cacheMutex);

    get_cache_path(dev, dev->cacheFilePath, sizeof(dev->cacheFilePath));

    /* Ensure cache directory exists */
    struct stat st = {0};
    if (stat(g_fexConfig.cacheDir, &st) != 0)
        mkdir(g_fexConfig.cacheDir, 0755);

    VkPipelineCacheCreateInfo ci = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        .pNext           = NULL,
        .flags           = 0,
        .initialDataSize = 0,
        .pInitialData    = NULL,
    };

    FILE *f = fopen(dev->cacheFilePath, "rb");
    void *blob = NULL;

    if (f) {
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        rewind(f);
        if (sz > 0) {
            blob = malloc(sz);
            if (blob && fread(blob, 1, sz, f) == (size_t)sz) {
                ci.initialDataSize = (size_t)sz;
                ci.pInitialData    = blob;
            }
        }
        fclose(f);
    }

    VkResult res = dev->CreatePipelineCache(
        dev->device, &ci, NULL, &dev->persistentCache);

    free(blob);

    if (res == VK_SUCCESS) {
        /* If initial data was corrupt, Vulkan creates an empty cache — that's fine */
    }

    pthread_mutex_unlock(&dev->cacheMutex);
}

/* Save current pipeline cache blob to disk */
void pipeline_cache_save(FexDevice *dev) {
    if (dev->persistentCache == VK_NULL_HANDLE) return;

    pthread_mutex_lock(&dev->cacheMutex);

    size_t dataSize = 0;
    VkResult res = dev->GetPipelineCacheData(
        dev->device, dev->persistentCache, &dataSize, NULL);

    if (res == VK_SUCCESS && dataSize > 0) {
        void *data = malloc(dataSize);
        res = dev->GetPipelineCacheData(
            dev->device, dev->persistentCache, &dataSize, data);

        if (res == VK_SUCCESS) {
            FILE *f = fopen(dev->cacheFilePath, "wb");
            if (f) {
                fwrite(data, 1, dataSize, f);
                fclose(f);
            }
        }
        free(data);
    }

    pthread_mutex_unlock(&dev->cacheMutex);
}
