# Design Spec: add preset trend window controls to the dashboard

Issue: #55
Branch: `feature/55-add-preset-trend-window-controls-to-the-dashboard`
Spec path: `docs/history/specs/55-add-preset-trend-window-controls-to-the-dashboard.md`

## Problem

Issue #55 asks for a preset trend-window control on the live dashboard so an
operator can compare a short recent sparkline view with a longer retained view
without leaving the main screen.

The current dashboard already renders sparklines from the in-memory patient
history, but it always uses the entire retained buffer and gives the operator no
explicit control over how much of that retained data is shown. That limits quick
visual comparison between recent excursions and the broader retained context.

The issue text uses "full in-session history", but the current product does not
retain full-session history. `PatientRecord` stores only `MAX_READINGS = 10`
samples, and the simulation timer reinitializes the record when that buffer is
full. If the design keeps the phrase "full history", the UI would overstate what
the system actually retains and create an avoidable interpretation risk.

## Goal

Add a narrow, deterministic, session-local trend-window selector that:

- lets the operator switch all vital-sign sparklines between a short recent
  subset and the full currently retained buffer
- makes the active window selection continuously visible in the dashboard UI
- resets the selection whenever patient/session context resets
- does not change patient data storage, alert logic, NEWS2 scoring, alarm
  limits, acquisition cadence, or persistence behavior

## Non-goals

- No persistent, long-horizon, or complete session-history review.
- No new storage model beyond the existing 10-sample retained buffer.
- No changes to vital classification, alerts, NEWS2, configurable alarm limits,
  authentication, localization behavior beyond adding needed UI strings, or
  simulation acquisition logic.
- No cursor review, export, print, zoom timeline, event markers, or vendor-like
  historical review workflow.
- No claim that this UI change improves diagnosis, triage, or early detection.

## Intended use impact, user population, operating environment, and foreseeable misuse

- Intended use impact: adds a secondary dashboard display aid for comparing a
  recent subset of retained trend data against the full retained buffer already
  available in memory.
- User population: authenticated clinical and admin dashboard users; not
  patient-facing.
- Operating environment: local Win32 dashboard, static-memory C codebase,
  session-scoped in-memory patient record, current simulation cadence of
  roughly one sample every 2 seconds.
- Foreseeable misuse: user interprets "all retained" as full-session history;
  user misses an earlier retained excursion while viewing a short preset; user
  assumes the selector changes clinical computations rather than display scope;
  stale preset survives a patient/session reset and is applied to a new context.

## Current behavior

- `src/gui_main.c` paints five vital-sign tiles with sparklines and one NEWS2
  tile without a sparkline.
- `paint_tiles()` always extracts up to `MAX_READINGS` values for each sparkline
  using `trend_extract_hr()`, `trend_extract_sbp()`, `trend_extract_temp()`,
  `trend_extract_spo2()`, and `trend_extract_rr()`.
- `PatientRecord` stores only 10 retained readings, oldest to newest.
- In simulation mode, `WM_TIMER` reinitializes the patient record when the
  buffer is full, then starts filling it again from an empty retained history.
- Admit/refresh, clear-session, scenario setup, and logout already establish or
  clear patient/session state, but there is no trend-window state to reset.
- New visible dashboard strings are localized through the existing static
  localization tables.

## Proposed change

Implement issue #55 as a global dashboard preset over the current retained
sparkline data only.

Recommended behavior:

1. Add a single dashboard-level trend-window control that applies to all five
   sparkline tiles at once. Do not add separate per-tile selectors.
2. Use neutral labels that reflect actual retained data. Recommended MVP
   presets:
   - `Last 5`
   - `All retained`
3. Default to `All retained`.
4. Display the active selection continuously in the control itself and with an
   adjacent localized label such as `Trend Window`.
5. Apply the selection only to sparkline extraction and rendering. Numeric tile
   values, status colors, alerts, NEWS2, reading history text, and patient data
   must remain unchanged.
6. If fewer readings are available than the selected preset expects, show only
   the available retained readings; do not pad, synthesize, or imply hidden
   history.
7. Reset the selection to `All retained` on:
   - dashboard creation after login
   - patient admit/refresh
   - clear session
   - scenario setup paths that create a new patient context through admit
   - logout, via normal session teardown and next-login default
   - timer-driven full-buffer reinitialization path in simulation mode
8. Keep the selector session-local only. Do not store it in `monitor.cfg` or
   any other persistence file.

Recommended implementation shape:

- Add a small `TrendWindowPreset` enum and a bounded helper in `trend.h` /
  `trend.c` that resolves the retained slice for a preset, for example by
  returning a validated start index and count for a given available reading
  count.
- Keep existing `trend_extract_*()` helpers and reuse them against the resolved
  subrange instead of duplicating extraction logic in multiple GUI paths.
- Store the selected preset in `AppState` and update it from a compact header
  control, preferably a localized combobox or equivalent two-state selector
  anchored with the existing header controls.
- Update `reposition_dash_controls()` so the selector remains visible on window
  resize without crowding primary alert or session controls.

This design keeps the logic testable in the trend module and keeps the Win32
paint path focused on rendering rather than slice math.

## Files expected to change

Expected implementation files:

- `src/gui_main.c`
- `include/trend.h`
- `src/trend.c`
- `tests/unit/test_trend.cpp`
- `include/localization.h`
- `src/localization.c`

Expected requirements and traceability files:

- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`

Files to inspect and modify only if the team records manual GUI verification
there:

- `dvt/DVT_Protocol.md`

Files that should not change for this issue:

- `src/vitals.c`
- `src/alerts.c`
- `src/news2.c`
- `src/alarm_limits.c`
- `src/patient.c`
- `src/app_config.c`
- authentication and account-management files
- CI and release workflow files

## Requirements and traceability impact

No new UNS is recommended for the narrowed MVP. Existing user needs already
cover dashboard presentation and retained reading history.

Recommended requirement updates:

- Add one new SYS requirement under the dashboard area, for example a system
  requirement stating that the dashboard shall let the operator select between a
  recent retained subset and all retained readings for sparkline display, and
  that the selection shall not affect stored readings or clinical calculations.
- Add one new GUI-level SWR for selector visibility, allowed presets, default
  state, and reset semantics.
- Amend `SWR-TRD-001` or add a narrowly scoped trend-unit SWR so the retained
  slice selection logic is defined in a unit-testable way.
- Add traceability rows mapping the new SYS/SWR entries to the implementation
  files above and to unit/manual verification evidence.

Recommended traceability anchors:

- Existing related coverage: `SYS-014`, `SWR-GUI-003`, `SWR-TRD-001`,
  `SWR-PAT-003`, `SWR-PAT-006`
- New verification should remain display-only and must explicitly state that
  preset changes do not alter alerts, NEWS2, or stored-reading order/count

If product ownership later wants true full-session or handoff history, that is a
separate capability and needs new storage requirements plus another risk pass.

## Medical-safety, security, and privacy impact

Medical-safety impact is low but not zero because this changes display context
for clinically relevant trend information.

- Safety risk: interpretive misuse of displayed context, especially if the short
  preset is mistaken for the full retained record or if labels imply more
  history than the system actually stores.
- Required controls:
  - use truthful labels such as `All retained`, not `Full history`
  - reset to the default on every patient/session reset path
  - keep numeric values, alert surfaces, and NEWS2 visually primary
  - ensure the selector cannot change stored data or alert computation

Security impact is low.

- No new network path, credential flow, privilege boundary, or file persistence.
- Main integrity risk is stale UI state crossing session boundaries; the reset
  requirements above are the control.

Privacy impact is none expected.

- No new data classes, exports, or retention paths are introduced.

## AI/ML impact assessment

This change does not add, change, remove, or depend on an AI-enabled device
software function.

- Model purpose: not applicable
- Input data: not applicable beyond existing deterministic vital-sign samples
- Output: not applicable
- Human-in-the-loop limits: not applicable
- Transparency needs: no AI transparency requirement; only truthful UI wording
  about retained data scope
- Dataset and bias considerations: not applicable
- Monitoring expectations: standard regression monitoring only
- PCCP impact: none

## Validation plan

Implementation validation should cover selector logic, reset behavior, and
regression protection against unintended clinical-output changes.

Unit validation:

- Add `test_trend.cpp` coverage for preset-to-slice resolution:
  - zero readings
  - fewer than 5 readings
  - exactly 5 readings
  - full 10-reading retained buffer
  - no out-of-range start/count results
- If the helper resolves subranges, verify oldest/newest boundaries match the
  selected preset exactly.

Manual GUI validation:

- Launch the dashboard in simulation mode and confirm the default is
  `All retained`.
- Toggle between `Last 5` and `All retained` with 0, partial, and full retained
  buffers.
- Confirm only sparkline scope changes; numeric tile values, alert banner,
  active alerts, reading history list, and NEWS2 remain unchanged.
- Confirm the selector resets to default after:
  - admit/refresh
  - clear session
  - logout/login
  - automatic buffer rollover in simulation mode
- Confirm the control remains readable and non-overlapping on the minimum
  supported dashboard window size and after resize.

Requirements/traceability validation:

- Update the requirement and RTM documents consistently.
- Ensure any new localized strings exist for all four supported languages.

Suggested implementation-time commands:

```powershell
cmake -S . -B build -G Ninja -DBUILD_TESTS=ON
cmake --build build
build/tests/test_unit.exe --gtest_filter=*Trend*
build/tests/test_unit.exe
```

## Rollback or failure handling

If implementation cannot fit a clear, non-crowding selector into the dashboard
layout without obscuring primary controls, stop and simplify the UI before
changing any clinical-facing behavior.

If the implementation appears to require:

- more than the retained 10-sample buffer
- selector persistence across sessions
- changes to patient storage, alerts, NEWS2, or alarm logic
- wording that still implies full-session history

then stop and reopen design/risk review rather than broadening issue #55 in
code.

If regression testing shows any change to alert generation, NEWS2 score, or
patient record contents when only the preset changes, revert the feature and
treat it as a design breach rather than a cosmetic defect.
