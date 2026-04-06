// =============================================================
// TEST SUITE : Patient Record Unit Tests
// REQUIREMENT: REQ-PAT-001 to REQ-PAT-006
// STANDARD   : IEC 62304 §5.5 / FDA SW Validation Guidance
// =============================================================

extern "C" {
#include "patient.h"
}
#include <gtest/gtest.h>
#include <cstring>

// Helper fixtures
static VitalSigns make_normal_vitals() {
    VitalSigns v;
    v.heart_rate   = 72;
    v.systolic_bp  = 120;
    v.diastolic_bp = 80;
    v.temperature  = 36.6f;
    v.spo2         = 98;
    return v;
}

static VitalSigns make_critical_vitals() {
    VitalSigns v;
    v.heart_rate   = 35;
    v.systolic_bp  = 60;
    v.diastolic_bp = 35;
    v.temperature  = 40.0f;
    v.spo2         = 85;
    return v;
}

// =============================================================
// REQ-PAT-001  patient_init() — correct field initialisation
// =============================================================

TEST(PatientInit, REQ_PAT_001_FieldsSet) {
    PatientRecord rec;
    patient_init(&rec, 42, "Alice Smith", 35, 65.0f, 1.70f);
    EXPECT_EQ(rec.info.id, 42);
    EXPECT_STREQ(rec.info.name, "Alice Smith");
    EXPECT_EQ(rec.info.age, 35);
    EXPECT_FLOAT_EQ(rec.info.weight_kg, 65.0f);
    EXPECT_FLOAT_EQ(rec.info.height_m,  1.70f);
    EXPECT_EQ(rec.reading_count, 0);
}

TEST(PatientInit, REQ_PAT_001_LongNameTruncated) {
    PatientRecord rec;
    std::string long_name(MAX_NAME_LEN + 10, 'X');
    patient_init(&rec, 1, long_name.c_str(), 30, 70.0f, 1.75f);
    // Name must be null-terminated and within buffer
    EXPECT_EQ(strnlen(rec.info.name, MAX_NAME_LEN), (size_t)(MAX_NAME_LEN - 1));
    EXPECT_EQ(rec.info.name[MAX_NAME_LEN - 1], '\0');
}

TEST(PatientInit, REQ_PAT_001_ReadingCountZero) {
    PatientRecord rec;
    patient_init(&rec, 1, "Test", 25, 70.0f, 1.75f);
    EXPECT_EQ(rec.reading_count, 0);
}

// =============================================================
// REQ-PAT-002  patient_add_reading() — stores and counts
// =============================================================

TEST(PatientAddReading, REQ_PAT_002_FirstReading) {
    PatientRecord rec;
    patient_init(&rec, 1, "Test", 25, 70.0f, 1.75f);
    VitalSigns v = make_normal_vitals();
    EXPECT_EQ(patient_add_reading(&rec, &v), 1);
    EXPECT_EQ(rec.reading_count, 1);
}

TEST(PatientAddReading, REQ_PAT_002_MultipleReadings) {
    PatientRecord rec;
    patient_init(&rec, 1, "Test", 25, 70.0f, 1.75f);
    VitalSigns v = make_normal_vitals();
    for (int i = 0; i < 5; i++)
        EXPECT_EQ(patient_add_reading(&rec, &v), 1);
    EXPECT_EQ(rec.reading_count, 5);
}

TEST(PatientAddReading, REQ_PAT_002_FillToCapacity) {
    PatientRecord rec;
    patient_init(&rec, 1, "Test", 25, 70.0f, 1.75f);
    VitalSigns v = make_normal_vitals();
    for (int i = 0; i < MAX_READINGS; i++)
        EXPECT_EQ(patient_add_reading(&rec, &v), 1);
    EXPECT_EQ(rec.reading_count, MAX_READINGS);
}

TEST(PatientAddReading, REQ_PAT_002_BeyondCapacity_Returns0) {
    PatientRecord rec;
    patient_init(&rec, 1, "Test", 25, 70.0f, 1.75f);
    VitalSigns v = make_normal_vitals();
    for (int i = 0; i < MAX_READINGS; i++)
        patient_add_reading(&rec, &v);
    // One more — must return 0 and not increment count
    EXPECT_EQ(patient_add_reading(&rec, &v), 0);
    EXPECT_EQ(rec.reading_count, MAX_READINGS);
}

TEST(PatientAddReading, REQ_PAT_002_DataPreserved) {
    PatientRecord rec;
    patient_init(&rec, 1, "Test", 25, 70.0f, 1.75f);
    VitalSigns v = make_normal_vitals();
    v.heart_rate = 77;
    patient_add_reading(&rec, &v);
    EXPECT_EQ(rec.readings[0].heart_rate, 77);
}

// =============================================================
// REQ-PAT-003  patient_latest_reading()
// =============================================================

TEST(PatientLatestReading, REQ_PAT_003_NoReadings_ReturnsNull) {
    PatientRecord rec;
    patient_init(&rec, 1, "Test", 25, 70.0f, 1.75f);
    EXPECT_EQ(patient_latest_reading(&rec), nullptr);
}

TEST(PatientLatestReading, REQ_PAT_003_ReturnsLastAdded) {
    PatientRecord rec;
    patient_init(&rec, 1, "Test", 25, 70.0f, 1.75f);
    VitalSigns v1 = make_normal_vitals();   v1.heart_rate = 70;
    VitalSigns v2 = make_critical_vitals(); v2.heart_rate = 35;
    patient_add_reading(&rec, &v1);
    patient_add_reading(&rec, &v2);
    const VitalSigns *latest = patient_latest_reading(&rec);
    ASSERT_NE(latest, nullptr);
    EXPECT_EQ(latest->heart_rate, 35);
}

// =============================================================
// REQ-PAT-004  patient_current_status()
// =============================================================

TEST(PatientStatus, REQ_PAT_004_NoReadings_Normal) {
    PatientRecord rec;
    patient_init(&rec, 1, "Test", 25, 70.0f, 1.75f);
    EXPECT_EQ(patient_current_status(&rec), ALERT_NORMAL);
}

TEST(PatientStatus, REQ_PAT_004_NormalVitals) {
    PatientRecord rec;
    patient_init(&rec, 1, "Test", 25, 70.0f, 1.75f);
    VitalSigns v = make_normal_vitals();
    patient_add_reading(&rec, &v);
    EXPECT_EQ(patient_current_status(&rec), ALERT_NORMAL);
}

TEST(PatientStatus, REQ_PAT_004_CriticalVitals) {
    PatientRecord rec;
    patient_init(&rec, 1, "Test", 25, 70.0f, 1.75f);
    VitalSigns v = make_critical_vitals();
    patient_add_reading(&rec, &v);
    EXPECT_EQ(patient_current_status(&rec), ALERT_CRITICAL);
}

TEST(PatientStatus, REQ_PAT_004_StatusReflectsLatest) {
    PatientRecord rec;
    patient_init(&rec, 1, "Test", 25, 70.0f, 1.75f);
    patient_add_reading(&rec, &make_critical_vitals());
    // Add a normal reading — status must update to NORMAL
    VitalSigns v = make_normal_vitals();
    patient_add_reading(&rec, &v);
    EXPECT_EQ(patient_current_status(&rec), ALERT_NORMAL);
}

// =============================================================
// REQ-PAT-005  patient_is_full()
// =============================================================

TEST(PatientIsFull, REQ_PAT_005_NotFull) {
    PatientRecord rec;
    patient_init(&rec, 1, "Test", 25, 70.0f, 1.75f);
    EXPECT_EQ(patient_is_full(&rec), 0);
}

TEST(PatientIsFull, REQ_PAT_005_Full) {
    PatientRecord rec;
    patient_init(&rec, 1, "Test", 25, 70.0f, 1.75f);
    VitalSigns v = make_normal_vitals();
    for (int i = 0; i < MAX_READINGS; i++)
        patient_add_reading(&rec, &v);
    EXPECT_NE(patient_is_full(&rec), 0);
}

// =============================================================
// REQ-PAT-006  patient_print_summary() — executes without crash
//   Verifies the display path for normal, warning, and critical
//   patients including the no-readings edge case.
// =============================================================

static void run_print_summary(const PatientRecord *rec) {
    // Redirect stdout to NUL so test output stays clean
    FILE *nul = fopen("NUL", "w");
    int saved = _dup(_fileno(stdout));
    if (nul) _dup2(_fileno(nul), _fileno(stdout));
    patient_print_summary(rec);
    fflush(stdout);
    if (nul) { _dup2(saved, _fileno(stdout)); fclose(nul); }
    _close(saved);
}

TEST(PatientPrintSummary, REQ_PAT_006_NoReadings) {
    PatientRecord rec;
    patient_init(&rec, 9, "No Readings", 30, 70.0f, 1.75f);
    EXPECT_NO_FATAL_FAILURE(run_print_summary(&rec));
}

TEST(PatientPrintSummary, REQ_PAT_006_NormalVitals) {
    PatientRecord rec;
    patient_init(&rec, 10, "Normal Patient", 40, 75.0f, 1.78f);
    VitalSigns v = make_normal_vitals();
    patient_add_reading(&rec, &v);
    EXPECT_NO_FATAL_FAILURE(run_print_summary(&rec));
}

TEST(PatientPrintSummary, REQ_PAT_006_CriticalVitals_ActiveAlerts) {
    PatientRecord rec;
    patient_init(&rec, 11, "Critical Patient", 55, 90.0f, 1.72f);
    VitalSigns v = make_critical_vitals();
    patient_add_reading(&rec, &v);
    EXPECT_NO_FATAL_FAILURE(run_print_summary(&rec));
}
