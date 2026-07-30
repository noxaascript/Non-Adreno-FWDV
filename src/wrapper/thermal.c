/* ============================================================
 * thermal.c — Thermal monitor + adaptive performance scaling
 * ============================================================ */

#include "fexdxvk_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

static pthread_t  s_thermalThread;
static volatile int s_stop = 0;
static int        s_threshold = 75;
static int        s_perfScale = 100;  /* 0-100 % */

static void wait_interruptible_ms(int milliseconds) {
    for (int elapsed = 0; elapsed < milliseconds && !s_stop; elapsed += 100)
        usleep(100000);
}

/* extern from fexdxvk_wrapper.c */
extern void fexdxvk_update_stats(const struct FexStats *s);

static int find_temp_sensor(char *pathOut, size_t sz) {
    /* Prefer CPU/SoC thermal zones */
    const char *types[] = {"cpu", "soc", "gpu", "tsens_tz_sensor", NULL};

    for (int idx = 0; idx < 32; idx++) {
        char typePath[128];
        snprintf(typePath, sizeof(typePath),
                 "/sys/class/thermal/thermal_zone%d/type", idx);
        FILE *f = fopen(typePath, "r");
        if (!f) break;
        char typeStr[64] = "";
        fgets(typeStr, sizeof(typeStr), f);
        fclose(f);

        for (int t = 0; types[t]; t++) {
            if (strstr(typeStr, types[t])) {
                snprintf(pathOut, sz,
                         "/sys/class/thermal/thermal_zone%d/temp", idx);
                return 1;
            }
        }
    }

    /* Fallback: zone0 */
    snprintf(pathOut, sz, "/sys/class/thermal/thermal_zone0/temp");
    return 1;
}

static int read_temp_celsius(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int raw = 0;
    fscanf(f, "%d", &raw);
    fclose(f);
    return raw > 1000 ? raw / 1000 : raw;
}

static void apply_perf_scale(int scale) {
    if (scale == s_perfScale) return;
    s_perfScale = scale;

    /* Write scale to tmpfs so the Vulkan layer / monitor can read it */
    FILE *f = fopen("/tmp/fexdxvk_perf_scale", "w");
    if (f) { fprintf(f, "%d\n", scale); fclose(f); }

    char scaleStr[8];
    snprintf(scaleStr, sizeof(scaleStr), "%d", scale);
    setenv("FEXDXVK_PERF_SCALE", scaleStr, 1);

    fprintf(stderr, "[fexdxvk-thermal] perf scale -> %d%%\n", scale);
}

static void *thermal_thread(void *arg) {
    (void)arg;
    char sensorPath[256];
    find_temp_sensor(sensorPath, sizeof(sensorPath));
    fprintf(stderr, "[fexdxvk-thermal] sensor: %s | threshold: %d°C\n",
            sensorPath, s_threshold);

    int prevTemp = 0;
    const int warnThreshold = s_threshold - 5;
    const int critThreshold = s_threshold + 5;

    while (!s_stop) {
        int temp = read_temp_celsius(sensorPath);
        if (temp < 0) { wait_interruptible_ms(5000); continue; }

        /* Anti-spike: temp jumped ≥ 8°C in one poll interval */
        if ((temp - prevTemp) >= 8 && prevTemp > 0) {
            fprintf(stderr, "[fexdxvk-thermal] spike +%d°C detected\n",
                    temp - prevTemp);
            apply_perf_scale(80);
            wait_interruptible_ms(10000);
        }

        /* Stepped performance scaling */
        if (temp >= critThreshold)      apply_perf_scale(50);
        else if (temp >= s_threshold)   apply_perf_scale(75);
        else if (temp >= warnThreshold) apply_perf_scale(90);
        else                            apply_perf_scale(100);

        prevTemp = temp;
        wait_interruptible_ms(5000);
    }
    return NULL;
}

void thermal_start(int thresholdCelsius) {
    s_threshold = thresholdCelsius;
    s_stop = 0;
    pthread_create(&s_thermalThread, NULL, thermal_thread, NULL);
}

void thermal_stop(void) {
    s_stop = 1;
    pthread_join(s_thermalThread, NULL);
}

int thermal_get_scale(void) { return s_perfScale; }
