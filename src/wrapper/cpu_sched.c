/* ============================================================
 * cpu_sched.c — CPU scheduling, thread affinity, worker pool
 * ============================================================ */

#define _GNU_SOURCE  /* cpu_set_t, CPU_SET, sched_setaffinity on Android NDK */
#include "fexdxvk_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <dirent.h>

static int s_cpuCount      = 0;
static int s_bigCoreMask   = 0;    /* bitmask of "big" core indices */
static int s_workerThreads = 0;

/* ---- Read max CPU freq (kHz) ---- */
static long read_cpu_max_freq(int cpuIdx) {
    char path[128];
    snprintf(path, sizeof(path),
             "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", cpuIdx);
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    long freq = 0;
    fscanf(f, "%ld", &freq);
    fclose(f);
    return freq;
}

/* ---- Build big/little core masks ---- */
static void build_core_topology(void) {
    s_cpuCount = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (s_cpuCount <= 0) s_cpuCount = 4;

    long maxFreq = 0;
    for (int i = 0; i < s_cpuCount; i++) {
        long f = read_cpu_max_freq(i);
        if (f > maxFreq) maxFreq = f;
    }

    s_bigCoreMask = 0;
    for (int i = 0; i < s_cpuCount; i++) {
        long f = read_cpu_max_freq(i);
        /* Cores within 20 % of max freq are "big" */
        if (f >= (maxFreq * 80 / 100))
            s_bigCoreMask |= (1 << i);
    }

    fprintf(stderr, "[fexdxvk-cpu] cores=%d bigMask=0x%x maxFreq=%ldkHz\n",
            s_cpuCount, s_bigCoreMask, maxFreq);
}

/* ---- Set thread CPU affinity ---- */
static void set_affinity_mask(pid_t pid, int mask) {
    if (!mask) return;
    cpu_set_t cs;
    CPU_ZERO(&cs);
    for (int i = 0; i < 32; i++)
        if (mask & (1 << i)) CPU_SET(i, &cs);

    if (sched_setaffinity(pid, sizeof(cs), &cs) == 0)
        fprintf(stderr, "[fexdxvk-cpu] affinity mask 0x%x set for pid %d\n",
                mask, (int)pid);
}

/* ---- Render thread optimization (SCHED_FIFO if permitted) ---- */
static void optimize_render_thread(void) {
    struct sched_param sp = { .sched_priority = 10 };
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp) == 0) {
        fprintf(stderr, "[fexdxvk-cpu] render thread: SCHED_FIFO prio=10\n");
    } else {
        /* Fall back to niceness */
        setpriority(PRIO_PROCESS, 0, -5);
        fprintf(stderr, "[fexdxvk-cpu] render thread: nice=-5\n");
    }

    /* Pin to big cores */
    if (s_bigCoreMask)
        set_affinity_mask(0 /* 0 = this thread */, s_bigCoreMask);
}

/* ---- Worker thread pool ---- */
typedef struct {
    pthread_t thread;
    volatile int active;
} WorkerThread;

#define MAX_WORKERS 16
static WorkerThread s_workers[MAX_WORKERS];
static volatile int s_workerStop = 0;
static int s_workersStarted = 0;

static void *worker_entry(void *arg) {
    WorkerThread *w = (WorkerThread *)arg;
    w->active = 1;
    /* Pin worker threads to big cores as well */
    if (s_bigCoreMask)
        set_affinity_mask(0, s_bigCoreMask);

    /* Worker loop: keep idle workers asleep instead of burning a 1 ms poll.
     * FEX/DXVK can opt in to this pool with FEXDXVK_WORKERS=1. */
    while (!s_workerStop) {
        usleep(20000);
    }
    w->active = 0;
    return NULL;
}

static void start_worker_pool(void) {
    s_workerThreads = s_cpuCount > 1 ? s_cpuCount - 1 : 1;
    if (s_workerThreads > MAX_WORKERS) s_workerThreads = MAX_WORKERS;

    s_workerStop = 0;
    for (int i = 0; i < s_workerThreads; i++) {
        pthread_create(&s_workers[i].thread, NULL, worker_entry, &s_workers[i]);
    }
    s_workersStarted = 1;
    fprintf(stderr, "[fexdxvk-cpu] worker pool: %d threads\n", s_workerThreads);
}

/* ---- CPU overhead reduction env vars ---- */
static void reduce_cpu_overhead(void) {
    /* Suppress Wine debug channels (huge CPU overhead source) */
    if (!getenv("WINEDEBUG"))
        setenv("WINEDEBUG", "-all", 0);

    /* Report accurate CPU topology to Wine */
    char topo[32];
    snprintf(topo, sizeof(topo), "%d:1", s_cpuCount);
    setenv("WINE_CPU_TOPOLOGY", topo, 0);

    /* Large address aware for 32-bit games */
    setenv("WINE_LARGE_ADDRESS_AWARE", "1", 0);

    /* FEX JIT: apply big-core affinity mask */
    if (s_bigCoreMask) {
        char maskHex[16];
        snprintf(maskHex, sizeof(maskHex), "0x%x", s_bigCoreMask);
        setenv("FEXDXVK_BIG_CORE_MASK", maskHex, 1);
    }

    fprintf(stderr, "[fexdxvk-cpu] WINE_CPU_TOPOLOGY=%s WINEDEBUG=%s\n",
            topo, getenv("WINEDEBUG"));
}

/* ============================================================
 * cpu_sched_init — main entry point
 * ============================================================ */
void cpu_sched_init(void) {
    build_core_topology();
    optimize_render_thread();
    reduce_cpu_overhead();
    if (getenv("FEXDXVK_WORKERS") &&
        strcmp(getenv("FEXDXVK_WORKERS"), "1") == 0)
        start_worker_pool();
    else
        fprintf(stderr, "[fexdxvk-cpu] worker pool disabled (low-overhead mode)\n");
}

void cpu_sched_shutdown(void) {
    if (!s_workersStarted) return;
    s_workerStop = 1;
    for (int i = 0; i < s_workerThreads; i++)
        pthread_join(s_workers[i].thread, NULL);
    s_workersStarted = 0;
    s_workerThreads = 0;
}
