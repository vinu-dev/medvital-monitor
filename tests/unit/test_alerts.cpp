// =============================================================
// TEST SUITE : Alerts Unit Tests
// REQUIREMENT: REQ-ALT-001 to REQ-ALT-004
// STANDARD   : IEC 62304 §5.5 / FDA SW Validation Guidance
// =============================================================

extern "C" {
#include "alerts.h"
}
#include <gtest/gtest.h>
#include <cstring>

// Helper — all-normal vitals
static VitalSigns normal_vitals() {
    VitalSigns v;
    v.heart_rate   = 80;
    v.systolic_bp  = 120;
    v.diastolic_bp = 80;
    v.temperature  = 36.6f;
    v.spo2         = 98;
    return v;
}

// =============================================================
// REQ-ALT-001  No alerts for fully normal vitals
// =============================================================

TEST(GenerateAlerts, REQ_ALT_001_AllNormal_ZeroAlerts) {
    VitalSigns v = normal_vitals();
    Alert out[MAX_ALERTS];
    EXPECT_EQ(generate_alerts(&v, out, MAX_ALERTS), 0);
}

// =============================================================
// REQ-ALT-002  One alert per abnormal parameter
// =============================================================

TEST(GenerateAlerts, REQ_ALT_002_HeartRate_Warning) {
    VitalSigns v = normal_vitals();
    v.heart_rate = 110;
    Alert out[MAX_ALERTS];
    int n = generate_alerts(&v, out, MAX_ALERTS);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].level, ALERT_WARNING);
    EXPECT_STREQ(out[0].parameter, "Heart Rate");
}

TEST(GenerateAlerts, REQ_ALT_002_SpO2_Critical) {
    VitalSigns v = normal_vitals();
    v.spo2 = 85;
    Alert out[MAX_ALERTS];
    int n = generate_alerts(&v, out, MAX_ALERTS);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].level, ALERT_CRITICAL);
    EXPECT_STREQ(out[0].parameter, "SpO2");
}

TEST(GenerateAlerts, REQ_ALT_002_Temperature_Warning) {
    VitalSigns v = normal_vitals();
    v.temperature = 38.5f;
    Alert out[MAX_ALERTS];
    int n = generate_alerts(&v, out, MAX_ALERTS);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].level, ALERT_WARNING);
    EXPECT_STREQ(out[0].parameter, "Temperature");
}

TEST(GenerateAlerts, REQ_ALT_002_BloodPressure_Warning) {
    VitalSigns v = normal_vitals();
    v.systolic_bp = 150;
    Alert out[MAX_ALERTS];
    int n = generate_alerts(&v, out, MAX_ALERTS);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].level, ALERT_WARNING);
    EXPECT_STREQ(out[0].parameter, "Blood Pressure");
}

// =============================================================
// REQ-ALT-003  Multiple alerts when multiple params abnormal
// =============================================================

TEST(GenerateAlerts, REQ_ALT_003_TwoParams_TwoAlerts) {
    VitalSigns v = normal_vitals();
    v.heart_rate = 160;   // critical
    v.spo2       = 92;    // warning
    Alert out[MAX_ALERTS];
    int n = generate_alerts(&v, out, MAX_ALERTS);
    EXPECT_EQ(n, 2);
}

TEST(GenerateAlerts, REQ_ALT_003_AllParams_FourAlerts) {
    VitalSigns v = {35, 60, 35, 39.8f, 88}; // all abnormal
    Alert out[MAX_ALERTS];
    int n = generate_alerts(&v, out, MAX_ALERTS);
    EXPECT_EQ(n, 4);
}

// =============================================================
// REQ-ALT-004  max_out cap — never writes beyond buffer size
// =============================================================

TEST(GenerateAlerts, REQ_ALT_004_MaxOut_Cap_Zero) {
    VitalSigns v = {35, 60, 35, 39.8f, 88}; // all abnormal
    Alert out[MAX_ALERTS] = {};
    int n = generate_alerts(&v, out, 0);     // cap = 0
    EXPECT_EQ(n, 0);
    // Verify no bytes were written (first alert untouched)
    EXPECT_EQ(out[0].level, ALERT_NORMAL);
}

TEST(GenerateAlerts, REQ_ALT_004_MaxOut_Cap_Two) {
    VitalSigns v = {35, 60, 35, 39.8f, 88}; // 4 abnormal params
    Alert out[MAX_ALERTS] = {};
    int n = generate_alerts(&v, out, 2);     // only 2 slots
    EXPECT_EQ(n, 2);
    // Slot 3 and 4 must be untouched
    EXPECT_EQ(out[2].level, ALERT_NORMAL);
    EXPECT_EQ(out[3].level, ALERT_NORMAL);
}

// =============================================================
// REQ-ALT-005  Alert messages are non-empty and null-terminated
// =============================================================

TEST(GenerateAlerts, REQ_ALT_005_MessageNonEmpty) {
    VitalSigns v = normal_vitals();
    v.spo2 = 85;
    Alert out[MAX_ALERTS];
    generate_alerts(&v, out, MAX_ALERTS);
    size_t len = strlen(out[0].message);
    EXPECT_GT(len, 0u);
    // snprintf null-terminates at the end of the written string
    EXPECT_EQ(out[0].message[len], '\0');
}

TEST(GenerateAlerts, REQ_ALT_005_ParameterNonEmpty) {
    VitalSigns v = normal_vitals();
    v.heart_rate = 180;
    Alert out[MAX_ALERTS];
    generate_alerts(&v, out, MAX_ALERTS);
    EXPECT_GT(strlen(out[0].parameter), 0u);
}
