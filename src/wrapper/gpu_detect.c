/* ============================================================
 * gpu_detect.c — GPU vendor / model detection via sysfs
 * Supports Mali (Midgard/Bifrost/Valhall) and PowerVR
 * ============================================================ */

#include "fexdxvk_wrapper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* Exposed via wrapper globals (see fexdxvk_wrapper.c) */
extern GpuVendor g_gpuVendor;
extern MaliModel g_maliModel;

/* ---- sysfs helpers ---- */
static int sysfs_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static int read_sysfs_str(const char *path, char *out, size_t sz) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    ssize_t n = read(fd, out, sz - 1);
    close(fd);
    if (n <= 0) return -1;
    out[n] = '\0';
    /* strip newline */
    char *nl = strchr(out, '\n');
    if (nl) *nl = '\0';
    return 0;
}

/* ---- Mali detection ---- */
static int detect_mali(GpuVendor *vendor, MaliModel *model) {
    /* Check for Mali device node */
    const char *mali_paths[] = {
        "/sys/class/misc/mali0",
        "/dev/mali0",
        "/sys/devices/platform/mali.0",
        "/sys/devices/platform/13000000.mali",  /* Exynos common */
        "/sys/devices/platform/gpu",            /* MediaTek common */
        NULL
    };

    for (int i = 0; mali_paths[i]; i++) {
        if (sysfs_exists(mali_paths[i])) {
            *vendor = GPU_VENDOR_MALI;
            break;
        }
    }

    if (*vendor != GPU_VENDOR_MALI) return 0;

    /* Determine Mali model */
    char gpuinfo[128] = "";
    const char *info_paths[] = {
        "/sys/class/misc/mali0/device/gpuinfo",
        "/sys/kernel/debug/mali/gpu_info",
        "/sys/devices/platform/gpu/gpuinfo",
        NULL
    };

    for (int i = 0; info_paths[i]; i++) {
        if (read_sysfs_str(info_paths[i], gpuinfo, sizeof(gpuinfo)) == 0)
            break;
    }

    /* Also try /proc/cpuinfo GPU line (MediaTek Dimensity) */
    if (!gpuinfo[0]) {
        FILE *f = fopen("/proc/cpuinfo", "r");
        if (f) {
            char line[256];
            while (fgets(line, sizeof(line), f)) {
                if (strstr(line, "GPU") || strstr(line, "gpu")) {
                    strncpy(gpuinfo, line, sizeof(gpuinfo) - 1);
                    break;
                }
            }
            fclose(f);
        }
    }

    if (strstr(gpuinfo, "G720") || strstr(gpuinfo, "g720"))
        *model = MALI_G720;
    else if (strstr(gpuinfo, "G615") || strstr(gpuinfo, "g615"))
        *model = MALI_G615;
    else if (strstr(gpuinfo, "G610") || strstr(gpuinfo, "g610"))
        *model = MALI_G610;
    else if (strstr(gpuinfo, "G68") || strstr(gpuinfo, "g68"))
        *model = MALI_G68;
    else if (strstr(gpuinfo, "G57") || strstr(gpuinfo, "g57"))
        *model = MALI_G57;
    else
        *model = MALI_MODEL_UNKNOWN;

    return 1;
}

/* ---- PowerVR detection ---- */
static int detect_powervr(GpuVendor *vendor) {
    const char *pvr_paths[] = {
        "/sys/module/pvrsrvkm",
        "/dev/pvr_sync",
        "/sys/kernel/debug/pvr",
        NULL
    };
    for (int i = 0; pvr_paths[i]; i++) {
        if (sysfs_exists(pvr_paths[i])) {
            *vendor = GPU_VENDOR_POWERVR;
            return 1;
        }
    }
    return 0;
}

/* ---- Apply env vars and DXVK hints after detection ---- */
static void apply_detection_env(GpuVendor vendor, MaliModel model) {
    const char *vendorStr = "generic";
    switch (vendor) {
        case GPU_VENDOR_MALI:    vendorStr = "mali";    break;
        case GPU_VENDOR_POWERVR: vendorStr = "powervr"; break;
        default: break;
    }
    setenv("FEXDXVK_GPU_VENDOR", vendorStr, 1);

    if (vendor == GPU_VENDOR_MALI) {
        char modelStr[16];
        snprintf(modelStr, sizeof(modelStr), "G%d", (int)model);
        setenv("FEXDXVK_MALI_MODEL", modelStr, 1);

        /* Tune DXVK chunk size based on Mali model:
         * G720/G615 can handle larger chunks than G57/G68 */
        if (model == MALI_G720 || model == MALI_G615 || model == MALI_G610)
            setenv("DXVK_CHUNK_SIZE", "8", 0);  /* 0 = don't override if already set */
        else
            setenv("DXVK_CHUNK_SIZE", "4", 0);
    }

    if (vendor == GPU_VENDOR_POWERVR) {
        /* PowerVR needs more conservative buffer sizes */
        setenv("DXVK_CHUNK_SIZE", "2", 0);
    }
}

/* ============================================================
 * gpu_detect_init — main entry point
 * ============================================================ */

/* These are accessed from fexdxvk_wrapper.c via the accessor fns */
static GpuVendor s_vendor = GPU_VENDOR_UNKNOWN;
static MaliModel s_model  = MALI_MODEL_UNKNOWN;

int gpu_detect_init(void) {
    s_vendor = GPU_VENDOR_UNKNOWN;
    s_model  = MALI_MODEL_UNKNOWN;

    if (!detect_mali(&s_vendor, &s_model))
        detect_powervr(&s_vendor);

    if (s_vendor == GPU_VENDOR_UNKNOWN)
        s_vendor = GPU_VENDOR_GENERIC;

    apply_detection_env(s_vendor, s_model);

    const char *names[] = {"unknown", "Mali", "PowerVR", "generic"};
    fprintf(stderr, "[fexdxvk] GPU detected: %s (model=%d)\n",
            names[s_vendor], (int)s_model);
    return 0;
}

/* ---- Power profile application ---- */
void gpu_apply_power_profile(GpuPowerProfile profile) {
    /* Try to write to devfreq governor */
    const char *govPaths[] = {
        "/sys/class/devfreq/13000000.mali/governor",
        "/sys/class/devfreq/gpu/governor",
        "/sys/devices/platform/mali.0/devfreq/governor",
        NULL
    };

    const char *governor;
    switch (profile) {
        case GPU_PROFILE_PERFORMANCE: governor = "performance";     break;
        case GPU_PROFILE_COOL:        governor = "powersave";       break;
        default:                      governor = "simple_ondemand"; break;
    }

    for (int i = 0; govPaths[i]; i++) {
        FILE *f = fopen(govPaths[i], "w");
        if (f) {
            fputs(governor, f);
            fclose(f);
            fprintf(stderr, "[fexdxvk] GPU governor -> %s\n", governor);
            return;
        }
    }
    /* Not an error — root may not be available; env vars still applied */
    setenv("FEXDXVK_GPU_PROFILE",
           profile == GPU_PROFILE_PERFORMANCE ? "performance" :
           profile == GPU_PROFILE_COOL        ? "cool" : "balanced", 1);
}
