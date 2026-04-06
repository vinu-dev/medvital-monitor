/**
 * @file vitals.h
 * @brief Vital signs data types, thresholds, and validation functions.
 *
 * @details
 * This module defines the core data structures and validation logic for
 * patient vital signs. Each parameter is evaluated against clinically
 * defined thresholds derived from AHA/ACC guidelines and classified into
 * one of three alert levels: NORMAL, WARNING, or CRITICAL.
 *
 * ### Threshold Reference Table
 * | Parameter      | Normal          | Warning                  | Critical          |
 * |----------------|-----------------|--------------------------|-------------------|
 * | Heart Rate     | 60–100 bpm      | 41–59 / 101–150 bpm      | ≤40 / ≥151 bpm    |
 * | Systolic BP    | 90–140 mmHg     | 71–89 / 141–180 mmHg     | ≤70 / ≥181 mmHg   |
 * | Diastolic BP   | 60–90 mmHg      | 41–59 / 91–120 mmHg      | ≤40 / ≥121 mmHg   |
 * | Temperature    | 36.1–37.2 °C    | 35.0–36.0 / 37.3–39.5 °C| <35.0 / >39.5 °C  |
 * | SpO2           | 95–100 %        | 90–94 %                  | <90 %             |
 *
 * ### IEC 62304 Traceability
 * - Software Unit: UNIT-VIT
 * - Requirements covered: REQ-VIT-001 through REQ-VIT-007
 *
 * @version 1.0.0
 * @date    2026-04-06
 * @author  vinu-engineer
 *
 * @copyright Medical Device Software — IEC 62304 Class B compliant.
 *            All rights reserved.
 */

#ifndef VITALS_H
#define VITALS_H

/* =========================================================================
 * Alert Level Enumeration
 * ========================================================================= */

/**
 * @brief Classification of a vital sign reading relative to clinical norms.
 *
 * @details The numeric ordering is intentional: ALERT_NORMAL < ALERT_WARNING
 * < ALERT_CRITICAL, allowing comparisons such as `if (level > ALERT_NORMAL)`.
 * This ordering is relied upon by overall_alert_level().
 */
typedef enum {
    ALERT_NORMAL   = 0, /**< Reading within normal clinical range.            */
    ALERT_WARNING  = 1, /**< Reading outside normal range; clinician review
                             recommended.                                      */
    ALERT_CRITICAL = 2  /**< Reading at potentially life-threatening level;
                             immediate intervention required.                  */
} AlertLevel;

/* =========================================================================
 * Vital Signs Structure
 * ========================================================================= */

/**
 * @brief A single set of patient vital sign measurements.
 *
 * @details All fields are captured at the same point in time and treated
 * as a single observation. Callers must ensure values are physically
 * plausible before passing them to validation functions.
 */
typedef struct {
    int   heart_rate;    /**< Heart rate in beats per minute (bpm).           */
    int   systolic_bp;   /**< Systolic blood pressure in mmHg.                */
    int   diastolic_bp;  /**< Diastolic blood pressure in mmHg.               */
    float temperature;   /**< Core body temperature in degrees Celsius (°C).  */
    int   spo2;          /**< Peripheral oxygen saturation as a percentage.   */
} VitalSigns;

/* =========================================================================
 * Individual Parameter Checks
 * ========================================================================= */

/**
 * @brief Classify a heart rate reading against clinical thresholds.
 *
 * @details
 * - **NORMAL**:   60–100 bpm (inclusive)\n
 * - **WARNING**:  41–59 bpm or 101–150 bpm\n
 * - **CRITICAL**: ≤40 bpm or ≥151 bpm
 *
 * @param[in] bpm Heart rate in beats per minute. Negative values are
 *                treated as CRITICAL.
 * @return AlertLevel classification for the given heart rate.
 *
 * @req REQ-VIT-001
 */
AlertLevel check_heart_rate(int bpm);

/**
 * @brief Classify a blood pressure reading against clinical thresholds.
 *
 * @details Both systolic and diastolic values are evaluated independently.
 * The more severe of the two classifications is returned.
 *
 * - **NORMAL**:   Systolic 90–140 mmHg AND diastolic 60–90 mmHg\n
 * - **WARNING**:  Systolic 71–89 / 141–180 mmHg OR diastolic 41–59 / 91–120 mmHg\n
 * - **CRITICAL**: Systolic ≤70 / ≥181 mmHg OR diastolic ≤40 / ≥121 mmHg
 *
 * @param[in] systolic  Systolic pressure in mmHg.
 * @param[in] diastolic Diastolic pressure in mmHg.
 * @return AlertLevel classification for the combined blood pressure reading.
 *
 * @req REQ-VIT-002
 */
AlertLevel check_blood_pressure(int systolic, int diastolic);

/**
 * @brief Classify a body temperature reading against clinical thresholds.
 *
 * @details
 * - **NORMAL**:   36.1–37.2 °C (inclusive)\n
 * - **WARNING**:  35.0–36.0 °C or 37.3–39.5 °C\n
 * - **CRITICAL**: <35.0 °C (hypothermia) or >39.5 °C (hyperpyrexia)
 *
 * @param[in] temp_c Body temperature in degrees Celsius.
 * @return AlertLevel classification for the given temperature.
 *
 * @req REQ-VIT-003
 */
AlertLevel check_temperature(float temp_c);

/**
 * @brief Classify a peripheral oxygen saturation reading.
 *
 * @details
 * - **NORMAL**:   95–100 % (inclusive)\n
 * - **WARNING**:  90–94 %\n
 * - **CRITICAL**: <90 % (hypoxaemia)
 *
 * @param[in] spo2 SpO2 as a percentage (0–100). Values outside this range
 *                 are treated as CRITICAL.
 * @return AlertLevel classification for the given SpO2 reading.
 *
 * @req REQ-VIT-004
 */
AlertLevel check_spo2(int spo2);

/* =========================================================================
 * Aggregate Evaluation
 * ========================================================================= */

/**
 * @brief Return the highest alert level across all vital sign parameters.
 *
 * @details Internally calls check_heart_rate(), check_blood_pressure(),
 * check_temperature(), and check_spo2(), then returns the maximum value.
 * This ensures that any single critical parameter elevates the overall
 * patient status to CRITICAL regardless of other parameters.
 *
 * @param[in] vitals Pointer to a valid VitalSigns structure. Must not be NULL.
 * @return The highest AlertLevel among all four parameters.
 *
 * @pre  vitals != NULL
 * @req  REQ-VIT-005
 */
AlertLevel overall_alert_level(const VitalSigns *vitals);

/* =========================================================================
 * BMI Calculation
 * ========================================================================= */

/**
 * @brief Calculate Body Mass Index (BMI) from weight and height.
 *
 * @details Uses the standard formula: BMI = weight_kg / (height_m * height_m).
 *
 * @param[in] weight_kg Body weight in kilograms.
 * @param[in] height_m  Height in metres. Must be > 0.
 * @return BMI value, or -1.0 if height_m is zero or negative.
 *
 * @req REQ-VIT-006
 */
float calculate_bmi(float weight_kg, float height_m);

/**
 * @brief Return the WHO BMI category string for a given BMI value.
 *
 * @details
 * | BMI Range    | Category       |
 * |--------------|----------------|
 * | < 18.5       | Underweight    |
 * | 18.5 – 24.9  | Normal weight  |
 * | 25.0 – 29.9  | Overweight     |
 * | ≥ 30.0       | Obese          |
 * | < 0          | Invalid        |
 *
 * @param[in] bmi BMI value as returned by calculate_bmi().
 * @return Null-terminated string literal (never NULL). Caller must not free.
 *
 * @req REQ-VIT-006
 */
const char *bmi_category(float bmi);

/* =========================================================================
 * String Helpers
 * ========================================================================= */

/**
 * @brief Convert an AlertLevel value to a human-readable string.
 *
 * @param[in] level The AlertLevel to convert.
 * @return One of: "NORMAL", "WARNING", "CRITICAL", or "UNKNOWN".
 *         Returned pointer is a string literal — caller must not free.
 *
 * @req REQ-VIT-007
 */
const char *alert_level_str(AlertLevel level);

#endif /* VITALS_H */
