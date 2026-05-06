/**
 * @file session_timeout.h
 * @brief Idle session-timeout policy helpers.
 *
 * Owns the approved timeout bounds plus pure helpers for validation and
 * elapsed-time expiry decisions.
 *
 * @req SWR-SEC-005
 */

#ifndef SESSION_TIMEOUT_H
#define SESSION_TIMEOUT_H

#include <stdint.h>

#define SESSION_TIMEOUT_MIN_MINUTES 1
#define SESSION_TIMEOUT_MAX_MINUTES 30
#define SESSION_TIMEOUT_DEFAULT_MINUTES 5

#ifdef __cplusplus
extern "C" {
#endif

int session_timeout_is_valid_minutes(int minutes);
int session_timeout_normalize_minutes(int minutes);
uint32_t session_timeout_duration_ms(int minutes);
int session_timeout_has_expired(uint32_t last_activity_tick,
                                uint32_t now_tick,
                                int minutes);

#ifdef __cplusplus
}
#endif

#endif /* SESSION_TIMEOUT_H */
