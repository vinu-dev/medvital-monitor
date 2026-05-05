/**
 * @file session_export.h
 * @brief Session review snapshot export API.
 *
 * @details
 * Provides deterministic local export of a bounded patient-session snapshot,
 * along with shared history-row and alert-row format helpers used by both the
 * dashboard and export artifact.
 *
 * @req SWR-EXP-001
 * @req SWR-EXP-002
 * @req SWR-EXP-003
 */

#ifndef SESSION_EXPORT_H
#define SESSION_EXPORT_H

#include <stddef.h>

#include "alarm_limits.h"
#include "patient.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SESSION_EXPORT_PATH_MAX 512
#define SESSION_EXPORT_FORMAT_VERSION "1.0"

typedef enum {
    SESSION_EXPORT_RESULT_OK = 0,
    SESSION_EXPORT_RESULT_NO_PATIENT,
    SESSION_EXPORT_RESULT_NO_READINGS,
    SESSION_EXPORT_RESULT_EXISTS,
    SESSION_EXPORT_RESULT_PATH_ERROR,
    SESSION_EXPORT_RESULT_IO_ERROR,
    SESSION_EXPORT_RESULT_TIME_ERROR,
    SESSION_EXPORT_RESULT_ARGUMENT_ERROR
} SessionExportResult;

int session_export_build_path(int patient_id,
                              const char *path_override,
                              char *out_path,
                              size_t out_path_len);

int session_export_format_history_row(const VitalSigns *reading,
                                      int sequence_number,
                                      char *out_text,
                                      size_t out_text_len);

int session_export_format_alert_row(const Alert *alert,
                                    char *out_text,
                                    size_t out_text_len);

SessionExportResult session_export_write_snapshot(const PatientRecord *patient,
                                                  int has_patient,
                                                  const AlarmLimits *alarm_limits,
                                                  int sim_enabled,
                                                  int sim_paused,
                                                  const char *path_override,
                                                  int allow_overwrite,
                                                  char *out_path,
                                                  size_t out_path_len);

#ifdef __cplusplus
}
#endif

#endif /* SESSION_EXPORT_H */
