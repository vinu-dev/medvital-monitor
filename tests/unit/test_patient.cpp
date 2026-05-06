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
#include <string>

// Helper fixtures
static VitalSigns make_normal_vitals() {
    VitalSigns v;
    v.heart_rate      = 72;
    v.systolic_bp     = 120;
    v.diastolic_bp    = 80;
    v.temperature     = 36.6f;
    v.spo2            = 98;
    v.respiration_rate = 0; /* 0 = not measured; skipped by overall_alert_level() */
    return v;
}

static VitalSigns make_critical_vitals() {
    VitalSigns v;
    v.heart_rate      = 35;
    v.systolic_bp     = 60;
    v.diastolic_bp    = 35;
    v.temperature     = 40.0f;
    v.spo2            = 85;
    v.respiration_rate = 0; /* 0 = not measured */
    return v;
}

static VitalSigns make_warning_hr_vitals() {
    VitalSigns v = make_normal_vitals();
    v.heart_rate = 108;
    return v;
}

static VitalSigns make_warning_spo2_vitals() {
    VitalSigns v = make_normal_vitals();
    v.spo2 = 93;
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
    VitalSigns crit = make_critical_vitals();
    patient_add_reading(&rec, &crit);
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
// REQ-PAT-007 / REQ-PAT-008  session alert-event review log
// =============================================================

TEST(PatientAlertEvents, REQ_PAT_007_FirstNormalReading_NoEvent) {
    PatientRecord rec;
    patient_init(&rec, 1, "Test", 25, 70.0f, 1.75f);

    VitalSigns normal = make_normal_vitals();
    ASSERT_EQ(patient_add_reading(&rec, &normal), 1);
    EXPECT_EQ(patient_alert_event_count(&rec), 0);
}

TEST(PatientAlertEvents, REQ_PAT_007_FirstAbnormalReading_CreatesEvent) {
    PatientRecord rec;
    patient_init(&rec, 1, "Test", 25, 70.0f, 1.75f);

    VitalSigns warning = make_warning_hr_vitals();
    ASSERT_EQ(patient_add_reading(&rec, &warning), 1);

    ASSERT_EQ(patient_alert_event_count(&rec), 1);
    const AlertEvent *event = patient_alert_event_at(&rec, 0);
    ASSERT_NE(event, nullptr);
    EXPECT_EQ(event->reading_index, 1);
    EXPECT_EQ(event->level, ALERT_WARNING);
    EXPECT_NE(std::string(event->summary).find("Heart Rate"), std::string::npos);
}

TEST(PatientAlertEvents, REQ_PAT_007_RepeatedSignature_NoDuplicateEvent) {
    PatientRecord rec;
    patient_init(&rec, 1, "Test", 25, 70.0f, 1.75f);

    VitalSigns warning1 = make_warning_hr_vitals();
    VitalSigns warning2 = make_warning_hr_vitals();
    warning2.heart_rate = 112;

    ASSERT_EQ(patient_add_reading(&rec, &warning1), 1);
    ASSERT_EQ(patient_add_reading(&rec, &warning2), 1);
    EXPECT_EQ(patient_alert_event_count(&rec), 1);
}

TEST(PatientAlertEvents, REQ_PAT_007_SeverityEscalation_CreatesNewEvent) {
    PatientRecord rec;
    patient_init(&rec, 1, "Test", 25, 70.0f, 1.75f);

    VitalSigns warning = make_warning_hr_vitals();
    VitalSigns critical = make_critical_vitals();

    ASSERT_EQ(patient_add_reading(&rec, &warning), 1);
    ASSERT_EQ(patient_add_reading(&rec, &critical), 1);

    ASSERT_EQ(patient_alert_event_count(&rec), 2);
    const AlertEvent *event = patient_alert_event_at(&rec, 1);
    ASSERT_NE(event, nullptr);
    EXPECT_EQ(event->reading_index, 2);
    EXPECT_EQ(event->level, ALERT_CRITICAL);
}

TEST(PatientAlertEvents, REQ_PAT_007_ParameterSetChangeSameSeverity_CreatesNewEvent) {
    PatientRecord rec;
    patient_init(&rec, 1, "Test", 25, 70.0f, 1.75f);

    VitalSigns hr_warning = make_warning_hr_vitals();
    VitalSigns spo2_warning = make_warning_spo2_vitals();

    ASSERT_EQ(patient_add_reading(&rec, &hr_warning), 1);
    ASSERT_EQ(patient_add_reading(&rec, &spo2_warning), 1);

    ASSERT_EQ(patient_alert_event_count(&rec), 2);
    const AlertEvent *first = patient_alert_event_at(&rec, 0);
    const AlertEvent *second = patient_alert_event_at(&rec, 1);
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_NE(std::string(first->summary).find("Heart Rate"), std::string::npos);
    EXPECT_NE(std::string(second->summary).find("SpO2"), std::string::npos);
}

TEST(PatientAlertEvents, REQ_PAT_007_RecoveryToNormal_CreatesClearEvent) {
    PatientRecord rec;
    patient_init(&rec, 1, "Test", 25, 70.0f, 1.75f);

    VitalSigns critical = make_critical_vitals();
    VitalSigns normal = make_normal_vitals();

    ASSERT_EQ(patient_add_reading(&rec, &critical), 1);
    ASSERT_EQ(patient_add_reading(&rec, &normal), 1);

    ASSERT_EQ(patient_alert_event_count(&rec), 2);
    const AlertEvent *recovery = patient_alert_event_at(&rec, 1);
    ASSERT_NE(recovery, nullptr);
    EXPECT_EQ(recovery->level, ALERT_NORMAL);
    EXPECT_EQ(recovery->abnormal_mask, 0u);
    EXPECT_NE(std::string(recovery->summary).find("Recovered to normal"), std::string::npos);
}

TEST(PatientAlertEvents, REQ_PAT_008_EventAccessAndReset) {
    PatientRecord rec;
    patient_init(&rec, 1, "Test", 25, 70.0f, 1.75f);

    VitalSigns warning = make_warning_hr_vitals();
    ASSERT_EQ(patient_add_reading(&rec, &warning), 1);

    ASSERT_EQ(patient_alert_event_count(&rec), 1);
    EXPECT_EQ(patient_alert_event_at(&rec, -1), nullptr);
    EXPECT_EQ(patient_alert_event_at(&rec, 1), nullptr);

    patient_init(&rec, 2, "Reset Patient", 40, 80.0f, 1.80f);
    EXPECT_EQ(patient_alert_event_count(&rec), 0);
    EXPECT_EQ(patient_alert_event_at(&rec, 0), nullptr);
}

TEST(PatientAlertEvents, REQ_PAT_008_SessionResetNoticeLifecycle) {
    PatientRecord rec;
    patient_init(&rec, 1, "Test", 25, 70.0f, 1.75f);

    EXPECT_EQ(patient_session_reset_notice(&rec), nullptr);

    patient_note_session_reset(&rec, MAX_READINGS);
    const char *notice = patient_session_reset_notice(&rec);
    ASSERT_NE(notice, nullptr);
    EXPECT_NE(std::string(notice).find("automatically after 10 readings"), std::string::npos);

    patient_init(&rec, 2, "Reset Patient", 40, 80.0f, 1.80f);
    EXPECT_EQ(patient_session_reset_notice(&rec), nullptr);
}

// =============================================================
// REQ-PAT-006  patient_print_summary()
//   Verifies the display path for normal, warning, critical, and
//   session-event review output including the no-readings edge case.
// =============================================================

static void run_print_summary(const PatientRecord *rec) {
    /* patient_print_summary() writes to stdout; the test verifies only
     * that the function completes without a crash or fatal assertion.
     * GTest captures and formats any output through its own runner.
     * Redirecting to NUL is unnecessary and flagged CWE-732; removed. */
    patient_print_summary(rec);
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

TEST(PatientPrintSummary, REQ_PAT_006_SessionAlarmEventsIncluded) {
    PatientRecord rec;
    patient_init(&rec, 12, "Review Patient", 47, 76.0f, 1.74f);

    VitalSigns warning = make_warning_hr_vitals();
    VitalSigns normal = make_normal_vitals();
    ASSERT_EQ(patient_add_reading(&rec, &warning), 1);
    ASSERT_EQ(patient_add_reading(&rec, &normal), 1);

    testing::internal::CaptureStdout();
    patient_print_summary(&rec);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Session Alarm Events:"), std::string::npos);
    EXPECT_NE(output.find("#1 [WARNING]"), std::string::npos);
    EXPECT_NE(output.find("#2 [NORMAL] Recovered to normal"), std::string::npos);
}

TEST(PatientPrintSummary, REQ_PAT_006_SessionResetNoticeIncluded) {
    PatientRecord rec;
    patient_init(&rec, 13, "Reset Review", 47, 76.0f, 1.74f);
    patient_note_session_reset(&rec, MAX_READINGS);

    testing::internal::CaptureStdout();
    patient_print_summary(&rec);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("NOTE: Session reset automatically after 10 readings"), std::string::npos);
    EXPECT_NE(output.find("None recorded in current session."), std::string::npos);
}
