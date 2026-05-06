# Design Spec: Issue 101

Issue: `#101`  
Branch: `feature/101-export-current-trend-data-to-csv`  
Spec path: `docs/history/specs/101-export-current-trend-data-to-csv.md`

## Problem

The current dashboard already retains and visualizes a bounded trend-review
window, but it does not provide any way to persist that window as a local
artifact:

- `src/gui_main.c` renders trend sparklines from the active
  `PatientRecord.readings[]` window and shows the same readings in
  `IDC_LIST_HISTORY`.
- `PatientRecord` already stores up to `MAX_READINGS` ordered readings for the
  current session, but those readings exist only in memory.
- The issue requests CSV export including timestamps and derived trend
  direction, but the current data model does not store wall-clock timestamps,
  and current trend logic (`trend_direction()`) is defined for a retained
  series/window, not for a per-row semantic.

Without a narrow design, implementation would either fabricate unsupported data
or push a patient-data file-export path into the product without clear scope,
privacy boundaries, or failure handling.

## Goal

Add a narrow, explicit local CSV export of the current patient session's
retained trend-review readings that:

- exports only the bounded in-memory reading window already visible in the
  dashboard
- uses a fixed, deterministic, locale-independent CSV schema
- reuses existing patient and alert semantics instead of introducing new
  clinical logic
- keeps file export out of the vital-sign classification and patient-history
  core where current data structures already suffice
- makes persistence and failure states obvious to the operator

## Non-goals

- Changing thresholds, NEWS2 scoring, alarm-limit behavior, acquisition cadence,
  or any live-monitoring clinical logic.
- Expanding retention beyond the current `MAX_READINGS` session window.
- Adding cloud sync, EMR/HL7 integration, printing, PDF generation, email, or
  background export.
- Adding wall-clock timestamps in this MVP.
- Adding per-row trend-direction semantics in this MVP.
- Exporting a full longitudinal chart, legal medical record, or multi-patient
  batch artifact.

## Intended Use Impact, User Population, Operating Environment, And Foreseeable Misuse

Intended use impact:

- The feature adds a deliberate review-and-handoff artifact for the currently
  retained reading window.
- It does not replace the live dashboard, active alerts, or session alarm event
  review as the primary runtime safety surfaces.

User population:

- Trained clinicians, internal testers, and reviewers using the local Win32
  desktop application.

Operating environment:

- Single-patient authenticated desktop workflow.
- Current local session only, using static in-memory storage and local file
  output chosen by the operator.

Foreseeable misuse:

- Treating the CSV as a complete clinical record rather than a bounded session
  artifact.
- Assuming the CSV contains exact capture timestamps even though the current
  model stores only reading order.
- Assuming any exported trend-direction value is row-specific when the current
  implementation only supports series-level direction.
- Sharing the file outside the intended workstation context because the export
  persists beyond logout or session clear.

## Current Behavior

- `PatientRecord` stores demographics plus up to `MAX_READINGS` ordered
  `VitalSigns` entries for the current session.
- `paint_tiles()` in `src/gui_main.c` uses `trend_extract_*()` helpers to feed
  the trend sparklines from that retained session window.
- `update_dashboard()` renders raw reading history rows and session alarm
  events, but no export action exists.
- The product already performs local file persistence for non-patient data such
  as config and account files, but there is no patient-data CSV export path.
- `VitalSigns` has no timestamp field.
- `respiration_rate == 0` is an internal sentinel meaning "not measured" and
  should not be exported as if it were a real zero measurement.

## Proposed Change

1. Add one explicit `Export Trend Data` action to the dashboard review area,
   near the existing retained-reading surfaces rather than under Settings.
2. Keep the action disabled unless there is an active patient with at least one
   retained reading. It should become unavailable after logout, manual clear,
   patient refresh/change, and automatic session rollover because those paths
   already invalidate the prior in-memory window.
3. Implement export through a dedicated helper module such as
   `trend_export.c` / `trend_export.h` that accepts the current
   `PatientRecord` plus a file path and emits CSV. Keep Win32 dialog handling in
   `gui_main.c`, but keep CSV formatting and file writing out of the window
   procedure.
4. Export only `PatientRecord.readings[0..reading_count-1]` in chronological
   order. No background export, no autosave, and no persistence of hidden
   historical windows.
5. Use a fixed UTF-8 CSV schema with locale-independent headers, `.` decimal
   formatting, and deterministic column order. The recommended MVP columns are:
   `patient_id`, `reading_index`, `heart_rate_bpm`, `systolic_bp_mmhg`,
   `diastolic_bp_mmhg`, `temperature_c`, `spo2_percent`,
   `respiration_rate_br_min`, `overall_alert_level`.
6. Use `reading_index` as the canonical ordering field. Do not add a timestamp
   column in this issue because current runtime data does not carry validated
   wall-clock provenance.
7. Reuse existing semantics for row content:
   - row order comes directly from `PatientRecord.readings[]`
   - `overall_alert_level()` supplies the exported status value
   - `respiration_rate == 0` exports as an empty CSV field or other explicitly
     specified "not measured" representation, not as a literal measured zero
8. Include the numeric `patient_id` but omit patient name in the CSV body for
   the MVP. This keeps enough context to reduce wrong-patient confusion while
   avoiding a more privacy-sensitive free-text identifier.
9. Do not include per-row trend-direction fields in this issue. The sequence of
   retained readings is itself the exported trend window, and current
   `trend_direction()` semantics are window-level only. If product management
   later requires timestamps or trend-direction export, that should be a
   follow-on issue that explicitly extends the requirements and data model.
10. Use a standard save dialog with overwrite confirmation and a sensible
    default filename derived from the current patient/session context, but keep
    the file contents deterministic regardless of UI language or chosen path.
11. Before the write is finalized, show a clear operator-facing warning that
    the CSV may contain patient data and may persist outside the application.
12. Treat cancellation and write failure as explicit outcomes:
    - cancel leaves state unchanged and reports no success
    - write errors surface a clear message
    - incomplete output should be removed or otherwise not presented as a
      successful export
13. Keep this feature out of the clinical core. This issue should not require
    changes to `VitalSigns`, `PatientRecord`, classification thresholds, trend
    algorithms, or session-retention limits.

## Files Expected To Change

Expected requirements and traceability files:

- `requirements/UNS.md`
- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`

Expected implementation files:

- `CMakeLists.txt`
- `include/trend_export.h`
- `src/trend_export.c`
- `src/gui_main.c`
- `include/localization.h`
- `src/localization.c`

Expected verification and user-facing docs:

- `tests/CMakeLists.txt`
- `tests/unit/test_trend_export.cpp`
- `dvt/DVT_Protocol.md`
- `README.md`

Files expected not to change:

- `include/vitals.h`
- `include/patient.h`
- `src/vitals.c`
- `src/patient.c`
- `src/trend.c`
- `src/news2.c`
- `src/alarm_limits.c`

## Requirements And Traceability Impact

- Current approved requirements do not explicitly authorize local export of
  patient-linked trend-review data, so this should not be implemented as an
  undocumented GUI convenience.
- Existing context that the export reuses:
  - `UNS-009` Vital Sign History
  - `UNS-010` Consolidated Status Summary
  - `SYS-009` Vital Sign Reading History
  - `SYS-011` Patient Status Summary Display
  - `SYS-012` Static Memory Allocation
  - `SWR-TRD-001` trend-review extraction context
- A new UNS-level statement is recommended because local export is a new
  user-visible behavior, not just a refinement of retention or display.
- New SYS/SWR requirements are expected for:
  - export availability conditions and reset/logout boundaries
  - CSV schema, units, ordering, encoding, and locale-independent formatting
  - handling of the RR "not measured" sentinel
  - patient-data disclosure and explicit failure/cancel behavior
  - omission of unsupported timestamp semantics in this MVP
- Traceability should explicitly show that exported row status is derived from
  existing `overall_alert_level()` logic rather than a new interpretation path.
- This issue should not introduce new requirements for alarm thresholds, NEWS2,
  session alarm events, or sparkline direction rules.

## Medical-Safety, Security, And Privacy Impact

Medical safety:

- No intended change to vital classification, NEWS2, alarm-limit behavior, or
  treatment guidance.
- Primary benefit is reduced transcription risk when a user needs the exact
  currently retained reading window outside the live GUI.
- Primary medical-safety risk is stale, wrong-patient, or misleading export
  semantics. Using `reading_index` rather than invented timestamps and omitting
  per-row trend-direction semantics materially reduces that risk.

Security:

- No network path is introduced.
- Export remains user-triggered and local only.
- The main security concern is the creation of a patient-linked file that
  persists outside the authenticated runtime session.

Privacy:

- This is the first patient-data file export path and therefore a material
  privacy change even though it is local only.
- Restricting the CSV to numeric `patient_id` plus retained readings is a
  narrower exposure than exporting full free-text demographics.
- The UI should warn that exported files may outlive logout, patient clear, or
  workstation handoff.

## AI/ML Impact Assessment

This change does not add, change, remove, or depend on an AI-enabled device
software function.

- Model purpose: not applicable
- Input data: existing deterministic vital-sign readings only
- Output: deterministic CSV rows and existing rule-based alert labels
- Human-in-the-loop limits: unchanged
- Transparency needs: the file must not imply unsupported timestamp or
  row-level trend semantics, but this is a deterministic export concern, not an
  AI explainability concern
- Dataset and bias considerations: not applicable
- Monitoring expectations: standard software verification and manual GUI review
- PCCP impact: none

## Validation Plan

Implementation should add focused serializer tests plus manual GUI export
verification.

Automated validation scope:

- unit tests for empty-record rejection or disabled-export behavior at the
  helper boundary
- unit tests for one-reading and `MAX_READINGS` exports
- unit tests for fixed header order, row ordering, decimal formatting, and
  deterministic repeated output
- unit tests proving `respiration_rate == 0` exports as "not measured" rather
  than a real zero reading
- unit tests proving exported `overall_alert_level` values match existing
  alert-engine semantics
- unit tests proving CSV content is stable across UI language changes

Manual / DVT validation scope:

- export from a manually entered session and verify clean opening in both a
  text editor and spreadsheet
- verify the button is unavailable before a patient is active and after clear
  or logout
- verify the warning about persisted patient data is shown before a successful
  export
- verify that the CSV contains `reading_index` and does not contain a timestamp
  column in this MVP
- verify overwrite, cancel, and write-failure messaging

Recommended validation commands:

```powershell
cmake -S . -B build -G Ninja -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure -R "Trend|Export|Localization"
```

Expected validation outcome:

- Export code remains isolated to GUI wiring, localization, build wiring, and a
  dedicated export helper.
- No changes are required in the clinical-data structures or classification
  modules.
- The exported file is deterministic, readable, and explicit about the bounded
  session scope it represents.

## Rollback Or Failure Handling

- If implementation cannot satisfy the issue without adding timestamps to
  `VitalSigns` or inventing per-row trend-direction semantics, stop and split a
  follow-on design issue rather than broadening this one silently.
- If product management later requires patient name, durable archive semantics,
  or enterprise interoperability, treat that as a separate scope increase with
  new privacy and traceability review.
- If the GUI cannot make patient-data persistence obvious, do not ship a silent
  export path; keep the feature blocked until warning and failure states are
  clear.
- Rollback is straightforward because the intended runtime behavior is additive:
  remove the export helper, GUI control, and related requirements/tests to
  return to the current in-memory-only trend-review workflow.
