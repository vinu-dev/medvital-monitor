/**
 * @file session_timeout.c
 * @brief Idle session-timeout policy helpers.
 *
 * @req SWR-SEC-005
 */

#include "session_timeout.h"

int session_timeout_is_valid_minutes(int minutes)
{
    return (minutes >= SESSION_TIMEOUT_MIN_MINUTES &&
            minutes <= SESSION_TIMEOUT_MAX_MINUTES) ? 1 : 0;
}

int session_timeout_normalize_minutes(int minutes)
{
    return session_timeout_is_valid_minutes(minutes)
        ? minutes
        : SESSION_TIMEOUT_DEFAULT_MINUTES;
}

uint32_t session_timeout_duration_ms(int minutes)
{
    const uint32_t normalized =
        (uint32_t)session_timeout_normalize_minutes(minutes);
    return normalized * 60u * 1000u;
}

int session_timeout_has_expired(uint32_t last_activity_tick,
                                uint32_t now_tick,
                                int minutes)
{
    const uint32_t elapsed = now_tick - last_activity_tick;
    return (elapsed >= session_timeout_duration_ms(minutes)) ? 1 : 0;
}
