/**
 * @file test_dashboard_freshness.cpp
 * @brief Unit tests for dashboard freshness-state calculation.
 *
 * @req SWR-GUI-014
 */

#include <gtest/gtest.h>

extern "C" {
#include "dashboard_freshness.h"
}

TEST(DashboardFreshness, REQ_GUI_014_DeviceModeSuppressesCue)
{
    DashboardFreshness freshness =
        dashboard_freshness_compute(0, 0, 1, 7000ULL, 1000ULL);

    EXPECT_EQ(DASHBOARD_FRESHNESS_DEVICE_MODE, freshness.state);
    EXPECT_EQ(0UL, freshness.age_seconds);
}

TEST(DashboardFreshness, REQ_GUI_014_NoReadingReturnsWaitingState)
{
    DashboardFreshness freshness =
        dashboard_freshness_compute(1, 0, 0, 7000ULL, 0ULL);

    EXPECT_EQ(DASHBOARD_FRESHNESS_NO_READING, freshness.state);
    EXPECT_EQ(0UL, freshness.age_seconds);
}

TEST(DashboardFreshness, REQ_GUI_014_LiveAgeUsesWholeSecondsSinceAcceptance)
{
    DashboardFreshness freshness =
        dashboard_freshness_compute(1, 0, 1, 9876ULL, 4321ULL);

    EXPECT_EQ(DASHBOARD_FRESHNESS_LIVE_AGE, freshness.state);
    EXPECT_EQ(5UL, freshness.age_seconds);
}

TEST(DashboardFreshness, REQ_GUI_014_PausedAgeReusesSameTickBasis)
{
    DashboardFreshness freshness =
        dashboard_freshness_compute(1, 1, 1, 12500ULL, 2000ULL);

    EXPECT_EQ(DASHBOARD_FRESHNESS_PAUSED_AGE, freshness.state);
    EXPECT_EQ(10UL, freshness.age_seconds);
}

TEST(DashboardFreshness, REQ_GUI_014_TickRegressionClampsAgeToZero)
{
    DashboardFreshness freshness =
        dashboard_freshness_compute(1, 0, 1, 1500ULL, 3000ULL);

    EXPECT_EQ(DASHBOARD_FRESHNESS_LIVE_AGE, freshness.state);
    EXPECT_EQ(0UL, freshness.age_seconds);
}
