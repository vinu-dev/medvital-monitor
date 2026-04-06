/**
 * @file patient.c
 * @brief Implementation of patient record management functions.
 *
 * @details
 * Implements all functions declared in patient.h. The module uses only
 * static memory (no heap allocation) in compliance with IEC 62304 Class B
 * guidance for embedded medical device software.
 *
 * ### Memory Safety
 * - `patient_init()` zero-fills the entire record before writing fields,
 *   preventing stale data from a previous session.
 * - `strncpy` is used with an explicit length limit and explicit
 *   null-termination to guard against buffer overrun.
 * - `patient_add_reading()` enforces the MAX_READINGS cap and returns a
 *   status code so callers can detect and handle overflow.
 *
 * @version 1.0.0
 * @date    2026-04-06
 * @author  vinu-engineer
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "patient.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Initialise a PatientRecord, zeroing all fields before populating.
 * @details memset(0) ensures that any unused readings array slots contain
 *          all-zero VitalSigns, which classify as CRITICAL across all
 *          parameters — a deliberately safe default that prevents silent
 *          display of uninitialised data.
 */
void patient_init(PatientRecord *rec, int id, const char *name,
                  int age, float weight_kg, float height_m)
{
    memset(rec, 0, sizeof(*rec));
    rec->info.id        = id;
    rec->info.age       = age;
    rec->info.weight_kg = weight_kg;
    rec->info.height_m  = height_m;
    strncpy(rec->info.name, name, MAX_NAME_LEN - 1);
    rec->info.name[MAX_NAME_LEN - 1] = '\0'; /* guarantee null-termination */
    rec->reading_count  = 0;
}

/**
 * @brief Append a vital signs reading, enforcing the MAX_READINGS cap.
 * @details VitalSigns is copied by value (struct assignment), so the caller
 *          retains ownership of the original and may reuse the variable.
 */
int patient_add_reading(PatientRecord *rec, const VitalSigns *v)
{
    if (rec->reading_count >= MAX_READINGS) return 0;
    rec->readings[rec->reading_count++] = *v;
    return 1;
}

/**
 * @brief Return a pointer to the most recent reading, or NULL if none exist.
 * @details Index arithmetic: the last valid slot is readings[reading_count - 1].
 */
const VitalSigns *patient_latest_reading(const PatientRecord *rec)
{
    if (rec->reading_count == 0) return NULL;
    return &rec->readings[rec->reading_count - 1];
}

/**
 * @brief Return the overall status from the latest reading.
 * @details Delegates entirely to overall_alert_level() to keep classification
 *          logic centralised in vitals.c — consistent with single-responsibility.
 */
AlertLevel patient_current_status(const PatientRecord *rec)
{
    const VitalSigns *v = patient_latest_reading(rec);
    if (v == NULL) return ALERT_NORMAL;
    return overall_alert_level(v);
}

/**
 * @brief Check if the reading buffer is at capacity.
 */
int patient_is_full(const PatientRecord *rec)
{
    return rec->reading_count >= MAX_READINGS;
}

/**
 * @brief Print a formatted patient summary including vitals and active alerts.
 * @details Generates alerts inline for the latest reading using generate_alerts().
 *          Output is formatted for an 80-column terminal with box-drawing borders.
 */
void patient_print_summary(const PatientRecord *rec)
{
    float bmi = calculate_bmi(rec->info.weight_kg, rec->info.height_m);
    const VitalSigns *latest = patient_latest_reading(rec);
    AlertLevel status = patient_current_status(rec);
    Alert alerts[MAX_ALERTS];
    int alert_count = 0;

    printf("+--------------------------------------------------+\n");
    printf("| PATIENT SUMMARY                                  |\n");
    printf("+--------------------------------------------------+\n");
    printf("  Name    : %s\n", rec->info.name);
    printf("  ID      : %d\n", rec->info.id);
    printf("  Age     : %d years\n", rec->info.age);
    printf("  BMI     : %.1f  (%s)\n", bmi, bmi_category(bmi));
    printf("  Readings: %d / %d\n", rec->reading_count, MAX_READINGS);

    if (latest) {
        printf("\n  Latest Vitals:\n");
        printf("    Heart Rate  : %3d bpm       [%s]\n",
               latest->heart_rate,
               alert_level_str(check_heart_rate(latest->heart_rate)));
        printf("    BP          : %3d/%-3d mmHg  [%s]\n",
               latest->systolic_bp, latest->diastolic_bp,
               alert_level_str(check_blood_pressure(latest->systolic_bp,
                                                     latest->diastolic_bp)));
        printf("    Temperature : %.1f C        [%s]\n",
               latest->temperature,
               alert_level_str(check_temperature(latest->temperature)));
        printf("    SpO2        : %3d%%           [%s]\n",
               latest->spo2,
               alert_level_str(check_spo2(latest->spo2)));

        alert_count = generate_alerts(latest, alerts, MAX_ALERTS);
    }

    printf("\n  Overall Status : %s\n", alert_level_str(status));

    if (alert_count > 0) {
        printf("\n  Active Alerts:\n");
        for (int i = 0; i < alert_count; i++) {
            const char *pfx = (alerts[i].level == ALERT_CRITICAL)
                              ? "  !! CRITICAL" : "  !  WARNING ";
            printf("%s  %s\n", pfx, alerts[i].message);
        }
    }
    printf("+--------------------------------------------------+\n");
}
