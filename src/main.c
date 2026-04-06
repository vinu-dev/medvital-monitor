#include <stdio.h>
#include "patient.h"

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

static void monitor_patient(void)
{
    PatientRecord rec;
    VitalSigns v;

    /* --- Admit patient ----------------------------------------- */
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

    /* --- Reading 1: normal admission vitals -------------------- */
    v = (VitalSigns){ 78, 122, 82, 36.7f, 98 };
    patient_add_reading(&rec, &v);
    print_reading(1, &v);

    /* --- Reading 2: mild deterioration — all WARNING ----------- */
    v = (VitalSigns){ 108, 148, 94, 37.9f, 93 };
    patient_add_reading(&rec, &v);
    print_reading(2, &v);

    /* --- Reading 3: acute crisis — CRITICAL -------------------- */
    v = (VitalSigns){ 135, 175, 112, 39.8f, 87 };
    patient_add_reading(&rec, &v);
    print_reading(3, &v);

    /* --- Final summary ----------------------------------------- */
    printf("\n");
    patient_print_summary(&rec);
    printf("\n");
}

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

    /* Stable patient — all normal */
    v = (VitalSigns){ 68, 118, 76, 36.5f, 99 };
    patient_add_reading(&rec, &v);
    print_reading(1, &v);

    /* Bradycardia episode */
    v = (VitalSigns){ 38, 110, 72, 36.6f, 97 };
    patient_add_reading(&rec, &v);
    print_reading(2, &v);

    printf("\n");
    patient_print_summary(&rec);
    printf("\n");
}

int main(void)
{
    monitor_patient();
    monitor_second_patient();
    return 0;
}
