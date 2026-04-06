/**
 * @file main.c
 * @brief Application entry point — simulated patient monitoring session.
 *
 * @details
 * Demonstrates the Patient Vital Signs Monitor with two simulated patients:
 *
 * **Patient 1 — Sarah Johnson (ID 1001)**
 * Simulates a deterioration trajectory across three readings:
 * - Reading 1: All normal (admission baseline)
 * - Reading 2: All WARNING (developing sepsis-like presentation)
 * - Reading 3: Temperature and SpO2 CRITICAL (acute crisis)
 *
 * **Patient 2 — David Okonkwo (ID 1002)**
 * Simulates an isolated bradycardia episode:
 * - Reading 1: All normal (stable)
 * - Reading 2: Heart rate CRITICAL (severe bradycardia at 38 bpm)
 *
 * ### Output Format
 * Each reading shows per-parameter classification in brackets and any
 * active alerts prefixed with `!  WARNING` or `!! CRITICAL`. A summary
 * block is printed at the end of each patient's session.
 *
 * @version 1.0.0
 * @date    2026-04-06
 * @author  vinu-engineer
 */

#include <stdio.h>
#include "patient.h"

/**
 * @brief Print a single vital sign reading with per-parameter status and alerts.
 *
 * @param[in] num  Reading sequence number (1-based), used in the heading.
 * @param[in] v    Pointer to the VitalSigns to display. Must not be NULL.
 */
static void print_reading(int num, const VitalSigns *v)
{
    Alert alerts[MAX_ALERTS];
    int   n, i;
    AlertLevel status = overall_alert_level(v);

    printf("\n  --- Reading #%d ---\n", num);
    printf("    Heart Rate  : %3d bpm       [%s]\n",
           v->heart_rate,
           alert_level_str(check_heart_rate(v->heart_rate)));
    printf("    BP          : %3d/%-3d mmHg  [%s]\n",
           v->systolic_bp, v->diastolic_bp,
           alert_level_str(check_blood_pressure(v->systolic_bp, v->diastolic_bp)));
    printf("    Temperature : %.1f C        [%s]\n",
           v->temperature,
           alert_level_str(check_temperature(v->temperature)));
    printf("    SpO2        : %3d%%           [%s]\n",
           v->spo2,
           alert_level_str(check_spo2(v->spo2)));
    printf("    Status      : %s\n", alert_level_str(status));

    n = generate_alerts(v, alerts, MAX_ALERTS);
    for (i = 0; i < n; i++) {
        const char *pfx = (alerts[i].level == ALERT_CRITICAL)
                          ? "    !! CRITICAL" : "    !  WARNING ";
        printf("%s  %s\n", pfx, alerts[i].message);
    }
}

/**
 * @brief Run the monitoring session for Patient 1 (deterioration scenario).
 *
 * @details Admits Sarah Johnson and records three readings that progress
 * from NORMAL → WARNING → CRITICAL to demonstrate full alert escalation.
 */
static void monitor_patient(void)
{
    PatientRecord rec;
    VitalSigns v;

    patient_init(&rec, 1001, "Sarah Johnson", 52, 72.5f, 1.66f);

    printf("==================================================\n");
    printf("  PATIENT VITAL SIGNS MONITOR  v1.0\n");
    printf("  Medical Device | IEC 62304 Compliant\n");
    printf("==================================================\n");
    printf("\n  Admitting: %s  (ID %d, Age %d)\n",
           rec.info.name, rec.info.id, rec.info.age);
    printf("  BMI : %.1f  (%s)\n",
           calculate_bmi(rec.info.weight_kg, rec.info.height_m),
           bmi_category(calculate_bmi(rec.info.weight_kg, rec.info.height_m)));

    v = (VitalSigns){ 78, 122, 82, 36.7f, 98 }; /* normal admission       */
    patient_add_reading(&rec, &v);
    print_reading(1, &v);

    v = (VitalSigns){ 108, 148, 94, 37.9f, 93 }; /* mild deterioration    */
    patient_add_reading(&rec, &v);
    print_reading(2, &v);

    v = (VitalSigns){ 135, 175, 112, 39.8f, 87 }; /* acute crisis         */
    patient_add_reading(&rec, &v);
    print_reading(3, &v);

    printf("\n");
    patient_print_summary(&rec);
    printf("\n");
}

/**
 * @brief Run the monitoring session for Patient 2 (bradycardia scenario).
 *
 * @details Admits David Okonkwo and records two readings, with the second
 * showing isolated severe bradycardia (38 bpm) while all other parameters
 * remain normal.
 */
static void monitor_second_patient(void)
{
    PatientRecord rec;
    VitalSigns v;

    patient_init(&rec, 1002, "David Okonkwo", 34, 85.0f, 1.80f);

    printf("\n==================================================\n");
    printf("  SECOND PATIENT\n");
    printf("==================================================\n");
    printf("\n  Admitting: %s  (ID %d, Age %d)\n",
           rec.info.name, rec.info.id, rec.info.age);
    printf("  BMI : %.1f  (%s)\n",
           calculate_bmi(rec.info.weight_kg, rec.info.height_m),
           bmi_category(calculate_bmi(rec.info.weight_kg, rec.info.height_m)));

    v = (VitalSigns){ 68, 118, 76, 36.5f, 99 }; /* stable baseline        */
    patient_add_reading(&rec, &v);
    print_reading(1, &v);

    v = (VitalSigns){ 38, 110, 72, 36.6f, 97 }; /* bradycardia episode    */
    patient_add_reading(&rec, &v);
    print_reading(2, &v);

    printf("\n");
    patient_print_summary(&rec);
    printf("\n");
}

/**
 * @brief Application entry point.
 * @return 0 on successful completion.
 */
int main(void)
{
    monitor_patient();
    monitor_second_patient();
    return 0;
}
