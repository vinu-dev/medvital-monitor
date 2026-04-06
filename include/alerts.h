/**
 * @file alerts.h
 * @brief Alert record generation from a set of vital sign measurements.
 *
 * @details
 * This module is responsible for translating a VitalSigns observation into
 * a structured list of Alert records — one per abnormal parameter. It acts
 * as the bridge between raw clinical data (vitals.h) and the patient
 * management layer (patient.h).
 *
 * ### Design Decisions
 * - Each abnormal parameter produces exactly one Alert record.
 * - The output buffer size is controlled by the caller via `max_out`, making
 *   the function safe for use in memory-constrained embedded targets.
 * - All-normal vitals produce zero alerts (return value 0).
 * - Alert messages are human-readable and suitable for display on a clinical
 *   workstation or serial console.
 *
 * ### IEC 62304 Traceability
 * - Software Unit: UNIT-ALT
 * - Requirements covered: REQ-ALT-001 through REQ-ALT-005
 *
 * @version 1.0.0
 * @date    2026-04-06
 * @author  vinu-engineer
 *
 * @copyright Medical Device Software — IEC 62304 Class B compliant.
 *            All rights reserved.
 */

#ifndef ALERTS_H
#define ALERTS_H

#include "vitals.h"

/* =========================================================================
 * Constants
 * ========================================================================= */

/** @brief Maximum number of alerts that can be generated from one VitalSigns. */
#define MAX_ALERTS    5

/** @brief Maximum length (including null terminator) of an alert message. */
#define ALERT_MSG_LEN 96

/* =========================================================================
 * Alert Record Structure
 * ========================================================================= */

/**
 * @brief A single alert record describing one out-of-range vital sign.
 *
 * @details Each Alert corresponds to exactly one parameter that deviated
 * from its normal range. Fields are populated by generate_alerts().
 */
typedef struct {
    AlertLevel level;           /**< Severity: ALERT_WARNING or ALERT_CRITICAL. */
    char parameter[32];         /**< Name of the affected parameter, e.g.
                                     "Heart Rate", "SpO2". Null-terminated.      */
    char message[ALERT_MSG_LEN];/**< Human-readable description including the
                                     measured value and normal range.
                                     Null-terminated.                            */
} Alert;

/* =========================================================================
 * Alert Generation
 * ========================================================================= */

/**
 * @brief Generate alert records for every vital sign parameter outside
 *        its normal clinical range.
 *
 * @details
 * Iterates over all four vital sign parameters in this fixed order:
 * -# Heart rate
 * -# Blood pressure (systolic / diastolic)
 * -# Temperature
 * -# SpO2
 *
 * For each parameter that is not ALERT_NORMAL, one Alert entry is written
 * to @p out. Writing stops when either all parameters have been checked or
 * @p max_out entries have been filled — whichever comes first.
 *
 * **Memory safety:** the function never writes beyond `out[max_out - 1]`.
 * Passing `max_out = 0` is valid and writes nothing.
 *
 * @param[in]  vitals   Pointer to a fully populated VitalSigns structure.
 *                      Must not be NULL.
 * @param[out] out      Caller-allocated array to receive Alert records.
 *                      Must hold at least @p max_out elements.
 * @param[in]  max_out  Maximum number of Alert entries to write.
 *
 * @return Number of Alert records written to @p out (0 if all parameters
 *         are normal, or if max_out == 0).
 *
 * @pre  vitals != NULL
 * @pre  out != NULL || max_out == 0
 * @post Return value <= max_out
 *
 * @req REQ-ALT-001
 * @req REQ-ALT-002
 * @req REQ-ALT-003
 * @req REQ-ALT-004
 * @req REQ-ALT-005
 */
int generate_alerts(const VitalSigns *vitals, Alert *out, int max_out);

#endif /* ALERTS_H */
