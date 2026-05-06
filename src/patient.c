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

typedef struct {
    AlertLevel  level;
    unsigned int abnormal_mask;
    Alert       alerts[MAX_ALERTS];
    int         alert_count;
} AlertSignature;

enum {
    ALERT_MASK_HEART_RATE     = 1u << 0,
    ALERT_MASK_BLOOD_PRESSURE = 1u << 1,
    ALERT_MASK_TEMPERATURE    = 1u << 2,
    ALERT_MASK_SPO2           = 1u << 3,
    ALERT_MASK_RESP_RATE      = 1u << 4
};

static unsigned int alert_parameter_mask(const char *parameter)
{
    if (strcmp(parameter, "Heart Rate") == 0) {
        return ALERT_MASK_HEART_RATE;
    }
    if (strcmp(parameter, "Blood Pressure") == 0) {
        return ALERT_MASK_BLOOD_PRESSURE;
    }
    if (strcmp(parameter, "Temperature") == 0) {
        return ALERT_MASK_TEMPERATURE;
    }
    if (strcmp(parameter, "SpO2") == 0) {
        return ALERT_MASK_SPO2;
    }
    if (strcmp(parameter, "Resp Rate") == 0) {
        return ALERT_MASK_RESP_RATE;
    }
    return 0u;
}

static void build_alert_signature(const VitalSigns *v, AlertSignature *sig)
{
    int i;

    memset(sig, 0, sizeof(*sig));
    sig->level = overall_alert_level(v);
    sig->alert_count = generate_alerts(v, sig->alerts, MAX_ALERTS);

    for (i = 0; i < sig->alert_count; ++i) {
        sig->abnormal_mask |= alert_parameter_mask(sig->alerts[i].parameter);
    }
}

static int should_record_alert_event(int had_previous,
                                     const AlertSignature *previous,
                                     const AlertSignature *current)
{
    if (!had_previous) {
        return current->alert_count > 0;
    }
    if (previous->alert_count == 0 && current->alert_count == 0) {
        return 0;
    }
    if (previous->alert_count == 0 || current->alert_count == 0) {
        return 1;
    }
    return previous->level != current->level
        || previous->abnormal_mask != current->abnormal_mask;
}

static void format_alert_event_summary(const AlertSignature *sig,
                                       char *out,
                                       size_t out_size)
{
    size_t used = 0;
    int i;

    if (sig->alert_count == 0) {
        snprintf(out, out_size,
                 "Recovered to normal; active abnormalities cleared");
        return;
    }

    used = (size_t)snprintf(out, out_size, "Abnormal parameters: ");
    if (used >= out_size) {
        out[out_size - 1] = '\0';
        return;
    }

    for (i = 0; i < sig->alert_count; ++i) {
        int written = snprintf(out + used, out_size - used, "%s%s",
                               (i == 0) ? "" : ", ",
                               sig->alerts[i].parameter);
        if (written < 0) {
            out[used] = '\0';
            return;
        }
        if ((size_t)written >= out_size - used) {
            out[out_size - 1] = '\0';
            return;
        }
        used += (size_t)written;
    }
}

static void append_alert_event(PatientRecord *rec, const AlertSignature *sig)
{
    AlertEvent *event;

    if (rec->alert_event_count >= MAX_ALERT_EVENTS) {
        return;
    }

    event = &rec->alert_events[rec->alert_event_count++];
    event->reading_index = rec->reading_count;
    event->level = sig->level;
    event->abnormal_mask = sig->abnormal_mask;
    format_alert_event_summary(sig, event->summary, sizeof(event->summary));
}

static void format_alert_event_line(const AlertEvent *event,
                                    char *out,
                                    size_t out_size)
{
    snprintf(out, out_size, "#%d [%s] %s",
             event->reading_index,
             alert_level_str(event->level),
             event->summary);
}

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
    char name_copy[MAX_NAME_LEN];

    /* Copy the name first in case the caller passes rec->info.name back in. */
    strncpy(name_copy, name, MAX_NAME_LEN - 1);
    name_copy[MAX_NAME_LEN - 1] = '\0';

    memset(rec, 0, sizeof(*rec));
    rec->info.id        = id;
    rec->info.age       = age;
    rec->info.weight_kg = weight_kg;
    rec->info.height_m  = height_m;
    strncpy(rec->info.name, name_copy, MAX_NAME_LEN - 1);
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
    AlertSignature previous = {0};
    AlertSignature current;
    int had_previous = rec->reading_count > 0;

    if (rec->reading_count >= MAX_READINGS) return 0;

    if (had_previous) {
        build_alert_signature(&rec->readings[rec->reading_count - 1], &previous);
    }

    rec->readings[rec->reading_count++] = *v;

    build_alert_signature(v, &current);
    if (should_record_alert_event(had_previous, &previous, &current)) {
        append_alert_event(rec, &current);
    }

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

void patient_note_session_reset(PatientRecord *rec, int previous_reading_count)
{
    if (previous_reading_count <= 0) {
        rec->session_reset_notice[0] = '\0';
        return;
    }

    snprintf(rec->session_reset_notice, sizeof(rec->session_reset_notice),
             "Session reset automatically after %d readings; earlier alarm "
             "events from the previous session are no longer retained.",
             previous_reading_count);
}

const char *patient_session_reset_notice(const PatientRecord *rec)
{
    if (rec->session_reset_notice[0] == '\0') {
        return NULL;
    }
    return rec->session_reset_notice;
}

int patient_alert_event_count(const PatientRecord *rec)
{
    return rec->alert_event_count;
}

const AlertEvent *patient_alert_event_at(const PatientRecord *rec, int index)
{
    if (index < 0 || index >= rec->alert_event_count) {
        return NULL;
    }
    return &rec->alert_events[index];
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
    int i;
    char event_line[256];
    const char *reset_notice = patient_session_reset_notice(rec);

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
        for (i = 0; i < alert_count; i++) {
            const char *pfx = (alerts[i].level == ALERT_CRITICAL)
                              ? "  !! CRITICAL" : "  !  WARNING ";
            printf("%s  %s\n", pfx, alerts[i].message);
        }
    }

    printf("\n  Session Alarm Events:\n");
    if (reset_notice != NULL) {
        printf("    NOTE: %s\n", reset_notice);
    }
    if (patient_alert_event_count(rec) == 0) {
        printf("    None recorded in current session.\n");
    } else {
        for (i = 0; i < patient_alert_event_count(rec); ++i) {
            const AlertEvent *event = patient_alert_event_at(rec, i);
            if (event == NULL) {
                continue;
            }
            format_alert_event_line(event, event_line, sizeof(event_line));
            printf("    %s\n", event_line);
        }
    }

    printf("+--------------------------------------------------+\n");
}
