# Validation Record: Issue 38 GUI-MAN-07

- Issue: `#38`
- Branch: `feature/38-export-patient-session-review-snapshot`
- Manual GUI exercise executed: `2026-05-06T10:41:51Z`
- Evidence record refreshed: `2026-05-06T12:41:27Z`
- Requirements covered: `SWR-GUI-014` with supporting permission evidence for `SWR-EXP-001`
- Result: `PASS`

## Scope

This record documents the executed manual export workflow for `GUI-MAN-07`.
The later repair in this cycle changed overwrite durability behind the same
operator-visible prompt path, so this record was refreshed with branch-scoped
metadata, reviewable repo-relative evidence links, and the current automated
validation totals.

## Procedure

1. Build the current branch head:
   `cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DBUILD_TESTS=ON -Wno-dev`
2. Rebuild the GUI and automated test targets:
   `cmake --build build --target patient_monitor_gui test_unit test_integration`
3. Re-run automated validation:
   `ctest --test-dir build --output-on-failure`
4. Launch `build/patient_monitor_gui.exe`, log in as `admin`, run `Demo: Deterioration`, then pause simulation before export so overwrite verification exercises the same clinical session content.
5. Export the session review snapshot once to confirm deterministic file creation, then export twice more to verify the `No` and `Yes` overwrite paths.
6. Inspect the created file permissions with `icacls`.
7. Re-run `python dvt/run_dvt.py --no-build --build-dir build` and `cmd /c "echo.|run_tests.bat"` so the automated evidence in this record matches the current branch head.

## Observations

| Check | Result | Evidence |
| --- | --- | --- |
| Export button visible on authenticated dashboard | PASS | `IDC_BTN_EXPORT` was present and visible on `PVM_Dash` after admin login. |
| Deterministic local file creation | PASS | The first export created `build/session-review-patient-1001.txt`. A tracked copy is stored in [38-session-review-patient-1001.txt](38-session-review-patient-1001.txt). |
| Exported status and alerts match the current session | PASS | The exported snapshot retained all 5 dashboard alert rows and all 3 dashboard history rows from the paused deterioration scenario and reported `Overall Status : CRITICAL`. |
| Alarm limits listed separately for session context | PASS | The snapshot contains a dedicated `Alarm Limit Context` section above the latest-vitals summary. |
| Overwrite confirmation shown when expected | PASS | The second export displayed the replacement prompt; selecting `No` left the file unchanged, and selecting `Yes` replaced it successfully. Because simulation was paused first, the replacement write changed only the UTC generation timestamp, not the clinical content. |
| Restricted local permissions | PASS | `icacls` output in [38-session-review-patient-1001.icacls.txt](38-session-review-patient-1001.icacls.txt) shows explicit access for the current user, `BUILTIN\\Administrators`, and `NT AUTHORITY\\SYSTEM`, with no `Everyone`, `Authenticated Users`, or `BUILTIN\\Users` ACEs. |
| Overwrite failure preserves the published snapshot | PASS | Automated regression `SessionExportTest.SWR_EXP_003_OverwriteFailurePreservesExistingSnapshot` now proves that an approved overwrite leaves the prior snapshot intact if the replacement step fails before publication. |

## Dashboard State Observed

- Alerts:
  - `[WARNING]  Heart rate 135 bpm [normal 60-100]`
  - `[WARNING]  BP 175/112 mmHg [normal 90-140 / 60-90]`
  - `[CRITICAL]  Temp 39.8 C [normal 36.1-37.2]`
  - `[CRITICAL]  SpO2 87% [normal 95-100%]`
  - `[CRITICAL]  RR 27 br/min [normal 12-20]`
- History:
  - `#1  HR 78 | BP 122/82 | Temp 36.7 C | SpO2 98% | RR 15 br/min  [NORMAL]`
  - `#2  HR 108 | BP 148/94 | Temp 37.9 C | SpO2 93% | RR 23 br/min  [WARNING]`
  - `#3  HR 135 | BP 175/112 | Temp 39.8 C | SpO2 87% | RR 27 br/min  [CRITICAL]`

## Validation Summary

- `ctest --test-dir build --output-on-failure` - PASS
- `python dvt/run_dvt.py --no-build --build-dir build` - PASS
  - `test_unit`: `303 / 303`
  - `test_integration`: `17 / 17`
- `cmd /c "echo.|run_tests.bat"` - PASS (`320 / 320`)

## Requirements Note

No requirement IDs changed in this repair cycle. The implementation repair adds
explicit overwrite-failure preservation coverage, and the documentation refresh
keeps the manual export evidence aligned with `GUI-MAN-07` and the current
branch totals.