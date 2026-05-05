/**
 * @file session_export.c
 * @brief Session review snapshot export implementation.
 *
 * @details
 * Implements deterministic local path resolution, overwrite-aware export
 * policy, restrictive file creation, and snapshot serialization using only
 * stack/static storage.
 *
 * @req SWR-EXP-001
 * @req SWR-EXP-002
 * @req SWR-EXP-003
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "session_export.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <sys/stat.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifndef _SH_DENYRW
#define _SH_DENYRW 0x10
#endif

#define SESSION_EXPORT_FILENAME_PREFIX "session-review-patient-"
#define SESSION_EXPORT_FILENAME_SUFFIX ".txt"

static int copy_path(const char *src, char *dst, size_t dst_len)
{
    size_t src_len;

    if (src == NULL || dst == NULL || dst_len == 0u) {
        return 0;
    }

    src_len = strlen(src);
    if (src_len >= dst_len) {
        return 0;
    }

    memcpy(dst, src, src_len + 1u);
    return 1;
}

static int file_exists(const char *path)
{
    if (path == NULL || path[0] == '\0') {
        return 0;
    }

#if defined(_WIN32)
    return _access(path, 0) == 0;
#else
    return access(path, F_OK) == 0;
#endif
}

static int format_timestamp_utc(char *out_text, size_t out_text_len)
{
    time_t now;
    struct tm tm_utc;
    int written;

    if (out_text == NULL || out_text_len == 0u) {
        return 0;
    }

    now = time(NULL);
    if (now == (time_t)-1) {
        return 0;
    }

#if defined(_WIN32)
    if (gmtime_s(&tm_utc, &now) != 0) {
        return 0;
    }
#else
    if (gmtime_r(&now, &tm_utc) == NULL) {
        return 0;
    }
#endif

    written = snprintf(out_text, out_text_len,
                       "%04d-%02d-%02dT%02d:%02d:%02dZ",
                       tm_utc.tm_year + 1900,
                       tm_utc.tm_mon + 1,
                       tm_utc.tm_mday,
                       tm_utc.tm_hour,
                       tm_utc.tm_min,
                       tm_utc.tm_sec);
    return written > 0 && (size_t)written < out_text_len;
}

static FILE *open_write_restricted(const char *path, int allow_overwrite)
{
#if defined(_WIN32)
    int fd = -1;
    int oflag = allow_overwrite
        ? (_O_CREAT | _O_WRONLY | _O_TRUNC)
        : (_O_CREAT | _O_WRONLY | _O_EXCL);
    FILE *fp;

    if (_sopen_s(&fd, path, oflag, _SH_DENYRW,
                 _S_IREAD | _S_IWRITE) != 0 || fd == -1) {
        return NULL;
    }

    fp = _fdopen(fd, "w");
    if (fp == NULL) {
        _close(fd);
        return NULL;
    }

    return fp;
#else
    int fd;
    int oflag = allow_overwrite
        ? (O_CREAT | O_WRONLY | O_TRUNC)
        : (O_CREAT | O_WRONLY | O_EXCL);
    FILE *fp;

    fd = open(path, oflag, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return NULL;
    }

    fp = fdopen(fd, "w");
    if (fp == NULL) {
        close(fd);
        return NULL;
    }

    return fp;
#endif
}

static SessionExportResult write_snapshot_body(FILE *fp,
                                               const PatientRecord *patient,
                                               const AlarmLimits *alarm_limits,
                                               int sim_enabled,
                                               int sim_paused)
{
    Alert alerts[MAX_ALERTS];
    AlertLevel status;
    const VitalSigns *latest;
    char line[256];
    char timestamp[32];
    float bmi;
    int alert_count;
    int i;

    if (!format_timestamp_utc(timestamp, sizeof(timestamp))) {
        return SESSION_EXPORT_RESULT_TIME_ERROR;
    }

    latest = patient_latest_reading(patient);
    if (latest == NULL) {
        return SESSION_EXPORT_RESULT_NO_READINGS;
    }

    bmi = calculate_bmi(patient->info.weight_kg, patient->info.height_m);
    status = patient_current_status(patient);
    alert_count = generate_alerts(latest, alerts, MAX_ALERTS);

    if (fprintf(fp, "Session Review Snapshot\n") < 0 ||
        fprintf(fp, "Format Version: %s\n", SESSION_EXPORT_FORMAT_VERSION) < 0 ||
        fprintf(fp, "Generated At (UTC): %s\n", timestamp) < 0 ||
        fprintf(fp, "Snapshot Scope: Current admitted session only; not a live monitor feed.\n") < 0 ||
        fprintf(fp, "Reading Capacity: %d / %d\n\n",
                patient->reading_count, MAX_READINGS) < 0) {
        return SESSION_EXPORT_RESULT_IO_ERROR;
    }

    if (fprintf(fp, "Patient Demographics\n") < 0 ||
        fprintf(fp, "  ID             : %d\n", patient->info.id) < 0 ||
        fprintf(fp, "  Name           : %s\n", patient->info.name) < 0 ||
        fprintf(fp, "  Age            : %d years\n", patient->info.age) < 0 ||
        fprintf(fp, "  Weight         : %.1f kg\n", patient->info.weight_kg) < 0 ||
        fprintf(fp, "  Height         : %.2f m\n", patient->info.height_m) < 0 ||
        fprintf(fp, "  BMI            : %.1f (%s)\n\n", bmi, bmi_category(bmi)) < 0) {
        return SESSION_EXPORT_RESULT_IO_ERROR;
    }

    if (fprintf(fp, "Mode Context\n") < 0 ||
        fprintf(fp, "  Simulation     : %s\n", sim_enabled ? "ENABLED" : "DISABLED") < 0 ||
        fprintf(fp, "  Acquisition    : %s\n\n",
                sim_enabled ? (sim_paused ? "PAUSED" : "LIVE") : "DEVICE MODE") < 0) {
        return SESSION_EXPORT_RESULT_IO_ERROR;
    }

    if (fprintf(fp, "Alarm Limit Context\n") < 0 ||
        fprintf(fp, "  Heart Rate     : low %d bpm / high %d bpm\n",
                alarm_limits->hr_low, alarm_limits->hr_high) < 0 ||
        fprintf(fp, "  Blood Pressure : SBP low %d mmHg / high %d mmHg; DBP low %d mmHg / high %d mmHg\n",
                alarm_limits->sbp_low, alarm_limits->sbp_high,
                alarm_limits->dbp_low, alarm_limits->dbp_high) < 0 ||
        fprintf(fp, "  Temperature    : low %.1f C / high %.1f C\n",
                alarm_limits->temp_low, alarm_limits->temp_high) < 0 ||
        fprintf(fp, "  SpO2           : low %d%% (no configured high limit)\n",
                alarm_limits->spo2_low) < 0 ||
        fprintf(fp, "  Respiration    : low %d br/min / high %d br/min\n\n",
                alarm_limits->rr_low, alarm_limits->rr_high) < 0) {
        return SESSION_EXPORT_RESULT_IO_ERROR;
    }

    if (fprintf(fp, "Latest Vital Signs\n") < 0 ||
        fprintf(fp, "  Heart Rate     : %d bpm [%s]\n",
                latest->heart_rate,
                alert_level_str(check_heart_rate(latest->heart_rate))) < 0 ||
        fprintf(fp, "  Blood Pressure : %d/%d mmHg [%s]\n",
                latest->systolic_bp,
                latest->diastolic_bp,
                alert_level_str(check_blood_pressure(latest->systolic_bp,
                                                     latest->diastolic_bp))) < 0 ||
        fprintf(fp, "  Temperature    : %.1f C [%s]\n",
                latest->temperature,
                alert_level_str(check_temperature(latest->temperature))) < 0 ||
        fprintf(fp, "  SpO2           : %d%% [%s]\n",
                latest->spo2,
                alert_level_str(check_spo2(latest->spo2))) < 0) {
        return SESSION_EXPORT_RESULT_IO_ERROR;
    }

    if (latest->respiration_rate != 0) {
        if (fprintf(fp, "  Respiration    : %d br/min [%s]\n",
                    latest->respiration_rate,
                    alert_level_str(check_respiration_rate(latest->respiration_rate))) < 0) {
            return SESSION_EXPORT_RESULT_IO_ERROR;
        }
    } else if (fprintf(fp, "  Respiration    : Not recorded\n") < 0) {
        return SESSION_EXPORT_RESULT_IO_ERROR;
    }

    if (fprintf(fp, "  Overall Status : %s\n\n", alert_level_str(status)) < 0) {
        return SESSION_EXPORT_RESULT_IO_ERROR;
    }

    if (fprintf(fp, "Active Alerts\n") < 0) {
        return SESSION_EXPORT_RESULT_IO_ERROR;
    }

    if (alert_count == 0) {
        if (fprintf(fp, "  No active alerts\n\n") < 0) {
            return SESSION_EXPORT_RESULT_IO_ERROR;
        }
    } else {
        for (i = 0; i < alert_count; ++i) {
            if (!session_export_format_alert_row(&alerts[i], line, sizeof(line)) ||
                fprintf(fp, "  %s\n", line) < 0) {
                return SESSION_EXPORT_RESULT_IO_ERROR;
            }
        }
        if (fprintf(fp, "\n") < 0) {
            return SESSION_EXPORT_RESULT_IO_ERROR;
        }
    }

    if (fprintf(fp, "Reading History\n") < 0) {
        return SESSION_EXPORT_RESULT_IO_ERROR;
    }

    for (i = 0; i < patient->reading_count; ++i) {
        if (!session_export_format_history_row(&patient->readings[i], i + 1,
                                               line, sizeof(line)) ||
            fprintf(fp, "  %s\n", line) < 0) {
            return SESSION_EXPORT_RESULT_IO_ERROR;
        }
    }

    if (fprintf(fp, "\nRetention Boundary\n") < 0 ||
        fprintf(fp, "  History in this snapshot is limited to the current admitted session and at most %d readings.\n",
                MAX_READINGS) < 0 ||
        fprintf(fp, "  This export is a point-in-time snapshot and shall not be treated as a live monitor feed.\n") < 0) {
        return SESSION_EXPORT_RESULT_IO_ERROR;
    }

    return SESSION_EXPORT_RESULT_OK;
}

int session_export_build_path(int patient_id,
                              const char *path_override,
                              char *out_path,
                              size_t out_path_len)
{
    int written;

    if (out_path == NULL || out_path_len == 0u || patient_id <= 0) {
        return 0;
    }

    out_path[0] = '\0';

    if (path_override != NULL && path_override[0] != '\0') {
        return copy_path(path_override, out_path, out_path_len);
    }

#if defined(_WIN32)
    {
        char exe_path[SESSION_EXPORT_PATH_MAX] = {0};
        char *last_sep = NULL;
        char *cursor = NULL;
        DWORD len = GetModuleFileNameA(NULL, exe_path,
                                       (DWORD)(sizeof(exe_path) - 1u));

        if (len == 0u || len >= (DWORD)(sizeof(exe_path) - 1u)) {
            return 0;
        }

        cursor = exe_path;
        while (*cursor != '\0') {
            if (*cursor == '\\' || *cursor == '/') {
                last_sep = cursor;
            }
            ++cursor;
        }

        if (last_sep != NULL) {
            *(last_sep + 1) = '\0';
        } else {
            exe_path[0] = '\0';
        }

        written = snprintf(out_path, out_path_len, "%s%s%d%s",
                           exe_path,
                           SESSION_EXPORT_FILENAME_PREFIX,
                           patient_id,
                           SESSION_EXPORT_FILENAME_SUFFIX);
    }
#else
    written = snprintf(out_path, out_path_len, "%s%d%s",
                       SESSION_EXPORT_FILENAME_PREFIX,
                       patient_id,
                       SESSION_EXPORT_FILENAME_SUFFIX);
#endif

    return written > 0 && (size_t)written < out_path_len;
}

int session_export_format_history_row(const VitalSigns *reading,
                                      int sequence_number,
                                      char *out_text,
                                      size_t out_text_len)
{
    int written;

    if (reading == NULL || out_text == NULL || out_text_len == 0u ||
        sequence_number <= 0) {
        return 0;
    }

    if (reading->respiration_rate != 0) {
        written = snprintf(out_text, out_text_len,
                           "#%d  HR %d | BP %d/%d | Temp %.1f C | SpO2 %d%% | RR %d br/min  [%s]",
                           sequence_number,
                           reading->heart_rate,
                           reading->systolic_bp,
                           reading->diastolic_bp,
                           reading->temperature,
                           reading->spo2,
                           reading->respiration_rate,
                           alert_level_str(overall_alert_level(reading)));
    } else {
        written = snprintf(out_text, out_text_len,
                           "#%d  HR %d | BP %d/%d | Temp %.1f C | SpO2 %d%%  [%s]",
                           sequence_number,
                           reading->heart_rate,
                           reading->systolic_bp,
                           reading->diastolic_bp,
                           reading->temperature,
                           reading->spo2,
                           alert_level_str(overall_alert_level(reading)));
    }

    return written > 0 && (size_t)written < out_text_len;
}

int session_export_format_alert_row(const Alert *alert,
                                    char *out_text,
                                    size_t out_text_len)
{
    int written;

    if (alert == NULL || out_text == NULL || out_text_len == 0u) {
        return 0;
    }

    written = snprintf(out_text, out_text_len, "[%s]  %s",
                       alert_level_str(alert->level), alert->message);
    return written > 0 && (size_t)written < out_text_len;
}

SessionExportResult session_export_write_snapshot(const PatientRecord *patient,
                                                  int has_patient,
                                                  const AlarmLimits *alarm_limits,
                                                  int sim_enabled,
                                                  int sim_paused,
                                                  const char *path_override,
                                                  int allow_overwrite,
                                                  char *out_path,
                                                  size_t out_path_len)
{
    char resolved_path[SESSION_EXPORT_PATH_MAX];
    char *target_path = resolved_path;
    size_t target_path_len = sizeof(resolved_path);
    FILE *fp;
    SessionExportResult result;

    if (patient == NULL || alarm_limits == NULL) {
        return SESSION_EXPORT_RESULT_ARGUMENT_ERROR;
    }

    if (!has_patient) {
        return SESSION_EXPORT_RESULT_NO_PATIENT;
    }

    if (patient->reading_count <= 0) {
        return SESSION_EXPORT_RESULT_NO_READINGS;
    }

    if (out_path != NULL && out_path_len > 0u) {
        target_path = out_path;
        target_path_len = out_path_len;
        out_path[0] = '\0';
    }

    if (!session_export_build_path(patient->info.id, path_override,
                                   target_path, target_path_len)) {
        return SESSION_EXPORT_RESULT_PATH_ERROR;
    }

    if (!allow_overwrite && file_exists(target_path)) {
        return SESSION_EXPORT_RESULT_EXISTS;
    }

    fp = open_write_restricted(target_path, allow_overwrite != 0);
    if (fp == NULL) {
        if (!allow_overwrite && file_exists(target_path)) {
            return SESSION_EXPORT_RESULT_EXISTS;
        }
        return SESSION_EXPORT_RESULT_IO_ERROR;
    }

    result = write_snapshot_body(fp, patient, alarm_limits,
                                 sim_enabled, sim_paused);
    if (fclose(fp) != 0 && result == SESSION_EXPORT_RESULT_OK) {
        result = SESSION_EXPORT_RESULT_IO_ERROR;
    }

    if (result != SESSION_EXPORT_RESULT_OK) {
        remove(target_path);
    }

    return result;
}
