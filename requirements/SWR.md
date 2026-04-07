# Software Requirements Specification (SWR)

**Document ID:** SWR-001-REV-A
**Project:** Patient Vital Signs Monitor
**Version:** 1.0.0
**Date:** 2026-04-06
**Status:** Approved
**Standard:** IEC 62304 §5.2

---

## Purpose

This document specifies the detailed software-level requirements derived from the
System Requirements (SYS). Each entry here maps directly to one or more functions
in the implementation and to one or more test cases. These IDs (`SWR-VIT-xxx`,
`SWR-ALT-xxx`, `SWR-PAT-xxx`) are the `@req` tags used in the Doxygen source
comments.

---

## Module UNIT-VIT — Vital Signs Validation (`vitals.c`)

### SWR-VIT-001 — Heart Rate Check
**Requirement:** `check_heart_rate(int bpm)` shall return:
- `ALERT_CRITICAL` if `bpm < 40` or `bpm > 150`
- `ALERT_WARNING` if `bpm < 60` or `bpm > 100` (and not critical)
- `ALERT_NORMAL` if `60 ≤ bpm ≤ 100`

Negative values shall be treated as CRITICAL.
**Traces to:** SYS-001
**Implemented in:** `src/vitals.c` — `check_heart_rate()`
**Verified by:** `tests/unit/test_vitals.cpp` — `HeartRate.*`

---

### SWR-VIT-002 — Blood Pressure Check
**Requirement:** `check_blood_pressure(int systolic, int diastolic)` shall:
1. Return `ALERT_CRITICAL` if `systolic < 70` or `systolic > 180` or
   `diastolic < 40` or `diastolic > 120`
2. Return `ALERT_WARNING` if `systolic < 90` or `systolic > 140` or
   `diastolic < 60` or `diastolic > 90` (and not critical)
3. Return `ALERT_NORMAL` otherwise

Critical conditions shall be evaluated before warning conditions (fail-safe
ordering).
**Traces to:** SYS-002
**Implemented in:** `src/vitals.c` — `check_blood_pressure()`
**Verified by:** `tests/unit/test_vitals.cpp` — `BloodPressure.*`

---

### SWR-VIT-003 — Temperature Check
**Requirement:** `check_temperature(float temp_c)` shall return:
- `ALERT_CRITICAL` if `temp_c < 35.0` or `temp_c > 39.5`
- `ALERT_WARNING` if `temp_c < 36.1` or `temp_c > 37.2` (and not critical)
- `ALERT_NORMAL` if `36.1 ≤ temp_c ≤ 37.2`

**Traces to:** SYS-003
**Implemented in:** `src/vitals.c` — `check_temperature()`
**Verified by:** `tests/unit/test_vitals.cpp` — `Temperature.*`

---

### SWR-VIT-004 — SpO2 Check
**Requirement:** `check_spo2(int spo2)` shall return:
- `ALERT_CRITICAL` if `spo2 < 90`
- `ALERT_WARNING` if `90 ≤ spo2 < 95`
- `ALERT_NORMAL` if `95 ≤ spo2 ≤ 100`

Values outside 0–100 (e.g. negative) shall be treated as CRITICAL.
**Traces to:** SYS-004
**Implemented in:** `src/vitals.c` — `check_spo2()`
**Verified by:** `tests/unit/test_vitals.cpp` — `SpO2.*`

---

### SWR-VIT-005 — Overall Alert Level
**Requirement:** `overall_alert_level(const VitalSigns *vitals)` shall call
`check_heart_rate()`, `check_blood_pressure()`, `check_temperature()`, and
`check_spo2()` and return the maximum `AlertLevel` value across all four results.
The function shall not modify `*vitals`.
**Traces to:** SYS-006
**Implemented in:** `src/vitals.c` — `overall_alert_level()`
**Verified by:** `tests/unit/test_vitals.cpp` — `OverallAlert.*`

---

### SWR-VIT-006 — BMI Calculation and Category
**Requirement:**
- `calculate_bmi(float weight_kg, float height_m)` shall return
  `weight_kg / (height_m * height_m)`. If `height_m ≤ 0`, return `-1.0f`.
- `bmi_category(float bmi)` shall return:
  `"Invalid"` for `bmi < 0`, `"Underweight"` for `bmi < 18.5`,
  `"Normal weight"` for `18.5 ≤ bmi < 25.0`, `"Overweight"` for
  `25.0 ≤ bmi < 30.0`, `"Obese"` for `bmi ≥ 30.0`.

Returned strings are null-terminated literals. The caller shall not free them.
**Traces to:** SYS-007
**Implemented in:** `src/vitals.c` — `calculate_bmi()`, `bmi_category()`
**Verified by:** `tests/unit/test_vitals.cpp` — `BMI.*`

---

### SWR-VIT-007 — Alert Level String Conversion
**Requirement:** `alert_level_str(AlertLevel level)` shall return `"NORMAL"`,
`"WARNING"`, or `"CRITICAL"` for the corresponding enum values. For any
unrecognised value, it shall return `"UNKNOWN"`.
**Traces to:** SYS-005, SYS-011
**Implemented in:** `src/vitals.c` — `alert_level_str()`
**Verified by:** `tests/unit/test_vitals.cpp` — `AlertStr.*`

---

## Module UNIT-ALT — Alert Generation (`alerts.c`)

### SWR-ALT-001 — One Alert Per Abnormal Parameter
**Requirement:** `generate_alerts()` shall write exactly one `Alert` record to
`out[]` for each parameter whose classification is not `ALERT_NORMAL`, in the
fixed order: heart rate, blood pressure, temperature, SpO2.
**Traces to:** SYS-005
**Implemented in:** `src/alerts.c` — `generate_alerts()`
**Verified by:** `tests/unit/test_alerts.cpp` — `GenerateAlerts.REQ_ALT_002_*`

---

### SWR-ALT-002 — Zero Alerts for Normal Vitals
**Requirement:** `generate_alerts()` shall return `0` and write nothing to `out[]`
when all four parameters are within their normal ranges.
**Traces to:** SYS-005
**Implemented in:** `src/alerts.c` — `generate_alerts()`
**Verified by:** `tests/unit/test_alerts.cpp` — `GenerateAlerts.REQ_ALT_001_*`

---

### SWR-ALT-003 — Output Buffer Cap
**Requirement:** `generate_alerts()` shall never write more than `max_out` entries
to `out[]`. Passing `max_out = 0` is valid and shall result in no writes and a
return value of `0`.
**Traces to:** SYS-012
**Implemented in:** `src/alerts.c` — `generate_alerts()`
**Verified by:** `tests/unit/test_alerts.cpp` — `GenerateAlerts.REQ_ALT_004_*`

---

### SWR-ALT-004 — Alert Field Population
**Requirement:** For each written `Alert` record:
- `Alert.parameter` shall be a null-terminated string of at most 31 characters
  identifying the parameter (e.g. `"Heart Rate"`, `"SpO2"`).
- `Alert.message` shall be a null-terminated string of at most
  `ALERT_MSG_LEN - 1` (95) characters describing the deviation.
- Both fields shall be null-terminated regardless of content length.

**Traces to:** SYS-005
**Implemented in:** `src/alerts.c` — `generate_alerts()` via `PUSH_ALERT` macro
**Verified by:** `tests/unit/test_alerts.cpp` — `GenerateAlerts.REQ_ALT_005_*`

---

## Module UNIT-PAT — Patient Record Management (`patient.c`)

### SWR-PAT-001 — Patient Initialisation
**Requirement:** `patient_init()` shall zero-fill the entire `PatientRecord`
before populating fields. The `name` field shall be copied with truncation to
`MAX_NAME_LEN - 1` characters and guaranteed null-termination at index
`MAX_NAME_LEN - 1`. `reading_count` shall be `0` after initialisation.
**Traces to:** SYS-008, SYS-012
**Implemented in:** `src/patient.c` — `patient_init()`
**Verified by:** `tests/unit/test_patient.cpp` — `PatientInit.*`

---

### SWR-PAT-002 — Add Vital Sign Reading
**Requirement:** `patient_add_reading()` shall copy the `VitalSigns` structure
by value into `readings[reading_count]` and increment `reading_count` by 1.
If `reading_count == MAX_READINGS`, the record shall remain unchanged and the
function shall return `0`. On success, the function shall return `1`.
**Traces to:** SYS-009, SYS-010
**Implemented in:** `src/patient.c` — `patient_add_reading()`
**Verified by:** `tests/unit/test_patient.cpp` — `PatientAddReading.*`

---

### SWR-PAT-003 — Latest Reading Access
**Requirement:** `patient_latest_reading()` shall return a pointer to
`readings[reading_count - 1]`. If `reading_count == 0`, it shall return `NULL`.
The pointer shall remain valid for the lifetime of the `PatientRecord`.
**Traces to:** SYS-009
**Implemented in:** `src/patient.c` — `patient_latest_reading()`
**Verified by:** `tests/unit/test_patient.cpp` — `PatientLatestReading.*`

---

### SWR-PAT-004 — Current Patient Status
**Requirement:** `patient_current_status()` shall return
`overall_alert_level(patient_latest_reading())`. If `patient_latest_reading()`
returns `NULL` (no readings), the function shall return `ALERT_NORMAL`.
**Traces to:** SYS-006, SYS-011
**Implemented in:** `src/patient.c` — `patient_current_status()`
**Verified by:** `tests/unit/test_patient.cpp` — `PatientStatus.*`

---

### SWR-PAT-005 — Buffer Full Detection
**Requirement:** `patient_is_full()` shall return a non-zero value when
`reading_count >= MAX_READINGS`, and `0` otherwise.
**Traces to:** SYS-010
**Implemented in:** `src/patient.c` — `patient_is_full()`
**Verified by:** `tests/unit/test_patient.cpp` — `PatientIsFull.*`

---

### SWR-PAT-006 — Patient Summary Display
**Requirement:** `patient_print_summary()` shall write to `stdout` a formatted
block containing: patient name, ID, age, BMI with WHO category, reading count,
the latest vital signs with per-parameter classifications, the aggregate alert
level, and the text of all active alert messages. If no readings exist, the
vitals and alerts sections shall be omitted.
**Traces to:** SYS-011
**Implemented in:** `src/patient.c` — `patient_print_summary()`
**Verified by:** `tests/unit/test_patient.cpp` — `PatientPrintSummary.*`

---

## Module UNIT-GUI — Graphical User Interface (`gui_main.c`, `gui_auth.c`)

### SWR-GUI-001 — Login Credential Validation
**Requirement:** `auth_validate(username, password)` shall return `1` if and
only if both the username and password exactly match the built-in credential.
It shall return `0` for any other input, including empty strings and NULL
pointers. NULL inputs shall not cause undefined behaviour.
**Traces to:** SYS-013
**Implemented in:** `src/gui_auth.c` — `auth_validate()`
**Verified by:** `tests/unit/test_auth.cpp` — `AuthValidation.*`

---

### SWR-GUI-002 — Session Management (Login / Logout)
**Requirement:** The application shall:
1. Display the login window on launch; no dashboard controls shall be
   accessible until `auth_validate()` returns `1`.
2. On successful authentication, record the display name via
   `auth_display_name()` and display it in the dashboard header.
3. On Logout, destroy the dashboard window, clear all session data, and
   recreate the login window.
4. On authentication failure, display an error message and clear the
   password field; do not identify which field was incorrect.

**Traces to:** SYS-013
**Implemented in:** `src/gui_main.c` — `attempt_login()`, `login_proc()`,
`dash_proc()` (IDC_BTN_LOGOUT handler)
**Verified by:** `tests/unit/test_auth.cpp` — `AuthDisplayName.*`

---

### SWR-GUI-003 — Colour-Coded Vital Signs Display
**Requirement:** The dashboard shall paint four status tiles (Heart Rate,
Blood Pressure, Temperature, SpO2). Each tile shall:
- Display the parameter label, measured value with unit, and a status badge.
- Use a green background and text for `ALERT_NORMAL`.
- Use an amber background and text for `ALERT_WARNING`.
- Use a red background and text for `ALERT_CRITICAL`.
The aggregate status banner shall use the same colour mapping.
All tiles and the banner shall repaint automatically each time
`update_dashboard()` is called.

**Traces to:** SYS-014, SYS-005, SYS-006
**Implemented in:** `src/gui_main.c` — `paint_tile()`, `paint_tiles()`,
`paint_status_banner()`, `update_dashboard()`
**Verified by:** Verified structurally by `AuthValidation.*`; visual
verification performed via GUI demonstration.

---

### SWR-GUI-004 — Patient Data Entry via GUI
**Requirement:** The dashboard shall provide labelled edit controls for:
patient ID, name, age, weight, height; heart rate, systolic BP, diastolic BP,
temperature, SpO2. The "Admit / Refresh" button shall call `patient_init()`.
The "Add Reading" button shall call `patient_add_reading()`. Invalid or empty
fields shall produce a `MessageBox` error and set focus to the offending
control. The dashboard shall include two pre-built clinical demonstration
scenarios (deterioration, bradycardia).

**Traces to:** SYS-014, SYS-008, SYS-009
**Implemented in:** `src/gui_main.c` — `create_dash_controls()`, `do_admit()`,
`do_add_reading()`, `do_scenario()`
**Verified by:** Verified via GUI demonstration.

---

### SWR-GUI-005 — Hardware Abstraction Layer Interface
**Requirement:** The GUI shall acquire vital signs exclusively through the
HAL interface defined in `hw_vitals.h` (`hw_init()`, `hw_get_next_reading()`).
No GUI source file shall directly reference simulation-specific data structures.
Swapping `sim_vitals.c` for a hardware driver translation unit shall be
sufficient to connect real hardware without modifying `gui_main.c`.
**Traces to:** UNS-015, SYS-015
**Implemented in:** `include/hw_vitals.h`, `src/gui_main.c`
**Verified by:** Architecture review — `gui_main.c` contains no references to
`SIM_SEQUENCE` or any sim-specific symbol.

---

### SWR-GUI-006 — Simulated Vital Signs Data Feed
**Requirement:** The simulation back-end (`sim_vitals.c`) shall:
1. Implement a 20-entry clinical scenario table cycling through four phases:
   STABLE NORMAL (indices 0–4), DETERIORATING to WARNING (5–8), CRITICAL
   (9–11), and RECOVERING (12–18), with entry 19 returning to stable.
2. Expose readings via `hw_get_next_reading()` with automatic wrap-around.
3. Be reset by `hw_init()` to index 0.
4. Use only static storage — no heap allocation.
**Traces to:** UNS-015, SYS-012, SYS-015
**Implemented in:** `src/sim_vitals.c`
**Verified by:** Verified visually via GUI demonstration (tiles cycle through
NORMAL → WARNING → CRITICAL → NORMAL colour states over time).

---

## Revision History

| Rev | Date       | Author          | Description          |
|-----|------------|-----------------|----------------------|
| A   | 2026-04-06 | vinu-engineer   | Initial release      |
| B   | 2026-04-07 | vinu-engineer   | Added UNIT-GUI module (SWR-GUI-001..004) |
| C   | 2026-04-07 | vinu-engineer   | Added SWR-GUI-005 (HAL), SWR-GUI-006 (sim) |
