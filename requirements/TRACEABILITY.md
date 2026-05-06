# Requirements Traceability Matrix (RTM)

**Document ID:** RTM-001-REV-M
**Project:** Patient Vital Signs Monitor
**Version:** 2.8.0
**Date:** 2026-05-06
**Status:** Approved
**Standard:** IEC 62304 Â§5.7.3 / FDA SW Validation Guidance

---

## How to Read This Matrix

Each row represents one Software Requirement (SWR) and shows the full
traceability chain:

```
User Need (UNS)
    â””â”€â”€ System Requirement (SYS)
            â””â”€â”€ Software Requirement (SWR)
                    â””â”€â”€ Implementation (file : function)
                            â””â”€â”€ Unit Tests
                            â””â”€â”€ Integration Tests
```

A gap in any column is a compliance finding â€” every SWR must have
implementation and test coverage, and every UNS must reach at least one SWR.

---

## Section 1 â€” Forward Traceability (UNS â†’ Test)

> Reading direction: left to right â€” from user need down to verification evidence.

| UNS | SYS | SWR | Implementation | Unit Tests | Integration Tests |
|-----|-----|-----|----------------|------------|-------------------|
| UNS-013, UNS-016 | SYS-022 | SWR-GUI-014 | `gui_main.c`, `app_config.c` : admin Session tab, `monitor.cfg` idle-timeout persistence | `ConfigTest.*` (17 tests) + manual GUI review (`GUI-MAN-07`) | â€” |
| UNS-013, UNS-016 | SYS-022 | SWR-SEC-005 | `session_timeout.c`, `gui_main.c` : timeout bounds, idle timer, lock/unlock flow | `SessionTimeout*.*` (7 tests) + manual GUI review (`GUI-MAN-07`) | â€” |

| UNS-001 | SYS-001 | SWR-VIT-001 | `vitals.c` : `check_heart_rate()` | `HeartRate.*` (14 tests) | `REQ_INT_ESC_002`, `REQ_INT_ESC_005` |
| UNS-002 | SYS-002 | SWR-VIT-002 | `vitals.c` : `check_blood_pressure()` | `BloodPressure.*` (12 tests) | `REQ_INT_ESC_003` |
| UNS-003 | SYS-003 | SWR-VIT-003 | `vitals.c` : `check_temperature()` | `Temperature.*` (10 tests) | `REQ_INT_MON_002`, `REQ_INT_MON_003` |
| UNS-004 | SYS-004 | SWR-VIT-004 | `vitals.c` : `check_spo2()` | `SpO2.*` (8 tests) | `REQ_INT_ESC_001`, `REQ_INT_ESC_005` |
| UNS-005, UNS-006 | SYS-006 | SWR-VIT-005 | `vitals.c` : `overall_alert_level()` | `OverallAlert.*` (5 tests) | `REQ_INT_MON_002`, `REQ_INT_MON_003`, `REQ_INT_MON_004` |
| UNS-007 | SYS-007 | SWR-VIT-006 | `vitals.c` : `calculate_bmi()`, `bmi_category()` | `BMI.*` (12 tests) | `REQ_INT_MON_001` |
| UNS-005, UNS-006, UNS-010 | SYS-005, SYS-011 | SWR-VIT-007 | `vitals.c` : `alert_level_str()` | `AlertStr.*` (4 tests) | All integration tests |
| UNS-005, UNS-006, UNS-014, UNS-015 | SYS-018 | SWR-VIT-008 | `vitals.c` : `check_respiration_rate()` | `RespRate.*` (12 tests), `OverallAlert.SWR_VIT_008_*` (3 tests) | â€” |
| UNS-005, UNS-006, UNS-010, UNS-014 | SYS-019 | SWR-NEW-001 | `news2.c` : `news2_calculate()` | `News2HR.*`, `News2RR.*`, `News2SpO2.*`, `News2SBP.*`, `News2Temp.*`, `News2Calc.*` (53 tests) | â€” |
| UNS-005, UNS-006 | SYS-002, SYS-003 | SWR-ALM-001 | `alarm_limits.c` : `alarm_limits_defaults()`, `alarm_check_*()` | `AlarmLimitsTest.*` (31 tests) | â€” |
| UNS-001, UNS-009 | SYS-001, SYS-002 | SWR-TRD-001 | `trend.c` : `trend_direction()`, `trend_extract_*()` | `TrendDirection.*`, `TrendExtract.*` (18 tests) | â€” |
| UNS-015 | SYS-015 | SWR-GUI-010 | `gui_main.c`, `app_config.c` : sim toggle + persistence | Manual GUI review + `ConfigTest.*` support (10 persistence checks) | â€” |
| UNS-015 | SYS-005 | SWR-GUI-011 | `gui_main.c` : rolling message in sim mode (`paint_status_banner()`, scroll offset) | Manual visual review | â€” |
| UNS-014 | SYS-014 | SWR-GUI-012 | `gui_main.c`, `localization.c`, `app_config.c` : language tab, selector strings, `monitor.cfg` persistence/load | `LocalizationTest.*` (8 tests) + supplemental `DVT-GUI-16` | â€” |
| UNS-005, UNS-006 | SYS-005 | SWR-ALT-001 | `alerts.c` : `generate_alerts()` | `REQ_ALT_002_*` (4 tests) | `REQ_INT_MON_004`, `REQ_INT_ESC_002`, `REQ_INT_ESC_003` |
| UNS-005 | SYS-005 | SWR-ALT-002 | `alerts.c` : `generate_alerts()` | `REQ_ALT_001_*` (1 test) | `REQ_INT_ESC_004` |
| UNS-011 | SYS-012 | SWR-ALT-003 | `alerts.c` : `generate_alerts()` | `REQ_ALT_004_*` (2 tests) | â€” |
| UNS-005, UNS-006 | SYS-005 | SWR-ALT-004 | `alerts.c` : `generate_alerts()` via `PUSH_ALERT` | `REQ_ALT_005_*` (2 tests) | â€” |
| UNS-008 | SYS-008, SYS-012 | SWR-PAT-001 | `patient.c` : `patient_init()` | `PatientInit.*` (3 tests) | `REQ_INT_MON_001`, `REQ_INT_MON_006` |
| UNS-009, UNS-011 | SYS-009, SYS-010 | SWR-PAT-002 | `patient.c` : `patient_add_reading()` | `PatientAddReading.*` (5 tests) | `REQ_INT_MON_002`, `REQ_INT_MON_005`, `REQ_INT_MON_006` |
| UNS-009 | SYS-009 | SWR-PAT-003 | `patient.c` : `patient_latest_reading()` | `PatientLatestReading.*` (2 tests) | `REQ_INT_MON_004` |
| UNS-010 | SYS-006, SYS-011 | SWR-PAT-004 | `patient.c` : `patient_current_status()` | `PatientStatus.*` (4 tests) | `REQ_INT_MON_002`, `REQ_INT_MON_003`, `REQ_INT_ESC_001`â€“`REQ_INT_ESC_005` |
| UNS-011 | SYS-010 | SWR-PAT-005 | `patient.c` : `patient_is_full()` | `PatientIsFull.*` (2 tests) | `REQ_INT_MON_005` |
| UNS-010, UNS-017 | SYS-011, SYS-021 | SWR-PAT-006 | `patient.c` : `patient_print_summary()` | `PatientPrintSummary.*` (5 tests) | â€” |
| UNS-017 | SYS-020, SYS-012 | SWR-PAT-007 | `patient.c` : `patient_add_reading()`, alert-event helpers | `PatientAlertEvents.REQ_PAT_007_*` (6 tests) | `REQ_INT_MON_007`, `REQ_INT_ESC_006` |
| UNS-017 | SYS-020, SYS-021 | SWR-PAT-008 | `patient.c` : `patient_init()`, `patient_alert_event_count()`, `patient_alert_event_at()`, `patient_note_session_reset()`, `patient_session_reset_notice()` | `PatientAlertEvents.REQ_PAT_008_*` (2 tests) | â€” |
| UNS-013 | SYS-013 | SWR-GUI-001 | `gui_auth.c` : `auth_validate()` | `UsersTest.REQ_GUI_001_*` (10 tests) | â€” |
| UNS-013 | SYS-013 | SWR-GUI-002 | `gui_main.c` : `attempt_login()`, `login_proc()`, logout handler | `UsersTest.REQ_GUI_002_*` (5 tests) | â€” |
| UNS-014, UNS-005, UNS-006 | SYS-014 | SWR-GUI-003 | `gui_main.c` : `paint_tile()`, `paint_tiles()`, `paint_status_banner()`, `update_dashboard()` | GUI demo | â€” |
| UNS-014, UNS-008, UNS-009 | SYS-014 | SWR-GUI-004 | `gui_main.c` : `create_dash_controls()`, `do_admit()`, `do_add_reading()`, `do_scenario()` | GUI demo | â€” |
| UNS-015 | SYS-015 | SWR-GUI-005 | `hw_vitals.h` (interface); `gui_main.c` uses only HAL calls | Architecture review | â€” |
| UNS-015 | SYS-015, SYS-012 | SWR-GUI-006 | `sim_vitals.c` : `hw_init()`, `hw_get_next_reading()` | GUI demo (tiles cycle NORMALâ†’WARNINGâ†’CRITICALâ†’NORMAL) | â€” |
| UNS-016 | SYS-016 | SWR-SEC-001 | `gui_users.c` : `users_init()`, `users_authenticate()`, `users_save()` | `UsersTest.REQ_SEC_001_*` (6 tests) | â€” |
| UNS-016 | SYS-016 | SWR-SEC-002 | `gui_users.c` : `users_authenticate()` NULL guard on `role_out` | `UsersTest.REQ_SEC_002_RoleOutNullDoesNotCrash` (1 test) | â€” |
| UNS-016 | SYS-017 | SWR-SEC-003 | `gui_users.c` : `users_change_password()`, `users_admin_set_password()` | `UsersTest.REQ_SEC_003_*` (8 tests) | â€” |
| UNS-016 | SYS-017 | SWR-SEC-004 | `src/pw_hash.c` : `pw_hash()`; `gui_users.c` : all credential paths | `UsersTest.REQ_SEC_004_*` (3 tests) | â€” |
| UNS-016 | SYS-016 | SWR-GUI-007 | `gui_users.c` : `users_add()`, `users_remove()`, `users_count()`, `users_get_by_index()` | `UsersTest.REQ_GUI_007_*` (8 tests) | â€” |
| UNS-016 | SYS-017 | SWR-GUI-008 | `gui_main.c` : role-conditional `WM_CREATE`, `draw_pill()`, `IDC_BTN_SETTINGS`, `IDC_BTN_ACCOUNT` | GUI demo (Adminâ†’Settings, Clinicalâ†’My Account) | â€” |
| UNS-016 | SYS-016, SYS-017 | SWR-GUI-009 | `gui_main.c` : `settings_proc()`, `pwddlg_proc()`, `adduser_proc()` | GUI demo (Settings panel Add/Remove/Set Password) | â€” |
| UNS-017, UNS-014 | SYS-021, SYS-014 | SWR-GUI-013 | `gui_main.c` : `create_dash_controls()`, `reposition_dash_controls()`, `update_dashboard()`; `localization.c` : session event label string | Manual GUI review (`GUI-MAN-06`) | â€” |

---

## Section 2 â€” Backward Traceability (Test â†’ UNS)

> Reading direction: right to left â€” every test must justify its existence via a user need.

### Unit Tests

| Test Suite | Test ID Pattern | SWR | SYS | UNS |
|------------|-----------------|-----|-----|-----|
| `ConfigTest` | `ConfigTest.*` | SWR-GUI-014 | SYS-022 | UNS-013, UNS-016 |
| `HeartRate` | `REQ_VIT_001_*` | SWR-VIT-001 | SYS-001 | UNS-001, UNS-005, UNS-006 |
| `BloodPressure` | `REQ_VIT_002_*` | SWR-VIT-002 | SYS-002 | UNS-002, UNS-005, UNS-006 |
| `Temperature` | `REQ_VIT_003_*` | SWR-VIT-003 | SYS-003 | UNS-003, UNS-005, UNS-006 |
| `SpO2` | `REQ_VIT_004_*` | SWR-VIT-004 | SYS-004 | UNS-004, UNS-005, UNS-006 |
| `OverallAlert` | `REQ_VIT_005_*` | SWR-VIT-005 | SYS-006 | UNS-005, UNS-006 |
| `BMI` | `REQ_VIT_006_*` | SWR-VIT-006 | SYS-007 | UNS-007 |
| `AlertStr` | `REQ_VIT_007_*` | SWR-VIT-007 | SYS-005, SYS-011 | UNS-005, UNS-006, UNS-010 |
| `GenerateAlerts` | `REQ_ALT_001_*` | SWR-ALT-002 | SYS-005 | UNS-005 |
| `GenerateAlerts` | `REQ_ALT_002_*` | SWR-ALT-001 | SYS-005 | UNS-005, UNS-006 |
| `GenerateAlerts` | `REQ_ALT_003_*` | SWR-ALT-001 | SYS-005 | UNS-005, UNS-006 |
| `GenerateAlerts` | `REQ_ALT_004_*` | SWR-ALT-003 | SYS-012 | UNS-011 |
| `GenerateAlerts` | `REQ_ALT_005_*` | SWR-ALT-004 | SYS-005 | UNS-005, UNS-006 |
| `PatientInit` | `REQ_PAT_001_*` | SWR-PAT-001 | SYS-008, SYS-012 | UNS-008, UNS-011 |
| `PatientAddReading` | `REQ_PAT_002_*` | SWR-PAT-002 | SYS-009, SYS-010 | UNS-009, UNS-011 |
| `PatientLatestReading` | `REQ_PAT_003_*` | SWR-PAT-003 | SYS-009 | UNS-009 |
| `PatientStatus` | `REQ_PAT_004_*` | SWR-PAT-004 | SYS-006, SYS-011 | UNS-010 |
| `PatientIsFull` | `REQ_PAT_005_*` | SWR-PAT-005 | SYS-010 | UNS-011 |
| `PatientAlertEvents` | `REQ_PAT_007_*` | SWR-PAT-007 | SYS-020, SYS-012 | UNS-017, UNS-011 |
| `PatientAlertEvents` | `REQ_PAT_008_*` | SWR-PAT-008 | SYS-020, SYS-021 | UNS-017 |
| `PatientPrintSummary` | `REQ_PAT_006_*` | SWR-PAT-006 | SYS-011, SYS-021 | UNS-010, UNS-017 |
| `UsersTest` | `REQ_GUI_001_*` | SWR-GUI-001 | SYS-013 | UNS-013 |
| `UsersTest` | `REQ_GUI_002_*` | SWR-GUI-002 | SYS-013 | UNS-013 |
| `UsersTest` | `REQ_SEC_001_*` | SWR-SEC-001 | SYS-016 | UNS-016 |
| `UsersTest` | `REQ_SEC_002_*` | SWR-SEC-002 | SYS-016 | UNS-016 |
| `UsersTest` | `REQ_SEC_003_*` | SWR-SEC-003 | SYS-017 | UNS-016 |
| `UsersTest` | `REQ_SEC_004_*` | SWR-SEC-004 | SYS-017 | UNS-016 |
| `UsersTest` | `REQ_GUI_007_*` | SWR-GUI-007 | SYS-016 | UNS-016 |
| `SessionTimeout` | `SessionTimeout*.*` | SWR-SEC-005 | SYS-022 | UNS-013, UNS-016 |
| `LocalizationTest` | `LocalizationTest.*` | SWR-GUI-012 | SYS-014 | UNS-014 |
| `RespRate` | `REQ_VIT_008_*` | SWR-VIT-008 | SYS-018 | UNS-005, UNS-006, UNS-014, UNS-015 |
| `News2HR`, `News2RR`, etc. | `News2*.*` | SWR-NEW-001 | SYS-019 | UNS-005, UNS-006, UNS-010, UNS-014 |
| `AlarmLimitsTest` | `AlarmLimitsTest.*` | SWR-ALM-001 | SYS-002, SYS-003 | UNS-005, UNS-006 |
| `TrendDirection`, `TrendExtract` | `Trend*.*` | SWR-TRD-001 | SYS-001, SYS-002 | UNS-001, UNS-009 |

Supporting implementation checks:
- `HALTest`, `HALTestNoInit`, and `SimSequenceTest` exercise HAL safety and simulator sequencing, but approved verification for `SWR-GUI-005` and `SWR-GUI-006` remains architecture review / GUI demonstration.
- `ConfigTest.*` directly verifies `SWR-GUI-014` idle-timeout persistence and also provides supporting persistence evidence for `SWR-GUI-010`.

### Integration Tests

| Test Suite | Test ID | SWR | SYS | UNS |
|------------|---------|-----|-----|-----|
| `PatientMonitoring` | `REQ_INT_MON_001` | SWR-PAT-001, SWR-VIT-006 | SYS-007, SYS-008 | UNS-007, UNS-008 |
| `PatientMonitoring` | `REQ_INT_MON_002` | SWR-PAT-002, SWR-PAT-004, SWR-VIT-005 | SYS-006, SYS-009 | UNS-005, UNS-006, UNS-009 |
| `PatientMonitoring` | `REQ_INT_MON_003` | SWR-PAT-002, SWR-PAT-004, SWR-VIT-005 | SYS-006, SYS-009 | UNS-005, UNS-006, UNS-009 |
| `PatientMonitoring` | `REQ_INT_MON_004` | SWR-PAT-003, SWR-PAT-004, SWR-ALT-001 | SYS-005, SYS-006 | UNS-005, UNS-006 |
| `PatientMonitoring` | `REQ_INT_MON_005` | SWR-PAT-002, SWR-PAT-005 | SYS-010 | UNS-011 |
| `PatientMonitoring` | `REQ_INT_MON_006` | SWR-PAT-001, SWR-PAT-002, SWR-PAT-004 | SYS-008, SYS-009 | UNS-008, UNS-009 |
| `PatientMonitoring` | `REQ_INT_MON_007` | SWR-PAT-007, SWR-ALT-002 | SYS-020, SYS-021 | UNS-017, UNS-010 |
| `AlertEscalation` | `REQ_INT_ESC_001` | SWR-VIT-004, SWR-PAT-004 | SYS-004, SYS-006 | UNS-004, UNS-005, UNS-006 |
| `AlertEscalation` | `REQ_INT_ESC_002` | SWR-VIT-001, SWR-ALT-001 | SYS-001, SYS-005 | UNS-001, UNS-005, UNS-006 |
| `AlertEscalation` | `REQ_INT_ESC_003` | SWR-VIT-001â€“004, SWR-ALT-001 | SYS-001â€“005 | UNS-001â€“006 |
| `AlertEscalation` | `REQ_INT_ESC_004` | SWR-PAT-004, SWR-ALT-002 | SYS-005, SYS-006 | UNS-005, UNS-006 |
| `AlertEscalation` | `REQ_INT_ESC_005` | SWR-VIT-001, SWR-VIT-004, SWR-PAT-004 | SYS-001, SYS-004, SYS-006 | UNS-001, UNS-004, UNS-005, UNS-006 |
| `AlertEscalation` | `REQ_INT_ESC_006` | SWR-PAT-007, SWR-ALT-001 | SYS-020, SYS-005 | UNS-017, UNS-005, UNS-006 |

---

## Section 3 â€” UNS Coverage Summary

> Every User Need must be covered. Any row marked âœ— is a compliance gap.

| UNS ID | User Need (short) | SYS IDs | SWR IDs | Covered? |
|--------|-------------------|---------|---------|----------|
| UNS-001 | Heart rate monitoring | SYS-001 | SWR-VIT-001 | âœ“ |
| UNS-002 | Blood pressure monitoring | SYS-002 | SWR-VIT-002 | âœ“ |
| UNS-003 | Temperature monitoring | SYS-003 | SWR-VIT-003 | âœ“ |
| UNS-004 | SpO2 monitoring | SYS-004 | SWR-VIT-004 | âœ“ |
| UNS-005 | Automatic alerting | SYS-005, SYS-006, SYS-018, SYS-019 | SWR-VIT-005, SWR-VIT-007, SWR-VIT-008, SWR-NEW-001, SWR-ALT-001, SWR-ALT-002 | âœ“ |
| UNS-006 | Alert severity differentiation | SYS-005, SYS-006, SYS-018, SYS-019 | SWR-VIT-005, SWR-VIT-007, SWR-VIT-008, SWR-NEW-001, SWR-ALT-001, SWR-ALT-004 | âœ“ |
| UNS-007 | BMI display | SYS-007 | SWR-VIT-006 | âœ“ |
| UNS-008 | Patient identification | SYS-008 | SWR-PAT-001 | âœ“ |
| UNS-009 | Vital sign history | SYS-009 | SWR-PAT-002, SWR-PAT-003 | âœ“ |
| UNS-010 | Consolidated summary | SYS-011, SYS-019, SYS-021 | SWR-PAT-004, SWR-PAT-006, SWR-VIT-007, SWR-NEW-001 | âœ“ |
| UNS-011 | Data integrity | SYS-010, SYS-012 | SWR-PAT-002, SWR-PAT-005, SWR-ALT-003, SWR-PAT-001 | âœ“ |
| UNS-012 | Platform compatibility | SYS-012 | SWR-PAT-001, SWR-ALT-003 | âœ“ |
| UNS-013 | User authentication | SYS-013, SYS-022 | SWR-GUI-001, SWR-GUI-002, SWR-GUI-014, SWR-SEC-005 | âœ“ |
| UNS-014 | Graphical dashboard | SYS-014, SYS-018, SYS-019, SYS-021 | SWR-GUI-003, SWR-GUI-004, SWR-GUI-012, SWR-GUI-013, SWR-VIT-008, SWR-NEW-001 | âœ“ |
| UNS-015 | Live monitoring feed | SYS-015, SYS-018 | SWR-GUI-005, SWR-GUI-006, SWR-GUI-010, SWR-GUI-011, SWR-VIT-008 | âœ“ |
| UNS-016 | Role-based access / multi-user | SYS-016, SYS-017, SYS-022 | SWR-SEC-001, SWR-SEC-002, SWR-SEC-003, SWR-GUI-007, SWR-GUI-008, SWR-GUI-009, SWR-GUI-014, SWR-SEC-005 | âœ“ |
| UNS-017 | Session alarm event review | SYS-020, SYS-021 | SWR-PAT-006, SWR-PAT-007, SWR-PAT-008, SWR-GUI-013 | âœ“ |

**Result: 17 / 17 User Needs covered âœ“**

---

## Section 4 â€” SWR Coverage Summary

> Every SWR must have implementation and at least one test. Any gap is a finding.

| SWR ID | Implementation | Unit Tests | Integration Tests | Covered? |
|--------|----------------|------------|-------------------|----------|
| SWR-VIT-001 | `check_heart_rate()` | 14 | 2 | âœ“ |
| SWR-VIT-002 | `check_blood_pressure()` | 12 | 1 | âœ“ |
| SWR-VIT-003 | `check_temperature()` | 10 | 2 | âœ“ |
| SWR-VIT-004 | `check_spo2()` | 8 | 2 | âœ“ |
| SWR-VIT-005 | `overall_alert_level()` | 8 | 5 | âœ“ |
| SWR-VIT-006 | `calculate_bmi()`, `bmi_category()` | 12 | 1 | âœ“ |
| SWR-VIT-007 | `alert_level_str()` | 4 | all | âœ“ |
| SWR-ALT-001 | `generate_alerts()` | 4 | 3 | âœ“ |
| SWR-ALT-002 | `generate_alerts()` | 1 | 1 | âœ“ |
| SWR-ALT-003 | `generate_alerts()` | 2 | â€” | âœ“ |
| SWR-ALT-004 | `generate_alerts()` | 2 | â€” | âœ“ |
| SWR-PAT-001 | `patient_init()` | 3 | 2 | âœ“ |
| SWR-PAT-002 | `patient_add_reading()` | 5 | 3 | âœ“ |
| SWR-PAT-003 | `patient_latest_reading()` | 2 | 1 | âœ“ |
| SWR-PAT-004 | `patient_current_status()` | 4 | 7 | âœ“ |
| SWR-PAT-005 | `patient_is_full()` | 2 | 1 | âœ“ |
| SWR-PAT-006 | `patient_print_summary()` | 5 | â€” | âœ“ |
| SWR-PAT-007 | `patient_add_reading()`, alert-event helpers | 6 | 2 | âœ“ |
| SWR-PAT-008 | `patient_init()`, `patient_alert_event_count()`, `patient_alert_event_at()`, `patient_note_session_reset()`, `patient_session_reset_notice()` | 2 | â€” | âœ“ |
| SWR-GUI-001 | `auth_validate()` | 10 | â€” | âœ“ |
| SWR-GUI-002 | `attempt_login()`, `login_proc()`, logout | 5 | â€” | âœ“ |
| SWR-GUI-003 | `paint_tile()`, `paint_tiles()`, `paint_status_banner()` | GUI demo | â€” | âœ“ |
| SWR-GUI-004 | `create_dash_controls()`, `do_admit()`, `do_add_reading()` | GUI demo | â€” | âœ“ |
| SWR-GUI-005 | `hw_vitals.h` HAL interface | Architecture review | â€” | âœ“ |
| SWR-GUI-006 | `sim_vitals.c` : `hw_init()`, `hw_get_next_reading()` | GUI demo | â€” | âœ“ |
| SWR-SEC-001 | `users_init()`, `users_authenticate()`, `users_save()` | 6 | â€” | âœ“ |
| SWR-SEC-002 | `users_authenticate()` NULL guard | 1 | â€” | âœ“ |
| SWR-SEC-003 | `users_change_password()`, `users_admin_set_password()` | 8 | â€” | âœ“ |
| SWR-SEC-004 | `pw_hash()` + all credential paths in `gui_users.c` | 3 | â€” | âœ“ |
| SWR-SEC-005 | `session_timeout.c`, `gui_main.c` : idle policy + lock/unlock flow | `SessionTimeout*.*` (7) + manual GUI review (`GUI-MAN-07`) | â€” | âœ“ |
| SWR-GUI-007 | `users_add()`, `users_remove()`, `users_count()`, `users_get_by_index()` | 8 | â€” | âœ“ |
| SWR-GUI-008 | Role-conditional `WM_CREATE`, `draw_pill()`, `IDC_BTN_SETTINGS` | GUI demo | â€” | âœ“ |
| SWR-GUI-009 | `settings_proc()`, `pwddlg_proc()`, `adduser_proc()` | GUI demo | â€” | âœ“ |
| SWR-GUI-010 | `gui_main.c`, `app_config.c` : mode toggle + persistence | Manual GUI review + `ConfigTest.*` support (10) | â€” | âœ“ |
| SWR-GUI-011 | `gui_main.c` : `paint_status_banner()`, scroll offset | Manual visual review | â€” | âœ“ |
| SWR-GUI-012 | `gui_main.c`, `localization.c`, `app_config.c` : selector strings, persistence/load | `LocalizationTest.*` (8) + supplemental `DVT-GUI-16` | â€” | âœ“ |
| SWR-GUI-013 | `create_dash_controls()`, `reposition_dash_controls()`, `update_dashboard()` | Manual GUI review (`GUI-MAN-06`) | â€” | âœ“ |
| SWR-GUI-014 | `gui_main.c`, `app_config.c` : Session tab + idle-timeout persistence | `ConfigTest.*` (17) + manual GUI review (`GUI-MAN-07`) | â€” | âœ“ |
| SWR-VIT-008 | `vitals.c` : `check_respiration_rate()` | 15 | â€” | âœ“ |
| SWR-NEW-001 | `news2.c` : `news2_calculate()` | 53 | â€” | âœ“ |
| SWR-ALM-001 | `alarm_limits.c` : `alarm_limits_defaults()`, `alarm_check_*()` | 31 | â€” | âœ“ |
| SWR-TRD-001 | `trend.c` : `trend_direction()`, `trend_extract_*()` | 18 | â€” | âœ“ |

**Result: 42 / 42 SWRs implemented and tested âœ“**

---

## Section 5 â€” Code Coverage Summary

**Standard:** IEC 62304 Class B â€” Statement + Branch Coverage Target: 100%

| Source File | Lines | Covered | % | Branches | Covered | % | Notes |
|-------------|-------|---------|---|----------|---------|---|-------|
| `src/vitals.c` | 143 | 143 | 100% | 52 | 52 | 100% | `REQ_VIT_007_Unknown` test covers `default:` branch in `alert_level_str()` |
| `src/alerts.c` | 48 | 48 | 100% | 18 | 18 | 100% | â€” |
| `src/patient.c` | 61 | 61 | 100% | 14 | 14 | 100% | â€” |
| `src/gui_auth.c` | 28 | 28 | 100% | 6 | 6 | 100% | Delegation layer fully covered by test_auth.cpp |
| `src/gui_users.c` | ~120 | ~120 | 100% | ~40 | ~40 | 100% | All user management paths covered by REQ_SEC_* and REQ_GUI_007_* tests |
| `src/news2.c` | ~90 | ~90 | 100% | ~30 | ~30 | 100% | All NEWS2 scoring paths covered by News2* tests |
| `src/alarm_limits.c` | ~80 | ~80 | 100% | ~20 | ~20 | 100% | All alarm check paths covered by AlarmLimitsTest |
| `src/trend.c` | ~60 | ~60 | 100% | ~12 | ~12 | 100% | All trend direction + extract paths covered by Trend* tests |
| `src/pw_hash.c` | ~40 | ~40 | 100% | ~6 | ~6 | 100% | SHA-256 hashing covered by REQ_SEC_004 tests |

**Coverage Rationale â€” `alert_level_str()` `default:` branch:**
The `default: return "UNKNOWN"` branch in `alert_level_str()` (`vitals.c`) is a
defensive IEC 62304 guard against undefined enum casts â€” it is unreachable in
normal production execution because all call sites pass only valid `AlertLevel`
enum values. A dedicated test (`REQ_VIT_007_Unknown`) uses an explicit
out-of-range cast `(AlertLevel)99` to exercise this path, achieving 100% branch
coverage. This approach is accepted under IEC 62304 and is documented here as
the formal rationale per Â§5.5.4 (software unit acceptance criteria).

**GUI source files** (`gui_main.c`, `sim_vitals.c`) contain primarily Win32 API
message-handling code (WndProc chains, GDI painting) that requires a running
Windows message loop and is not amenable to automated unit testing without a
full GUI test harness. These files are verified by structured GUI demonstration
per IEC 62304 Â§5.6.3 (integration testing) and Â§5.6.7 (GUI verification).
This is recorded as an accepted coverage exclusion with a documented rationale.

---

## Section 6 â€” Test Count Summary

| Test File | Tests | SWRs Verified |
|-----------|-------|---------------|
| `tests/unit/test_vitals.cpp` | 80 | SWR-VIT-001 â€“ SWR-VIT-008 |
| `tests/unit/test_alerts.cpp` | 11 | SWR-ALT-001 â€“ SWR-ALT-004 |
| `tests/unit/test_patient.cpp` | 29 | SWR-PAT-001 â€“ SWR-PAT-008 |
| `tests/unit/test_auth.cpp` | 41 | SWR-GUI-001, SWR-GUI-002, SWR-SEC-001â€“004, SWR-GUI-007 |
| `tests/unit/test_news2.cpp` | 53 | SWR-NEW-001 |
| `tests/unit/test_alarm_limits.cpp` | 31 | SWR-ALM-001 |
| `tests/unit/test_trend.cpp` | 18 | SWR-TRD-001 |
| `tests/unit/test_hal.cpp` | 12 | Supporting HAL / simulator checks only; no direct SWR verification claim |
| `tests/unit/test_config.cpp` | 17 | SWR-GUI-014 + supporting config persistence evidence for SWR-GUI-010 |
| `tests/unit/test_session_timeout.cpp` | 7 | SWR-SEC-005 |
| `tests/unit/test_localization.cpp` | 8 | SWR-GUI-012 |
| `tests/integration/test_patient_monitoring.cpp` | 7 | SWR-PAT-*, SWR-VIT-*, SWR-ALT-* |
| `tests/integration/test_alert_escalation.cpp` | 7 | SWR-VIT-*, SWR-ALT-*, SWR-PAT-004, SWR-PAT-007 |
| **Total** | **321** | **42 SWRs covered across automated, architecture-review, and GUI-demo/manual evidence** |

---

## Revision History

| Rev | Date       | Author          | Description          |
|-----|------------|-----------------|----------------------|
| A   | 2026-04-06 | vinu-engineer   | Initial release      |
| B   | 2026-04-07 | vinu-engineer   | Added SWR-GUI-001..004; 14/14 UNS, 21/21 SWR, 121 tests |
| C   | 2026-04-07 | vinu-engineer   | Added UNS-015, SWR-GUI-005/006 (HAL + sim); 15/15 UNS, 23/23 SWR, 121 tests |
| D   | 2026-04-07 | vinu-engineer   | v1.6.0: added UNS-016, SYS-016/017, SWR-SEC-001..003 + SWR-GUI-007..009; 16/16 UNS, 29/29 SWR, 145 tests; 100% branch coverage |
| E   | 2026-04-08 | vinu-engineer   | v1.7.0: added SWR-SEC-004 (SHA-256 hashing); 30/30 SWR, 148 tests; CodeQL/cppcheck findings resolved |
| F   | 2026-04-08 | vinu-engineer   | v2.6.0: added SWR-VIT-008 (RR), SWR-NEW-001 (NEWS2), SWR-ALM-001 (alarm limits), SWR-TRD-001 (trend), SWR-GUI-010 (sim toggle); 35/35 SWR, 287 tests |
| G   | 2026-04-08 | claude          | v2.7.0: added SWR-GUI-011 (rolling message in simulation mode); 36/36 SWR, 287 tests |
| H   | 2026-05-03 | codex           | Reconciled v2.7.0 SWR counts and existing SYS mappings; 36/36 SWR, 287 tests |
| I   | 2026-05-03 | Codex implementer | Added SWR-GUI-012 localization traceability and 8 automated localization tests; 37/37 SWR, 295 tests |
| J   | 2026-05-05 | Codex implementer | Restored defensible SYS-level traceability for RR and NEWS2 requirements; 37/37 SWR, 295 tests |
| K   | 2026-05-05 | Codex implementer | Added session alarm event review traceability: UNS-017, SYS-020/021, SWR-PAT-007/008, SWR-GUI-013; 40/40 SWR, 305 tests |
| L   | 2026-05-06 | Codex implementer | Added session-reset disclosure traceability and updated automated totals to 307 tests |
| M   | 2026-05-06 | Codex implementer | Added idle session-lock traceability: SYS-022, SWR-SEC-005, SWR-GUI-014; 42/42 SWR, 321 tests |
