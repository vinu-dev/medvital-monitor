#include "dashboard_freshness.h"

DashboardFreshness dashboard_freshness_compute(int simulation_enabled,
                                               int simulation_paused,
                                               int has_last_reading_tick,
                                               uint64_t current_tick_ms,
                                               uint64_t last_reading_tick_ms)
{
    DashboardFreshness freshness;

    freshness.age_seconds = 0UL;

    if (!simulation_enabled) {
        freshness.state = DASHBOARD_FRESHNESS_DEVICE_MODE;
        return freshness;
    }

    if (!has_last_reading_tick) {
        freshness.state = DASHBOARD_FRESHNESS_NO_READING;
        return freshness;
    }

    freshness.state = simulation_paused
        ? DASHBOARD_FRESHNESS_PAUSED_AGE
        : DASHBOARD_FRESHNESS_LIVE_AGE;

    if (current_tick_ms >= last_reading_tick_ms) {
        freshness.age_seconds =
            (unsigned long)((current_tick_ms - last_reading_tick_ms) / 1000ULL);
    }

    return freshness;
}
