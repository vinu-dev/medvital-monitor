// =============================================================
// TEST SUITE : Alert Escalation (Integration)
// REQUIREMENT: REQ-INT-ESC-001 to REQ-INT-ESC-005
//
// Validates that the alert pipeline responds correctly to
// clinical alarm scenarios: single-parameter escalation,
// multi-parameter events, boundary transitions, and that
// deescalation (improvement) is detected reliably.
// STANDARD   : IEC 62304 §5.7 / FDA SW Validation Guidance
// =============================================================

extern "C" {
#include "patient.h"
}
#include <gtest/gtest.h>

// =============================================================
// REQ-INT-ESC-001  Single parameter crosses critical threshold
//   SpO2 drop from normal -> warning -> critical step by step
// =============================================================

TEST(AlertEscalation, REQ_INT_ESC_001_SpO2_StepwiseEscalation) {
    PatientRecord rec;
    patient_init(&rec, 3001, "SpO2 Test", 45, 70.0f, 1.75f);

    auto add_spo2 = [&](int spo2) {
        VitalSigns v = {72, 120, 80, 36.6f, spo2};
        patient_add_reading(&rec, &v);
        return patient_current_status(&rec);
    };

    EXPECT_EQ(add_spo2(98), ALERT_NORMAL);   // baseline
    EXPECT_EQ(add_spo2(94), ALERT_WARNING);  // dropped to warning
    EXPECT_EQ(add_spo2(88), ALERT_CRITICAL); // dropped to critical
}

// =============================================================
// REQ-INT-ESC-002  Heart rate escalation — bradycardia
// =============================================================

TEST(AlertEscalation, REQ_INT_ESC_002_Bradycardia_Escalation) {
    PatientRecord rec;
    patient_init(&rec, 3002, "Bradycardia Test", 60, 75.0f, 1.78f);

    VitalSigns v60  = {60, 120, 80, 36.6f, 98};  // normal lower boundary
    VitalSigns v50  = {50, 120, 80, 36.6f, 98};  // warning
    VitalSigns v38  = {38, 120, 80, 36.6f, 98};  // critical

    patient_add_reading(&rec, &v60);
    EXPECT_EQ(patient_current_status(&rec), ALERT_NORMAL);

    patient_add_reading(&rec, &v50);
    EXPECT_EQ(patient_current_status(&rec), ALERT_WARNING);

    patient_add_reading(&rec, &v38);
    EXPECT_EQ(patient_current_status(&rec), ALERT_CRITICAL);

    // Confirm a critical HR alert is generated
    Alert alerts[MAX_ALERTS];
    int n = generate_alerts(&v38, alerts, MAX_ALERTS);
    ASSERT_GE(n, 1);
    EXPECT_EQ(alerts[0].level, ALERT_CRITICAL);
    EXPECT_STREQ(alerts[0].parameter, "Heart Rate");
}

// =============================================================
// REQ-INT-ESC-003  Multi-parameter crisis
//   All four parameters simultaneously critical
// =============================================================

TEST(AlertEscalation, REQ_INT_ESC_003_AllParameters_Critical) {
    PatientRecord rec;
    patient_init(&rec, 3003, "Multi-Crisis Test", 70, 80.0f, 1.72f);

    VitalSigns crisis = {30, 55, 30, 41.0f, 80};
    patient_add_reading(&rec, &crisis);

    EXPECT_EQ(patient_current_status(&rec), ALERT_CRITICAL);

    Alert alerts[MAX_ALERTS];
    int n = generate_alerts(&crisis, alerts, MAX_ALERTS);
    EXPECT_EQ(n, 4); // one per parameter

    for (int i = 0; i < n; i++) {
        EXPECT_EQ(alerts[i].level, ALERT_CRITICAL)
            << "Expected CRITICAL for param: " << alerts[i].parameter;
    }
}

// =============================================================
// REQ-INT-ESC-004  Deescalation — improvement is detected
//   A patient who recovers must reflect lower alert levels
// =============================================================

TEST(AlertEscalation, REQ_INT_ESC_004_Deescalation_CriticalToNormal) {
    PatientRecord rec;
    patient_init(&rec, 3004, "Recovery Test", 48, 68.0f, 1.70f);

    // Crisis reading
    VitalSigns crisis = {30, 55, 30, 41.0f, 80};
    patient_add_reading(&rec, &crisis);
    EXPECT_EQ(patient_current_status(&rec), ALERT_CRITICAL);

    // Stable reading
    VitalSigns stable = {72, 118, 78, 36.7f, 97};
    patient_add_reading(&rec, &stable);
    EXPECT_EQ(patient_current_status(&rec), ALERT_NORMAL);

    // Confirm zero alerts on stable reading
    Alert alerts[MAX_ALERTS];
    EXPECT_EQ(generate_alerts(&stable, alerts, MAX_ALERTS), 0);
}

// =============================================================
// REQ-INT-ESC-005  Boundary transition accuracy
//   Tests exact boundary values where normal/warning/critical
//   transitions occur to prevent off-by-one classification errors
// =============================================================

TEST(AlertEscalation, REQ_INT_ESC_005_BoundaryTransitions_HeartRate) {
    PatientRecord rec;
    patient_init(&rec, 3005, "Boundary Test", 40, 70.0f, 1.75f);

    // Threshold: < 40 -> CRITICAL, so 40 is WARNING; > 150 -> CRITICAL, so 150 is WARNING
    struct { int hr; AlertLevel expected; } cases[] = {
        {39,  ALERT_CRITICAL},  // one below critical threshold
        {40,  ALERT_WARNING},   // critical lower boundary — still warning
        {41,  ALERT_WARNING},   // warning zone
        {59,  ALERT_WARNING},   // top of warning zone
        {60,  ALERT_NORMAL},    // normal lower boundary
        {100, ALERT_NORMAL},    // normal upper boundary
        {101, ALERT_WARNING},   // just above normal
        {150, ALERT_WARNING},   // critical upper boundary — still warning
        {151, ALERT_CRITICAL},  // one above critical threshold
    };

    for (auto &c : cases) {
        VitalSigns v = {c.hr, 120, 80, 36.6f, 98};
        patient_add_reading(&rec, &v);
        EXPECT_EQ(patient_current_status(&rec), c.expected)
            << "Heart rate " << c.hr << " bpm should be "
            << alert_level_str(c.expected);
    }
}

TEST(AlertEscalation, REQ_INT_ESC_005_BoundaryTransitions_SpO2) {
    PatientRecord rec;
    patient_init(&rec, 3006, "SpO2 Boundary Test", 40, 70.0f, 1.75f);

    struct { int spo2; AlertLevel expected; } cases[] = {
        {89, ALERT_CRITICAL},
        {90, ALERT_WARNING},
        {94, ALERT_WARNING},
        {95, ALERT_NORMAL},
        {100, ALERT_NORMAL},
    };

    for (auto &c : cases) {
        VitalSigns v = {72, 120, 80, 36.6f, c.spo2};
        patient_add_reading(&rec, &v);
        EXPECT_EQ(patient_current_status(&rec), c.expected)
            << "SpO2 " << c.spo2 << "% should be "
            << alert_level_str(c.expected);
    }
}
