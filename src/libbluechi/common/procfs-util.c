/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <errno.h>

#include "libbluechi/common/common.h"
#include "procfs-util.h"

#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

static uint64_t calc_gcd64(uint64_t a, uint64_t b) {
        while (b > 0) {
                uint64_t t;

                t = a % b;

                a = b;
                b = t;
        }

        return a;
}

int procfs_cpu_get_usage(uint64_t *ret) {
        unsigned long user_ticks, nice_ticks, system_ticks, irq_ticks, softirq_ticks, guest_ticks = 0,
                                                                                      guest_nice_ticks = 0;
        long ticks_per_second;
        uint64_t sum, gcd, a, b;
        _cleanup_fclose_ FILE *f = NULL;
        char buf[512 - 64];

        assert(ret);

        f = fopen("/proc/stat", "re");
        if (!f) {
                return -errno;
        }

        if (!fgets(buf, sizeof(buf), f) || buf[0] != 'c' /* not "cpu" */) {
                return -EINVAL;
        }

        if (sscanf(buf,
                   "cpu %lu %lu %lu %*u %*u %lu %lu %*u %lu %lu",
                   &user_ticks,
                   &nice_ticks,
                   &system_ticks,
                   &irq_ticks,
                   &softirq_ticks,
                   &guest_ticks,
                   &guest_nice_ticks) < 5) /* we only insist on the first five fields */ {
                return -EINVAL;
        }

        ticks_per_second = sysconf(_SC_CLK_TCK);
        if (ticks_per_second < 0) {
                return -errno;
        }
        assert(ticks_per_second > 0);

        sum = (uint64_t) user_ticks + (uint64_t) nice_ticks + (uint64_t) system_ticks + (uint64_t) irq_ticks +
                        (uint64_t) softirq_ticks + (uint64_t) guest_ticks + (uint64_t) guest_nice_ticks;

        /* Let's reduce this fraction before we apply it to avoid overflows when converting this to μsec */
        gcd = calc_gcd64(1000000000ULL, ticks_per_second);

        a = (uint64_t) 1000000000ULL / gcd;
        b = (uint64_t) ticks_per_second / gcd;

        *ret = DIV_ROUND_UP(sum * a, b);
        return 0;
}

int procfs_memory_get(uint64_t *ret_total, uint64_t *ret_used) {
        uint64_t mem_total = UINT64_MAX, mem_available = UINT64_MAX;
        _cleanup_fclose_ FILE *f = NULL;
        char buf[60];

        f = fopen("/proc/meminfo", "re");
        if (!f) {
                return -errno;
        }

        while (fgets(buf, sizeof(buf), f) != NULL) {
                char *c = strchr(buf, ':');
                if (!c) {
                        continue;
                }
                *c = '\0';

                if (streq(buf, "MemTotal")) {
                        mem_total = strtoul(c + 1, NULL, 10);
                        if ((errno == ERANGE && mem_total == UINT64_MAX) || (errno != 0 && mem_total == 0)) {
                                return -errno;
                        }
                        continue;
                }

                if (streq(buf, "MemAvailable")) {
                        mem_available = strtoul(c + 1, NULL, 10);
                        if ((errno == ERANGE && mem_available == UINT64_MAX) ||
                            (errno != 0 && mem_available == 0)) {
                                return -errno;
                        }
                        continue;
                }

                if (mem_total != UINT64_MAX && mem_available != UINT64_MAX) {
                        break;
                }
        }

        if (mem_available > mem_total) {
                return -EINVAL;
        }

        if (ret_total) {
                *ret_total = mem_total;
        }

        if (ret_used) {
                *ret_used = mem_total - mem_available;
        }

        return 0;
}
