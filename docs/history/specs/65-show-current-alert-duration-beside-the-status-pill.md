# Design Spec: Show current alert duration beside the status pill

Issue: #65
Branch: `feature/65-show-current-alert-duration-beside-the-status-pill`
Spec path: `docs/history/specs/65-show-current-alert-duration-beside-the-status-pill.md`

## Problem

Issue #65 asks the dashboard to show how long the current aggregate abnormal
state has been active so an operator can distinguish a fresh `WARNING` or
`CRITICAL` change from a persistent one.

The current implementation already computes aggregate severity from the latest
reading, but it does not expose elapsed time for that state. `patient_current_status()`
returns only `ALERT_NORMAL`, `ALERT_WARNING`, or `ALERT_CRITICAL`. `VitalSigns`
and `PatientRecord` contain no timestamps, so the GUI cannot infer a defensible
"time in state" value from stored readings alone.

That gap is operationally relevant because the dashboard currently answers only
"how severe is the current state?" and not "how long has this same abnormal
state been active?" The risk note for #65 correctly narrows the hazard to human
misinterpretation, not threshold corruption: a stale or misleading duration cue
could make a fresh escalation look old, or make an old alert look clinically
validated when it is only a presentation-layer timer.

## Goal

Add a narrow, display-only duration cue beside the aggregate dashboard status
surface so the operator can see the current abnormal aggregate level and the
elapsed time since the software entered that same abnormal aggregate state.

The design goal is to make the feature:

- aggregate-state based, not per-parameter based
- presentation-layer only, with no changes to thresholds or patient-record data
- clearly subordinate to existing severity colour and text
- resettable whenever monitoring is no longer actively representing the same
  abnormal state

## Non-goals

- No changes to `overall_alert_level()`, `patient_current_status()`,
  `generate_alerts()`, NEWS2 scoring, alarm limits, or audible behavior.
- No persisted timestamps in `VitalSigns`, `PatientRecord`, or any other domain
  model.
- No acknowledgement, muting, escalation, export, audit-history, or alarm
  analytics workflow.
- No claim that the cue represents physiological onset time, treatment delay,
  clinician acknowledgement age, or validated clinical benefit.
- No extension of this issue into the CLI patient summary in `patient.c`.
- No reuse of the header's existing role pill (`ADMIN` / `CLINICAL`) as the
  abnormal-state duration surface.

## Intended use impact, user population, operating environment, and foreseeable misuse

- Intended use impact: provide a concise elapsed-time cue for the current
  abnormal aggregate dashboard state during active monitoring.
- User population: trained clinicians or operators already using the dashboard
  to review live patient-monitoring status.
- Operating environment: current Windows dashboard flow in live simulation
  today, with the design remaining compatible with a future live HAL-backed
  monitoring path.
- Foreseeable misuse: a user may read the value as time since physiological
  onset rather than time since the software entered the current abnormal
  aggregate state.
- Foreseeable misuse: a user may read the cue as an acknowledgement timer,
  escalation timer, or muted-alarm timer even though the product implements no
  such workflow.
- Foreseeable misuse: if the cue remains visible during paused simulation,
  cleared session, logout, or non-monitoring mode, a user may assume active
  monitoring continued when it did not.

## Current behavior

- `patient_current_status()` in `src/patient.c` returns the aggregate severity
  for the latest reading and intentionally carries no timing metadata.
- `PatientRecord` in `include/patient.h` stores only patient demographics,
  `VitalSigns readings[MAX_READINGS]`, and `reading_count`.
- `VitalSigns` in `include/vitals.h` has no observation timestamp field.
- `paint_status_banner()` in `src/gui_main.c` uses aggregate severity to choose
  banner colour during simulation mode, but it currently renders generic
  simulation/device-mode messaging rather than an elapsed abnormal-state age.
- `update_dashboard()` rebuilds the alerts and history lists when the dashboard
  changes, but it does not maintain a separate elapsed-time state.
- The only current recurring dashboard timer is `TIMER_SIM` at 2000 ms for
  synthetic data ingestion. There is no dedicated 1 Hz repaint path for a
  duration cue.
- Session-clearing and monitoring-state transitions already exist in
  `gui_main.c` via admit/refresh, clear session, simulation enable/disable,
  pause/resume, and logout flows. Those are the right reset boundaries for the
  new cue.

## Proposed change

Implement the feature as a GUI-local abnormal-state timer with explicit reset
semantics.

1. Add GUI-owned timing state to `AppState` in `src/gui_main.c`.
   Recommended fields are the currently tracked abnormal aggregate level and the
   tick count when that same abnormal level became active. This state should
   exist only in the presentation layer.
2. Add a small helper in `gui_main.c` that derives the current display state
   from live dashboard context:
   - if monitoring is not actively live, clear the duration state
   - if no patient is active, clear the duration state
   - if the aggregate level is `ALERT_NORMAL`, clear the duration state
   - if the aggregate level changes between `WARNING` and `CRITICAL`, restart
     the timer at zero for the new level
   - if the aggregate level remains the same abnormal level, retain the
     original entered-state timestamp
3. Use wall-clock or monotonic GUI tick time such as `GetTickCount64()` to
   compute elapsed time. Do not derive elapsed time from `reading_count`, the
   simulation sample cadence, or historical-record backfill.
4. Render the cue adjacent to the aggregate status surface in or immediately
   beside `paint_status_banner()`. The implementation should keep severity as
   the primary cue and render duration as secondary text or a subordinate badge.
   It should not overload the header role pill.
5. Preserve current mode-state clarity. If the existing simulation/device-mode
   wording needs to move or shrink to make room, it may do so, but the final UI
   must still communicate whether monitoring is live, paused, or unavailable.
6. Add a dedicated repaint cadence for the duration display while the cue is
   visible. A 1 Hz UI timer is preferred so the displayed value matches real
   elapsed time rather than stepping only on new synthetic readings.
7. Use a bounded, unambiguous duration format such as `MM:SS`, with optional
   promotion to `HH:MM:SS` for longer runs if layout allows. The wording must
   make clear that the timer is "time in current alert state", not onset time.
8. Route any new connective text or templates through the existing localization
   layer rather than hard-coding English UI strings in `gui_main.c`.
9. Reset or clear the cue on all of the following:
   - `WARNING` to `CRITICAL`
   - `CRITICAL` to `WARNING`
   - abnormal state to `NORMAL`
   - clear session
   - admit/refresh into a new monitoring session
   - logout or dashboard teardown
   - simulation pause
   - simulation disable / device mode / other non-monitoring state
10. Keep the implementation local to the dashboard presentation path. If
    implementation discovers a need for persisted timestamps, patient-history
    replay, or acknowledgement workflow, stop and split that work into a new
    issue.

## Files expected to change

Expected implementation files:

- `src/gui_main.c`
- `include/localization.h`
- `src/localization.c`

Expected requirements and evidence files:

- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`

Expected files to inspect but likely not modify:

- `docs/history/risk/65-show-current-alert-duration-beside-the-status-pill.md`
- `docs/ARCHITECTURE.md`
- `README.md`

Optional files only if the implementer extracts a pure formatting/helper unit
for testability:

- `tests/CMakeLists.txt`
- a new narrow unit test file for duration-format or reset-state helper logic

Files that should not change for this issue:

- `src/patient.c`
- `src/vitals.c`
- `src/alerts.c`
- `src/news2.c`
- `include/patient.h`
- `include/vitals.h`
- persistence, authentication, CI, and release workflow files

## Requirements and traceability impact

This issue should remain traceable as a dashboard-display enhancement, not a
clinical logic change.

Recommended requirement updates:

- Update `SYS-014` to state that the graphical dashboard may display elapsed
  time for the current abnormal aggregate alert state during active monitoring.
- Keep `SYS-006` unchanged because aggregate severity logic is not changing.
- Keep `SYS-011` unchanged unless the team explicitly decides to add the same
  cue to the CLI patient summary. That is out of scope for issue #65.
- Keep `SWR-PAT-004` unchanged because `patient_current_status()` remains the
  source of truth for severity only.
- Either extend `SWR-GUI-003` with the duration-display semantics or, more
  cleanly, add a new GUI-level requirement dedicated to abnormal-state elapsed
  time and reset behavior. A separate `SWR-GUI-013` is preferred because the
  reset semantics are specific and testable.

Recommended traceability outcome if a new SWR is added:

- map the new GUI duration requirement to `src/gui_main.c`
- verify via GUI demonstration or targeted helper-unit tests plus manual GUI
  review
- avoid adding any trace rows that imply changes to `patient.c`, `vitals.c`,
  `alerts.c`, or NEWS2 behavior

## Medical-safety, security, and privacy impact

Medical-safety impact is low to moderate and driven by interpretation, not by
classification logic. The cue can influence triage urgency, so the design must
ensure that severity remains primary and duration semantics remain narrow:
"time since the software entered the current abnormal aggregate state."

The main controls are:

- derive the timer from aggregate-state transitions only
- clear it whenever monitoring is not actively live
- do not display it for `NORMAL`
- keep it visually subordinate to severity colour, severity text, and raw vital
  values

Security impact is low. The change does not introduce new privileges, storage,
network paths, or credential flows. The only meaningful integrity concern is
that the displayed duration must not be stale or misleading.

Privacy impact is none expected. No new patient-data class, export path, log,
or persistence mechanism is introduced.

## AI/ML impact assessment

This change does not add, change, remove, or depend on an AI-enabled device
software function.

- Model purpose: none
- Model inputs: none
- Model outputs: none
- Human-in-the-loop limits: unchanged
- Transparency needs: unchanged beyond ordinary UI wording clarity
- Dataset and bias considerations: none
- Monitoring expectations: none beyond ordinary GUI verification
- PCCP impact: none

## Validation plan

Primary design-validation expectations:

- Verify `NORMAL -> WARNING` starts the timer at the first abnormal aggregate
  state.
- Verify `WARNING -> CRITICAL` resets the displayed duration to the new
  critical-state entry time.
- Verify `CRITICAL -> WARNING` resets the displayed duration to the new warning
  state rather than carrying forward critical age.
- Verify abnormal `-> NORMAL` clears the cue completely.
- Verify the timer does not reset while the aggregate level remains unchanged
  across multiple live updates.
- Verify pause, clear session, admit/refresh, logout, and simulation disable
  clear the cue and prevent stale carryover into the next session.
- Verify the cue is absent in device/non-monitoring mode and does not imply
  live monitoring when the system is paused or inactive.
- Verify the final layout keeps severity visually primary and does not obscure
  the existing banner or header controls.

Recommended verification commands during implementation:

```powershell
run_tests.bat
ctest --test-dir build --output-on-failure -R Patient|Alert|GUI
git diff --stat
rg -n "SYS-014|SWR-GUI-003|SWR-GUI-013|65-show-current-alert-duration" requirements docs/history
```

Manual GUI scenarios should include at minimum:

- steady `WARNING`
- steady `CRITICAL`
- `WARNING -> CRITICAL`
- `CRITICAL -> WARNING`
- abnormal `-> NORMAL`
- pause and resume
- clear session and re-admit
- simulation disable / device mode transition

## Rollback or failure handling

If implementation cannot satisfy the feature without adding timestamps to
`VitalSigns` or `PatientRecord`, stop and split that work into a separate
issue. That would exceed the intended presentation-only scope.

If implementation requires acknowledgement workflow, historical alarm review,
cross-session continuity, or new persistence, stop and split the scope instead
of broadening issue #65 silently.

If layout constraints make the cue visually dominate severity or conflict with
live/paused/device-mode messaging, prefer a smaller secondary presentation or
hide the cue outside live abnormal monitoring rather than forcing a larger UI
refactor.

If requirement updates drift toward rewriting aggregate-alert logic rather than
describing display semantics, revert that expansion and keep the change scoped
to dashboard presentation behavior.
