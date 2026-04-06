/**
 * @file alerts.c
 * @brief Implementation of alert record generation from vital sign readings.
 *
 * @details
 * Implements generate_alerts() declared in alerts.h. Each abnormal
 * parameter produces exactly one Alert record written into the caller's
 * buffer. The internal PUSH_ALERT macro encapsulates the bounds check and
 * string formatting to keep the function body concise and auditable.
 *
 * ### Implementation Notes
 * - `_CRT_SECURE_NO_WARNINGS` suppresses MSVC deprecation warnings for
 *   `strncpy` and `snprintf`. These functions are used with explicit
 *   length limits and guaranteed null-termination, so the usage is safe.
 * - The macro parameter is named `_al` (not `level`) to avoid shadowing
 *   the struct field `Alert.level` inside the macro expansion body.
 *
 * @version 1.0.0
 * @date    2026-04-06
 * @author  vinu-engineer
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "alerts.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Generate structured alert records for out-of-range vital signs.
 *
 * @details
 * Internal flow for each parameter:
 * -# Call the corresponding check_*() function.
 * -# If the result is not ALERT_NORMAL, write an Alert record via PUSH_ALERT.
 * -# PUSH_ALERT enforces the max_out cap before writing.
 *
 * The PUSH_ALERT macro copies the parameter name with strncpy (max 31 chars
 * + null) and formats the message with snprintf (max ALERT_MSG_LEN - 1 chars
 * + null), guaranteeing null-termination in both fields.
 */
int generate_alerts(const VitalSigns *vitals, Alert *out, int max_out)
{
    int count = 0;
    AlertLevel lvl;

/**
 * @internal
 * @brief Write one Alert record if count < max_out.
 * @param _al   AlertLevel value (renamed to avoid shadowing Alert.level).
 * @param param Parameter name string literal.
 * @param fmt   printf-style format string for the message field.
 */
#define PUSH_ALERT(_al, param, fmt, ...)                                    \
    do {                                                                     \
        if (count < max_out) {                                               \
            out[count].level = (_al);                                        \
            strncpy(out[count].parameter, (param), 31);                      \
            out[count].parameter[31] = '\0';                                 \
            snprintf(out[count].message, ALERT_MSG_LEN, fmt, __VA_ARGS__);   \
            count++;                                                          \
        }                                                                     \
    } while (0)

    lvl = check_heart_rate(vitals->heart_rate);
    if (lvl != ALERT_NORMAL)
        PUSH_ALERT(lvl, "Heart Rate",
                   "Heart rate %d bpm [normal 60-100]",
                   vitals->heart_rate);

    lvl = check_blood_pressure(vitals->systolic_bp, vitals->diastolic_bp);
    if (lvl != ALERT_NORMAL)
        PUSH_ALERT(lvl, "Blood Pressure",
                   "BP %d/%d mmHg [normal 90-140 / 60-90]",
                   vitals->systolic_bp, vitals->diastolic_bp);

    lvl = check_temperature(vitals->temperature);
    if (lvl != ALERT_NORMAL)
        PUSH_ALERT(lvl, "Temperature",
                   "Temp %.1f C [normal 36.1-37.2]",
                   (double)vitals->temperature);

    lvl = check_spo2(vitals->spo2);
    if (lvl != ALERT_NORMAL)
        PUSH_ALERT(lvl, "SpO2",
                   "SpO2 %d%% [normal 95-100%%]",
                   vitals->spo2);

#undef PUSH_ALERT
    return count;
}
