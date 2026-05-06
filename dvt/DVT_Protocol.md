# Design Verification Test (DVT) Protocol

**Document ID:** DVT-001-REV-F
**Project:** Patient Vital Signs Monitor  
**Version Under Test:** 2.7.0  
**Date:** 2026-05-06
**Standard:** IEC 62304 §5.7, §5.8  
**Author:** vinu-engineer  
**Status:** Approved  

---

## 1. Purpose

This protocol defines the Design Verification Test (DVT) evidence set for the
Patient Vital Signs Monitor software. DVT confirms that the released software
version is verified against the approved software requirements and that the
recorded evidence matches the executable test assets used for release review.

---

## 2. Scope and Evidence Sources

This DVT package is split into three evidence classes:

| Evidence class | Artefact | Role in release evidence |
|---|---|---|
| Automated unit + integration tests | `test_unit.exe`, `test_integration.exe`, `dvt/run_dvt.py` | Primary automated PASS/FAIL evidence for approved SWR IDs covered by GTest |
| Automated GUI checks | `dvt/automation/run_dvt.py` (`DVT-GUI-01` .. `DVT-GUI-20`) | Supplemental black-box evidence for GUI workflows and settings behavior |
| Residual manual GUI checks | Checklist in §4.3 | Visual/manual evidence for presentation-only behavior not reliably asserted by automation |

Approved SWR coverage in this protocol is limited to IDs defined in
`requirements/SWR.md` and traced in `requirements/TRACEABILITY.md`.

---

## 3. Entry Criteria

1. The feature branch under verification is rebased onto `origin/main`.
2. CMake configures successfully with `BUILD_TESTS=ON` when a build is required.
3. The approved SWR and RTM documents for the version under test are present.
4. No open P1/P2 defects remain against the version under test.

---

## 4. GUI Verification Approach

### 4.1 Automated GUI Evidence

| Evidence ID range | Primary scope | Approved SWR status |
|---|---|---|
| `DVT-GUI-01` .. `DVT-GUI-15` | Login, dashboard, patient entry, alerts, admin settings, sim controls | May be cited as supplemental evidence for approved GUI/SEC/ALM requirements |
| `DVT-GUI-16` | Localization selector presence | Supplemental GUI evidence for approved requirement `SWR-GUI-012` |
| `DVT-GUI-17` .. `DVT-GUI-20` | Alarm limits, sim controls, logout, version display | Supplemental GUI evidence |

Automated GUI checks supplement but do not replace the GTest evidence used by
`dvt/run_dvt.py` for the automated release verdict.

### 4.2 Approved GUI Requirements Covered by Automation vs Manual Review

| Requirement | Evidence mode | Notes |
|---|---|---|
| `SWR-GUI-001` | Automated GUI + unit tests | Login workflow and credential handling |
| `SWR-GUI-002` | Automated GUI + unit tests | Session/login flow evidence |
| `SWR-GUI-003` | Manual visual review | Colour rendering and repaint behavior |
| `SWR-GUI-004` | Automated GUI + manual review | Control presence can be automated; presentation remains visual |
| `SWR-GUI-005` | Architecture review | `test_hal.cpp` is supporting implementation evidence only; approved verification remains architecture review of HAL isolation |
| `SWR-GUI-006` | Manual GUI demo | `test_hal.cpp` exercises simulator behavior, but approved DVT evidence remains GUI demonstration of the clinical scenario cycle |
| `SWR-GUI-007` | Unit tests + automated GUI | Account-management API plus settings workflow |
| `SWR-GUI-008` | Manual visual review | Role badge/icon/presentation evidence |
| `SWR-GUI-009` | Automated GUI + manual review | Settings panel/tab structure plus visual confirmation |
| `SWR-GUI-010` | Manual GUI review + supplemental automation | `ConfigTest` covers persistence and GUI automation checks control presence, but full mode-switch behavior remains manual GUI verification |
| `SWR-GUI-011` | Manual visual review | Rolling status message content/motion is not asserted by current automation |
| `SWR-GUI-012` | Unit tests + supplemental automation | `LocalizationTest.*` covers API and persistence; `DVT-GUI-16` confirms selector presence with four options |
| `SWR-GUI-013` | Manual GUI review | Dedicated session alarm event list must stay distinct from current active alerts and disclose any bounded session reset |
| `SWR-GUI-014` | Manual GUI review + export-service tests | GUI outcome remains manual because overwrite prompts and local file placement are operator-facing; `SessionExportTest.*` supports the underlying export service |

### 4.3 Residual Manual GUI Checklist

The following items remain manual because they depend on rendered appearance,
layout behavior, or animated presentation rather than stable control state.

| ID | Requirement / Area | Test Action | Expected Result | Pass/Fail |
|---|---|---|---|---|
| GUI-MAN-01 | `SWR-GUI-003` | Observe warning/critical transitions | Tile colours and banner colours match alert severity | |
| GUI-MAN-02 | `SWR-GUI-008` | Inspect role badge and application icon | Correct role styling and application icon visible | |
| GUI-MAN-03 | `SWR-GUI-011` | Enable simulation mode and observe banner | Rolling message scrolls continuously on current alert background | |
| GUI-MAN-04 | Layout robustness | Resize dashboard window | Controls repaint and scale without clipping/overlap | |
| GUI-MAN-05 | Layout robustness | Maximize dashboard window | Full layout remains legible and anchored correctly | |
| GUI-MAN-06 | `SWR-GUI-013` | Drive a warning, critical, then recovery sequence and continue until the bounded session resets | Session Alarm Events retains the historical rows while Active Alerts returns to latest-only state, then shows an explicit session-reset disclosure when earlier rows are cleared | |
| GUI-MAN-07 | `SWR-GUI-014` | Export an active session review snapshot | Button is visible, overwrite confirmation appears when expected, file name is deterministic, exported status/alerts match the current session, and the created file shows owner-only read/write permissions when inspected with `icacls` | |

---

## 5. Automated GTest Evidence

### 5.1 Unit Tests (`test_unit.exe`)

| Test file / suites | Count | Automated scope / supporting evidence |
|---|---:|---|
| `tests/unit/test_vitals.cpp` (`HeartRate`, `BloodPressure`, `Temperature`, `SpO2`, `RespRate`, `OverallAlert`, `BMI`, `AlertStr`) | 80 | `SWR-VIT-001` .. `SWR-VIT-008` |
| `tests/unit/test_alerts.cpp` (`GenerateAlerts`) | 11 | `SWR-ALT-001` .. `SWR-ALT-004` |
| `tests/unit/test_patient.cpp` (`PatientInit`, `PatientAddReading`, `PatientLatestReading`, `PatientStatus`, `PatientIsFull`, `PatientAlertEvents`, `PatientPrintSummary`) | 29 | `SWR-PAT-001` .. `SWR-PAT-008` |
| `tests/unit/test_auth.cpp` (`UsersTest`) | 41 | `SWR-GUI-001`, `SWR-GUI-002`, `SWR-GUI-007`, `SWR-SEC-001` .. `SWR-SEC-004` |
| `tests/unit/test_news2.cpp` (`News2*`) | 53 | `SWR-NEW-001` |
| `tests/unit/test_alarm_limits.cpp` (`AlarmLimitsTest`) | 31 | `SWR-ALM-001` |
| `tests/unit/test_trend.cpp` (`TrendDirection`, `TrendExtract`) | 18 | `SWR-TRD-001` |
| `tests/unit/test_hal.cpp` (`HALTest`, `HALTestNoInit`, `SimSequenceTest`) | 12 | Supporting checks for HAL safety and simulator sequence behavior; not an approved automated DVT claim for `SWR-GUI-005` / `SWR-GUI-006` |
| `tests/unit/test_config.cpp` (`ConfigTest`) | 10 | Supporting persistence checks for `monitor.cfg`; not a full approved automated DVT claim for `SWR-GUI-010` |
| `tests/unit/test_localization.cpp` (`LocalizationTest`) | 8 | `SWR-GUI-012` |
| `tests/unit/test_session_export.cpp` (`SessionExportTest`) | 8 | `SWR-EXP-001` .. `SWR-EXP-003` |
| **Total** | **302** | |

### 5.2 Integration Tests (`test_integration.exe`)

| Test file / suites | Count | Primary approved requirements |
|---|---:|---|
| `tests/integration/test_patient_monitoring.cpp` (`PatientMonitoring`) | 7 | `SWR-PAT-001` .. `SWR-PAT-005`, `SWR-PAT-007`, `SWR-VIT-005`, `SWR-VIT-006`, `SWR-ALT-001`, `SWR-ALT-002` |
| `tests/integration/test_alert_escalation.cpp` (`AlertEscalation`) | 7 | `SWR-VIT-001` .. `SWR-VIT-004`, `SWR-PAT-004`, `SWR-PAT-007`, `SWR-ALT-001`, `SWR-ALT-002` |
| `tests/integration/test_session_export.cpp` (`SessionExportIntegrationTest`) | 3 | `SWR-EXP-001` .. `SWR-EXP-003` |
| **Total** | **17** | |

---

## 6. Pass Criteria

| Criterion | Threshold |
|---|---|
| Unit test pass rate | 100% (`302 / 302`) |
| Integration test pass rate | 100% (`17 / 17`) |
| Automated GTest total | 100% (`319 / 319`) |
| Automated GUI checks for approved IDs | All executed approved-ID checks pass |
| Manual GUI checklist | All applicable manual items marked Pass |

Manual-only GUI items are not part of the
automated PASS/FAIL verdict generated by `dvt/run_dvt.py`.

Supporting implementation tests such as `test_hal.cpp` and `test_config.cpp`
may execute during validation, but they do not convert `SWR-GUI-005`,
`SWR-GUI-006`, or `SWR-GUI-010` into automated PASS claims in the DVT report.

---

## 7. Exit Criteria

1. `dvt/run_dvt.py` reports PASS for all automated GTest evidence.
2. Any automated GUI run performed for release review is attached with its
   generated artefacts, including the `DVT-GUI-16` localization-selector check.
3. Residual manual GUI checklist items are completed and signed off.
4. No obsolete or nonexistent SWR IDs are shown as PASS in the DVT evidence set.

---

## 8. Traceability References

- Approved SWR: `requirements/SWR.md`
- Approved RTM: `requirements/TRACEABILITY.md`
- Non-GUI DVT runner: `dvt/run_dvt.py`
- GUI DVT automation: `dvt/automation/run_dvt.py`

---

## Revision History

| Rev | Date | Author | Description |
|---|---|---|---|
| A | 2026-04-07 | vinu-engineer | Initial DVT protocol for v2.0.0 evidence set |
| B | 2026-05-03 | Codex implementer | Aligned protocol with v2.7.0 test counts, approved SWR IDs, GUI automation split, and pending localization traceability note |
| C | 2026-05-03 | Codex implementer | Approved SWR-GUI-012 localization evidence and updated automated totals |
| D | 2026-05-05 | Codex implementer | Added session alarm event review evidence, GUI-MAN-06, and updated automated totals to 305 tests |
| E | 2026-05-06 | Codex implementer | Added session-reset disclosure evidence expectations and updated automated totals to 307 tests |
| F | 2026-05-06 | Codex implementer | Added session review snapshot export evidence, GUI-MAN-07, and updated automated totals to 319 tests |
