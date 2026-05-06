# Design Spec: Issue 88

Issue: `#88`  
Branch: `feature/88-show-active-alert-count-next-to-dashboard-alert-list`  
Spec path: `docs/history/specs/88-show-active-alert-count-next-to-dashboard-alert-list.md`

## Problem

The dashboard's `Active Alerts` section currently shows only the row list.
Operators must scan the full list to learn whether there are zero, one, or
multiple current alerts.

- `src/gui_main.c` `create_dash_controls()` renders the `Active Alerts` label
  and a listbox (`IDC_LIST_ALERTS`), but no compact count or summary text is
  shown beside that list.
- `update_dashboard()` already derives the current alert rows from
  `generate_alerts(latest, alerts, MAX_ALERTS)` and either writes a no-alert
  placeholder or one row per current alert.
- The dashboard already has a separate `Session Alarm Events` list, so the UI
  currently distinguishes current alerts from historical alert transitions only
  by section placement and label text.
- Reset boundaries already exist in code:
  - `do_admit()` initializes a new patient session
  - `do_clear()` clears the current session and returns to no-patient state
  - `apply_sim_mode()` clears session state when simulation is disabled and
    auto-creates a default patient when simulation is enabled
  - logout destroys the dashboard session
  - the `WM_TIMER` path reinitializes the patient when `patient_is_full()`
    triggers bounded-session rollover

This matters because the issue asks for an at-a-glance alarm-burden cue, but a
count that is not tied to the exact same current-alert source of truth as
`IDC_LIST_ALERTS` would become stale, misleading, or easy to confuse with the
historical `Session Alarm Events` surface.

## Goal

Add a compact, read-only active-alert count indicator adjacent to the dashboard
`Active Alerts` section so an operator can tell the number of current active
alerts without scanning every list row.

The feature must:

- remain display-only and reuse the existing current-alert computation path
- summarize current active alerts only, never session history
- distinguish no-patient state from zero-current-alert state
- clear or recompute on every existing session/reset boundary
- stay visually secondary to the existing alert rows and severity banner

## Non-goals

- Changing alert thresholds, NEWS2 logic, alarm limits, alert generation, or
  any patient-care workflow.
- Adding alert acknowledgement, silence, escalation, export, persistence,
  networking, or audit behavior.
- Replacing the `Active Alerts` list, the aggregate severity banner, or the
  `Session Alarm Events` list with a summary-only display.
- Introducing severity-by-count logic, for example treating two warnings as
  higher urgency than one critical alert.
- Adding historical alert counters, per-severity counts, trend analytics, or a
  central-station style alarm dashboard.

## Intended Use Impact, User Population, Operating Environment, And Foreseeable Misuse

Intended use impact:

- The feature adds a read-only orientation aid in the dashboard's current-alert
  area.
- It does not alter how alerts are generated, prioritized, reviewed, or stored.

User population:

- bedside clinical staff, internal testers, and reviewers using the local
  Win32 workstation application

Operating environment:

- the existing single-patient local dashboard, using simulator-fed or manually
  entered readings, with in-process state and no remote alert aggregator

Foreseeable misuse:

- treating the count as a severity score instead of a quantity cue
- assuming `0` means no recent instability rather than no current active
  alerts
- assuming the count includes `Session Alarm Events`
- relying on the count without reading the detailed alert rows or the existing
  severity banner

## Current Behavior

- `create_dash_controls()` creates a static section label using
  `localization_get_string(STR_ACTIVE_ALERTS)` and places the `IDC_LIST_ALERTS`
  listbox directly beneath it.
- `reposition_dash_controls()` stretches the alert list horizontally with the
  window, but there is no separate control associated with current-alert count
  presentation.
- `update_dashboard()`:
  - clears `IDC_LIST_HISTORY`, `IDC_LIST_ALERTS`, and `IDC_LIST_EVENTS`
  - returns early with `No patient admitted yet.` in the alert and event lists
    when `!g_app.has_patient`
  - calls `patient_latest_reading(&g_app.patient)`
  - calls `generate_alerts(latest, alerts, MAX_ALERTS)` when a latest reading
    exists
  - writes either:
    - `No active alerts - all parameters within normal range.`, or
    - one list row per alert using the existing `alerts[i]` array
- `Session Alarm Events` are populated separately from
  `patient_alert_event_count()` and `patient_alert_event_at()`.
- Localization already covers the section labels (`STR_ACTIVE_ALERTS`,
  `STR_SESSION_ALARM_EVENTS`, `STR_READING_HISTORY`), but there is no existing
  string ID dedicated to a compact current-alert count summary.

## Proposed Change

1. Add a dedicated read-only dashboard control for current active-alert count
   adjacent to the `Active Alerts` section label.
2. Keep the count entirely in the presentation layer. Do not store it in
   `PatientRecord`, config, or any persistent artifact.
3. Derive the indicator exclusively from the same `ac` value returned by
   `generate_alerts(latest, alerts, MAX_ALERTS)` inside `update_dashboard()`.
   Do not maintain an independent cached count.
4. Update the count control and `IDC_LIST_ALERTS` atomically in the same
   `update_dashboard()` execution path.
5. Define the indicator as current-alert quantity only. It shall never include:
   - `Session Alarm Events`
   - reading-history rows
   - prior bounded sessions
   - paused or cleared session state not represented by the current patient
6. Define the visible states explicitly:
   - no patient admitted: show an explicit no-patient state or suppress the
     badge in a way that cannot be confused with `0 active`
   - patient admitted with no current alerts: show a zero-current-alert state
   - patient admitted with one current alert: show `1` clearly
   - patient admitted with multiple current alerts: show the exact count
7. Prefer short, localization-friendly wording rather than long English-only
   prose. A concise pattern such as `0 active`, `1 active`, `3 active`, or an
   equivalent localized compact label is preferred over repeating the full list
   placeholder sentence in badge form.
8. Keep the indicator visually secondary:
   - neutral text treatment rather than severity color-coding
   - no blinking, countdown, or badge styling that competes with the existing
     status banner
   - no new clinical-priority semantics derived from the count
9. Place the count within the `Active Alerts` header row so it reads as a
   summary of that list, not as a separate dashboard tile or banner.
10. Update `reposition_dash_controls()` so the count remains readable at the
    current minimum width and does not overlap the section label or listbox.
11. Extend localization support if the implementation introduces new visible
    wording for the count states. New strings should be added to the existing
    static localization tables rather than hard-coded in only one language.
12. Preserve all current reset semantics by recomputing or clearing the count
    through the existing `update_dashboard()` callers after:
    - `do_admit()`
    - `do_clear()`
    - `apply_sim_mode()` enable and disable transitions
    - logout
    - automatic bounded-session rollover in the timer path
13. Keep the existing alert-row placeholder and detailed rows. The count is a
    supplement, not a replacement, for the list text users act on.

## Files Expected To Change

Expected requirements and traceability files:

- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`

Expected implementation files:

- `src/gui_main.c`
- `include/localization.h`
- `src/localization.c`

Expected verification files:

- `dvt/DVT_Protocol.md`
- `dvt/automation/run_dvt.py` if the team chooses to extend automated GUI
  presence checks for the new control
- `tests/unit/test_localization.cpp` only if the implementation adds explicit
  assertions around newly introduced string IDs

Files expected not to change for this issue:

- `src/alerts.c`
- `include/alerts.h`
- `src/patient.c`
- `include/patient.h`
- `src/vitals.c`
- `src/news2.c`
- `src/alarm_limits.c`
- `src/app_config.c`

If implementation decides to use a custom control ID for the new count widget,
that remains within normal `gui_main.c` UI plumbing and does not justify a
broader architectural change.

## Requirements And Traceability Impact

This issue does not require a new user need. Existing needs already cover the
dashboard surface and alert-summary context.

Existing trace anchors most likely to apply:

- `UNS-014` graphical dashboard
- `UNS-010` consolidated summary
- `UNS-005` automatic alerting
- `UNS-006` alert severity differentiation
- `SYS-014` graphical vital signs dashboard
- `SYS-011` patient status summary display
- `SWR-ALT-001` through `SWR-ALT-004` as unchanged alert-generation semantics

Recommended traceability shape:

- refine `SYS-014` so the dashboard requirement explicitly includes a compact
  current-active-alert summary adjacent to the active-alert list
- add one new GUI-level SWR for the active-alert count indicator, for example:
  - derivation from the same current alert set used by `IDC_LIST_ALERTS`
  - explicit no-patient, zero, one, and many states
  - refresh/reset behavior across the existing session boundaries
  - visual separation from `Session Alarm Events`
- add RTM entries mapping that SWR to `src/gui_main.c` and manual GUI evidence
  (plus optional supplemental automation if added)

Traceability must state clearly that:

- the count is derived from current active alerts only
- no alert semantics or severity logic change
- the count is a dashboard-summary aid, not an independent alarm-status source

## Medical-Safety, Security, And Privacy Impact

Medical safety:

- No change to thresholds, NEWS2 scoring, alarm generation, or treatment
  guidance is intended.
- Primary benefit is faster recognition that multiple current abnormalities are
  active.
- Primary risk is misrepresentation or misinterpretation if the count lags the
  list, survives a reset, or is read as a severity ranking.
- Residual safety is acceptable only if the indicator stays synchronized with
  `generate_alerts()` output and remains visually secondary to the alert rows
  and aggregate severity banner.

Security:

- No new authentication, authorization, networking, or storage path is
  introduced.
- The count remains inside the current authenticated dashboard session.

Privacy:

- No new PHI category is introduced.
- The indicator exposes only the quantity of alert rows already visible to the
  authenticated operator.

## AI/ML Impact Assessment

This change does not add, change, remove, or depend on an AI-enabled device
software function.

- Model purpose: not applicable
- Input data: deterministic in-process alert rows only
- Output: compact GUI count text only
- Human-in-the-loop limits: unchanged
- Transparency needs: standard GUI wording only; no AI explainability issue
- Dataset and bias considerations: not applicable
- Monitoring expectations: standard software verification and GUI evidence only
- PCCP impact: none

## Validation Plan

Recommended implementation validation:

```powershell
build.bat
run_tests.bat
```

Required GUI smoke checks:

- no-patient state: confirm the count cannot be mistaken for `0 active`
- normal scenario: confirm the count reports zero current alerts while the
  existing placeholder row remains correct
- single warning and single critical scenarios: confirm the count reports `1`
  while the detailed row and severity banner remain the primary urgency cues
- multi-alert scenario: confirm the count equals the number of rows added to
  `IDC_LIST_ALERTS`
- `Session Alarm Events` scenario: confirm the count ignores historical event
  rows even when the session-event list is populated
- Admit / Refresh: confirm the count refreshes to the new patient's current
  alert state without stale carryover
- Clear Session: confirm the count clears or switches to the documented
  no-patient state immediately
- Logout: confirm no prior session count is visible after returning to login
- simulation disable / device mode: confirm the count clears with patient state
- automatic rollover at `MAX_READINGS`: confirm the count recomputes for the
  new bounded session and does not preserve the previous session's burden
- resize the dashboard to the supported minimum width and confirm the label,
  count, and list remain readable without overlap or clipping
- switch across all four supported languages and confirm the count text remains
  readable and correctly aligned if localized wording is introduced

Expected verification outcome:

- implementation remains localized to GUI presentation and traceability docs
- alert semantics remain unchanged
- no stale count survives any documented reset boundary

## Rollback Or Failure Handling

- If implementation cannot keep the count tied exactly to the existing
  `generate_alerts()` result used for the current list, do not ship a separate
  count based on independent state.
- If minimum-width layout becomes crowded, prefer a shorter localized count
  rendering over expanding the feature into a second row or new tile.
- If localization support for compact count wording becomes awkward, prefer a
  minimal numeric presentation adjacent to the existing localized section label
  rather than hard-coding a long English phrase.
- If design review concludes that users will over-read the count as a severity
  signal, remove the indicator and keep the current alert rows and banner as
  the only active-alert surfaces.
- Rollback is straightforward because the feature is additive and GUI-local:
  remove the count control, its update logic, and its traceability entry, then
  restore current dashboard behavior.
