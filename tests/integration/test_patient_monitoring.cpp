// =============================================================
// TEST SUITE : Patient Monitoring (Integration)
// REQUIREMENT: REQ-INT-MON-001 to REQ-INT-MON-005
//
// Validates end-to-end patient admission and monitoring
// workflows combining vitals, alerts, and patient record
// modules as they are exercised in real clinical sequences.
// STANDARD   : IEC 62304 §5.7 / FDA SW Validation Guidance
// =============================================================

extern "C" {
#include "patient.h"
}
#include <gtest/gtest.h>

// =============================================================
// REQ-INT-MON-001  Full admission workflow
//   Admit a patient, record vitals, verify status and BMI
// =============================================================

TEST(PatientMonitoring, REQ_INT_MON_001_AdmissionWorkflow) {
    PatientRecord rec;
    patient_init(&rec, 1001, "Sarah Johnson", 52, 72.5f, 1.66f);

    EXPECT_EQ(rec.reading_count, 0);
    EXPECT_EQ(patient_current_status(&rec), ALERT_NORMAL); // no readings yet

    VitalSigns v = {78, 122, 82, 36.7f, 98};
    EXPECT_EQ(patient_add_reading(&rec, &v), 1);
    EXPECT_EQ(rec.reading_count, 1);
    EXPECT_EQ(patient_current_status(&rec), ALERT_NORMAL);

    float bmi = calculate_bmi(rec.info.weight_kg, rec.info.height_m);
    EXPECT_NEAR(bmi, 26.3f, 0.1f); // overweight range
    EXPECT_STREQ(bmi_category(bmi), "Overweight");
}

// =============================================================
// REQ-INT-MON-002  Deterioration trajectory
//   Normal -> Warning -> Critical across sequential readings
// =============================================================

TEST(PatientMonitoring, REQ_INT_MON_002_DeteriorationTrajectory) {
    PatientRecord rec;
    patient_init(&rec, 1002, "Test Patient", 45, 80.0f, 1.75f);

    // Reading 1: normal
    VitalSigns v1 = {78, 120, 80, 36.6f, 98};
    patient_add_reading(&rec, &v1);
    EXPECT_EQ(patient_current_status(&rec), ALERT_NORMAL);

    // Reading 2: warning
    VitalSigns v2 = {108, 148, 94, 37.9f, 93};
    patient_add_reading(&rec, &v2);
    EXPECT_EQ(patient_current_status(&rec), ALERT_WARNING);

    // Reading 3: critical
    VitalSigns v3 = {35, 60, 35, 40.1f, 85};
    patient_add_reading(&rec, &v3);
    EXPECT_EQ(patient_current_status(&rec), ALERT_CRITICAL);

    EXPECT_EQ(rec.reading_count, 3);
}

// =============================================================
// REQ-INT-MON-003  Recovery trajectory
//   Critical -> Warning -> Normal (patient improves)
// =============================================================

TEST(PatientMonitoring, REQ_INT_MON_003_RecoveryTrajectory) {
    PatientRecord rec;
    patient_init(&rec, 1003, "Recovering Patient", 38, 75.0f, 1.78f);

    VitalSigns critical  = {35,  60,  35, 40.1f, 84};
    VitalSigns warning   = {108, 148, 94, 38.5f, 92};
    VitalSigns normal    = {72,  118, 78, 36.8f, 97};

    patient_add_reading(&rec, &critical);
    EXPECT_EQ(patient_current_status(&rec), ALERT_CRITICAL);

    patient_add_reading(&rec, &warning);
    EXPECT_EQ(patient_current_status(&rec), ALERT_WARNING);

    patient_add_reading(&rec, &normal);
    EXPECT_EQ(patient_current_status(&rec), ALERT_NORMAL);
}

// =============================================================
// REQ-INT-MON-004  Alert generation matches patient status
//   When vitals trigger alerts, generate_alerts must produce
//   records consistent with patient_current_status
// =============================================================

TEST(PatientMonitoring, REQ_INT_MON_004_AlertsConsistentWithStatus) {
    PatientRecord rec;
    patient_init(&rec, 1004, "Consistency Test", 50, 70.0f, 1.72f);

    VitalSigns v = {160, 185, 115, 40.2f, 87};
    patient_add_reading(&rec, &v);

    AlertLevel status = patient_current_status(&rec);
    EXPECT_EQ(status, ALERT_CRITICAL);

    Alert alerts[MAX_ALERTS];
    const VitalSigns *latest = patient_latest_reading(&rec);
    int n = generate_alerts(latest, alerts, MAX_ALERTS);
    EXPECT_GT(n, 0);

    // At least one alert must be critical to match patient status
    bool has_critical = false;
    for (int i = 0; i < n; i++) {
        if (alerts[i].level == ALERT_CRITICAL) { has_critical = true; break; }
    }
    EXPECT_TRUE(has_critical);
}

// =============================================================
// REQ-INT-MON-005  Buffer capacity enforcement in workflow
//   Adding MAX_READINGS+1 readings must not corrupt the record
// =============================================================

TEST(PatientMonitoring, REQ_INT_MON_005_ReadingBufferFull) {
    PatientRecord rec;
    patient_init(&rec, 1005, "Buffer Test", 30, 68.0f, 1.70f);

    VitalSigns v = {72, 120, 80, 36.6f, 98};
    for (int i = 0; i < MAX_READINGS; i++)
        EXPECT_EQ(patient_add_reading(&rec, &v), 1);

    EXPECT_NE(patient_is_full(&rec), 0);

    // Attempt to add beyond capacity
    VitalSigns extra = {35, 60, 35, 40.0f, 84};
    EXPECT_EQ(patient_add_reading(&rec, &extra), 0);
    EXPECT_EQ(rec.reading_count, MAX_READINGS);

    // Latest reading must still be the last successful one (normal)
    const VitalSigns *latest = patient_latest_reading(&rec);
    ASSERT_NE(latest, nullptr);
    EXPECT_EQ(latest->heart_rate, 72);
}

// =============================================================
// REQ-INT-MON-006  Two independent patients don't share state
// =============================================================

TEST(PatientMonitoring, REQ_INT_MON_006_TwoPatientsIndependent) {
    PatientRecord recA, recB;
    patient_init(&recA, 2001, "Patient A", 40, 70.0f, 1.75f);
    patient_init(&recB, 2002, "Patient B", 55, 90.0f, 1.80f);

    VitalSigns critical = {35, 60, 35, 40.1f, 84};
    VitalSigns normal   = {72, 120, 80, 36.6f, 98};

    patient_add_reading(&recA, &critical);
    patient_add_reading(&recB, &normal);

    EXPECT_EQ(patient_current_status(&recA), ALERT_CRITICAL);
    EXPECT_EQ(patient_current_status(&recB), ALERT_NORMAL);
    EXPECT_EQ(recA.reading_count, 1);
    EXPECT_EQ(recB.reading_count, 1);
}
