/* ============================================================
 * fexdxvk_wrapper.c — FexDXVK NonAdreno Wrapper Library
 * Builds to: libfexdxvk_wrapper.so
 *
 * Entry points: fexdxvk_init / fexdxvk_shutdown /
 *               fexdxvk_get_stats / fexdxvk_set_profile
 * ============================================================ */

#include "fexdxvk_wrapper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Forward declarations */
int  gpu_detect_init(void);
void gpu_apply_power_profile(GpuPowerProfile profile);
void thermal_start(int thresholdCelsius);
void thermal_stop(void);
void cpu_sched_init(void);
void memory_pool_init(int poolMb);
void monitor_start(void);
void monitor_stop(void);

/* ---- Internal state ---- */
static struct {
    GpuVendor       vendor;
    MaliModel       maliModel;
    GpuPowerProfile powerProfile;
    int             thermalThreshold;
    bool            initialized;
    pthread_mutex_t statsMutex;
    FexStats        stats;
} g_wrapper = {
    .vendor          = GPU_VENDOR_UNKNOWN,
    .maliModel       = MALI_MODEL_UNKNOWN,
    .powerProfile    = GPU_PROFILE_BALANCED,
    .thermalThreshold= 75,
    .initialized     = false,
    .statsMutex      = PTHREAD_MUTEX_INITIALIZER,
};

/* ---- Minimal JSON parser for wrapper.json ----
 * (no external dependencies — parses key:value strings/bools/ints) */
static void parse_json_str(const char *json, const char *key,
                            char *out, size_t outSz) {
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(json, pattern);
    if (!p) { out[0] = '\0'; return; }
    p = strchr(p, ':');
    if (!p) { out[0] = '\0'; return; }
    while (*p == ':' || *p == ' ' || *p == '"') p++;
    size_t i = 0;
    while (*p && *p != '"' && *p != ',' && *p != '\n' && i < outSz - 1)
        out[i++] = *p++;
    out[i] = '\0';
}

static int parse_json_int(const char *json, const char *key, int def) {
    char buf[32];
    parse_json_str(json, key, buf, sizeof(buf));
    if (!buf[0]) return def;
    /* strip any trailing non-digit chars */
    for (char *p = buf; *p; p++) {
        if (*p < '0' || *p > '9') { *p = '\0'; break; }
    }
    return buf[0] ? atoi(buf) : def;
}

static bool parse_json_bool(const char *json, const char *key, bool def) {
    char buf[16];
    parse_json_str(json, key, buf, sizeof(buf));
    if (!buf[0]) return def;
    return (buf[0] == 't');
}

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    char *buf = malloc(sz + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

/* ============================================================
 * fexdxvk_init
 * ============================================================ */
int fexdxvk_init(const char *wrapperJsonPath) {
    if (g_wrapper.initialized) return 0;

    fprintf(stderr, "[fexdxvk] init: %s\n",
            wrapperJsonPath ? wrapperJsonPath : "(null)");

    /* Parse wrapper.json */
    char profileStr[32] = "balanced";
    int  thermal = 75;

    if (wrapperJsonPath) {
        char *json = read_file(wrapperJsonPath);
        if (json) {
            parse_json_str(json, "profile", profileStr, sizeof(profileStr));
            thermal = parse_json_int(json, "thermalThresholdCelsius", 75);
            free(json);
        }
    }

    /* GPU detection */
    gpu_detect_init();

    /* Apply power profile */
    if (strcmp(profileStr, "performance") == 0) {
        g_wrapper.powerProfile = GPU_PROFILE_PERFORMANCE;
    } else if (strcmp(profileStr, "cool") == 0) {
        g_wrapper.powerProfile = GPU_PROFILE_COOL;
    } else {
        g_wrapper.powerProfile = GPU_PROFILE_BALANCED;
    }
    gpu_apply_power_profile(g_wrapper.powerProfile);

    /* CPU scheduling */
    cpu_sched_init();

    /* Memory pool */
    memory_pool_init(256); /* 256 MB default; overridden by auto-detect in memory.c */

    /* Thermal monitor */
    g_wrapper.thermalThreshold = thermal;
    thermal_start(thermal);

    /* Performance monitor */
    monitor_start();

    g_wrapper.initialized = true;
    fprintf(stderr, "[fexdxvk] ready — vendor=%d mali=%d profile=%s thermal=%d°C\n",
            g_wrapper.vendor, g_wrapper.maliModel, profileStr, thermal);
    return 0;
}

/* ============================================================
 * fexdxvk_shutdown
 * ============================================================ */
void fexdxvk_shutdown(void) {
    if (!g_wrapper.initialized) return;
    monitor_stop();
    thermal_stop();
    g_wrapper.initialized = false;
    fprintf(stderr, "[fexdxvk] shutdown complete\n");
}

/* ============================================================
 * fexdxvk_get_stats
 * ============================================================ */
void fexdxvk_get_stats(FexStats *out) {
    if (!out) return;
    pthread_mutex_lock(&g_wrapper.statsMutex);
    *out = g_wrapper.stats;
    pthread_mutex_unlock(&g_wrapper.statsMutex);
}

/* Called by monitor.c to update stats */
void fexdxvk_update_stats(const FexStats *s) {
    pthread_mutex_lock(&g_wrapper.statsMutex);
    g_wrapper.stats = *s;
    pthread_mutex_unlock(&g_wrapper.statsMutex);
}

/* ============================================================
 * fexdxvk_set_profile
 * ============================================================ */
int fexdxvk_set_profile(const char *profile) {
    if (!profile) return -1;

    GpuPowerProfile p;
    if (strcmp(profile, "performance") == 0)      p = GPU_PROFILE_PERFORMANCE;
    else if (strcmp(profile, "cool") == 0)        p = GPU_PROFILE_COOL;
    else                                           p = GPU_PROFILE_BALANCED;

    g_wrapper.powerProfile = p;
    gpu_apply_power_profile(p);
    fprintf(stderr, "[fexdxvk] profile changed: %s\n", profile);
    return 0;
}

/* ============================================================
 * Accessors
 * ============================================================ */
GpuVendor fexdxvk_get_gpu_vendor(void) { return g_wrapper.vendor; }
MaliModel fexdxvk_get_mali_model(void) { return g_wrapper.maliModel; }

/* ---- Constructor / Destructor (auto-called on dlopen/dlclose) ---- */
__attribute__((constructor))
static void fexdxvk_constructor(void) {
    const char *cfgPath = getenv("FEXDXVK_CONFIG");
    fexdxvk_init(cfgPath ? cfgPath : NULL);
}

__attribute__((destructor))
static void fexdxvk_destructor(void) {
    fexdxvk_shutdown();
}
