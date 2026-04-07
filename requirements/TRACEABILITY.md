# Requirements Traceability Matrix (RTM)

**Document ID:** RTM-001-REV-C
**Project:** Patient Vital Signs Monitor
**Version:** 1.4.0
**Date:** 2026-04-07
**Status:** Approved
**Standard:** IEC 62304 §5.7.3 / FDA SW Validation Guidance

---

## How to Read This Matrix

Each row represents one Software Requirement (SWR) and shows the full
traceability chain:

```
User Need (UNS)
    └── System Requirement (SYS)
            └── Software Requirement (SWR)
                    └── Implementation (file : function)
                            └── Unit Tests
                            └── Integration Tests
```

A gap in any column is a compliance finding — every SWR must have
implementation and test coverage, and every UNS must reach at least one SWR.

---

## Section 1 — Forward Traceability (UNS → Test)

> Reading direction: left to right — from user need down to verification evidence.

| UNS | SYS | SWR | Implementation | Unit Tests | Integration Tests |
|-----|-----|-----|----------------|------------|-------------------|
| UNS-001 | SYS-001 | SWR-VIT-001 | `vitals.c` : `check_heart_rate()` | `HeartRate.*` (14 tests) | `REQ_INT_ESC_002`, `REQ_INT_ESC_005` |
| UNS-002 | SYS-002 | SWR-VIT-002 | `vitals.c` : `check_blood_pressure()` | `BloodPressure.*` (12 tests) | `REQ_INT_ESC_003` |
| UNS-003 | SYS-003 | SWR-VIT-003 | `vitals.c` : `check_temperature()` | `Temperature.*` (10 tests) | `REQ_INT_MON_002`, `REQ_INT_MON_003` |
| UNS-004 | SYS-004 | SWR-VIT-004 | `vitals.c` : `check_spo2()` | `SpO2.*` (8 tests) | `REQ_INT_ESC_001`, `REQ_INT_ESC_005` |
| UNS-005, UNS-006 | SYS-006 | SWR-VIT-005 | `vitals.c` : `overall_alert_level()` | `OverallAlert.*` (5 tests) | `REQ_INT_MON_002`, `REQ_INT_MON_003`, `REQ_INT_MON_004` |
| UNS-007 | SYS-007 | SWR-VIT-006 | `vitals.c` : `calculate_bmi()`, `bmi_category()` | `BMI.*` (12 tests) | `REQ_INT_MON_001` |
| UNS-005, UNS-006, UNS-010 | SYS-005, SYS-011 | SWR-VIT-007 | `vitals.c` : `alert_level_str()` | `AlertStr.*` (3 tests) | All integration tests |
| UNS-005, UNS-006 | SYS-005 | SWR-ALT-001 | `alerts.c` : `generate_alerts()` | `REQ_ALT_002_*` (4 tests) | `REQ_INT_MON_004`, `REQ_INT_ESC_002`, `REQ_INT_ESC_003` |
| UNS-005 | SYS-005 | SWR-ALT-002 | `alerts.c` : `generate_alerts()` | `REQ_ALT_001_*` (1 test) | `REQ_INT_ESC_004` |
| UNS-011 | SYS-012 | SWR-ALT-003 | `alerts.c` : `generate_alerts()` | `REQ_ALT_004_*` (2 tests) | — |
| UNS-005, UNS-006 | SYS-005 | SWR-ALT-004 | `alerts.c` : `generate_alerts()` via `PUSH_ALERT` | `REQ_ALT_005_*` (2 tests) | — |
| UNS-008 | SYS-008, SYS-012 | SWR-PAT-001 | `patient.c` : `patient_init()` | `PatientInit.*` (3 tests) | `REQ_INT_MON_001`, `REQ_INT_MON_006` |
| UNS-009, UNS-011 | SYS-009, SYS-010 | SWR-PAT-002 | `patient.c` : `patient_add_reading()` | `PatientAddReading.*` (5 tests) | `REQ_INT_MON_002`, `REQ_INT_MON_005`, `REQ_INT_MON_006` |
| UNS-009 | SYS-009 | SWR-PAT-003 | `patient.c` : `patient_latest_reading()` | `PatientLatestReading.*` (2 tests) | `REQ_INT_MON_004` |
| UNS-010 | SYS-006, SYS-011 | SWR-PAT-004 | `patient.c` : `patient_current_status()` | `PatientStatus.*` (4 tests) | `REQ_INT_MON_002`, `REQ_INT_MON_003`, `REQ_INT_ESC_001` – `REQ_INT_ESC_005` |
| UNS-011 | SYS-010 | SWR-PAT-005 | `patient.c` : `patient_is_full()` | `PatientIsFull.*` (2 tests) | `REQ_INT_MON_005` |
| UNS-010 | SYS-011 | SWR-PAT-006 | `patient.c` : `patient_print_summary()` | `PatientPrintSummary.*` (3 tests) | — |
| UNS-013 | SYS-013 | SWR-GUI-001 | `gui_auth.c` : `auth_validate()` | `AuthValidation.*` (10 tests) | — |
| UNS-013 | SYS-013 | SWR-GUI-002 | `gui_main.c` : `attempt_login()`, `login_proc()`, logout handler | `AuthDisplayName.*` (5 tests) | — |
| UNS-014, UNS-005, UNS-006 | SYS-014 | SWR-GUI-003 | `gui_main.c` : `paint_tile()`, `paint_tiles()`, `paint_status_banner()`, `update_dashboard()` | GUI demo | — |
| UNS-014, UNS-008, UNS-009 | SYS-014 | SWR-GUI-004 | `gui_main.c` : `create_dash_controls()`, `do_admit()`, `do_add_reading()`, `do_scenario()` | GUI demo | — |
| UNS-015 | SYS-015 | SWR-GUI-005 | `hw_vitals.h` (interface); `gui_main.c` uses only HAL calls | Architecture review | — |
| UNS-015 | SYS-015, SYS-012 | SWR-GUI-006 | `sim_vitals.c` : `hw_init()`, `hw_get_next_reading()` | GUI demo (tiles cycle NORMAL→WARNING→CRITICAL→NORMAL) | — |

---

## Section 2 — Backward Traceability (Test → UNS)

> Reading direction: right to left — every test must justify its existence via a user need.

### Unit Tests

| Test Suite | Test ID Pattern | SWR | SYS | UNS |
|------------|-----------------|-----|-----|-----|
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
| `PatientPrintSummary` | `REQ_PAT_006_*` | SWR-PAT-006 | SYS-011 | UNS-010 |
| `AuthValidation` | `REQ_GUI_001_*` | SWR-GUI-001 | SYS-013 | UNS-013 |
| `AuthDisplayName` | `REQ_GUI_002_*` | SWR-GUI-002 | SYS-013 | UNS-013 |

### Integration Tests

| Test Suite | Test ID | SWR | SYS | UNS |
|------------|---------|-----|-----|-----|
| `PatientMonitoring` | `REQ_INT_MON_001` | SWR-PAT-001, SWR-VIT-006 | SYS-007, SYS-008 | UNS-007, UNS-008 |
| `PatientMonitoring` | `REQ_INT_MON_002` | SWR-PAT-002, SWR-PAT-004, SWR-VIT-005 | SYS-006, SYS-009 | UNS-005, UNS-006, UNS-009 |
| `PatientMonitoring` | `REQ_INT_MON_003` | SWR-PAT-002, SWR-PAT-004, SWR-VIT-005 | SYS-006, SYS-009 | UNS-005, UNS-006, UNS-009 |
| `PatientMonitoring` | `REQ_INT_MON_004` | SWR-PAT-003, SWR-PAT-004, SWR-ALT-001 | SYS-005, SYS-006 | UNS-005, UNS-006 |
| `PatientMonitoring` | `REQ_INT_MON_005` | SWR-PAT-002, SWR-PAT-005 | SYS-010 | UNS-011 |
| `PatientMonitoring` | `REQ_INT_MON_006` | SWR-PAT-001, SWR-PAT-002, SWR-PAT-004 | SYS-008, SYS-009 | UNS-008, UNS-009 |
| `AlertEscalation` | `REQ_INT_ESC_001` | SWR-VIT-004, SWR-PAT-004 | SYS-004, SYS-006 | UNS-004, UNS-005, UNS-006 |
| `AlertEscalation` | `REQ_INT_ESC_002` | SWR-VIT-001, SWR-ALT-001 | SYS-001, SYS-005 | UNS-001, UNS-005, UNS-006 |
| `AlertEscalation` | `REQ_INT_ESC_003` | SWR-VIT-001–004, SWR-ALT-001 | SYS-001–005 | UNS-001–006 |
| `AlertEscalation` | `REQ_INT_ESC_004` | SWR-PAT-004, SWR-ALT-002 | SYS-005, SYS-006 | UNS-005, UNS-006 |
| `AlertEscalation` | `REQ_INT_ESC_005` | SWR-VIT-001, SWR-VIT-004, SWR-PAT-004 | SYS-001, SYS-004, SYS-006 | UNS-001, UNS-004, UNS-005, UNS-006 |

---

## Section 3 — UNS Coverage Summary

> Every User Need must be covered. Any row marked ✗ is a compliance gap.

| UNS ID | User Need (short) | SYS IDs | SWR IDs | Covered? |
|--------|-------------------|---------|---------|----------|
| UNS-001 | Heart rate monitoring | SYS-001 | SWR-VIT-001 | ✓ |
| UNS-002 | Blood pressure monitoring | SYS-002 | SWR-VIT-002 | ✓ |
| UNS-003 | Temperature monitoring | SYS-003 | SWR-VIT-003 | ✓ |
| UNS-004 | SpO2 monitoring | SYS-004 | SWR-VIT-004 | ✓ |
| UNS-005 | Automatic alerting | SYS-005, SYS-006 | SWR-VIT-005, SWR-VIT-007, SWR-ALT-001, SWR-ALT-002 | ✓ |
| UNS-006 | Alert severity differentiation | SYS-005, SYS-006 | SWR-VIT-005, SWR-VIT-007, SWR-ALT-001, SWR-ALT-004 | ✓ |
| UNS-007 | BMI display | SYS-007 | SWR-VIT-006 | ✓ |
| UNS-008 | Patient identification | SYS-008 | SWR-PAT-001 | ✓ |
| UNS-009 | Vital sign history | SYS-009 | SWR-PAT-002, SWR-PAT-003 | ✓ |
| UNS-010 | Consolidated summary | SYS-011 | SWR-PAT-004, SWR-PAT-006, SWR-VIT-007 | ✓ |
| UNS-011 | Data integrity | SYS-010, SYS-012 | SWR-PAT-002, SWR-PAT-005, SWR-ALT-003, SWR-PAT-001 | ✓ |
| UNS-012 | Platform compatibility | SYS-012 | SWR-PAT-001, SWR-ALT-003 | ✓ |
| UNS-013 | User authentication | SYS-013 | SWR-GUI-001, SWR-GUI-002 | ✓ |
| UNS-014 | Graphical dashboard | SYS-014 | SWR-GUI-003, SWR-GUI-004 | ✓ |
| UNS-015 | Live monitoring feed | SYS-015 | SWR-GUI-005, SWR-GUI-006 | ✓ |

**Result: 15 / 15 User Needs covered ✓**

---

## Section 4 — SWR Coverage Summary

> Every SWR must have implementation and at least one test. Any gap is a finding.

| SWR ID | Implementation | Unit Tests | Integration Tests | Covered? |
|--------|----------------|------------|-------------------|----------|
| SWR-VIT-001 | `check_heart_rate()` | 14 | 2 | ✓ |
| SWR-VIT-002 | `check_blood_pressure()` | 12 | 1 | ✓ |
| SWR-VIT-003 | `check_temperature()` | 10 | 2 | ✓ |
| SWR-VIT-004 | `check_spo2()` | 8 | 2 | ✓ |
| SWR-VIT-005 | `overall_alert_level()` | 5 | 5 | ✓ |
| SWR-VIT-006 | `calculate_bmi()`, `bmi_category()` | 12 | 1 | ✓ |
| SWR-VIT-007 | `alert_level_str()` | 3 | all | ✓ |
| SWR-ALT-001 | `generate_alerts()` | 4 | 3 | ✓ |
| SWR-ALT-002 | `generate_alerts()` | 1 | 1 | ✓ |
| SWR-ALT-003 | `generate_alerts()` | 2 | — | ✓ |
| SWR-ALT-004 | `generate_alerts()` | 2 | — | ✓ |
| SWR-PAT-001 | `patient_init()` | 3 | 2 | ✓ |
| SWR-PAT-002 | `patient_add_reading()` | 5 | 3 | ✓ |
| SWR-PAT-003 | `patient_latest_reading()` | 2 | 1 | ✓ |
| SWR-PAT-004 | `patient_current_status()` | 4 | 7 | ✓ |
| SWR-PAT-005 | `patient_is_full()` | 2 | 1 | ✓ |
| SWR-PAT-006 | `patient_print_summary()` | 3 | — | ✓ |
| SWR-GUI-001 | `auth_validate()` | 10 | — | ✓ |
| SWR-GUI-002 | `attempt_login()`, `login_proc()`, logout | 5 | — | ✓ |
| SWR-GUI-003 | `paint_tile()`, `paint_tiles()`, `paint_status_banner()` | GUI demo | — | ✓ |
| SWR-GUI-004 | `create_dash_controls()`, `do_admit()`, `do_add_reading()` | GUI demo | — | ✓ |
| SWR-GUI-005 | `hw_vitals.h` HAL interface | Architecture review | — | ✓ |
| SWR-GUI-006 | `sim_vitals.c` : `hw_init()`, `hw_get_next_reading()` | GUI demo | — | ✓ |

**Result: 23 / 23 SWRs implemented and tested ✓**

---

## Section 5 — Test Count Summary

| Test File | Tests | SWRs Verified |
|-----------|-------|---------------|
| `tests/unit/test_vitals.cpp` | 64 | SWR-VIT-001 – SWR-VIT-007 |
| `tests/unit/test_alerts.cpp` | 11 | SWR-ALT-001 – SWR-ALT-004 |
| `tests/unit/test_patient.cpp` | 19 | SWR-PAT-001 – SWR-PAT-006 |
| `tests/integration/test_patient_monitoring.cpp` | 6 | SWR-PAT-*, SWR-VIT-* |
| `tests/integration/test_alert_escalation.cpp` | 6 | SWR-VIT-*, SWR-ALT-*, SWR-PAT-004 |
| `tests/unit/test_auth.cpp` | 15 | SWR-GUI-001, SWR-GUI-002 |
| **Total** | **121** | **21 SWRs** |

---

## Revision History

| Rev | Date       | Author          | Description          |
|-----|------------|-----------------|----------------------|
| A   | 2026-04-06 | vinu-engineer   | Initial release      |
| B   | 2026-04-07 | vinu-engineer   | Added SWR-GUI-001..004; 14/14 UNS, 21/21 SWR, 121 tests |
| C   | 2026-04-07 | vinu-engineer   | Added UNS-015, SWR-GUI-005/006 (HAL + sim); 15/15 UNS, 23/23 SWR, 121 tests |
