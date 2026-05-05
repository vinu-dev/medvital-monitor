# Design Spec: Historical reading drill-down review

Issue: `#42`  
Branch: `feature/42-historical-reading-drill-down-review`  
Spec path: `docs/history/specs/42-historical-reading-drill-down-review.md`

## Problem

The product already stores a bounded in-session history of readings and shows
that history in `IDC_LIST_HISTORY`, but the dashboard itself remains latest-only.
Today:

- `paint_tiles()` derives the visible vital tiles and NEWS2 tile from
  `patient_latest_reading(&g_app.patient)`.
- `paint_status_banner()` derives banner severity from
  `patient_current_status(&g_app.patient)`, which also resolves from the latest
  reading.
- `update_dashboard()` repopulates the history list for every stored reading,
  but generates the alert list from the latest reading only.
- `dash_proc()` does not handle `IDC_LIST_HISTORY` selection changes, so the
  history list is review text only and cannot drive a point-in-time snapshot.

That leaves the issue goal unmet: a clinician or tester cannot inspect what the
monitor looked like at reading `#N`, even though the stored session data and
derived-alert logic already exist. The risk note for `#42` also identifies the
main hazard if this is implemented naively: a historical abnormal reading could
be mistaken for the current live state unless the mode is explicit and live
acquisition cannot drift underneath the reviewed snapshot.

## Goal

Add a narrow, review-only drill-down for the current session so selecting a row
in `IDC_LIST_HISTORY` enters an explicit historical-review mode that:

- shows the selected stored reading across the dashboard's reviewed surfaces;
- clearly distinguishes historical review from latest/live mode;
- provides a one-action return to latest/live mode; and
- does not change thresholds, NEWS2 scoring, storage limits, persistence, or
  hardware behavior.

## Non-goals

- No change to vital classification thresholds, NEWS2 scoring, alarm limits, or
  alert-generation logic.
- No change to authentication, authorization, localization architecture,
  persistence, CI, release flow, or traceability outside the feature's own
  requirement additions.
- No multi-patient, multi-session, export, print, audit-log, or remote-sharing
  workflow.
- No longitudinal record, event timeline, waveform cursor, or central-station
  UX.
- No previous/next abnormal-reading navigation in the MVP. The issue marked
  that behavior as optional; the MVP should stop at direct list selection plus
  return-to-live.
- No change to sparkline extraction or session-history storage semantics beyond
  making one selected reading reviewable.

## Intended Use Impact, User Population, Operating Environment, And Foreseeable Misuse

Intended use impact:

- This feature improves retrospective review within one active session. It does
  not change how readings are acquired, classified, or alarmed.

User population:

- Bedside clinicians, ward staff, and internal demo/test operators already
  within the product's intended trained-user group.

Operating environment:

- One local Windows workstation.
- One active patient session.
- One bounded in-memory reading history with `MAX_READINGS = 10`.
- Simulation mode or HAL-fed acquisition using the existing 2-second timer path.

Foreseeable misuse:

- mistaking a historical snapshot for the current patient state;
- reading historical alerts as currently active alarms;
- expecting the feature to behave like a persistent chart or audit system;
- continuing review while live acquisition silently advances or resets the
  session.

The design must make those misuse cases obvious and bounded rather than relying
on training alone.

## Current Behavior

Relevant current implementation behavior:

- `src/gui_main.c` creates `IDC_LIST_HISTORY` and repopulates it from
  `g_app.patient.readings[]`, but no dashboard handler uses list selection to
  switch the reviewed snapshot.
- `paint_tiles()` always renders the most recent reading and computes the
  tile-level classifications and NEWS2 summary from that latest sample.
- `update_dashboard()` always computes `generate_alerts()` from the latest
  reading and therefore cannot show the alert set for a prior abnormal reading.
- `paint_status_banner()` always reflects the latest aggregate status path.
- The simulation timer continues to append readings every 2 seconds while
  `g_app.sim_enabled && !g_app.sim_paused`.
- When the simulation history is full, the timer path reinitializes the patient
  session before appending a new reading. That rollover makes background review
  particularly unsafe unless the reviewed state is frozen.

Relevant requirements state:

- `SYS-009` and `UNS-009` cover retaining chronological history.
- `SYS-011` and `UNS-010` cover showing a latest-reading summary.
- `SWR-GUI-003`, `SWR-GUI-004`, and `SWR-TRD-001` cover the existing dashboard
  surfaces, but there is no approved requirement for an explicit historical
  review mode that retargets those surfaces from the latest reading to a
  selected stored snapshot.

## Proposed Change

### 1. Add an explicit GUI review state

Implement the MVP entirely in the GUI layer by adding review-state fields to
`AppState` in `src/gui_main.c`:

- one field that records the selected historical reading index, with `-1`
  meaning latest/live mode;
- one field that records whether entering historical review auto-paused live
  simulation, so return-to-live can restore the prior pause state correctly.

For the MVP, no new `patient.c` accessor is required. `gui_main.c` already owns
the history-list population and sparkline extraction from
`g_app.patient.readings[]`, so the narrowest implementation is to keep review
selection in the GUI layer and avoid widening the domain API in this issue.

### 2. Enter historical review from the existing history list

Extend the dashboard `WM_COMMAND` handling so `IDC_LIST_HISTORY` with
`LBN_SELCHANGE` enters historical review for the selected valid row.

On entering review:

- store the selected index;
- if simulation is enabled and not already manually paused, force
  `g_app.sim_paused = 1` and record that review mode caused the pause;
- keep the selected row visually selected in the list;
- reveal or enable a dedicated return-to-live control.

If the selection is invalid or the session has no patient/readings, the handler
shall stay in latest/live mode.

### 3. Use one helper for the displayed snapshot

Add a small internal helper in `src/gui_main.c`, equivalent to
`dashboard_selected_reading()`, that returns:

- the selected historical reading when review mode is active; or
- `patient_latest_reading(&g_app.patient)` when review mode is inactive.

Every reviewed dashboard surface shall resolve from that same helper:

- `paint_tiles()` for the visible vital values and NEWS2 tile;
- `paint_status_banner()` for severity and mode text;
- `update_dashboard()` for the alert list contents.

This prevents the mixed-context failure mode where some surfaces show the
selected snapshot while others continue to show live/latest data.

### 4. Make historical review unmistakable

The MVP shall add explicit UI cues that the operator is no longer viewing live
state:

- a persistent status-banner message such as `Reviewing reading #N of M`;
- a one-click control labelled for returning to latest/live mode;
- alert-list wording that makes clear the listed alerts belong to the selected
  historical reading, not currently active live alarms.

The live-mode rolling simulation banner should not remain visible unchanged
while review mode is active. Review mode should replace it with a static,
obvious historical-review message in the normal glance path.

### 5. Return-to-live behavior

Add a dedicated return-to-live action, preferably a small button near the
history section or header. When invoked, it shall:

- clear the review index back to `-1`;
- remove the explicit historical-review banner state;
- if review mode auto-paused simulation, restore the prior running state;
- if the operator had already paused simulation before entering review, preserve
  that paused state instead of resuming unexpectedly;
- refresh the dashboard back to latest/live behavior immediately.

### 6. Exit review mode on session-invalidating actions

Any action that changes or invalidates the reviewed record shall clear review
mode before refreshing the dashboard. That includes at least:

- `Admit / Refresh`;
- `Add Reading`;
- `Clear Session`;
- demo-scenario loading;
- simulation-mode enable/disable transitions;
- logout or dashboard recreation paths such as language refresh;
- timer-driven session reset when the bounded history is full.

For the MVP, the safest design is to avoid the timer-driven reset while review
mode is active by auto-pausing acquisition on review entry. That is the design
choice this spec adopts for implementation.

### 7. Keep session history and sparklines bounded and unchanged

The feature shall continue to use the existing bounded in-memory history and the
existing sparkline extraction helpers. The sparkline remains a session-context
display, not a point-selection widget, in the MVP. No new storage, persistence,
or archive behavior is introduced.

### 8. Requirements updates

Do not weaken the existing latest/live requirements. Instead, add a new
traceable requirement chain for historical review.

Recommended requirement additions:

- `SYS-020 - Historical Session Reading Review`
- `SWR-GUI-013 - Historical Reading Drill-Down Review`

Recommended scope for `SYS-020`:

- the system shall allow a user to select one stored reading from the current
  session history and view a clearly identified retrospective summary for that
  reading without changing the underlying stored history or clinical rules;
- the system shall provide a direct return to latest/live mode;
- the system shall prevent silent ambiguity between historical review and
  current live state.

Recommended scope for `SWR-GUI-013`:

- `IDC_LIST_HISTORY` selection enters historical review for the chosen session
  reading;
- the dashboard shall derive vital tiles, NEWS2 tile, status banner, and alert
  list from that selected snapshot while review mode is active;
- the UI shall show an explicit historical-review indicator and return-to-live
  action;
- simulation review shall not silently drift while a historical reading is
  selected in the MVP pilot.

No new UNS entry is expected. Existing `UNS-009` and `UNS-010` are sufficient
anchors for session-history review plus consolidated status presentation.

## Files Expected To Change

Expected implementation files:

- `src/gui_main.c`
- `include/localization.h`
- `src/localization.c`
- `tests/unit/test_localization.cpp`
- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`
- DVT or GUI-verification artifacts that record the new historical-review
  workflow

Files that should not change in this issue:

- `src/vitals.c`
- `src/alerts.c`
- `src/news2.c`
- `src/patient.c`
- `include/patient.h`
- authentication, persistence, alarm-limit, CI, release, or installer files

## Requirements And Traceability Impact

Existing requirement relationships that remain relevant:

- `UNS-009` -> `SYS-009` for session history retention
- `UNS-010` -> `SYS-011` for consolidated summary presentation
- `SYS-014` -> `SWR-GUI-003` / `SWR-GUI-004` for dashboard presentation
- `SWR-TRD-001` for unchanged sparkline context

New traceability needed:

- `UNS-009`, `UNS-010` -> `SYS-020`
- `SYS-020`, `SYS-014` -> `SWR-GUI-013`
- `SWR-GUI-013` -> `src/gui_main.c`
- verification evidence for `SWR-GUI-013` through GUI/DVT review steps and any
  localized-string regression checks

`SWR-PAT-003` should remain latest-reading access. The MVP does not need to
redefine that requirement because latest/live mode still depends on
`patient_latest_reading()` exactly as today. Historical review should be added
as a separate, explicit requirement path rather than broadening "latest" to
mean two different things.

## Medical-Safety, Security, And Privacy Impact

Medical-safety impact:

- review-only feature, but not zero-risk;
- main hazard is confusing retrospective state with current live state;
- auto-pause on review entry, synchronized snapshot derivation, and explicit
  historical mode are the key design controls.

This issue must not alter:

- vital-sign thresholds;
- NEWS2 scoring or response bands;
- aggregate alert-level logic;
- alarm-limit settings;
- treatment guidance or diagnosis claims.

Security impact:

- none expected;
- no authentication, role, or privilege behavior changes are needed.

Privacy impact:

- low if the feature remains one-session, in-memory, and local-only;
- unacceptable scope expansion would include export, persistence, printing, or
  remote sharing of historical snapshots.

## AI/ML Impact Assessment

This change does not add, modify, remove, or depend on an AI-enabled device
software function.

- Model purpose: none
- Input data: none beyond existing deterministic vital-sign snapshots
- Output: none beyond existing deterministic dashboard rendering
- Human-in-the-loop limits: unchanged
- Transparency needs: unchanged beyond explicit historical-versus-live labeling
- Dataset and bias considerations: none
- Monitoring expectations: standard regression and DVT verification only
- PCCP impact: none

## Validation Plan

Implementation validation shall cover both behavior and safety controls.

GUI and DVT validation:

- select each stored history row and confirm the vital tiles, NEWS2 tile,
  aggregate-status banner, and alert list all resolve from that exact reading;
- confirm the historical-review banner text is always explicit and visible;
- confirm the return-to-live action immediately restores latest/live behavior;
- confirm entering review auto-pauses simulation only when simulation was
  previously running;
- confirm return-to-live preserves a pre-existing manual pause and resumes only
  if review mode caused the pause;
- confirm `Add Reading`, `Admit / Refresh`, `Clear Session`, scenario loading,
  simulation-mode changes, and dashboard recreation clear review mode cleanly;
- confirm no session rollover or live update can silently remap the selected
  historical context while review mode is active.

Regression commands after implementation:

```powershell
run_tests.bat
ctest --test-dir build --output-on-failure -R "Patient|Trend|Alert"
python dvt/run_dvt.py
```

Additional verification:

- update localization regression checks for any new UI strings;
- update traceability rows and DVT evidence to include the new requirement path.

## Rollback Or Failure Handling

If implementation cannot preserve an unmistakable separation between
historical-review mode and live/latest mode, revert to latest-only dashboard
behavior and return the issue to design rather than shipping an ambiguous
hybrid.

If implementation discovers it needs broader behavior such as:

- background live acquisition while historical review remains active;
- multi-session persistence;
- retrospective export or printing; or
- new patient-domain APIs beyond the narrow GUI need,

stop and return for follow-on design instead of expanding the MVP inside this
issue.
