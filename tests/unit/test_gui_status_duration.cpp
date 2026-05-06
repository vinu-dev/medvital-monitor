#include <gtest/gtest.h>

extern "C" {
#include "gui_status_duration.h"
}

TEST(StatusDuration, ResetClearsActiveState)
{
    GuiStatusDurationState state = {ALERT_CRITICAL, 15000, 1};

    gui_status_duration_reset(&state);

    EXPECT_FALSE(gui_status_duration_is_active(&state));
    EXPECT_EQ(ALERT_NORMAL, state.tracked_level);
    EXPECT_EQ(0ULL, state.entered_ms);
}

TEST(StatusDuration, StartsAtZeroOnFirstAbnormalState)
{
    GuiStatusDurationState state = {ALERT_NORMAL, 0, 0};

    gui_status_duration_update(&state, 1, 1, ALERT_WARNING, 5000);

    EXPECT_TRUE(gui_status_duration_is_active(&state));
    EXPECT_EQ(ALERT_WARNING, state.tracked_level);
    EXPECT_EQ(0ULL, gui_status_duration_elapsed_seconds(&state, 5000));
}

TEST(StatusDuration, KeepsOriginalStartWhileLevelIsUnchanged)
{
    GuiStatusDurationState state = {ALERT_NORMAL, 0, 0};

    gui_status_duration_update(&state, 1, 1, ALERT_WARNING, 5000);
    gui_status_duration_update(&state, 1, 1, ALERT_WARNING, 65000);

    EXPECT_EQ(ALERT_WARNING, state.tracked_level);
    EXPECT_EQ(5000ULL, state.entered_ms);
    EXPECT_EQ(60ULL, gui_status_duration_elapsed_seconds(&state, 65000));
}

TEST(StatusDuration, ResetsWhenSeverityEscalates)
{
    GuiStatusDurationState state = {ALERT_NORMAL, 0, 0};

    gui_status_duration_update(&state, 1, 1, ALERT_WARNING, 1000);
    gui_status_duration_update(&state, 1, 1, ALERT_CRITICAL, 7000);

    EXPECT_EQ(ALERT_CRITICAL, state.tracked_level);
    EXPECT_EQ(7000ULL, state.entered_ms);
    EXPECT_EQ(0ULL, gui_status_duration_elapsed_seconds(&state, 7000));
}

TEST(StatusDuration, ResetsWhenSeverityDeescalates)
{
    GuiStatusDurationState state = {ALERT_NORMAL, 0, 0};

    gui_status_duration_update(&state, 1, 1, ALERT_CRITICAL, 2000);
    gui_status_duration_update(&state, 1, 1, ALERT_WARNING, 11000);

    EXPECT_EQ(ALERT_WARNING, state.tracked_level);
    EXPECT_EQ(11000ULL, state.entered_ms);
    EXPECT_EQ(0ULL, gui_status_duration_elapsed_seconds(&state, 11000));
}

TEST(StatusDuration, ClearsWhenAggregateStatusReturnsNormal)
{
    GuiStatusDurationState state = {ALERT_NORMAL, 0, 0};

    gui_status_duration_update(&state, 1, 1, ALERT_WARNING, 1000);
    gui_status_duration_update(&state, 1, 1, ALERT_NORMAL, 9000);

    EXPECT_FALSE(gui_status_duration_is_active(&state));
}

TEST(StatusDuration, ClearsWhenMonitoringStops)
{
    GuiStatusDurationState state = {ALERT_NORMAL, 0, 0};

    gui_status_duration_update(&state, 1, 1, ALERT_CRITICAL, 1000);
    gui_status_duration_update(&state, 0, 1, ALERT_CRITICAL, 9000);

    EXPECT_FALSE(gui_status_duration_is_active(&state));
}

TEST(StatusDuration, ClearsWhenPatientSessionIsMissing)
{
    GuiStatusDurationState state = {ALERT_NORMAL, 0, 0};

    gui_status_duration_update(&state, 1, 1, ALERT_WARNING, 1000);
    gui_status_duration_update(&state, 1, 0, ALERT_WARNING, 9000);

    EXPECT_FALSE(gui_status_duration_is_active(&state));
}

TEST(StatusDuration, FormatsMinutesAndSecondsBelowOneHour)
{
    char formatted[16] = {0};

    ASSERT_TRUE(gui_status_duration_format(135ULL, formatted, sizeof(formatted)));
    EXPECT_STREQ("02:15", formatted);
}

TEST(StatusDuration, FormatsHoursMinutesAndSecondsAfterOneHour)
{
    char formatted[16] = {0};

    ASSERT_TRUE(gui_status_duration_format(3723ULL, formatted, sizeof(formatted)));
    EXPECT_STREQ("01:02:03", formatted);
}
