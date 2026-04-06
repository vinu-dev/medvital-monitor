#include "vitals.h"

/* ---------------------------------------------------------------
 * Thresholds (AHA/ACC clinical guidelines)
 *
 *  Heart Rate (bpm)
 *    CRITICAL : < 40  or > 150
 *    WARNING  : < 60  or > 100
 *    NORMAL   : 60 – 100
 *
 *  Systolic BP (mmHg)
 *    CRITICAL : < 70  or > 180
 *    WARNING  : < 90  or > 140
 *    NORMAL   : 90 – 140
 *
 *  Diastolic BP (mmHg)
 *    CRITICAL : < 40  or > 120
 *    WARNING  : < 60  or > 90
 *    NORMAL   : 60 – 90
 *
 *  Temperature (°C)
 *    CRITICAL : < 35.0 or > 39.5
 *    WARNING  : < 36.1 or > 37.2
 *    NORMAL   : 36.1 – 37.2
 *
 *  SpO2 (%)
 *    CRITICAL : < 90
 *    WARNING  : 90 – 94
 *    NORMAL   : 95 – 100
 * --------------------------------------------------------------- */

AlertLevel check_heart_rate(int bpm)
{
    if (bpm < 40  || bpm > 150) return ALERT_CRITICAL;
    if (bpm < 60  || bpm > 100) return ALERT_WARNING;
    return ALERT_NORMAL;
}

AlertLevel check_blood_pressure(int systolic, int diastolic)
{
    if (systolic  <  70 || systolic  > 180) return ALERT_CRITICAL;
    if (diastolic <  40 || diastolic > 120) return ALERT_CRITICAL;
    if (systolic  <  90 || systolic  > 140) return ALERT_WARNING;
    if (diastolic <  60 || diastolic >  90) return ALERT_WARNING;
    return ALERT_NORMAL;
}

AlertLevel check_temperature(float temp_c)
{
    if (temp_c < 35.0f || temp_c > 39.5f) return ALERT_CRITICAL;
    if (temp_c < 36.1f || temp_c > 37.2f) return ALERT_WARNING;
    return ALERT_NORMAL;
}

AlertLevel check_spo2(int spo2)
{
    if (spo2 < 90) return ALERT_CRITICAL;
    if (spo2 < 95) return ALERT_WARNING;
    return ALERT_NORMAL;
}

AlertLevel overall_alert_level(const VitalSigns *vitals)
{
    AlertLevel levels[4];
    AlertLevel max = ALERT_NORMAL;
    int i;

    levels[0] = check_heart_rate(vitals->heart_rate);
    levels[1] = check_blood_pressure(vitals->systolic_bp, vitals->diastolic_bp);
    levels[2] = check_temperature(vitals->temperature);
    levels[3] = check_spo2(vitals->spo2);

    for (i = 0; i < 4; i++) {
        if (levels[i] > max) max = levels[i];
    }
    return max;
}

float calculate_bmi(float weight_kg, float height_m)
{
    if (height_m <= 0.0f) return -1.0f;
    return weight_kg / (height_m * height_m);
}

const char *bmi_category(float bmi)
{
    if (bmi < 0.0f)  return "Invalid";
    if (bmi < 18.5f) return "Underweight";
    if (bmi < 25.0f) return "Normal weight";
    if (bmi < 30.0f) return "Overweight";
    return "Obese";
}

const char *alert_level_str(AlertLevel level)
{
    switch (level) {
        case ALERT_NORMAL:   return "NORMAL";
        case ALERT_WARNING:  return "WARNING";
        case ALERT_CRITICAL: return "CRITICAL";
        default:             return "UNKNOWN";
    }
}
