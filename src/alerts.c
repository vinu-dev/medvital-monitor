#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "alerts.h"
#include <stdio.h>
#include <string.h>

int generate_alerts(const VitalSigns *vitals, Alert *out, int max_out)
{
    int count = 0;
    AlertLevel lvl;

/* Rename macro param to _al to avoid shadowing the struct field 'level' */
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
