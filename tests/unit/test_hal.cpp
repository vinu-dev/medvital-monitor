/**
 * @file test_hal.cpp
 * @brief Unit tests for HAL interface and simulator back-end
 *
 * @req SWR-GUI-005 HAL interface contract
 * @req SWR-GUI-006 Simulator sequence and cycling behaviour
 */

#include <gtest/gtest.h>

extern "C" {
#include "vitals.h"
#include "hw_vitals.h"
}

// ---------------------------------------------------------------------------
// Helper: physiological validity check
// ---------------------------------------------------------------------------
static void assert_vitals_valid(const VitalSigns &v)
{
    EXPECT_GE(v.heart_rate,    0)   << "HR must be >= 0";
    EXPECT_LE(v.heart_rate,    300) << "HR must be <= 300";
    EXPECT_GT(v.systolic_bp,   0)   << "Systolic BP must be > 0";
    EXPECT_GT(v.diastolic_bp,  0)   << "Diastolic BP must be > 0";
    EXPECT_GE(v.temperature,   30.0f) << "Temperature must be >= 30 C";
    EXPECT_LE(v.temperature,   45.0f) << "Temperature must be <= 45 C";
    EXPECT_GE(v.spo2,          50)  << "SpO2 must be >= 50 %";
    EXPECT_LE(v.spo2,          100) << "SpO2 must be <= 100 %";
}

// ---------------------------------------------------------------------------
// Fixture for HAL interface contract tests  (SWR-GUI-005)
// ---------------------------------------------------------------------------
class HALTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        hw_init();
    }
};

// @req SWR-GUI-005 - hw_init must not crash
TEST_F(HALTest, InitDoesNotCrash)
{
    // If we reach this line hw_init() returned successfully
    SUCCEED();
}

// @req SWR-GUI-005 - single reading must be physiologically valid
TEST_F(HALTest, SingleReadingIsPhysiologicallyValid)
{
    VitalSigns v{};
    hw_get_next_reading(&v);
    assert_vitals_valid(v);
}

// @req SWR-GUI-005 - ten consecutive readings must all be physiologically valid
TEST_F(HALTest, TenConsecutiveReadingsAreValid)
{
    for (int i = 0; i < 10; ++i)
    {
        VitalSigns v{};
        hw_get_next_reading(&v);
        SCOPED_TRACE("reading index " + std::to_string(i));
        assert_vitals_valid(v);
    }
}

// @req SWR-GUI-005 - calling hw_get_next_reading without a prior hw_init
//   must not crash (defensive HAL requirement)
TEST(HALTestNoInit, ReadingWithoutInitDoesNotCrash)
{
    // Deliberately skip hw_init(); the function must still be safe to call
    VitalSigns v{};
    // As long as this does not abort/segfault the test passes
    hw_get_next_reading(&v);
    // Result may be anything, but physiological ranges are preferred
    // We only mandate no crash here
    SUCCEED();
}

// @req SWR-GUI-005 - null pointer passed to hw_get_next_reading must not crash
//   (implementations are expected to guard against NULL)
// NOTE: commented out by default as behaviour is implementation-defined;
//       enable if your HAL contract explicitly requires NULL safety.
// TEST_F(HALTest, NullPointerDoesNotCrash)
// {
//     hw_get_next_reading(nullptr);
//     SUCCEED();
// }

// ---------------------------------------------------------------------------
// Fixture for simulator sequence tests  (SWR-GUI-006)
// ---------------------------------------------------------------------------
class SimSequenceTest : public ::testing::Test
{
protected:
    static constexpr int SCENARIO_LEN = 20;

    void SetUp() override
    {
        hw_init();
    }

    // Read exactly SCENARIO_LEN readings and return them
    std::vector<VitalSigns> read_full_cycle()
    {
        std::vector<VitalSigns> readings(SCENARIO_LEN);
        for (int i = 0; i < SCENARIO_LEN; ++i)
            hw_get_next_reading(&readings[i]);
        return readings;
    }
};

// @req SWR-GUI-006 - full cycle of 20 readings contains all unique entries
TEST_F(SimSequenceTest, TwentyReadingsAreUnique)
{
    auto readings = read_full_cycle();

    // Build a set of (HR, systolic, diastolic, temp-x10, spo2) tuples
    // using a simple uniqueness check with a vector comparison
    for (int i = 0; i < SCENARIO_LEN; ++i)
    {
        for (int j = i + 1; j < SCENARIO_LEN; ++j)
        {
            bool same = (readings[i].heart_rate   == readings[j].heart_rate   &&
                         readings[i].systolic_bp  == readings[j].systolic_bp  &&
                         readings[i].diastolic_bp == readings[j].diastolic_bp &&
                         readings[i].temperature  == readings[j].temperature  &&
                         readings[i].spo2         == readings[j].spo2);
            if (same)
            {
                ADD_FAILURE() << "Readings at index " << i << " and " << j
                              << " are identical; all 20 entries must be unique";
            }
        }
    }
}

// @req SWR-GUI-006 - after 20 reads the sequence wraps back to the first entry
TEST_F(SimSequenceTest, SequenceCyclesAfterTwentyReads)
{
    // Capture the first reading of cycle 1
    VitalSigns first_of_cycle1{};
    hw_get_next_reading(&first_of_cycle1);

    // Read the remaining 19 entries to exhaust cycle 1
    for (int i = 1; i < SCENARIO_LEN; ++i)
    {
        VitalSigns tmp{};
        hw_get_next_reading(&tmp);
    }

    // The very next read should wrap and match the first entry
    VitalSigns first_of_cycle2{};
    hw_get_next_reading(&first_of_cycle2);

    EXPECT_EQ(first_of_cycle1.heart_rate,   first_of_cycle2.heart_rate);
    EXPECT_EQ(first_of_cycle1.systolic_bp,  first_of_cycle2.systolic_bp);
    EXPECT_EQ(first_of_cycle1.diastolic_bp, first_of_cycle2.diastolic_bp);
    EXPECT_FLOAT_EQ(first_of_cycle1.temperature, first_of_cycle2.temperature);
    EXPECT_EQ(first_of_cycle1.spo2,         first_of_cycle2.spo2);
}

// @req SWR-GUI-006 - hw_init resets the internal index so the next read
//   returns the first scenario entry again
TEST_F(SimSequenceTest, HwInitResetsIndex)
{
    // Read the very first entry
    VitalSigns first{};
    hw_get_next_reading(&first);

    // Advance a few steps into the sequence
    for (int i = 0; i < 5; ++i)
    {
        VitalSigns tmp{};
        hw_get_next_reading(&tmp);
    }

    // Re-initialise and the first entry must be returned again
    hw_init();
    VitalSigns after_reinit{};
    hw_get_next_reading(&after_reinit);

    EXPECT_EQ(first.heart_rate,   after_reinit.heart_rate);
    EXPECT_EQ(first.systolic_bp,  after_reinit.systolic_bp);
    EXPECT_EQ(first.diastolic_bp, after_reinit.diastolic_bp);
    EXPECT_FLOAT_EQ(first.temperature, after_reinit.temperature);
    EXPECT_EQ(first.spo2,         after_reinit.spo2);
}

// @req SWR-GUI-006 - multiple consecutive hw_init calls must not corrupt state
TEST_F(SimSequenceTest, MultipleInitCallsAreSafe)
{
    hw_init();
    hw_init();
    hw_init();

    // After repeated inits the first reading must still be physiologically valid
    VitalSigns v{};
    hw_get_next_reading(&v);
    assert_vitals_valid(v);
}

// @req SWR-GUI-006 - the full deterioration cycle must contain at least one
//   CRITICAL reading (HR > 120 OR temperature > 39.0 C)
TEST_F(SimSequenceTest, CycleContainsAtLeastOneCriticalReading)
{
    auto readings = read_full_cycle();

    bool found_critical = false;
    for (const auto &v : readings)
    {
        if (v.heart_rate > 120 || v.temperature > 39.0f)
        {
            found_critical = true;
            break;
        }
    }

    EXPECT_TRUE(found_critical)
        << "No CRITICAL reading (HR > 120 or Temp > 39.0 C) found in the "
           "20-entry scenario table";
}

// @req SWR-GUI-006 - the full cycle must contain at least one NORMAL reading
//   (HR in [60, 100] AND temperature < 37.5 C)
TEST_F(SimSequenceTest, CycleContainsAtLeastOneNormalReading)
{
    auto readings = read_full_cycle();

    bool found_normal = false;
    for (const auto &v : readings)
    {
        if (v.heart_rate >= 60 && v.heart_rate <= 100 && v.temperature < 37.5f)
        {
            found_normal = true;
            break;
        }
    }

    EXPECT_TRUE(found_normal)
        << "No NORMAL reading (HR 60-100 and Temp < 37.5 C) found in the "
           "20-entry scenario table";
}

// @req SWR-GUI-006 - every reading in two consecutive full cycles is valid
TEST_F(SimSequenceTest, TwoFullCyclesAllReadingsValid)
{
    for (int cycle = 0; cycle < 2; ++cycle)
    {
        for (int i = 0; i < SCENARIO_LEN; ++i)
        {
            VitalSigns v{};
            hw_get_next_reading(&v);
            SCOPED_TRACE("cycle " + std::to_string(cycle) +
                         " reading " + std::to_string(i));
            assert_vitals_valid(v);
        }
    }
}

// @req SWR-GUI-006 - second cycle must be byte-for-byte identical to first
TEST_F(SimSequenceTest, SecondCycleMatchesFirstCycle)
{
    auto cycle1 = read_full_cycle();
    auto cycle2 = read_full_cycle();

    for (int i = 0; i < SCENARIO_LEN; ++i)
    {
        SCOPED_TRACE("reading index " + std::to_string(i));
        EXPECT_EQ(cycle1[i].heart_rate,   cycle2[i].heart_rate);
        EXPECT_EQ(cycle1[i].systolic_bp,  cycle2[i].systolic_bp);
        EXPECT_EQ(cycle1[i].diastolic_bp, cycle2[i].diastolic_bp);
        EXPECT_FLOAT_EQ(cycle1[i].temperature, cycle2[i].temperature);
        EXPECT_EQ(cycle1[i].spo2,         cycle2[i].spo2);
    }
}
