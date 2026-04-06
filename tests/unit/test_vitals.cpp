// =============================================================
// TEST SUITE : Vitals Unit Tests
// REQUIREMENTS: REQ-VIT-001 to REQ-VIT-006
// STANDARD    : IEC 62304 §5.5 / FDA SW Validation Guidance
// TECHNIQUES  : Equivalence Partitioning, Boundary Value Analysis
// =============================================================

extern "C" {
#include "vitals.h"
}
#include <gtest/gtest.h>

// =============================================================
// REQ-VIT-001  check_heart_rate()
// =============================================================

TEST(HeartRate, REQ_VIT_001_Normal_Nominal)         { EXPECT_EQ(check_heart_rate(80),  ALERT_NORMAL);   }
TEST(HeartRate, REQ_VIT_001_Normal_LowerBound)      { EXPECT_EQ(check_heart_rate(60),  ALERT_NORMAL);   }
TEST(HeartRate, REQ_VIT_001_Normal_UpperBound)      { EXPECT_EQ(check_heart_rate(100), ALERT_NORMAL);   }

TEST(HeartRate, REQ_VIT_001_Warning_BelowNormal)    { EXPECT_EQ(check_heart_rate(59),  ALERT_WARNING);  }
TEST(HeartRate, REQ_VIT_001_Warning_AboveNormal)    { EXPECT_EQ(check_heart_rate(101), ALERT_WARNING);  }
TEST(HeartRate, REQ_VIT_001_Warning_LowerEdge)      { EXPECT_EQ(check_heart_rate(41),  ALERT_WARNING);  }
TEST(HeartRate, REQ_VIT_001_Warning_UpperEdge)      { EXPECT_EQ(check_heart_rate(149), ALERT_WARNING);  }

// threshold is < 40 and > 150, so 40 and 150 are still WARNING
TEST(HeartRate, REQ_VIT_001_Critical_TooLow)        { EXPECT_EQ(check_heart_rate(39),  ALERT_CRITICAL); }
TEST(HeartRate, REQ_VIT_001_Critical_TooLowEdge)    { EXPECT_EQ(check_heart_rate(0),   ALERT_CRITICAL); }
TEST(HeartRate, REQ_VIT_001_Critical_TooHigh)       { EXPECT_EQ(check_heart_rate(151), ALERT_CRITICAL); }
TEST(HeartRate, REQ_VIT_001_Critical_TooHighEdge)   { EXPECT_EQ(check_heart_rate(300), ALERT_CRITICAL); }
TEST(HeartRate, REQ_VIT_001_Warning_At40)           { EXPECT_EQ(check_heart_rate(40),  ALERT_WARNING);  }
TEST(HeartRate, REQ_VIT_001_Warning_At150)          { EXPECT_EQ(check_heart_rate(150), ALERT_WARNING);  }
TEST(HeartRate, REQ_VIT_001_Critical_Negative)      { EXPECT_EQ(check_heart_rate(-1),  ALERT_CRITICAL); }

// =============================================================
// REQ-VIT-002  check_blood_pressure()
// =============================================================

TEST(BloodPressure, REQ_VIT_002_Normal_Typical)
    { EXPECT_EQ(check_blood_pressure(120, 80), ALERT_NORMAL); }
TEST(BloodPressure, REQ_VIT_002_Normal_LowerBounds)
    { EXPECT_EQ(check_blood_pressure(90, 60),  ALERT_NORMAL); }
TEST(BloodPressure, REQ_VIT_002_Normal_UpperBounds)
    { EXPECT_EQ(check_blood_pressure(140, 90), ALERT_NORMAL); }

TEST(BloodPressure, REQ_VIT_002_Warning_SystolicLow)
    { EXPECT_EQ(check_blood_pressure(89, 75),  ALERT_WARNING); }
TEST(BloodPressure, REQ_VIT_002_Warning_SystolicHigh)
    { EXPECT_EQ(check_blood_pressure(141, 80), ALERT_WARNING); }
TEST(BloodPressure, REQ_VIT_002_Warning_DiastolicLow)
    { EXPECT_EQ(check_blood_pressure(110, 59), ALERT_WARNING); }
TEST(BloodPressure, REQ_VIT_002_Warning_DiastolicHigh)
    { EXPECT_EQ(check_blood_pressure(120, 91), ALERT_WARNING); }

TEST(BloodPressure, REQ_VIT_002_Critical_SystolicLow)
    { EXPECT_EQ(check_blood_pressure(69, 80),  ALERT_CRITICAL); }
TEST(BloodPressure, REQ_VIT_002_Critical_SystolicHigh)
    { EXPECT_EQ(check_blood_pressure(181, 80), ALERT_CRITICAL); }
TEST(BloodPressure, REQ_VIT_002_Critical_DiastolicLow)
    { EXPECT_EQ(check_blood_pressure(110, 39), ALERT_CRITICAL); }
TEST(BloodPressure, REQ_VIT_002_Critical_DiastolicHigh)
    { EXPECT_EQ(check_blood_pressure(120, 121),ALERT_CRITICAL); }
TEST(BloodPressure, REQ_VIT_002_Critical_Both)
    { EXPECT_EQ(check_blood_pressure(200, 130),ALERT_CRITICAL); }

// =============================================================
// REQ-VIT-003  check_temperature()
// =============================================================

TEST(Temperature, REQ_VIT_003_Normal_Typical)    { EXPECT_EQ(check_temperature(36.6f), ALERT_NORMAL);   }
TEST(Temperature, REQ_VIT_003_Normal_LowerBound) { EXPECT_EQ(check_temperature(36.1f), ALERT_NORMAL);   }
TEST(Temperature, REQ_VIT_003_Normal_UpperBound) { EXPECT_EQ(check_temperature(37.2f), ALERT_NORMAL);   }

TEST(Temperature, REQ_VIT_003_Warning_Hypothermia)
    { EXPECT_EQ(check_temperature(35.5f), ALERT_WARNING);  }
TEST(Temperature, REQ_VIT_003_Warning_LowGradeFever)
    { EXPECT_EQ(check_temperature(37.9f), ALERT_WARNING);  }
TEST(Temperature, REQ_VIT_003_Warning_JustBelowNormal)
    { EXPECT_EQ(check_temperature(36.0f), ALERT_WARNING);  }
TEST(Temperature, REQ_VIT_003_Warning_JustAboveNormal)
    { EXPECT_EQ(check_temperature(37.3f), ALERT_WARNING);  }

TEST(Temperature, REQ_VIT_003_Critical_Hypothermia)
    { EXPECT_EQ(check_temperature(34.9f), ALERT_CRITICAL); }
TEST(Temperature, REQ_VIT_003_Critical_HighFever)
    { EXPECT_EQ(check_temperature(39.6f), ALERT_CRITICAL); }
TEST(Temperature, REQ_VIT_003_Critical_Extreme)
    { EXPECT_EQ(check_temperature(42.0f), ALERT_CRITICAL); }

// =============================================================
// REQ-VIT-004  check_spo2()
// =============================================================

TEST(SpO2, REQ_VIT_004_Normal_100)       { EXPECT_EQ(check_spo2(100), ALERT_NORMAL);   }
TEST(SpO2, REQ_VIT_004_Normal_95)        { EXPECT_EQ(check_spo2(95),  ALERT_NORMAL);   }
TEST(SpO2, REQ_VIT_004_Normal_Typical)   { EXPECT_EQ(check_spo2(98),  ALERT_NORMAL);   }

TEST(SpO2, REQ_VIT_004_Warning_94)       { EXPECT_EQ(check_spo2(94),  ALERT_WARNING);  }
TEST(SpO2, REQ_VIT_004_Warning_90)       { EXPECT_EQ(check_spo2(90),  ALERT_WARNING);  }

TEST(SpO2, REQ_VIT_004_Critical_89)      { EXPECT_EQ(check_spo2(89),  ALERT_CRITICAL); }
TEST(SpO2, REQ_VIT_004_Critical_Zero)    { EXPECT_EQ(check_spo2(0),   ALERT_CRITICAL); }
TEST(SpO2, REQ_VIT_004_Critical_Neg)     { EXPECT_EQ(check_spo2(-1),  ALERT_CRITICAL); }

// =============================================================
// REQ-VIT-005  overall_alert_level() — highest level wins
// =============================================================

TEST(OverallAlert, REQ_VIT_005_AllNormal) {
    VitalSigns v = {80, 120, 80, 36.6f, 98};
    EXPECT_EQ(overall_alert_level(&v), ALERT_NORMAL);
}
TEST(OverallAlert, REQ_VIT_005_OneWarning) {
    VitalSigns v = {105, 120, 80, 36.6f, 98}; // HR warning only
    EXPECT_EQ(overall_alert_level(&v), ALERT_WARNING);
}
TEST(OverallAlert, REQ_VIT_005_OneCritical) {
    VitalSigns v = {80, 120, 80, 36.6f, 85}; // SpO2 critical only
    EXPECT_EQ(overall_alert_level(&v), ALERT_CRITICAL);
}
TEST(OverallAlert, REQ_VIT_005_CriticalOverridesWarning) {
    VitalSigns v = {105, 120, 80, 40.0f, 98}; // HR=warning, Temp=critical
    EXPECT_EQ(overall_alert_level(&v), ALERT_CRITICAL);
}
TEST(OverallAlert, REQ_VIT_005_AllCritical) {
    VitalSigns v = {30, 50, 30, 34.0f, 80};
    EXPECT_EQ(overall_alert_level(&v), ALERT_CRITICAL);
}

// =============================================================
// REQ-VIT-006  calculate_bmi() and bmi_category()
// =============================================================

TEST(BMI, REQ_VIT_006_NormalWeight)   { EXPECT_NEAR(calculate_bmi(70.0f, 1.75f), 22.86f, 0.01f); }
TEST(BMI, REQ_VIT_006_Underweight)    { EXPECT_NEAR(calculate_bmi(50.0f, 1.75f), 16.33f, 0.01f); }
TEST(BMI, REQ_VIT_006_Overweight)     { EXPECT_NEAR(calculate_bmi(85.0f, 1.75f), 27.76f, 0.01f); }
TEST(BMI, REQ_VIT_006_Obese)          { EXPECT_NEAR(calculate_bmi(100.0f,1.75f), 32.65f, 0.01f); }
TEST(BMI, REQ_VIT_006_ZeroHeight)     { EXPECT_EQ(calculate_bmi(70.0f, 0.0f), -1.0f); }
TEST(BMI, REQ_VIT_006_NegativeHeight) { EXPECT_EQ(calculate_bmi(70.0f, -1.0f), -1.0f); }

TEST(BMI, REQ_VIT_006_Category_Underweight)  { EXPECT_STREQ(bmi_category(16.0f), "Underweight");  }
TEST(BMI, REQ_VIT_006_Category_NormalLow)    { EXPECT_STREQ(bmi_category(18.5f), "Normal weight");}
TEST(BMI, REQ_VIT_006_Category_NormalHigh)   { EXPECT_STREQ(bmi_category(24.9f), "Normal weight");}
TEST(BMI, REQ_VIT_006_Category_Overweight)   { EXPECT_STREQ(bmi_category(27.0f), "Overweight");   }
TEST(BMI, REQ_VIT_006_Category_Obese)        { EXPECT_STREQ(bmi_category(32.0f), "Obese");        }
TEST(BMI, REQ_VIT_006_Category_Invalid)      { EXPECT_STREQ(bmi_category(-1.0f), "Invalid");      }

// =============================================================
// REQ-VIT-007  alert_level_str() — string mapping
// =============================================================

TEST(AlertStr, REQ_VIT_007_Normal)   { EXPECT_STREQ(alert_level_str(ALERT_NORMAL),   "NORMAL");   }
TEST(AlertStr, REQ_VIT_007_Warning)  { EXPECT_STREQ(alert_level_str(ALERT_WARNING),  "WARNING");  }
TEST(AlertStr, REQ_VIT_007_Critical) { EXPECT_STREQ(alert_level_str(ALERT_CRITICAL), "CRITICAL"); }
