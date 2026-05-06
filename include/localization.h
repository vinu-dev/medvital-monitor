/**
 * @file localization.h
 * @brief Multi-language support for Patient Vital Signs Monitor
 *
 * Supports: English, Spanish, French, German
 * IEC 62304 Class B - localization layer
 */

#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#ifdef __cplusplus
extern "C" {
#endif

/* ===================================================================
 * Language enumeration
 * =================================================================== */
typedef enum {
    LOC_LANG_ENGLISH = 0,
    LOC_LANG_SPANISH = 1,
    LOC_LANG_FRENCH = 2,
    LOC_LANG_GERMAN = 3,
    LOC_LANG_COUNT = 4
} Language;

/* ===================================================================
 * UI String IDs (enum for easy reference)
 * =================================================================== */
typedef enum {
    /* Common UI */
    STR_APP_TITLE,
    STR_VERSION,
    STR_SIGN_IN,
    STR_LOGOUT,
    STR_SETTINGS,
    STR_ABOUT,
    STR_OK,
    STR_CANCEL,
    STR_SAVE,
    STR_RESET,

    /* Login */
    STR_USERNAME,
    STR_PASSWORD,
    STR_LOGIN_ERROR,
    STR_INVALID_CREDENTIALS,

    /* Patient */
    STR_PATIENT_ID,
    STR_PATIENT_NAME,
    STR_AGE,
    STR_WEIGHT_KG,
    STR_HEIGHT_M,
    STR_ADMIT,
    STR_ADMIT_REFRESH,

    /* Vitals */
    STR_HR_BPM,
    STR_SYSTOLIC,
    STR_DIASTOLIC,
    STR_TEMP_C,
    STR_SPO2_PERCENT,
    STR_RR_BR_MIN,
    STR_BMI,
    STR_NEWS2_SCORE,

    /* Buttons */
    STR_ADD_READING,
    STR_CLEAR_SESSION,
    STR_DEMO_DETERIORATION,
    STR_DEMO_BRADYCARDIA,
    STR_PAUSE_SIM,
    STR_RESUME_SIM,

    /* Alerts */
    STR_ACTIVE_ALERTS,
    STR_READING_HISTORY,
    STR_SESSION_ALARM_EVENTS,
    STR_ALERT_NORMAL,
    STR_ALERT_WARNING,
    STR_ALERT_CRITICAL,

    /* Simulation */
    STR_SIM_MODE,
    STR_SIMULATION_DISABLED,
    STR_SIMULATION_ENABLED,
    STR_DEVICE_MODE,
    STR_IN_SIMULATION_MODE,

    /* Settings */
    STR_LANGUAGE,
    STR_READABILITY_MODE,
    STR_ENABLE_READABILITY_MODE,
    STR_READABILITY_MODE_NOTE,
    STR_ALARM_LIMITS,
    STR_USER_MANAGEMENT,
    STR_MY_ACCOUNT,
    STR_CHANGE_PASSWORD,
    STR_ADD_USER,
    STR_REMOVE_USER,
    STR_ROLE,
    STR_ADMIN,
    STR_CLINICAL,

    /* Status messages */
    STR_STATUS_NORMAL,
    STR_STATUS_WARNING,
    STR_STATUS_CRITICAL,
    STR_DEVICE_MODE_MSG,
    STR_SIM_MODE_MSG,

    STR_COUNT  /* Total number of strings */
} StringID;

/* ===================================================================
 * Functions
 * =================================================================== */

/**
 * Set the current language
 * @param lang Language enumeration value
 */
void localization_set_language(Language lang);

/**
 * Get the current language
 * @return Current Language
 */
Language localization_get_language(void);

/**
 * Get a localized string
 * @param id String ID
 * @return Pointer to localized string (null-terminated)
 */
const char* localization_get_string(StringID id);

/**
 * Get language name
 * @param lang Language enumeration
 * @return Language name in English (e.g., "English", "Español")
 */
const char* localization_get_language_name(Language lang);

#ifdef __cplusplus
}
#endif

#endif /* LOCALIZATION_H */
