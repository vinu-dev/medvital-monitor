/**
 * @file patient.h
 * @brief Patient record management — demographics, vital sign history, and
 *        session alert-event review data.
 *
 * @details
 * This module provides the top-level data structure for a monitored patient
 * and the functions to manage their vital sign history. It composes the
 * vitals and alerts modules to provide a complete patient-status API.
 *
 * ### Architecture Position
 * PatientRecord sits at the top of the module hierarchy:
 * @code
 *   patient.h
 *     └── vitals.h   (classification of individual readings)
 *     └── alerts.h   (alert record generation)
 * @endcode
 *
 * ### Storage Model
 * Readings are stored in a fixed-size array (MAX_READINGS = 10). Once full,
 * further additions are rejected (patient_add_reading() returns 0). This
 * design avoids dynamic memory allocation, a requirement for safety-critical
 * embedded software under IEC 62304.
 *
 * ### IEC 62304 Traceability
 * - Software Unit: UNIT-PAT
 * - Requirements covered: SWR-PAT-001 through SWR-PAT-006
 *
 * @version 1.0.0
 * @date    2026-04-06
 * @author  vinu-engineer
 *
 * @copyright Medical Device Software — IEC 62304 Class B compliant.
 *            All rights reserved.
 */

#ifndef PATIENT_H
#define PATIENT_H

#include "vitals.h"
#include "alerts.h"

/* =========================================================================
 * Constants
 * ========================================================================= */

/** @brief Maximum number of vital sign readings stored per patient. */
#define MAX_READINGS 10

/** @brief Maximum number of alert-event records stored per patient session. */
#define MAX_ALERT_EVENTS MAX_READINGS

/** @brief Maximum length of a patient name string (including null terminator). */
#define MAX_NAME_LEN 64

/** @brief Maximum length of a stored alert-event summary string. */
#define ALERT_EVENT_SUMMARY_LEN 160

/** @brief Maximum length of a retained session-reset disclosure string. */
#define PATIENT_SESSION_NOTICE_LEN 160

/* =========================================================================
 * Data Structures
 * ========================================================================= */

/**
 * @brief Demographic information for a monitored patient.
 *
 * @details This structure stores static patient data that does not change
 * during a monitoring session. It is embedded within PatientRecord.
 */
typedef struct {
    int   id;                  /**< Unique patient identifier.                 */
    char  name[MAX_NAME_LEN];  /**< Full name. Null-terminated, truncated to
                                    MAX_NAME_LEN - 1 characters if necessary.  */
    int   age;                 /**< Age in years.                              */
    float weight_kg;           /**< Body weight in kilograms.                  */
    float height_m;            /**< Height in metres.                          */
} PatientInfo;

/**
 * @brief One historical alert-state transition recorded within the session.
 *
 * @details
 * Each event is derived from the validated alert engine for a successfully
 * appended reading. The event captures the 1-based reading index where the
 * change occurred, the resulting aggregate alert severity, a compact abnormal
 * parameter signature, and a summary string reused by review surfaces.
 */
typedef struct {
    int         reading_index;                        /**< 1-based reading index. */
    AlertLevel  level;                                /**< Resulting aggregate
                                                           level after the
                                                           transition.        */
    unsigned int abnormal_mask;                       /**< Bitmask of abnormal
                                                           parameters. Zero
                                                           indicates recovery
                                                           to normal.         */
    char        summary[ALERT_EVENT_SUMMARY_LEN];     /**< Human-readable,
                                                           session-scoped
                                                           event summary.     */
} AlertEvent;

/**
 * @brief Complete patient record including demographics and vital sign history.
 *
 * @details The readings array acts as a sequential log: index 0 holds the
 * first reading, index reading_count-1 holds the most recent. The alert_events
 * array stores the derived alert-state transitions for the same session.
 * The record is considered full when reading_count == MAX_READINGS.
 *
 * @note No dynamic memory is used. All storage is stack/static allocated.
 */
typedef struct {
    PatientInfo info;                            /**< Demographic data.              */
    VitalSigns  readings[MAX_READINGS];          /**< Historical vital sign
                                                      readings.                    */
    int         reading_count;                   /**< Number of valid entries in
                                                      readings[]. Range:
                                                      0-MAX_READINGS.             */
    AlertEvent  alert_events[MAX_ALERT_EVENTS];  /**< Derived alert-state review
                                                      events for this session.    */
    int         alert_event_count;               /**< Number of valid entries in
                                                      alert_events[]. Range:
                                                      0-MAX_ALERT_EVENTS.         */
    char        session_reset_notice[PATIENT_SESSION_NOTICE_LEN];
                                                 /**< Optional disclosure shown
                                                      after an automatic
                                                      session reset clears
                                                      earlier review data.       */
} PatientRecord;

/* =========================================================================
 * Lifecycle
 * ========================================================================= */

/**
 * @brief Initialise a PatientRecord with demographic data.
 *
 * @details Zero-fills the entire structure before populating fields.
 * reading_count is set to 0. The name string is copied with truncation
 * to MAX_NAME_LEN - 1 characters and guaranteed null-termination.
 *
 * @param[out] rec        Pointer to the record to initialise. Must not be NULL.
 * @param[in]  id         Unique patient identifier.
 * @param[in]  name       Null-terminated patient name string. Must not be NULL.
 * @param[in]  age        Patient age in years.
 * @param[in]  weight_kg  Body weight in kilograms.
 * @param[in]  height_m   Height in metres.
 *
 * @pre rec != NULL
 * @pre name != NULL
 * @post rec->reading_count == 0
 *
 * @par Requirement
 * SWR-PAT-001
 */
void patient_init(PatientRecord *rec, int id, const char *name,
                  int age, float weight_kg, float height_m);

/* =========================================================================
 * Reading Management
 * ========================================================================= */

/**
 * @brief Append a vital sign reading to the patient's history.
 *
 * @details The VitalSigns structure is copied by value into the internal
 * array. If the record is already full (reading_count == MAX_READINGS),
 * the record is left unchanged and 0 is returned.
 *
 * @param[in,out] rec Pointer to an initialised PatientRecord. Must not be NULL.
 * @param[in]     v   Pointer to the VitalSigns reading to append. Copied
 *                    by value; the caller may free or reuse @p v afterwards.
 *
 * @return 1 if the reading was successfully appended, 0 if the buffer is full.
 *
 * @pre  rec != NULL
 * @pre  v   != NULL
 * @post On success: rec->reading_count is incremented by 1.
 * @post On failure: rec is unchanged.
 *
 * @par Requirement
 * SWR-PAT-002, SWR-PAT-007
 */
int patient_add_reading(PatientRecord *rec, const VitalSigns *v);

/**
 * @brief Return a pointer to the most recently added vital sign reading.
 *
 * @param[in] rec Pointer to an initialised PatientRecord. Must not be NULL.
 *
 * @return Pointer to the latest VitalSigns entry within @p rec, or NULL if
 *         no readings have been recorded (reading_count == 0).
 *
 * @warning The returned pointer points into the PatientRecord's internal
 *          array. It becomes invalid if @p rec is destroyed or moved.
 *
 * @par Requirement
 * SWR-PAT-003
 */
const VitalSigns *patient_latest_reading(const PatientRecord *rec);

/* =========================================================================
 * Status Queries
 * ========================================================================= */

/**
 * @brief Return the overall alert status derived from the latest reading.
 *
 * @details Delegates to overall_alert_level() on the most recent reading.
 * Returns ALERT_NORMAL if no readings have been recorded yet.
 *
 * @param[in] rec Pointer to an initialised PatientRecord. Must not be NULL.
 * @return AlertLevel based on the latest VitalSigns, or ALERT_NORMAL if
 *         reading_count == 0.
 *
 * @par Requirement
 * SWR-PAT-004
 */
AlertLevel patient_current_status(const PatientRecord *rec);

/**
 * @brief Check whether the patient record's reading buffer is full.
 *
 * @param[in] rec Pointer to an initialised PatientRecord. Must not be NULL.
 * @return Non-zero (true) if reading_count == MAX_READINGS, zero otherwise.
 *
 * @par Requirement
 * SWR-PAT-005
 */
int patient_is_full(const PatientRecord *rec);

/**
 * @brief Return the number of stored session alert events.
 *
 * @param[in] rec Pointer to an initialised PatientRecord. Must not be NULL.
 * @return Number of valid entries in alert_events[].
 *
 * @par Requirement
 * SWR-PAT-008
 */
int patient_alert_event_count(const PatientRecord *rec);

/**
 * @brief Return a pointer to one stored session alert event.
 *
 * @param[in] rec   Pointer to an initialised PatientRecord. Must not be NULL.
 * @param[in] index Zero-based event index into alert_events[].
 *
 * @return Pointer to alert_events[index], or NULL if @p index is out of range.
 *
 * @par Requirement
 * SWR-PAT-008
 */
const AlertEvent *patient_alert_event_at(const PatientRecord *rec, int index);

/**
 * @brief Record a disclosure message for a session reset boundary.
 *
 * @details Callers use this after reinitialising a patient session because a
 * retention boundary was reached. The notice is retained until the next
 * patient_init() clears the record.
 *
 * @param[in,out] rec Pointer to an initialised PatientRecord. Must not be NULL.
 * @param[in] previous_reading_count Number of readings retained before reset.
 *
 * @par Requirement
 * SWR-PAT-008
 */
void patient_note_session_reset(PatientRecord *rec, int previous_reading_count);

/**
 * @brief Return the current session-reset disclosure, if any.
 *
 * @param[in] rec Pointer to an initialised PatientRecord. Must not be NULL.
 * @return Pointer to the retained disclosure string, or NULL if none is set.
 *
 * @par Requirement
 * SWR-PAT-008
 */
const char *patient_session_reset_notice(const PatientRecord *rec);

/* =========================================================================
 * Display
 * ========================================================================= */

/**
 * @brief Print a formatted patient summary to stdout.
 *
 * @details Outputs demographic data, BMI, the latest vital signs with
 * individual classifications, the overall status, any active alerts, and the
 * session alert-event review log. Output is suitable for a 80-column terminal.
 *
 * @param[in] rec Pointer to an initialised PatientRecord. Must not be NULL.
 *
 * @note This function writes to stdout. In unit tests, redirect stdout
 *       before calling to keep test output clean.
 *
 * @par Requirement
 * SWR-PAT-006, SWR-PAT-008
 */
void patient_print_summary(const PatientRecord *rec);

#endif /* PATIENT_H */
