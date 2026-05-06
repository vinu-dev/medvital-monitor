# Design Spec: Issue 91

Issue: `#91`  
Branch: `feature/91-export-session-alarm-event-log-to-csv`  
Spec path: `docs/history/specs/91-export-session-alarm-event-log-to-csv.md`

## Problem

The current session alarm event review feature is useful only while the user is
inside the application:

- `src/gui_main.c` rebuilds `IDC_LIST_EVENTS` from the current in-memory
  `PatientRecord` and shows the rows only in the dashboard.
- `src/patient.c` prints the same session event history only to the console
  summary.
- `PatientRecord` stores only the event reading index, aggregate severity,
  abnormal-parameter mask, and summary text. It does not retain an event
  timestamp or an export-oriented source field.

That means a clinician, tester, or reviewer cannot hand off or archive the
exact session alarm review list they inspected. The current design also cannot
faithfully produce the issue's requested CSV columns (`timestamp`,
`severity/priority`, `source`, `summary`) because timestamps are not captured
today and `source` is only implicit in `abnormal_mask`.

## Goal

Add a narrow, local-only `Export Alarm Log` workflow for the session alarm event
review list that:

- exports the currently visible session review rows in visible order
- writes one deterministic UTF-8 CSV artifact to a user-chosen local path
- reuses the existing alert-event model and alert semantics as the source of
  truth
- discloses session scope and simulation/device mode clearly enough that the
  file is not mistaken for a live alarm view or a complete medical record
- does not change acquisition, NEWS2, alarm limits, authentication policy, or
  any alert-classification logic

## Non-goals

- Changing alert thresholds, NEWS2 scoring, alarm-limit logic, or clinician
  guidance.
- Adding background export, scheduled export, network upload, cloud sync, EMR
  integration, email, or printing/PDF output.
- Exporting raw vital-sign history, waveforms, multi-patient data, or a full
  legal medical record.
- Inventing a new alarm-priority taxonomy separate from the existing
  `AlertLevel` values already used by the application.
- Adding collaborative review workflow, annotations, acknowledgements, or
  response tracking.
- Bundling the filter feature from issue `#89` into this issue. If that feature
  lands first, export should reuse its active view state rather than re-solve
  filtering here.

## Intended Use Impact, User Population, Operating Environment, And Foreseeable Misuse

Intended use impact:

- The feature adds a retrospective handoff/archive artifact for the existing
  session alarm review surface.
- It does not change the primary live-monitoring workflow or the active-alert
  list.

User population:

- Trained clinical staff, internal testers, and product reviewers using the
  local Windows workstation application.

Operating environment:

- One authenticated desktop session with one active patient/session and the
  existing bounded session alarm event review list.

Foreseeable misuse:

- Treating the exported CSV as a complete or current patient record.
- Treating a filtered or reset-bounded export as the full session history.
- Exporting simulation-mode data and later presenting it as real clinical
  evidence.
- Saving the artifact to a shared or synced path without recognizing the
  privacy impact.
- Opening the CSV in spreadsheet software that interprets text cells as
  formulas unless the export neutralizes that risk.

## Current Behavior

- `PatientRecord` stores up to `MAX_ALERT_EVENTS` derived `AlertEvent` rows for
  the current patient session.
- `AlertEvent` currently contains:
  - `reading_index`
  - `level`
  - `abnormal_mask`
  - `summary`
- `patient_add_reading()` derives event rows from the existing validated alert
  engine and appends them only when the alert signature changes.
- `update_dashboard()` repopulates `IDC_LIST_EVENTS` directly from
  `patient_alert_event_count()` and `patient_alert_event_at()`, preserving
  order and optionally inserting the session-reset notice ahead of current
  rows.
- Empty states are rendered as UI placeholder strings, not as exportable data.
- `include/vitals.h` and `include/patient.h` do not retain observation-time
  metadata, so the requested event timestamp column cannot be reconstructed
  later from stored domain data.
- The repo has an existing protected local-file creation pattern in
  `src/gui_users.c` (`open_write_restricted()`), but no alarm-log export
  service, no save dialog flow, and no CSV escaping/formula-neutralization
  logic.

## Proposed Change

1. Keep the feature intentionally narrow: a manual `Export Alarm Log` action on
   the dashboard session review surface only. No automatic, background, or
   remote export path belongs in this MVP.
2. Add a dashboard export control adjacent to the `Session Alarm Events` list.
   The control should be disabled when there is no exportable session-review
   content and enabled only when the visible review surface contains at least
   one real row:
   - one or more current-session event rows, or
   - a session-reset disclosure row that is currently shown to the user
3. Do not export placeholder strings such as `No patient admitted yet.` or
   `No session alarm events recorded in current session.` Those strings are UI
   state, not review evidence.
4. Introduce a small shared "session alarm review view" representation in the
   GUI/service layer so both dashboard rendering and CSV export consume the same
   ordered rows. This avoids future drift between what is visible and what is
   written.
5. Treat issue `#89` as a view-state integration point, not as a blocker. If
   filter controls are present when implementation happens, export must reuse
   the active filtered view. If not, `view_scope` is simply the full current
   session review list.
6. Extend the stored session event model so each recorded alert event carries a
   deterministic event timestamp captured when the reading is successfully
   appended. The timestamp must be captured at event-creation time, not
   fabricated during export.
7. Keep the timestamp representation platform-neutral and bounded. A fixed-size
   ISO 8601 local-time string (or equivalent deterministic text form) stored on
   the event record is preferred because it avoids locale-dependent rendering
   during export and does not require heap allocation.
8. Do not invent new clinical logic to produce the CSV `source` column. Derive
   it deterministically from the existing abnormal-parameter signature:
   - abnormal events export a comma-separated parameter source derived from
     `abnormal_mask`
   - recovery rows export a literal source such as `Recovery`
9. Use the existing `AlertLevel` vocabulary (`NORMAL`, `WARNING`, `CRITICAL`)
   as the exported severity column. Do not add a second "priority" taxonomy
   unless the requirements are explicitly expanded.
10. Export a fixed CSV schema in a stable column order. The recommended MVP
    schema is:

    ```text
    row_kind,view_scope,session_mode,reading_index,event_timestamp,severity,source,summary
    ```

    Where:
    - `row_kind` distinguishes `alarm_event` from `session_reset_notice`
    - `view_scope` captures whether the export is the full current session or a
      filtered subset
    - `session_mode` is `simulation` or `device`
    - `reading_index`, `event_timestamp`, `severity`, `source`, and `summary`
      preserve the review evidence the user is exporting
11. Preserve visible ordering exactly. If a session-reset disclosure is visible,
    it must appear in the CSV before the current-session event rows, just as it
    appears in the review list.
12. Keep the default payload minimal. Do not add patient name, patient ID,
    location, account name, or other direct identifiers to the CSV in this MVP.
    If product ownership later requires identifiers, treat that as a separate
    requirement and privacy/risk decision.
13. Use deterministic CSV escaping and spreadsheet hardening:
    - fixed header row and fixed column order
    - quoted/escaped text fields
    - formula-neutralization for any text field that begins with `=`, `+`, `-`,
      or `@`
    - no locale-dependent numeric or date formatting at export time
14. Use a Windows-appropriate local save flow with a default filename that
    conveys artifact scope without adding identifiers. A pattern such as
    `session-alarm-log-YYYYMMDD-HHMMSS[-sim].csv` is sufficient.
15. Reuse the existing protected-file pattern from `src/gui_users.c` or extract
    it into a shared helper so exported files are created with best-effort
    owner-only permissions where the selected path supports them.
16. Prefer complete-file write semantics over partial output:
    - create/write a temporary sibling file
    - flush and close successfully
    - replace or rename to the requested target only after successful write
    - surface overwrite, cancel, and write-failure outcomes explicitly in the UI
17. Keep the export read-only. Export must never mutate stored event rows,
    clear session state, or affect active alerts, reading history, or session
    rollover behavior.
18. Keep implementation ownership narrow:
    - `patient.c` / `patient.h` own any added event metadata needed for truthful
      export
    - a new export service module should own CSV formatting and restricted file
      creation
    - `gui_main.c` should own the button, save dialog, success/failure feedback,
      and integration with the visible review list

## Files Expected To Change

Expected requirements and traceability files:

- `requirements/UNS.md`
- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`

Expected implementation files:

- `include/patient.h`
- `src/patient.c`
- `src/gui_main.c`
- `include/localization.h`
- `src/localization.c`
- `include/session_alarm_export.h`
- `src/session_alarm_export.c`

Expected verification files:

- `tests/CMakeLists.txt`
- `tests/unit/test_patient.cpp`
- `tests/unit/test_session_alarm_export.cpp`
- `tests/integration/test_patient_monitoring.cpp`
- `dvt/DVT_Protocol.md`
- `dvt/run_dvt.py`

Files expected not to change:

- `src/vitals.c`
- `src/alerts.c`
- `src/news2.c`
- `src/alarm_limits.c`
- `src/gui_users.c` behavior beyond possible helper reuse for restricted file
  creation

## Requirements And Traceability Impact

Existing requirements directly impacted:

- `UNS-011` Data Integrity
- `UNS-013` User Authentication
- `UNS-017` Session Alarm Event Review
- `SYS-012` Static Memory Allocation
- `SYS-020` Session Alarm Event Review Storage
- `SYS-021` Session Alarm Event Review Presentation
- `SWR-PAT-007` Session Alarm Event Capture
- `SWR-PAT-008` Session Alarm Event Access and Reset
- `SWR-GUI-013` Session Alarm Event Review List
- `SWR-SEC-001` only as a reuse pattern for restricted local-file creation

New derived requirements are likely needed for:

- a user-visible need for exporting the session review artifact, if the team
  wants that behavior represented above SYS level rather than folded into
  `UNS-017`
- deterministic event timestamp capture for review/export
- deterministic source rendering from the stored event signature
- CSV schema, encoding, quoting, and formula-neutralization rules
- dashboard control behavior for export-enabled, export-disabled, cancel,
  overwrite, success, and failure states
- parity between the visible review rows and the exported rows
- explicit exclusion of background, networked, or multi-patient export

Traceability expectations:

- export logic should trace back to the existing session alarm review feature,
  not to any new clinical algorithm
- timestamp capture should trace to data-integrity requirements, because a CSV
  timestamp must reflect when the event was recorded, not when it was exported
- privacy/security controls should trace to authenticated local use and
  restricted artifact creation, not to a new access-control model

## Medical-Safety, Security, And Privacy Impact

Medical safety:

- No intended change to alert thresholds, NEWS2 scoring, alarm limits, or live
  alert generation.
- The main safety benefit is better retrospective handoff/review of transient
  alarm episodes.
- The main safety risk is interpretive: a user may mistake the CSV for a live
  alarm view, a complete patient record, or the full session history if filter
  or reset boundaries are not obvious.
- The design remains acceptable only if the export stays clearly labeled as a
  session-scoped review artifact and preserves the visible reset/filter context.

Security:

- The feature introduces a new portable data artifact and therefore a new local
  persistence path.
- File creation should follow the existing best-effort owner-only restriction
  model and must never create an implicit background export path.
- Formula-neutralization is required because spreadsheet execution is a realistic
  workstation threat when CSV files are opened outside the app.

Privacy:

- This is the main risk area. Exported rows may contain patient-linked review
  evidence and can be copied outside the application boundary.
- The MVP should therefore keep the payload minimal, omit direct identifiers by
  default, and require explicit user action and file-path choice for every
  export.
- If product ownership wants patient identifiers, location, or broader context
  columns later, require separate approval and requirements updates before
  implementation.

## AI/ML Impact Assessment

This change does not add, change, remove, or depend on an AI-enabled device
software function.

- Model purpose: not applicable
- Input data: deterministic vital-sign and derived alert-event data only
- Output: deterministic CSV rows for retrospective review
- Human-in-the-loop limits: unchanged from the current system
- Transparency needs: the artifact must distinguish review scope from live
  status, but that is a conventional software disclosure concern, not an AI
  explainability concern
- Dataset and bias considerations: not applicable
- Monitoring expectations: standard software verification and GUI/manual review
- PCCP impact: none

## Validation Plan

Implementation should add automated coverage around the export seam plus manual
GUI validation of the operator workflow.

Automated validation scope:

- unit tests for timestamp retention on stored session events
- unit tests for source-column rendering from abnormal-parameter masks and
  recovery events
- unit tests for CSV schema/order, quoting, escaping, and formula-neutralization
- unit tests for exporting a reset-notice row ahead of event rows when that
  notice is visible
- integration tests proving the exported rows match the same ordered review view
  used by the dashboard for:
  - warning to critical to recovery transitions
  - reset-boundary disclosure
  - simulation mode labeling
  - filtered-view parity if issue `#89` is already present when implementation
    happens

Recommended validation commands:

```powershell
cmake -S . -B build -G Ninja -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure -R "Patient|Alert|Export|GUI"
```

Manual validation scope:

- verify the export button is disabled when no exportable review rows exist
- verify the export button enables once review evidence exists
- verify save cancel leaves no file and no state change
- verify overwrite confirmation and explicit success/failure feedback
- verify a simulation-mode export is marked as simulation in both filename and
  CSV content
- verify the exported row order matches the on-screen review list exactly
- verify the CSV does not contain direct identifiers unless the requirements are
  explicitly changed first

## Rollback Or Failure Handling

- If truthful event timestamps cannot be added without a broader patient-data
  refactor, do not ship a CSV that fabricates timestamps at export time. Split
  the timestamp substrate into a prerequisite issue and keep this issue blocked.
- If implementation cannot guarantee parity between the visible review rows and
  the export output, stop and centralize the shared review-view builder instead
  of duplicating row-selection logic.
- If secure local-file creation cannot be applied on a selected path, fail
  visibly rather than silently falling back to a looser mode.
- If product ownership expands scope to identifiers, remote export, or broader
  patient-history output, split that work into a new issue with new risk review
  instead of stretching this MVP.
- Rollback is straightforward because the feature is additive: remove the
  export button, export service, and any added event metadata while preserving
  the existing in-app session review surfaces.
