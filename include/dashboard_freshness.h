#ifndef DASHBOARD_FRESHNESS_H
#define DASHBOARD_FRESHNESS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DASHBOARD_FRESHNESS_DEVICE_MODE = 0,
    DASHBOARD_FRESHNESS_NO_READING,
    DASHBOARD_FRESHNESS_LIVE_AGE,
    DASHBOARD_FRESHNESS_PAUSED_AGE
} DashboardFreshnessState;

typedef struct {
    DashboardFreshnessState state;
    unsigned long age_seconds;
} DashboardFreshness;

DashboardFreshness dashboard_freshness_compute(int simulation_enabled,
                                               int simulation_paused,
                                               int has_last_reading_tick,
                                               uint64_t current_tick_ms,
                                               uint64_t last_reading_tick_ms);

#ifdef __cplusplus
}
#endif

#endif /* DASHBOARD_FRESHNESS_H */
