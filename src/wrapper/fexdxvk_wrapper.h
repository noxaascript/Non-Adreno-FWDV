#pragma once
/* ============================================================
 * fexdxvk_wrapper.h
 * FexDXVK NonAdreno — Android/Linux Wrapper Library
 * Builds to: libfexdxvk_wrapper.so
 *
 * Loaded at Wine/FEX startup via LD_PRELOAD or dlopen().
 * Provides: GPU detection, thermal management, CPU affinity,
 *           memory pool, monitoring, and config loading.
 * ============================================================ */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- GPU vendor enum ---- */
typedef enum {
    GPU_VENDOR_UNKNOWN  = 0,
    GPU_VENDOR_MALI     = 1,
    GPU_VENDOR_POWERVR  = 2,
    GPU_VENDOR_GENERIC  = 3,
} GpuVendor;

typedef enum {
    MALI_MODEL_UNKNOWN = 0,
    MALI_G57    = 57,
    MALI_G68    = 68,
    MALI_G610   = 610,
    MALI_G615   = 615,
    MALI_G720   = 720,
} MaliModel;

typedef enum {
    GPU_PROFILE_BALANCED    = 0,
    GPU_PROFILE_PERFORMANCE = 1,
    GPU_PROFILE_COOL        = 2,
} GpuPowerProfile;

/* ---- Runtime stats (written every ~2 s by monitor thread) ---- */
typedef struct FexStats {
    float   fps;
    float   frameTimeMs;
    int     cpuLoadPct;
    int     gpuLoadPct;
    int     tempCelsius;
    int     perfScalePct;
    uint64_t frameCount;
} FexStats;

/* ---- Public API ---- */

/**
 * fexdxvk_init()
 * Call once at startup (before Wine/FEX launches the game).
 * Reads wrapper.json, detects GPU, starts thermal & monitor threads.
 * Returns 0 on success.
 */
int fexdxvk_init(const char *wrapperJsonPath);

/**
 * fexdxvk_shutdown()
 * Call at exit to flush caches, save stats, stop threads.
 */
void fexdxvk_shutdown(void);

/**
 * fexdxvk_get_stats()
 * Thread-safe snapshot of current performance counters.
 */
void fexdxvk_get_stats(FexStats *out);

/**
 * fexdxvk_set_profile()
 * Dynamically switch performance profile at runtime.
 * profile: "balanced" | "performance" | "cool"
 */
int fexdxvk_set_profile(const char *profile);

/**
 * fexdxvk_get_gpu_vendor() / fexdxvk_get_mali_model()
 */
GpuVendor fexdxvk_get_gpu_vendor(void);
MaliModel fexdxvk_get_mali_model(void);

/* Internal lifecycle hooks used by the wrapper implementation. */
GpuVendor gpu_detect_vendor(void);
MaliModel gpu_detect_mali_model(void);
void cpu_sched_shutdown(void);

#ifdef __cplusplus
}
#endif
