# Design Spec: DVT protocol and runner evidence alignment

Issue: #13  
Branch: `feature/13-dvt-align-protocol-runner-mappings`  
Spec path: `docs/history/specs/13-dvt-align-protocol-runner-mappings.md`

## Problem

The DVT protocol and DVT runner no longer match the current v2.7.0 evidence set.
`dvt/DVT_Protocol.md` still identifies version under test `2.0.0`, lists 121
unit tests and 21 integration tests, and references obsolete/nonexistent
patient requirement IDs `SWR-PAT-007` and `SWR-PAT-008`. The current RTM and
README describe 287 tests total: 275 unit tests and 12 integration tests.

The executable DVT mapping in `dvt/run_dvt.py` has the same stale patient
requirement references, while `dvt/automation/run_dvt.py` now records automated
GUI evidence from `DVT-GUI-01` through `DVT-GUI-20`, including newer checks for
alarm limits and localization-related GUI behavior. This leaves release evidence
internally inconsistent even when the code and tests pass.

## Goal

Align DVT documentation and DVT runner requirement mappings with the current
v2.7.0 requirement and test evidence without changing runtime behavior.

The implementation should:

- Update `dvt/DVT_Protocol.md` to describe v2.7.0 evidence, current test counts,
  current requirement IDs, and the split between automated GTest evidence,
  automated GUI evidence, and residual manual GUI checks.
- Update `dvt/run_dvt.py` so `REQUIREMENT_MAP` contains only valid SWR IDs from
  the approved requirements set, with current test suite names and descriptions.
- Reconcile or explicitly flag GUI automation evidence for `SWR-GUI-011` and
  `SWR-GUI-012` so generated DVT reports do not silently imply obsolete or
  nonexistent traceability.
- Keep all changes limited to DVT documentation/test-harness evidence mapping.

## Non-goals

- Do not change production source code, clinical threshold logic, NEWS2 scoring,
  alarm-limit behavior, authentication, persistence, or GUI behavior.
- Do not add new clinical claims, new requirement IDs, or new acceptance criteria
  beyond existing project requirements.
- Do not replace the DVT framework or CI pipeline.
- Do not commit generated DVT result files unless a separate release process
  explicitly requests that.

## Current behavior

`dvt/DVT_Protocol.md`:

- Lists `Version Under Test: 2.0.0`.
- States GUI-only requirements `SWR-GUI-001..010` are verified by manual
  inspection, despite automated GUI verification in `dvt/automation/run_dvt.py`.
- Maps unit tests to stale ranges including `SWR-PAT-001..008`.
- Maps integration tests to nonexistent `SWR-PAT-007` and stale counts.
- Uses pass criteria of 121 unit tests and 21 integration tests.
- Points traceability at `RTM-001-REV-E`, while `requirements/TRACEABILITY.md`
  is `RTM-001-REV-G`.

`dvt/run_dvt.py`:

- Includes `SWR-PAT-007` and `SWR-PAT-008` in `REQUIREMENT_MAP`, but
  `requirements/SWR.md` currently defines only `SWR-PAT-001` through
  `SWR-PAT-006`.
- Omits explicit mappings for newer GUI evidence such as `SWR-GUI-011`; it also
  does not account for the GUI automation script's `SWR-GUI-012` reference.

`dvt/automation/run_dvt.py`:

- Produces black-box GUI evidence IDs `DVT-GUI-01` through `DVT-GUI-20`.
- Records checks for `SWR-ALM-001`, `SWR-GUI-010`, and `SWR-GUI-012`, but the
  approved SWR and RTM files in the current branch do not yet define
  `SWR-GUI-012`.

## Proposed change

1. Update the DVT protocol header and traceability references.
   - Set the version under test to `2.7.0`.
   - Reference `requirements/TRACEABILITY.md` as `RTM-001-REV-G`.
   - Update the protocol scope to distinguish:
     - automated GTest evidence from `test_unit.exe` and `test_integration.exe`;
     - automated GUI evidence from `dvt/automation/run_dvt.py`;
     - remaining visual/manual checks such as color rendering, icon appearance,
       resize behavior, and maximize behavior.

2. Replace stale DVT protocol requirement ranges and test counts.
   - Use current patient IDs `SWR-PAT-001..006` only.
   - Include current newer modules: `SWR-VIT-008`, `SWR-NEW-001`,
     `SWR-ALM-001`, `SWR-TRD-001`, and `SWR-GUI-010..011`.
   - Update pass criteria to 275/275 unit tests and 12/12 integration tests, for
     287 automated GTest tests total, matching README and RTM evidence.

3. Update `dvt/run_dvt.py` requirement mapping.
   - Remove `SWR-PAT-007` and `SWR-PAT-008`.
   - Map overflow behavior to `SWR-PAT-002` and/or `SWR-PAT-005` as applicable,
     because those are the approved patient storage and capacity requirements.
   - Keep DVT runner mappings aligned to actual GTest suite names visible in the
     generated XML: `HeartRate`, `BloodPressure`, `Temperature`, `SpO2`,
     `OverallAlert`, `BMI`, `AlertStr`, `RespRate`, `GenerateAlerts`,
     `PatientInit`, `PatientAddReading`, `PatientLatestReading`,
     `PatientStatus`, `PatientIsFull`, `PatientPrintSummary`, `UsersTest`,
     `News2*`, `AlarmLimitsTest`, `TrendDirection`, `TrendExtract`,
     `HALTestNoInit`, and `ConfigTest`.
   - Treat GUI requirements as manual or GUI-automation evidence, rather than
     mapping them to unrelated GTest suites.

4. Handle `SWR-GUI-012` explicitly.
   - If `SWR-GUI-012` is still absent from `requirements/SWR.md` and
     `requirements/TRACEABILITY.md` at implementation time, do not present it as
     an approved DVT requirement in `dvt/run_dvt.py`.
   - The protocol may note that `dvt/automation/run_dvt.py` currently emits a
     localization-related `SWR-GUI-012` reference that must be reconciled by the
     localization traceability work before release evidence can claim full
     coverage for that ID.
   - If a separate traceability change has already approved `SWR-GUI-012`, then
     update the DVT protocol and GUI automation evidence table to cite it
     consistently.

5. Keep generated reports deterministic enough for audit review.
   - Ensure the DVT report's requirement summary cannot show `PASS` for
     nonexistent SWR IDs.
   - Ensure manual-only GUI items are clearly identified as not part of the
     automated pass/fail decision.

## Files expected to change

Expected implementation files:

- `dvt/DVT_Protocol.md`
- `dvt/run_dvt.py`

Possible implementation files, only if needed to align evidence labels and not
runtime behavior:

- `dvt/automation/run_dvt.py`
- `README.md`
- `requirements/TRACEABILITY.md`

`README.md` and `requirements/TRACEABILITY.md` should only change if the
implementation discovers a current evidence mismatch that must be corrected to
keep the DVT protocol truthful. Do not change production code under `src/`,
`include/`, or application build logic for this issue.

## Requirements and traceability impact

This is an evidence-alignment change. It should not modify system or software
requirements, but it touches traceability and must be reviewed as a regulated
documentation/test-harness change.

Affected requirement evidence:

- `SWR-PAT-001` through `SWR-PAT-006`: remove stale DVT references to
  nonexistent `SWR-PAT-007` and `SWR-PAT-008`.
- `SWR-GUI-001` through `SWR-GUI-011`: clarify whether evidence is automated
  GUI verification, manual visual verification, or GTest coverage.
- `SWR-GUI-012`: do not treat as approved in DVT evidence unless the SWR and RTM
  define it by implementation time.
- `SWR-VIT-008`, `SWR-NEW-001`, `SWR-ALM-001`, and `SWR-TRD-001`: include in the
  DVT protocol and runner evidence model where current tests already cover them.

Traceability safety rule callout: this change touches DVT, requirements
traceability, and release evidence. It does not change clinical behavior.

## Medical-safety, security, and privacy impact

Medical-safety impact is indirect and positive: release reviewers should see DVT
evidence that matches the executable tests and approved requirement IDs. The
change must not alter vital classification, NEWS2 scoring, alarm limits, or any
clinical response behavior.

Security impact is limited to evidence mapping for existing authentication and
account-management requirements. Do not change credential storage,
authentication behavior, role enforcement, file permissions, or persistence.

Privacy impact is none expected. The change should not add patient data,
credentials, telemetry, or generated reports containing real patient
information.

## Validation plan

Run static searches first:

```powershell
rg -n "Version Under Test|121|21|287|SWR-PAT-007|SWR-PAT-008|SWR-GUI-011|SWR-GUI-012" dvt requirements README.md
```

Expected after implementation:

- `dvt/DVT_Protocol.md` no longer references version `2.0.0`, 121/21 pass
  criteria, or `SWR-PAT-007`/`SWR-PAT-008`.
- `dvt/run_dvt.py` no longer contains `SWR-PAT-007` or `SWR-PAT-008`.
- Any remaining `SWR-GUI-012` references are either backed by approved SWR/RTM
  entries or clearly identified as pending traceability reconciliation.

Run DVT/test validation where the local environment supports it:

```powershell
python dvt/run_dvt.py --no-build --build-dir build
python dvt/automation/run_dvt.py build\patient_monitor_gui.exe
run_tests.bat
```

If GUI automation or a built GUI executable is unavailable on the validation
host, document that limitation in the implementation handoff and still validate
the non-GUI DVT runner plus the protocol text changes.

## Rollback or failure handling

If the implementation cannot reconcile the DVT protocol without adding or
changing approved requirements, stop and leave the issue in `ready-for-design`
with a comment describing the required split. In particular, do not define
`SWR-GUI-012` inside DVT artifacts as a workaround for missing SWR/RTM content.

If DVT runner changes cause report generation to mark valid automated evidence
as `NOT_RUN`, revert the runner mapping portion and keep the protocol-only
correction as a smaller documentation change.

Rollback is straightforward because this issue should only touch DVT
documentation and test-harness mapping: revert the affected DVT docs/script
commit and restore the previous generated report behavior.
