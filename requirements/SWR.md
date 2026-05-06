# Software Requirements Specification (SWR)

**Document ID:** SWR-001-REV-I
**Project:** Patient Vital Signs Monitor
**Version:** 2.7.0
**Date:** 2026-05-03
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

### SWR-VIT-008 — Respiration Rate Check
**Requirement:** `check_respiration_rate(int rr_bpm)` shall return:
- `ALERT_CRITICAL` if `rr_bpm ≤ 8` (apnoea/severe bradypnoea) or `rr_bpm ≥ 25` (severe tachypnoea)
- `ALERT_WARNING`  if `9 ≤ rr_bpm ≤ 11` (bradypnoea) or `21 ≤ rr_bpm ≤ 24` (tachypnoea)
- `ALERT_NORMAL`   if `12 ≤ rr_bpm ≤ 20` (normal adult respiratory rate)

The `VitalSigns.respiration_rate` field shall be the sixth member of the struct.
A value of `0` shall be treated as "not measured" by `overall_alert_level()`:
the RR check shall be skipped and `ALERT_NORMAL` assumed, so that absence of an
RR sensor does not generate spurious alarms.

The dashboard shall display a dedicated **RESP RATE** tile (6th tile, second row,
middle column in the 3×2 grid). The manual entry panel shall include an
**RR (br/min)** input field. The simulation sequence shall include clinically
realistic RR values (12–20 in the normal phase, 21–24 in deterioration, 26–28
in the critical phase, recovering to 14–19).

**Threshold source:** Royal College of Physicians NEWS2 / NICE guidelines.
**Traces to:** SYS-018
**Implemented in:** `src/vitals.c` — `check_respiration_rate()`, `overall_alert_level()`;
`src/sim_vitals.c` — respiration_rate in all 20 SIM_SEQUENCE entries;
`src/gui_main.c` — RESP RATE tile, IDC_VIT_RR field;
`src/alerts.c` — RR alert generation
**Verified by:** `tests/unit/test_vitals.cpp` — `RespRate.*` (12 tests),
`OverallAlert.SWR_VIT_008_*` (3 tests)

---

## Module UNIT-NEW — NEWS2 Early Warning Score (`news2.c`)

### SWR-NEW-001 — NEWS2 Aggregate Clinical Risk Score
**Requirement:** The system shall implement the National Early Warning Score 2 (NEWS2)
as defined by the Royal College of Physicians (2017). `news2_calculate()` shall:

1. Compute individual sub-scores from a `VitalSigns` structure using the RCP NEWS2
   scoring tables for each of five physiological parameters plus AVPU:

| Parameter         | 3        | 2          | 1          | 0           | 1          | 2          | 3          |
|-------------------|----------|------------|------------|-------------|------------|------------|------------|
| Resp Rate (br/min)| ≤8       |            | 9–11       | 12–20       |            | 21–24      | ≥25        |
| SpO2 (%)          | ≤91      | 92–93      | 94–95      | ≥96         |            |            |            |
| Systolic BP (mmHg)| ≤90      | 91–100     | 101–110    | 111–219     |            |            | ≥220       |
| HR (bpm)          | ≤40      |            | 41–50      | 51–90       | 91–110     | 111–130    | ≥131       |
| Temperature (°C)  | ≤35.0    |            | 35.1–36.0  | 36.1–38.0   | 38.1–39.0  | ≥39.1      |            |
| AVPU              | ≠ Alert  |            |            | Alert       |            |            |            |

2. Sum all sub-scores to a `total_score`.
3. Classify risk as follows:
   - `total_score ≥ 7` → `NEWS2_HIGH` ("EMERGENCY")
   - `total_score 5–6` OR any single score = 3 → `NEWS2_MEDIUM` ("Urgent review")
   - `total_score 1–4` → `NEWS2_LOW_M` ("Increase monitoring")
   - `total_score = 0` → `NEWS2_LOW` ("Routine monitoring")
4. Populate all fields of the caller-supplied `News2Result` structure,
   including `risk_label` and `response` string literals (never NULL).
5. If `respiration_rate == 0` (not measured), the RR sub-score shall be 0
   and shall not inflate the aggregate total.

The dashboard shall display the NEWS2 score in the 6th tile (3rd column, 2nd row)
colour-coded: green (LOW/LOW_M), amber (MEDIUM), red (HIGH).

**Traces to:** SYS-019
**Implemented in:** `src/news2.c` — `news2_score_hr()`, `news2_score_rr()`,
`news2_score_spo2()`, `news2_score_sbp()`, `news2_score_temp()`, `news2_calculate()`;
`src/gui_main.c` — 6th tile (NEWS2 SCORE)
**Verified by:** `tests/unit/test_news2.cpp` — `News2HR.*`, `News2RR.*`, `News2SpO2.*`,
`News2SBP.*`, `News2Temp.*`, `News2Calc.*`

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
level, the text of all active alert messages, and a `Session Alarm Events`
section rendered from the stored session event log in reading order. If no
readings exist, the vitals and active-alert sections shall be omitted. If no
session alarm events exist, the session event section shall state that none are
recorded in the current session. If a reset disclosure is recorded for the
current patient session, the session event section shall print that notice
before any current-session rows or placeholders.
**Traces to:** SYS-011, SYS-021
**Implemented in:** `src/patient.c` — `patient_print_summary()`
**Verified by:** `tests/unit/test_patient.cpp` — `PatientPrintSummary.*`

---

### SWR-PAT-007 — Session Alarm Event Capture
**Requirement:** `patient_add_reading()` shall derive the alert signature for
the newly appended reading using the existing alert semantics
(`overall_alert_level()` and `generate_alerts()`) and append at most one
`AlertEvent` to `alert_events[]` when any of the following occurs:

1. The first abnormal reading in the current session is added.
2. The aggregate alert severity changes between adjacent readings.
3. The abnormal-parameter set changes between adjacent abnormal readings.
4. The patient recovers from an abnormal state to `ALERT_NORMAL`.

`patient_add_reading()` shall not append a session alarm event for the first
normal reading of a session or for repeated adjacent readings with the same
aggregate severity and abnormal-parameter set. Each stored event shall record
the 1-based reading index, resulting aggregate alert level, abnormal parameter
signature, and summary text. `MAX_ALERT_EVENTS` shall equal `MAX_READINGS`.
**Traces to:** SYS-020, SYS-012
**Implemented in:** `src/patient.c` — `patient_add_reading()`, alert-event helpers
**Verified by:** `tests/unit/test_patient.cpp` — `PatientAlertEvents.REQ_PAT_007_*`;
`tests/integration/test_patient_monitoring.cpp` — `REQ_INT_MON_007`;
`tests/integration/test_alert_escalation.cpp` — `REQ_INT_ESC_006`

---

### SWR-PAT-008 — Session Alarm Event Access and Reset
**Requirement:** `patient_alert_event_count()` shall return the number of valid
session alarm events stored in `alert_events[]`. `patient_alert_event_at()`
shall return a pointer to `alert_events[index]` for valid zero-based indices
and `NULL` otherwise. `patient_init()` shall clear all stored session alarm
events by resetting the count to zero whenever a patient session is
reinitialized. `patient_note_session_reset()` shall record a bounded disclosure
string when a caller reinitializes the session after a retention boundary, and
`patient_session_reset_notice()` shall return that disclosure string or `NULL`
when no reset notice is pending.
**Traces to:** SYS-020, SYS-021
**Implemented in:** `src/patient.c` — `patient_init()`,
`patient_alert_event_count()`, `patient_alert_event_at()`,
`patient_note_session_reset()`, `patient_session_reset_notice()`
**Verified by:** `tests/unit/test_patient.cpp` — `PatientAlertEvents.REQ_PAT_008_*`

---

## Module UNIT-GUI — Graphical User Interface (`gui_main.c`, `gui_auth.c`)

### SWR-GUI-001 — Login Credential Validation
**Requirement:** `auth_validate(username, password)` shall return `1` if and
only if both the username and password exactly match the built-in credential.
It shall return `0` for any other input, including empty strings and NULL
pointers. NULL inputs shall not cause undefined behaviour.
**Traces to:** SYS-013
**Implemented in:** `src/gui_auth.c` — `auth_validate()`
**Verified by:** `tests/unit/test_auth.cpp` — `UsersTest.REQ_GUI_001_*`

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
**Verified by:** `tests/unit/test_auth.cpp` — `UsersTest.REQ_GUI_002_*`

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
**Verified by:** GUI demonstration.

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

## Module UNIT-SEC — Multi-User Authentication and Account Management (`gui_users.c`, `gui_auth.c`)

### SWR-SEC-001 — Multi-User Authentication with Role Detection
**Requirement:** `users_authenticate(username, password, role_out)` shall:
1. Return `1` if `username` and `password` exactly match an active account.
2. On success, write the account's `UserRole` to `*role_out` if `role_out` is non-NULL.
3. Return `0` for any unknown username or incorrect password without
   revealing which field was incorrect.
4. Return `0` without undefined behaviour when `username` or `password` is NULL.

`users_init()` shall load accounts from `users.dat`; if absent or unreadable
it shall fall back to built-in defaults (one ADMIN, one CLINICAL account).
`users_save()` shall persist the current account list to `users.dat` and
return `1` on success, `0` on failure.

**Traces to:** SYS-016
**Implemented in:** `src/gui_users.c` — `users_init()`, `users_authenticate()`, `users_save()`
**Verified by:** `tests/unit/test_auth.cpp` — `UsersTest.REQ_SEC_001_*` (6 tests)

---

### SWR-SEC-002 — NULL-Safe Role Output Parameter
**Requirement:** `users_authenticate()` shall accept `NULL` as the `role_out`
argument without undefined behaviour. Authentication logic and return value
shall be unaffected by whether `role_out` is NULL.

**Traces to:** SYS-016
**Implemented in:** `src/gui_users.c` — `users_authenticate()` (NULL guard on `role_out`)
**Verified by:** `tests/unit/test_auth.cpp` — `UsersTest.REQ_SEC_002_RoleOutNullDoesNotCrash`

---

### SWR-SEC-004 — Password Hashing (No Plaintext Storage)
**Requirement:** The system shall never store, log, or transmit passwords in
plaintext. All passwords shall be transformed to a SHA-256 hex digest
(`pw_hash()`, FIPS PUB 180-4) before storage. Authentication shall compare
the SHA-256 digest of the candidate password against the stored digest.
File format version "v2" stores 64-character lowercase hex digests in
`users.dat`; v1 (plaintext) files are rejected on load and replaced by
built-in defaults. The `pw_hash` implementation shall use only stack
storage and have no external dependencies.

**Traces to:** SYS-017
**Implemented in:** `src/pw_hash.c` — `pw_hash()`; `src/gui_users.c` —
`load_defaults()`, `users_authenticate()`, `users_change_password()`,
`users_admin_set_password()`, `users_add()`, `users_save()`
**Verified by:** `tests/unit/test_auth.cpp` — `UsersTest.REQ_SEC_004_*`
(3 tests: not-plaintext, 64-char hex format, transparent authentication)

---

### SWR-SEC-003 — Password Management
**Requirement:**
- `users_change_password(username, old_password, new_password)` shall
  change the stored password for `username` only when `old_password`
  matches the current password **and** `new_password` is at least
  `USERS_MIN_PASSWORD_LEN` (8) characters long. Returns `1` on success,
  `0` otherwise.
- `users_admin_set_password(username, new_password)` shall change any
  account's password without requiring the current password, provided
  `username` identifies an active account and `new_password` is at least
  `USERS_MIN_PASSWORD_LEN` characters long. Returns `1` on success, `0`
  otherwise.
- Both functions shall return `0` without undefined behaviour when any
  argument is NULL.

**Traces to:** SYS-017
**Implemented in:** `src/gui_users.c` — `users_change_password()`, `users_admin_set_password()`
**Verified by:** `tests/unit/test_auth.cpp` — `UsersTest.REQ_SEC_003_*` (8 tests)

---

### SWR-GUI-007 — User Account Management API
**Requirement:** The account management subsystem shall provide:
- `users_add(username, display_name, password, role)` — add a new active
  account; reject if the username already exists, if the password is shorter
  than `USERS_MIN_PASSWORD_LEN`, or if the account limit
  (`USERS_MAX_ACCOUNTS` = 8) is reached. Returns `1`/`0`.
- `users_remove(username)` — deactivate an account; reject removal if it
  would leave the system with zero ADMIN accounts. Returns `1`/`0`.
- `users_count()` — return the number of currently active accounts.
- `users_get_by_index(idx, out)` — copy the account at logical index `idx`
  into `*out`. Returns `1` for valid index, `0` for out-of-range or negative
  index.
All functions shall use only static memory (no heap allocation).

**Traces to:** SYS-016
**Implemented in:** `src/gui_users.c` — `users_add()`, `users_remove()`, `users_count()`, `users_get_by_index()`
**Verified by:** `tests/unit/test_auth.cpp` — `UsersTest.REQ_GUI_007_*` (8 tests)

---

### SWR-GUI-008 — Role-Based UI Differentiation
**Requirement:** After successful authentication the dashboard shall:
1. Display a **gold "ADMIN"** role badge and a **"Settings"** button for
   `ROLE_ADMIN` users.
2. Display a **teal "CLINICAL"** role badge and a **"My Account"** button for
   `ROLE_CLINICAL` users.
3. The "Settings" button shall be present **only** for ADMIN sessions;
   CLINICAL users shall not be able to access user management controls.
4. The role badge and available buttons shall be determined exclusively by
   the `UserRole` returned by `users_authenticate()` at login time.

**Traces to:** SYS-017
**Implemented in:** `src/gui_main.c` — `WM_CREATE` handler in `dash_proc()`,
`draw_pill()`, `IDC_BTN_SETTINGS`, `IDC_BTN_ACCOUNT`
**Verified by:** Verified via GUI demonstration (role-conditional control
creation in `dash_proc WM_CREATE`; Admin shows Settings, Clinical shows My Account).

---

### SWR-GUI-009 — User Management Settings Panel
**Requirement:** The Settings panel (accessible to `ROLE_ADMIN` only) shall:
1. Present a tabbed interface containing at minimum a **Users** tab and an
   **About** tab.
2. The Users tab shall list all active accounts in a listbox and provide:
   - **Add** — opens a dialog to enter username, display name, password,
     and role; rejects passwords shorter than `USERS_MIN_PASSWORD_LEN` and
     duplicate usernames.
   - **Remove** — removes the selected account; enforces the
     minimum-one-admin invariant (matching SWR-GUI-007).
   - **Set Password** — allows the admin to set a new password for the
     selected account without requiring the account's current password.
3. All user management operations shall call the corresponding
   `gui_users.c` API functions.

**Traces to:** SYS-016, SYS-017
**Implemented in:** `src/gui_main.c` — `settings_proc()`, `pwddlg_proc()`,
`adduser_proc()`
**Verified by:** Verified via GUI demonstration (Settings panel opens for
Admin; Add/Remove/Set Password operate via `users_add()`, `users_remove()`,
`users_admin_set_password()`).

---

### SWR-GUI-010 — Simulation Mode Toggle

**Requirement:** The dashboard shall provide a **Sim Mode** toggle button in the
header bar that switches between two operating modes:

1. **Simulation mode (ON):** The software reads synthetic vital signs from the
   HAL back-end (`sim_vitals.c`), updates the vital-signs tiles every 2 s, and
   displays a `* SIM LIVE` / `SIM PAUSED` badge in the header. The status
   banner shall append `[ SIMULATION MODE ]` to normal-status text.
2. **Device mode (OFF):** The timer is stopped, all vital-signs tiles shall
   display `N/A`, the patient bar shall read
   `"DEVICE MODE — Simulation disabled. Connect real hardware for live data."`,
   and the status banner shall read
   `"DEVICE MODE — Enable simulation in Settings to use synthetic data"`.

The selected mode shall be persisted to `monitor.cfg` (key `sim_enabled=0|1`)
and restored when the application is next launched.

**Traces to:** SYS-004, SYS-005
**Implemented in:** `src/gui_main.c` — `IDC_BTN_SIM_MODE` handler,
`config_load()`, `config_save()`, `paint_tiles()`, `paint_status_banner()`,
`paint_patient_bar()`
**Verified by:** GUI demonstration — toggle Sim: OFF shows N/A tiles and
device-mode banners; Sim: ON resumes live data.

---

### SWR-GUI-011 — Rolling Status Message in Simulation Mode

**Requirement:** When simulation mode is enabled (`sim_enabled = 1`), the status
banner shall display a continuously scrolling/rolling message that reads:
`"hi lee how are you"` with decorative separators. The message shall:

1. Scroll horizontally across the status banner from right to left at a steady pace
2. Repeat indefinitely, looping when it reaches the left edge
3. Update position every timer tick (approximately 2 seconds per complete scroll)
4. Be displayed in white text on the current alert-level background color
5. Only be shown when simulation mode is **ON**; when OFF, the banner shows the
   device-mode message as per SWR-GUI-010

This feature provides visual feedback that the application is in demonstration/
simulation mode and adds a personalized greeting element to the UI.

**Traces to:** SYS-005
**Implemented in:** `src/gui_main.c` — `sim_msg_scroll_offset` state variable,
WM_TIMER handler (advancing offset), `paint_status_banner()` function
**Verified by:** Visual GUI demonstration — enable simulation mode and observe
rolling message in status banner.

---

## Module UNIT-ALM — Configurable Alarm Limits (`alarm_limits.c`)

### SWR-ALM-001 — Per-Patient Configurable Alarm Limits
**Requirement:** The system shall provide configurable alarm limits per vital
sign parameter with sensible clinical defaults per IEC 60601-1-8:

- HR: low 60 / high 100 bpm
- SBP: low 90 / high 140 mmHg
- DBP: low 60 / high 90 mmHg
- Temp: low 36.1 / high 37.2 C
- SpO2: low 95 %
- RR: low 12 / high 20 br/min

The alarm limits shall be:
1. Editable in a Settings tab ("Alarm Limits")
2. Persisted to `alarm_limits.cfg` (key=value format)
3. Restorable to factory defaults via a "Reset Defaults" button
4. Checked via `alarm_check_*()` functions that return `ALERT_NORMAL`,
   `ALERT_WARNING`, or `ALERT_CRITICAL`

**Traces to:** SYS-002, SYS-003
**Implemented in:** `src/alarm_limits.c`, `include/alarm_limits.h`,
`src/gui_main.c` — Alarm Limits tab in Settings
**Verified by:** `tests/unit/test_alarm_limits.cpp` — 31 tests

---

## Module UNIT-TRD — Vital Signs Trend Analysis (`trend.c`)

### SWR-TRD-001 — Trend Sparkline and Direction Detection
**Requirement:** The system shall display a mini sparkline graph in each
vital sign dashboard tile showing the last up to `MAX_READINGS` data points.
The system shall also compute a trend direction for each parameter:

- `TREND_STABLE`: when the first-half and second-half means differ by less
  than 5% of the overall mean
- `TREND_RISING`: when the second-half mean is significantly higher
- `TREND_FALLING`: when the second-half mean is significantly lower

Helper functions `trend_extract_hr()`, `trend_extract_sbp()`,
`trend_extract_temp()`, `trend_extract_spo2()`, `trend_extract_rr()` shall
extract per-parameter arrays from a `VitalSigns` history buffer.

**Traces to:** SYS-001, SYS-002
**Implemented in:** `src/trend.c`, `include/trend.h`,
`src/gui_main.c` — `paint_sparkline()`, `paint_tiles()`
**Verified by:** `tests/unit/test_trend.cpp` — 18 tests

---

### SWR-GUI-012 - Localization Selection and Persistence

**Requirement:** The application shall support exactly four static UI languages:
English, Spanish, French, and German. The localization layer and Settings
dialog language selector shall:

1. Lists the four approved language options.
2. Expose localized strings and language names for the selected language
   through the static localization API.
3. Persist the selected language to `monitor.cfg` using `language=0..3`.
4. Load the persisted language value from `monitor.cfg` for reuse by the
   application.
5. Use only static storage for localization strings and state; no heap
   allocation is permitted in the localization layer.

Invalid or missing persisted language values shall fall back to English.

**Traces to:** SYS-014
**Implemented in:** `src/localization.c` - `localization_set_language()`,
`localization_get_language()`, `localization_get_string()`,
`localization_get_language_name()`; `src/gui_main.c` - Settings Language tab
and language selector population; `src/app_config.c` -
`app_config_load_language()`, `app_config_save_language()`
**Verified by:** `tests/unit/test_localization.cpp` - `LocalizationTest.*`
(8 tests); supplemental GUI automation `DVT-GUI-16`

---

### SWR-GUI-013 — Session Alarm Event Review List

**Requirement:** The dashboard shall display a dedicated read-only list labeled
`Session Alarm Events`, separate from both `Active Alerts` and `Reading
History`. `update_dashboard()` shall repopulate this list from
`patient_alert_event_count()` and `patient_alert_event_at()` and render each
stored event with its reading index, resulting severity, and summary text. If
`patient_session_reset_notice()` reports that the previous bounded session was
cleared, the list shall show that reset/retention disclosure before any
current-session rows. If no session alarm events exist for the current session,
the list shall show an explicit placeholder rather than remaining blank.
`IDC_LIST_ALERTS` shall continue to show only active alerts from the latest
reading.

**Traces to:** SYS-014, SYS-021
**Implemented in:** `src/gui_main.c` — `create_dash_controls()`,
`reposition_dash_controls()`, `update_dashboard()`; `src/localization.c` —
session event label string
**Verified by:** Manual GUI review (`GUI-MAN-06`)

---

## Revision History

| Rev | Date       | Author          | Description          |
|-----|------------|-----------------|----------------------|
| A   | 2026-04-06 | vinu-engineer   | Initial release      |
| B   | 2026-04-07 | vinu-engineer   | Added UNIT-GUI module (SWR-GUI-001..004) |
| C   | 2026-04-07 | vinu-engineer   | Added SWR-GUI-005 (HAL), SWR-GUI-006 (sim) |
| D   | 2026-04-07 | vinu-engineer   | Added UNIT-SEC module: SWR-SEC-001/002/003, SWR-GUI-007/008/009 |
| E   | 2026-04-07 | vinu-engineer   | Added SWR-GUI-010 (simulation mode toggle) — v1.8.0 |
| F   | 2026-04-08 | vinu-engineer   | Added SWR-VIT-008 (RR), SWR-NEW-001 (NEWS2), SWR-ALM-001 (alarm limits), SWR-TRD-001 (trend) — v2.6.0 |
| G   | 2026-04-08 | claude          | Added SWR-GUI-011 (rolling message in simulation mode) — v2.7.0 |
| H   | 2026-05-03 | codex           | Reconciled SWR-VIT-008 and SWR-NEW-001 to existing alerting and aggregate-risk SYS links; no clinical behavior changes |
| I   | 2026-05-03 | Codex implementer | Added SWR-GUI-012 (localization selection and persistence) |
| J   | 2026-05-05 | vinu           | Refreshed SWR-GUI-001..003 verification references |
| K   | 2026-05-05 | Codex implementer | Restored defensible SYS-level traceability for SWR-VIT-008 and SWR-NEW-001; no clinical behavior changes |
| L   | 2026-05-05 | Codex implementer | Added SWR-PAT-007/008 and SWR-GUI-013 for session alarm event review |
| M   | 2026-05-06 | Codex implementer | Added explicit session-reset disclosure expectations for session review surfaces |
