/* ============================================================
 * monitor.c — FPS / frame-time / CPU / GPU / temp monitor
 * Writes JSON stats to /tmp/fexdxvk_stats every ~2 s
 * ============================================================ */

#include "fexdxvk_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>

/* extern from fexdxvk_wrapper.c */
extern void fexdxvk_update_stats(const FexStats *s);
extern int  thermal_get_scale(void);

static pthread_t s_monThread;
static volatile int s_stop = 0;

static void wait_interruptible_ms(int milliseconds) {
    for (int elapsed = 0; elapsed < milliseconds && !s_stop; elapsed += 100)
        usleep(100000);
}

/* ---- CPU load (two /proc/stat snapshots) ---- */
typedef struct { long idle, total; } CpuSnap;

static CpuSnap read_cpu_snap(void) {
    CpuSnap s = {0, 0};
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return s;
    long user, nice, sys, idle, iowait, irq, softirq;
    fscanf(f, "cpu %ld %ld %ld %ld %ld %ld %ld",
           &user, &nice, &sys, &idle, &iowait, &irq, &softirq);
    fclose(f);
    s.idle  = idle;
    s.total = user + nice + sys + idle + iowait + irq + softirq;
    return s;
}

static int cpu_load_pct(CpuSnap a, CpuSnap b) {
    long dTotal = b.total - a.total;
    long dIdle  = b.idle  - a.idle;
    if (dTotal <= 0) return 0;
    return (int)((dTotal - dIdle) * 100 / dTotal);
}

/* ---- GPU load ---- */
static int read_gpu_load(void) {
    const char *paths[] = {
        "/sys/class/misc/mali0/device/utilization",
        "/sys/kernel/debug/mali/utilization",
        NULL
    };
    for (int i = 0; paths[i]; i++) {
        FILE *f = fopen(paths[i], "r");
        if (!f) continue;
        int util = 0;
        fscanf(f, "%d", &util);
        fclose(f);
        return util > 100 ? 100 : util;
    }
    return -1;  /* unavailable */
}

/* ---- Temperature ---- */
static int read_temp(void) {
    const char *path = getenv("FEXDXVK_TEMP_PATH");
    if (!path) path = "/sys/class/thermal/thermal_zone0/temp";
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int raw = 0;
    fscanf(f, "%d", &raw);
    fclose(f);
    return raw > 1000 ? raw / 1000 : raw;
}

/* ---- FPS from DXVK HUD file ---- */
static float read_fps(void) {
    FILE *f = fopen("/tmp/fexdxvk_fps", "r");
    if (!f) return 0;
    float fps = 0;
    fscanf(f, "%f", &fps);
    fclose(f);
    return fps;
}

/* ---- Write JSON stats ---- */
static void write_stats_json(const FexStats *s) {
    FILE *f = fopen("/tmp/fexdxvk_stats", "w");
    if (!f) return;
    fprintf(f,
        "{\n"
        "  \"fps\": %.1f,\n"
        "  \"frame_time_ms\": %.2f,\n"
        "  \"cpu_load_pct\": %d,\n"
        "  \"gpu_load_pct\": %d,\n"
        "  \"temp_celsius\": %d,\n"
        "  \"perf_scale\": %d,\n"
        "  \"frame_count\": %llu\n"
        "}\n",
        s->fps,
        s->frameTimeMs,
        s->cpuLoadPct,
        s->gpuLoadPct,
        s->tempCelsius,
        s->perfScalePct,
        (unsigned long long)s->frameCount);
    fclose(f);
}

/* ---- Monitor thread ---- */
static void *monitor_thread(void *arg) {
    (void)arg;
    uint64_t frameCount = 0;

    while (!s_stop) {
        /* CPU: two snapshots 1 s apart */
        CpuSnap snap1 = read_cpu_snap();
        wait_interruptible_ms(1000);
        CpuSnap snap2 = read_cpu_snap();

        FexStats st;
        memset(&st, 0, sizeof(st));
        st.fps          = read_fps();
        st.frameTimeMs  = st.fps > 0 ? 1000.0f / st.fps : 0;
        st.cpuLoadPct   = cpu_load_pct(snap1, snap2);
        st.gpuLoadPct   = read_gpu_load();
        st.tempCelsius  = read_temp();
        st.perfScalePct = thermal_get_scale();
        st.frameCount   = ++frameCount;

        fexdxvk_update_stats(&st);
        write_stats_json(&st);

        wait_interruptible_ms(1000);   /* total ~2 s between polls */
    }
    return NULL;
}

void monitor_start(void) {
    s_stop = 0;
    /* Monitoring is opt-in: HUD/file polling can add frame-time jitter. */
    const char *enabled = getenv("FEXDXVK_MONITOR");
    if (!enabled || strcmp(enabled, "1") != 0) {
        fprintf(stderr, "[fexdxvk-mon] monitor disabled (low-overhead mode)\n");
        return;
    }
    setenv("DXVK_HUD", "fps,frametime", 0);
    pthread_create(&s_monThread, NULL, monitor_thread, NULL);
    fprintf(stderr, "[fexdxvk-mon] monitor started\n");
}

void monitor_stop(void) {
    if (!s_monThread) return;
    s_stop = 1;
    pthread_join(s_monThread, NULL);
    s_monThread = 0;
}
