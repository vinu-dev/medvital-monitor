#ifndef VITALS_H
#define VITALS_H

/* ---------------------------------------------------------------
 * Vital Signs — core data types and validation
 * REQ-VIT: Validate each parameter against clinically defined
 *          normal, warning, and critical thresholds.
 * Ref: AHA/ACC guidelines; IEC 62304 §5.5
 * --------------------------------------------------------------- */

typedef enum {
    ALERT_NORMAL   = 0,
    ALERT_WARNING  = 1,
    ALERT_CRITICAL = 2
} AlertLevel;

typedef struct {
    int   heart_rate;    /* bpm        */
    int   systolic_bp;   /* mmHg       */
    int   diastolic_bp;  /* mmHg       */
    float temperature;   /* Celsius    */
    int   spo2;          /* %          */
} VitalSigns;

/* Individual parameter checks */
AlertLevel  check_heart_rate(int bpm);
AlertLevel  check_blood_pressure(int systolic, int diastolic);
AlertLevel  check_temperature(float temp_c);
AlertLevel  check_spo2(int spo2);

/* Aggregate: returns the highest alert level across all parameters */
AlertLevel  overall_alert_level(const VitalSigns *vitals);

/* BMI */
float       calculate_bmi(float weight_kg, float height_m);
const char *bmi_category(float bmi);

/* String helpers */
const char *alert_level_str(AlertLevel level);

#endif /* VITALS_H */
