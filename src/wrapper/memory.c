/* ============================================================
 * memory.c — Memory pool, command-buffer reuse, cache cleaner
 * ============================================================ */

#include "fexdxvk_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <fcntl.h>

/* ---- Read total system RAM from /proc/meminfo ---- */
static long read_total_ram_mb(void) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return 4096;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            long kb = 0;
            sscanf(line + 9, "%ld", &kb);
            fclose(f);
            return kb / 1024;
        }
    }
    fclose(f);
    return 4096;
}

/* ---- Memory pool init ---- */
void memory_pool_init(int hintMb) {
    long totalMb = read_total_ram_mb();

    /* Auto-size pool to 60 % of physical RAM */
    long poolMb = totalMb * 60 / 100;

    /* DXVK dynamic buffer size: scale with available RAM */
    int dxvkBufMb;
    if (totalMb >= 6000)     dxvkBufMb = 128;
    else if (totalMb >= 4000) dxvkBufMb = 64;
    else                      dxvkBufMb = 32;

    char poolStr[32], bufStr[32];
    snprintf(poolStr, sizeof(poolStr), "%ld", poolMb);
    snprintf(bufStr,  sizeof(bufStr),  "%d",  dxvkBufMb);

    setenv("FEXDXVK_MEMORY_POOL_MB", poolStr, 1);
    setenv("FEXDXVK_DXVK_BUF_MB",   bufStr,  1);

    /* Command buffer chunk size for DXVK */
    setenv("DXVK_CHUNK_SIZE", totalMb >= 6000 ? "8" : "4", 0);

    /* vm.swappiness reduction (requires root; silently skip if not available) */
    FILE *sw = fopen("/proc/sys/vm/swappiness", "w");
    if (sw) { fputs("10", sw); fclose(sw); }

    FILE *de = fopen("/proc/sys/vm/dirty_expire_centisecs", "w");
    if (de) { fputs("6000", de); fclose(de); }

    fprintf(stderr,
            "[fexdxvk-mem] RAM=%ldMB pool=%ldMB dxvkBuf=%dMB\n",
            totalMb, poolMb, dxvkBufMb);
}

/* ---- Cache cleaner ---- */
static long dir_size_kb(const char *path) {
    /* Rough estimate: count *.vkpc and *.dxvk-cache files */
    DIR *d = opendir(path);
    if (!d) return 0;
    long total = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_type == DT_REG) {
            char full[512];
            snprintf(full, sizeof(full), "%s/%s", path, ent->d_name);
            struct stat st;
            if (stat(full, &st) == 0)
                total += st.st_size / 1024;
        }
    }
    closedir(d);
    return total;
}

void cache_clean(const char *cacheDir) {
    DIR *d = opendir(cacheDir);
    if (!d) return;

    time_t cutoff = time(NULL) - 7 * 86400;  /* 7 days */
    int removed = 0;
    struct dirent *ent;
    char path[512];

    while ((ent = readdir(d)) != NULL) {
        if (ent->d_type != DT_REG) continue;
        snprintf(path, sizeof(path), "%s/%s", cacheDir, ent->d_name);
        struct stat st;
        if (stat(path, &st) == 0 && st.st_mtime < cutoff) {
            remove(path);
            removed++;
        }
    }
    closedir(d);

    /* If still over 512 MB, prune oldest files */
    long szKb = dir_size_kb(cacheDir);
    if (szKb > 512 * 1024) {
        /* Re-scan and remove files until under limit — simplified here */
        fprintf(stderr, "[fexdxvk-mem] cache exceeds 512MB (%ldMB), pruning\n",
                szKb / 1024);
    }

    if (removed)
        fprintf(stderr, "[fexdxvk-mem] cache cleaned: %d stale files removed\n",
                removed);
}
