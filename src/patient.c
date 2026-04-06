#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "patient.h"
#include <stdio.h>
#include <string.h>

void patient_init(PatientRecord *rec, int id, const char *name,
                  int age, float weight_kg, float height_m)
{
    memset(rec, 0, sizeof(*rec));
    rec->info.id        = id;
    rec->info.age       = age;
    rec->info.weight_kg = weight_kg;
    rec->info.height_m  = height_m;
    strncpy(rec->info.name, name, MAX_NAME_LEN - 1);
    rec->info.name[MAX_NAME_LEN - 1] = '\0';
    rec->reading_count  = 0;
}

int patient_add_reading(PatientRecord *rec, const VitalSigns *v)
{
    if (rec->reading_count >= MAX_READINGS) return 0;
    rec->readings[rec->reading_count++] = *v;
    return 1;
}

const VitalSigns *patient_latest_reading(const PatientRecord *rec)
{
    if (rec->reading_count == 0) return NULL;
    return &rec->readings[rec->reading_count - 1];
}

AlertLevel patient_current_status(const PatientRecord *rec)
{
    const VitalSigns *v = patient_latest_reading(rec);
    if (v == NULL) return ALERT_NORMAL;
    return overall_alert_level(v);
}

int patient_is_full(const PatientRecord *rec)
{
    return rec->reading_count >= MAX_READINGS;
}

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
               latest->heart_rate, alert_level_str(check_heart_rate(latest->heart_rate)));
        printf("    BP          : %3d/%-3d mmHg  [%s]\n",
               latest->systolic_bp, latest->diastolic_bp,
               alert_level_str(check_blood_pressure(latest->systolic_bp, latest->diastolic_bp)));
        printf("    Temperature : %.1f C        [%s]\n",
               latest->temperature, alert_level_str(check_temperature(latest->temperature)));
        printf("    SpO2        : %3d%%           [%s]\n",
               latest->spo2, alert_level_str(check_spo2(latest->spo2)));

        alert_count = generate_alerts(latest, alerts, MAX_ALERTS);
    }

    printf("\n  Overall Status : %s\n", alert_level_str(status));

    if (alert_count > 0) {
        printf("\n  Active Alerts:\n");
        for (int i = 0; i < alert_count; i++) {
            const char *prefix = (alerts[i].level == ALERT_CRITICAL) ? "  !! CRITICAL" : "  !  WARNING ";
            printf("%s  %s\n", prefix, alerts[i].message);
        }
    }
    printf("+--------------------------------------------------+\n");
}
