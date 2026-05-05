# Design Spec: Issue 38

Issue: `#38`  
Branch: `feature/38-export-patient-session-review-snapshot`  
Spec path: `docs/history/specs/38-export-patient-session-review-snapshot.md`

## Problem

The current product can show a live single-patient session in the Win32 GUI and
can print a console summary via `patient_print_summary()`, but it cannot create
a reusable artifact from the active session for handoff, design verification
evidence, or product demonstrations.

That leaves operators with two bad options:

- manually transcribe patient/session state from the dashboard
- rely on transient GUI state or console output that is not preserved as an
  explicit session snapshot

Because the requested artifact contains patient/demo data, the gap is not only
about convenience. Without an intentional export design, any future ad hoc file
write would risk wrong-patient snapshots, hidden `MAX_READINGS` truncation, or
weak local-file privacy controls.

## Goal

Add a narrow, manual, authenticated export flow that writes a deterministic
local `Session Review Snapshot` text artifact for the current patient session
only. The MVP must:

- stay output-only and leave monitoring, alerting, NEWS2, and alarm behavior
  unchanged
- reuse existing validated session state and alert/status logic
- disclose that history is bounded to the current session and at most
  `MAX_READINGS`
- create a reviewable artifact suitable for local handoff, DVT attachments, and
  product review

## Non-goals

- No CSV, PDF, HL7, EMR, network, email, sync, or background archival in this
  MVP. The first implementation should export UTF-8 text only.
- No multi-patient, central-station, or post-discharge workflow.
- No new thresholds, alarm-limit logic, NEWS2 logic, diagnosis, treatment
  advice, or alarm acknowledgement behavior.
- No change to authentication roles or privileged-access rules. Both existing
  authenticated roles may use the export because both already access the active
  patient session.
- No attempt to make the artifact a legal medical record, live monitoring
  surface, or de-identified shareable report.

## Intended Use Impact, User Population, Operating Environment, And Foreseeable Misuse

- Intended use impact: provide an operator-triggered local snapshot of the
  current session for review and evidence capture without changing clinical
  monitoring behavior.
- User population: authenticated admin or clinical users already using the
  desktop monitor.
- Operating environment: local Windows workstation running the GUI in
  simulation mode or device mode.
- Foreseeable misuse:
  - treating the snapshot as a live monitor view instead of a point-in-time
    export
  - assuming the snapshot contains more than the current bounded session
  - exporting immediately after patient clear/re-admit and assuming an older
    session is still represented
  - sharing the local text file outside the pilot without privacy review

## Current Behavior

- `src/gui_main.c` maintains the active session in `g_app.patient`,
  `g_app.has_patient`, `g_app.sim_enabled`, `g_app.sim_paused`, and
  `g_app.alarm_limits`.
- The dashboard already derives two useful text surfaces from that state:
  - `update_dashboard()` formats the reading-history list from
    `PatientRecord.readings[]` and active-alert list from `generate_alerts()`.
  - `paint_patient_bar()` and `paint_status_banner()` derive current session and
    status context from the same in-memory session.
- `src/patient.c` provides `patient_print_summary()`, but that writes only to
  `stdout`, includes only the latest vitals and active alerts, and does not
  include simulation/device context, alarm-limit context, or the full bounded
  reading history.
- Session-boundary hooks already exist:
  - `do_admit()` reinitializes `g_app.patient`
  - `do_clear()` zeroes the in-memory session
  - `apply_sim_mode()` clears the patient state when simulation is disabled
  - logout zeroes the session before returning to the login window
- There is no export button, no export module, no deterministic snapshot path,
  and no restricted patient-artifact file write path today.

## Proposed Change

1. Add a manual dashboard-level action labelled `Export Session Review`.
   Preferred placement is alongside other session actions so it is visible to
   both authenticated roles without entering Settings.

2. Export only when there is an admitted patient and at least one recorded
   reading. If `g_app.has_patient == 0` or `reading_count == 0`, show a
   `MessageBox` and write nothing.

3. Use a new application-service module for the export, for example
   `include/session_export.h` and `src/session_export.c`, instead of pushing
   Windows file-path and permission logic into `patient.c`. The module should
   accept the current session state as plain inputs:
   - `PatientRecord`
   - `AlarmLimits`
   - simulation/device context (`sim_enabled`, `sim_paused`)
   - optional path override for tests

4. Export format for the MVP is UTF-8 text only. Choose text, not CSV, because
   it best matches the existing human-readable summary semantics and can carry
   alert text, mode context, and bounded-history disclosure without inventing a
   more complex schema. CSV can be a follow-on issue if needed.

5. Write the artifact to the executable directory, using the same resolution
   model already used by `app_config.c`. Use a deterministic filename that does
   not expose the patient name in the path:

   `session-review-patient-<patient-id>.txt`

   Example:

   `session-review-patient-1001.txt`

6. If that file already exists, do not silently overwrite it. Prompt the user
   for explicit confirmation before replacement. If the user cancels, leave the
   current session unchanged and report no export success.

7. Create the file with restrictive local permissions at least as strict as the
   `users.dat` write path in `src/gui_users.c`. The export module should own
   this behavior rather than broadening access through generic `fopen("w")`.

8. The artifact must clearly identify itself as a bounded snapshot, not a live
   feed. Required sections:
   - title: `Session Review Snapshot`
   - snapshot format/version marker
   - generation timestamp
   - patient demographics: ID, name, age, weight, height, BMI category
   - mode context: simulation enabled/disabled and paused/live state
   - alarm-limit context: current low/high values from `g_app.alarm_limits`
   - latest vital signs with the same classifications used by current domain
     logic
   - overall patient status
   - active alerts, or explicit `No active alerts`
   - reading history for the current session in recorded order
   - explicit retention boundary text stating that history is limited to the
     current session and at most `MAX_READINGS` readings

9. Reuse existing validated derivation logic rather than duplicating new
   clinical rules:
   - use `patient_latest_reading()`
   - use `patient_current_status()` / `overall_alert_level()`
   - use `generate_alerts()`
   - use existing per-parameter classification helpers and `alert_level_str()`

10. Reduce cross-surface drift by extracting small shared text-format helpers
    for history rows and alert rows from `update_dashboard()` into a reusable
    helper layer owned by the export module or a nearby shared formatter.
    `patient_print_summary()` may remain console-only in the first pass, but
    exported latest-vitals and alert semantics must stay aligned with it.

11. Keep export read-only and session-scoped. No saved export should feed back
    into monitoring, acknowledgement, or patient-state mutation.

## Files Expected To Change

Expected production files:

- `src/gui_main.c`
- `src/session_export.c` (new)
- `include/session_export.h` (new)
- `src/localization.c`
- `include/localization.h`

Expected test/build files:

- `tests/CMakeLists.txt`
- `tests/unit/test_session_export.cpp` (new)
- one integration test file for session-boundary and parity checks, either:
  - `tests/integration/test_patient_monitoring.cpp`, or
  - `tests/integration/test_session_export.cpp` (new)

Expected requirements and evidence files:

- `requirements/UNS.md`
- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`
- `README.md`
- `dvt/DVT_Protocol.md`

Files that should not change unless an implementer makes a narrowly justified
refactor:

- `src/vitals.c`
- `src/alerts.c`
- `src/news2.c`
- `src/alarm_limits.c`
- authentication logic in `src/gui_users.c` and `src/gui_auth.c`

Possible but not required:

- `src/patient.c`
- `include/patient.h`

Those should change only if the implementer extracts a genuinely shared summary
formatter and keeps the diff reviewable.

## Requirements And Traceability Impact

This feature needs explicit new requirements. It should not be implemented as a
GUI-only convenience patch without traceability.

Preferred traceability shape:

- Add one new user need for manual export of the active patient session for
  local review/evidence.
- Add new system requirements covering:
  - authenticated manual snapshot export
  - deterministic local file naming and overwrite policy
  - bounded-session disclosure and restrictive local file handling
- Add a new software-requirement section for the export module (preferred:
  dedicated export requirement IDs rather than overloading `SWR-PAT-006`)
- Add or amend one GUI requirement for button presence and basic user feedback
  if that is clearer than hiding the UI trigger inside the export module text

Important numbering note:

- `requirements/SWR.md` already contains a stale historical reference to
  nonexistent `SYS-018` under `SWR-NEW-001`.
- Do not reuse `SYS-018` for this export feature unless that inconsistency is
  explicitly cleaned up at the same time.
- The implementer should allocate the next approved non-conflicting SYS ID(s),
  or resolve the `SYS-018` anomaly first with human review.

Existing requirement context that remains relevant:

- `UNS-008`, `UNS-009`, `UNS-010`, `UNS-013`
- `SYS-008`, `SYS-009`, `SYS-011`, `SYS-013`, `SYS-014`
- `SWR-PAT-006`, `SWR-GUI-003`, `SWR-GUI-004`, `SWR-GUI-010`

Traceability follow-through required at implementation time:

- RTM rows for the new export requirement(s)
- unit and integration evidence references
- DVT protocol update for manual snapshot verification

## Medical-Safety, Security, And Privacy Impact

- Medical safety: no direct change to acquisition, classification, thresholds,
  NEWS2, or alarm escalation. The safety impact is indirect because humans may
  use the exported artifact during handoff or retrospective review.
- Main safety risks:
  - stale or wrong-session data exported after clear/admit/logout transitions
  - hidden bounded-history truncation
  - drift between dashboard state, `patient_print_summary()`, and exported text
- Security: no new privilege model is introduced, but the feature adds a new
  persistence surface for patient/demo data. The file must stay local-only and
  be created with restricted permissions.
- Privacy: material impact because patient/demo data leaves RAM and lands on
  disk. File naming should avoid patient names in the path, and the UI/file
  contents should not imply that unrestricted sharing is safe.

## AI/ML Impact Assessment

This change does not add, change, remove, or depend on an AI-enabled device
software function.

- Model purpose: none
- Input data to a model: none
- Model output: none
- Human-in-the-loop limits: unchanged
- Transparency needs: unchanged beyond clearly labelling the export as a
  bounded session snapshot
- Dataset or bias considerations: none
- Monitoring expectations: no AI monitoring needed
- PCCP impact: none

## Validation Plan

Implementation validation should combine targeted automated checks with one
manual evidence step.

Required automated coverage:

- unit tests for deterministic path/filename generation
- unit tests for text serialization content, including:
  - patient demographics
  - mode context
  - alarm-limit context
  - latest status and active alerts
  - bounded reading history disclosure
- unit tests for no-patient / no-reading refusal behavior
- unit tests for overwrite policy behavior
- integration tests for session-boundary resets across:
  - `do_admit()`
  - `do_clear()`
  - simulation disable/reset behavior
  - logout/relogin path if the test seam allows it
- parity checks showing that exported alert/status/history values come from the
  same session state as the dashboard and current domain helpers

Manual evidence:

- add one DVT/manual verification step that exports a snapshot from a known
  session and confirms:
  - the file is written to the intended local directory
  - the file name matches the deterministic naming rule
  - the file declares the bounded-history limit
  - the exported latest status and alerts match the on-screen session

Suggested commands after implementation:

```powershell
cmake --build build
ctest --test-dir build --output-on-failure -R "SessionExport|PatientMonitoring"
python dvt/run_dvt.py
```

Diff-scope checks:

```powershell
git diff --name-only
rg -n "Session Review Snapshot|session-review-patient-|Export Session Review" src include tests requirements README.md dvt
```

## Rollback Or Failure Handling

- If the export file cannot be created with the intended restrictive local
  permissions, fail the export and show an error. Do not fall back to a broader
  access mode silently.
- If implementation pressure expands scope into CSV, PDF, network export,
  multi-patient review, or background persistence, stop and split the work into
  follow-on issues instead of stretching this MVP.
- If the product owner rejects executable-relative storage or overwrite
  confirmation, treat that as a design-scope change and reopen requirements
  review rather than improvising a new retention policy during coding.
- Rollback is straightforward because the change is additive: remove the export
  button and export module, and leave existing dashboard/session behavior
  unchanged.
