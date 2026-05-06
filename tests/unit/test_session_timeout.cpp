/**
 * @file test_session_timeout.cpp
 * @brief Unit tests for idle session-timeout helpers.
 *
 * @req SWR-SEC-005  Idle timeout validation and expiry logic.
 */

#include <gtest/gtest.h>

#include <cstdint>

extern "C" {
#include "session_timeout.h"
}

TEST(SessionTimeoutBounds, AcceptsApprovedBounds)
{
    EXPECT_EQ(1, session_timeout_is_valid_minutes(SESSION_TIMEOUT_MIN_MINUTES));
    EXPECT_EQ(1, session_timeout_is_valid_minutes(SESSION_TIMEOUT_MAX_MINUTES));
}

TEST(SessionTimeoutBounds, RejectsValuesOutsideApprovedRange)
{
    EXPECT_EQ(0, session_timeout_is_valid_minutes(0));
    EXPECT_EQ(0, session_timeout_is_valid_minutes(-5));
    EXPECT_EQ(0, session_timeout_is_valid_minutes(SESSION_TIMEOUT_MAX_MINUTES + 1));
}

TEST(SessionTimeoutBounds, NormalizesInvalidValueToDefault)
{
    EXPECT_EQ(SESSION_TIMEOUT_DEFAULT_MINUTES,
              session_timeout_normalize_minutes(0));
    EXPECT_EQ(SESSION_TIMEOUT_DEFAULT_MINUTES,
              session_timeout_normalize_minutes(99));
}

TEST(SessionTimeoutBounds, PreservesValidConfiguredValue)
{
    EXPECT_EQ(7, session_timeout_normalize_minutes(7));
    EXPECT_EQ(SESSION_TIMEOUT_MAX_MINUTES,
              session_timeout_normalize_minutes(SESSION_TIMEOUT_MAX_MINUTES));
}

TEST(SessionTimeoutTiming, DurationUsesNormalizedMinutes)
{
    EXPECT_EQ(5u * 60u * 1000u, session_timeout_duration_ms(5));
    EXPECT_EQ((uint32_t)SESSION_TIMEOUT_DEFAULT_MINUTES * 60u * 1000u,
              session_timeout_duration_ms(0));
}

TEST(SessionTimeoutTiming, NotExpiredBeforeThreshold)
{
    const uint32_t last_tick = 1000u;
    const uint32_t now_tick = last_tick + session_timeout_duration_ms(3) - 1u;

    EXPECT_EQ(0, session_timeout_has_expired(last_tick, now_tick, 3));
}

TEST(SessionTimeoutTiming, ExpiresAtThresholdAndAcrossTickWrap)
{
    const uint32_t duration = session_timeout_duration_ms(1);

    EXPECT_EQ(1, session_timeout_has_expired(2000u, 2000u + duration, 1));
    EXPECT_EQ(1, session_timeout_has_expired(0xFFFFFF00u,
                                             0xFFFFFF00u + duration,
                                             1));
}
