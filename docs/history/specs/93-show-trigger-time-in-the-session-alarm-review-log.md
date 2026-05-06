# Design Spec: Issue 93

Issue: `#93`  
Branch: `feature/93-show-trigger-time-in-the-session-alarm-review-log`  
Spec path: `docs/history/specs/93-show-trigger-time-in-the-session-alarm-review-log.md`

## Problem

The current session alarm review surface preserves only sequence, not
chronology:

- `include/patient.h` defines `AlertEvent` with `reading_index`, `level`,
  `abnormal_mask`, and `summary`, but no stored time field.
- `src/patient.c` formats each historical row as `#<reading> [<severity>]
  <summary>`.
- `src/gui_main.c` renders `IDC_LIST_EVENTS` as a simple listbox, so the
  dashboard shows row order only and has no true column for time metadata.

That leaves clinicians, testers, and reviewers unable to see when within the
current session a warning, escalation, or recovery occurred. They can infer
order from reading number, but not time spacing or recency.

## Goal

Add a narrow, read-only trigger-time field to each stored session alarm event
and present it as a dedicated review column so that:

- event chronology is visible without changing alert logic
- the feature remains session-scoped, deterministic, and bounded by static
  storage
- the GUI and `patient_print_summary()` reuse the same stored event metadata
- the displayed time is clearly historical review data, not an active-alarm or
  response-time workflow

## Non-goals

- Changing alarm thresholds, NEWS2 logic, configurable alarm-limit behavior, or
  any active-alert semantics.
- Introducing alarm acknowledgement, escalation workflow, clinician
  response-time tracking, or audit-grade enterprise timestamps.
- Persisting event-time data beyond the current local patient session.
- Adding export, cloud sync, EMR integration, or multi-patient surveillance.
- Redesigning the rest of the dashboard beyond what is needed to render the
  session event time field clearly.

## Intended Use Impact, User Population, Operating Environment, And Foreseeable Misuse

Intended use impact:

- The feature improves retrospective review of session alarm transitions that
  are already stored locally.
- It does not alter live monitoring, diagnosis, treatment guidance, or active
  alert prioritization.

User population:

- Trained clinicians, testers, and reviewers using the local Windows desktop
  application.

Operating environment:

- Single-patient local session, static in-process storage, simulator-fed or
  manually entered readings, and existing authenticated workstation access.

Foreseeable misuse:

- Treating the trigger time as a legal charting timestamp.
- Inferring clinician acknowledgement or response time from the review log.
- Assuming the event log spans prior sessions or survives logout/patient reset.
- Confusing a historical review row with the current active-alert surface.

## Current Behavior

- `patient_add_reading()` appends a reading and derives at most one historical
  alert event when severity or abnormal-parameter signature changes.
- Each event stores only reading order and summary text; no time metadata is
  captured.
- `update_dashboard()` renders session alarm events into `IDC_LIST_EVENTS` as
  single formatted strings, not structured columns.
- `patient_print_summary()` prints the same event rows without time metadata.
- Automatic rollover, manual clear, new admission, logout, and simulation-off
  flows clear the current patient/session record by calling `patient_init()` or
  zeroing the patient state.
- There is no existing reusable time seam in the domain layer; the current
  codebase does not already expose session uptime, monotonic clock state, or
  timestamp capture for event creation.

## Proposed Change

1. Use **session-elapsed time** as the authoritative MVP representation for the
   new field, not local wall-clock time.
   Reason:
   session-elapsed time is monotonic, avoids implying charting or audit
   authority, and fits the issue's stated MVP option of trigger time or
   session-elapsed time.
2. Extend `AlertEvent` with a stored elapsed-time field such as
   `unsigned int trigger_elapsed_seconds` while retaining `reading_index` as
   the stable ordering anchor.
3. Keep alert-event semantics centralized in `patient.c`, but do not introduce
   OS-specific time acquisition there. Instead, add a caller-supplied elapsed
   time seam, for example `patient_add_reading_with_elapsed(...)`, and route
   all production ingestion paths through it.
4. Reset the session clock anchor whenever the patient/session is reinitialized:
   admit/refresh, manual clear, logout, simulation disable, and automatic
   rollover after the bounded reading buffer resets.
5. Source elapsed time from the application layer using a monotonic local clock
   rather than formatted wall-clock time. The GUI can compute elapsed seconds
   relative to the current session start and pass that value into the patient
   module when a reading is accepted.
6. Capture the elapsed time once, at the same point the event record is
   appended. Repaints, reformatting, and later state changes must not mutate
   previously stored event times.
7. Preserve the existing event-creation rules from issue `#37`:
   - first normal reading: no event
   - first abnormal reading: create event
   - severity change or abnormal-parameter-set change: create event
   - recovery to normal: create event
   - repeated identical signature: no duplicate event
8. Present the stored elapsed time in a constrained historical format:
   - default display: `T+MM:SS`
   - extended display when needed: `T+HH:MM:SS`
   - `T+` prefix is required so the UI does not resemble a charting clock or
     an acknowledged-at timestamp
9. Replace the current `IDC_LIST_EVENTS` listbox with a read-only column-capable
   control, preferably a report-style `SysListView32`, so the dashboard can
   show true fields instead of a single concatenated string.
10. Use the following session-event columns:
    - `Trigger Time`
    - `Reading`
    - `Severity`
    - `Summary`
11. Preserve the active-alert list as the primary live state surface. The
    session event list remains historical review only and must not reuse active
    alert wording that implies present-tense action.
12. Preserve the reset disclosure ahead of current-session rows. In the new
    column layout, show the reset notice as a dedicated leading row or banner
    associated with the event list rather than silently dropping it.
13. Update `patient_print_summary()` to print the same stored time metadata, for
    example:

```text
T+00:14  #3 [CRITICAL] Abnormal parameters: Heart Rate, SpO2
```

14. Keep the change bounded to session review. Do not add sorting, filtering,
    export, operator notes, acknowledgement controls, or response analytics in
    this issue.

## Files Expected To Change

Expected requirements and traceability files:

- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`

Expected implementation files:

- `include/patient.h`
- `src/patient.c`
- `src/gui_main.c`
- `include/localization.h`
- `src/localization.c`
- `src/main.c`

Expected verification files:

- `tests/unit/test_patient.cpp`
- `tests/integration/test_patient_monitoring.cpp`
- `tests/integration/test_alert_escalation.cpp`
- `dvt/DVT_Protocol.md`

Files expected not to change:

- `src/vitals.c`
- `src/alerts.c`
- `src/news2.c`
- `src/alarm_limits.c`
- authentication/authorization rules beyond existing session-clear boundaries

## Requirements And Traceability Impact

No new user need is required. The feature should remain under existing
`UNS-017` session alarm event review scope.

Update existing SYS requirements rather than inventing a separate workflow:

- `SYS-020` should state that each stored session alarm event includes the
  session-elapsed trigger time captured when the event is appended.
- `SYS-021` should state that GUI and summary review surfaces present the same
  stored trigger-time metadata and preserve event order distinctly from current
  active alerts.

Update existing SWR requirements as follows:

- `SWR-PAT-007`
  Add the elapsed-time field to stored event capture and require capture from a
  caller-supplied session-elapsed value at event-append time.
- `SWR-PAT-008`
  Clarify that trigger-time metadata is cleared whenever session event history
  is cleared and remains accessible through the same event accessor path.
- `SWR-GUI-013`
  Require a read-only session-event review control with a dedicated
  `Trigger Time` column plus `Reading`, `Severity`, and `Summary`, while
  retaining the reset disclosure and keeping active alerts separate.

Update traceability to show:

- `UNS-017 -> SYS-020/SYS-021 -> SWR-PAT-007/SWR-PAT-008/SWR-GUI-013`
- new automated verification that stored elapsed time is captured, preserved,
  reset correctly, and rendered consistently

This issue should not create new traceability for:

- response-time measurement
- audit logging
- alert acknowledgement
- persistent longitudinal history

## Medical-Safety, Security, And Privacy Impact

Medical safety:

- No change to alert generation, severity thresholds, NEWS2 scoring, or
  treatment guidance.
- The principal safety control is semantic clarity: `Trigger Time` must mean
  historical session-elapsed time only.
- Retaining `reading_index` alongside elapsed time preserves an audit-friendly
  sequence anchor when times are close together or formatted coarsely.
- The column layout must remain readable at supported window sizes so severity
  and summary content are not hidden by the new time field.

Security:

- No new permission model, background service, or network path is introduced.
- Time metadata remains within the authenticated local session boundary.

Privacy:

- Patient-linked chronology metadata becomes visible on the same local review
  surfaces, but no new external persistence or transmission path is added.
- Session reset, logout, and patient-change paths must clear time metadata with
  the corresponding event rows.

## AI/ML Impact Assessment

This change does not add, change, remove, or depend on an AI-enabled device
software function.

- Model purpose: not applicable
- Input data: deterministic vital-sign readings plus caller-supplied
  session-elapsed seconds
- Output: deterministic historical review metadata
- Human-in-the-loop limits: unchanged
- Transparency needs: historical-vs-live distinction only; no model
  explainability impact
- Dataset and bias considerations: not applicable
- Monitoring expectations: standard software verification and GUI/DVT review
- PCCP impact: none

## Validation Plan

Automated validation should use deterministic elapsed values supplied by tests,
not wall-clock sleeps.

Unit-test scope:

- first abnormal event stores the supplied elapsed seconds
- escalation and recovery events store their own distinct elapsed seconds
- repeated identical signatures do not create duplicate events or mutate prior
  stored times
- `patient_init()` clears stored time metadata together with event history
- summary formatting includes the expected `T+...` token for stored events

Integration-test scope:

- warning -> critical -> normal sequence preserves monotonic elapsed times in
  the historical event log while active alerts still reflect only the latest
  reading
- parameter-set changes at the same severity produce distinct rows with the
  supplied elapsed values preserved
- automatic session rollover clears prior event times and preserves the reset
  disclosure semantics

Manual GUI / DVT scope:

- `Trigger Time` column is visibly historical and distinct from active alerts
- the column layout remains readable at the current minimum supported window
  width
- the reset notice is still visible before current-session rows after rollover
- simulation and manual entry both populate the time column correctly

Recommended validation commands:

```powershell
run_tests.bat
ctest --test-dir build --output-on-failure -R "Patient|Alert"
python dvt/run_dvt.py
```

## Rollback Or Failure Handling

- If implementation cannot preserve deterministic session-elapsed semantics
  without introducing ambiguous wall-clock behavior, stop and keep the issue in
  design rather than shipping a misleading timestamp.
- If the event surface cannot support a clearly readable time column in the
  current layout, prefer a narrow control upgrade over a fake aligned string;
  do not merge a design that only appears columnar in one font/width.
- If adding the elapsed-time seam forces broader reading-model changes outside
  session alarm review, split that work into a follow-on issue.
- Rollback is straightforward because the change is additive and session-local:
  remove the stored elapsed-time field, restore the previous event rendering,
  and keep the existing event-review behavior from issue `#37`.
