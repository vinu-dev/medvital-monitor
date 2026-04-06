#ifndef PATIENT_H
#define PATIENT_H

/* ---------------------------------------------------------------
 * Patient record — stores demographic info and a ring of
 * recent vital-sign readings.
 * REQ-PAT: A PatientRecord shall hold up to MAX_READINGS
 *          readings; attempting to add beyond capacity shall
 *          leave the record unchanged and return 0.
 * --------------------------------------------------------------- */

#include "vitals.h"
#include "alerts.h"

#define MAX_READINGS 10
#define MAX_NAME_LEN 64

typedef struct {
    int   id;
    char  name[MAX_NAME_LEN];
    int   age;
    float weight_kg;
    float height_m;
} PatientInfo;

typedef struct {
    PatientInfo info;
    VitalSigns  readings[MAX_READINGS];
    int         reading_count;
} PatientRecord;

void              patient_init(PatientRecord *rec, int id, const char *name,
                               int age, float weight_kg, float height_m);
int               patient_add_reading(PatientRecord *rec, const VitalSigns *v);
const VitalSigns *patient_latest_reading(const PatientRecord *rec);
AlertLevel        patient_current_status(const PatientRecord *rec);
int               patient_is_full(const PatientRecord *rec);
void              patient_print_summary(const PatientRecord *rec);

#endif /* PATIENT_H */
