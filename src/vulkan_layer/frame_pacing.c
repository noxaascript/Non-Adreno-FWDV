/* ============================================================
 * frame_pacing.c — Vulkan frame pacing via present throttle
 * Ensures frames are submitted at a steady cadence to avoid
 * GPU command queue back-pressure and display judder on Mali.
 * ============================================================ */

#include "vk_layer_fexdxvk.h"
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static inline uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static void precise_sleep_until(uint64_t deadlineNs) {
    uint64_t now = now_ns();
    if (now >= deadlineNs) return;

    /* Use clock_nanosleep for sub-ms precision */
    struct timespec ts = {
        .tv_sec  = (time_t)(deadlineNs / 1000000000ULL),
        .tv_nsec = (long)(deadlineNs % 1000000000ULL),
    };

    int ret;
    do {
        ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL);
    } while (ret == EINTR);
}

/* Called from vkQueuePresentKHR intercept */
void frame_pacing_present(FexDevice *dev) {
    const char *enabled = getenv("FEXDXVK_FRAME_PACING");
    if (!enabled || strcmp(enabled, "1") != 0)
        return;
    uint64_t now = now_ns();

    if (dev->lastPresentNs != 0) {
        uint64_t target = dev->lastPresentNs + dev->targetFrameNs;

        /* Only sleep if we are ahead of schedule.
         * Never sleep longer than 1 full frame period (guard against hangs). */
        if (now < target && (target - now) < dev->targetFrameNs)
            precise_sleep_until(target);
    }

    dev->lastPresentNs = now_ns();
    dev->frameIndex++;
}
