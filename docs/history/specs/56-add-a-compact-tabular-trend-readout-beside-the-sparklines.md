# Design Spec: Issue #56

Date: 2026-05-06
Issue: `#56` - Feature: add a compact tabular trend readout beside the sparklines
Branch: `feature/56-add-a-compact-tabular-trend-readout-beside-the-sparklines`
Risk note: `docs/history/risk/56-add-a-compact-tabular-trend-readout-beside-the-sparklines.md`

## Problem

The dashboard already shows exact current values, a free-form retained reading
history list, and per-parameter sparklines. It does not provide a compact,
same-glance numeric view of the most recent retained samples near the sparkline
region. Operators who want the exact recent values behind the trend shape must
scan the lower history list or mentally reconstruct values from the line.

That gap is a usability issue, not a clinical-logic gap. The current retained
history model is still bounded to `MAX_READINGS = 10`, session-local, and
untimestamped.

## Goal

Add a read-only compact retained-sample table to the dashboard trend area so an
operator can see exact recent values for the active retained window without
leaving the main dashboard and without changing any alerting, NEWS2, alarm
limits, authentication, storage, or persistence behavior.

## Non-goals

- No change to vital-sign acquisition, validation, alert generation, NEWS2, or
  alarm-limit logic.
- No change to the `PatientRecord` storage model, buffer size, or rollover
  behavior.
- No timestamps, elapsed-time claims, or handoff-history claims.
- No export, print, persistence, audit-trail, or review-workflow additions.
- No vendor-specific trend-review language, cursor workflows, or zoom panes.
- No replacement of the existing primary surfaces: current tile values, status
  banner, active alerts list, or reading-history list.

## Intended use impact, user population, operating environment, and foreseeable misuse

Intended use impact:

- The feature is a secondary dashboard review aid for already captured values.
- It does not change the product's intended use or introduce new clinical
  decision support.

User population:

- Authorized clinical users and admins already permitted to view the dashboard.

Operating environment:

- Win32 desktop dashboard.
- Local in-memory patient session only.
- Retained sample window of at most 10 readings.
- Simulation mode, manual entry, and scenario-driven local review.

Foreseeable misuse:

- Interpreting the compact table as complete session history.
- Misreading sample order and confusing oldest with newest.
- Assuming time-based spacing or timestamps that do not exist.
- Treating the compact table as more authoritative than the current-value tiles
  or alert surfaces.
- Reading stale rows after admit, clear-session, logout, or automatic rollover
  reset if the implementation does not derive directly from current state.

## Current behavior

- `src/gui_main.c` paints a 3x2 tile grid with exact current values and
  sparklines for HR, BP, Temp, SpO2, and RR, plus a NEWS2 tile.
- `src/gui_main.c` derives sparkline arrays from `g_app.patient.readings` via
  `trend_extract_*()` helpers and repaints them during `update_dashboard()`.
- `src/gui_main.c` also renders a lower retained reading-history list, but each
  entry is a free-form string rather than a compact numeric comparison surface.
- `include/patient.h` and `src/patient.c` define a bounded reading buffer of
  `MAX_READINGS = 10`.
- On the simulation timer path, `src/gui_main.c` reinitializes the patient
  record when the retained buffer is full before adding the next reading.
- The data model carries sample order only. It does not store timestamps for
  each reading.

## Proposed change

### UX shape

- Add a compact "Retained Trend Samples" companion panel in the dashboard trend
  zone adjacent to the sparkline tiles.
- At normal dashboard widths, place the panel beside the tile grid so it reads
  as a same-glance companion to the sparklines.
- If the available width cannot preserve readable tile labels and table cells,
  the implementation may stack the panel immediately below the tile band, but it
  must keep the same data semantics and remain visually secondary to the tiles.

### Data shown

- Show only direct vital-sign rows that already have sparkline support:
  `HR`, `BP`, `Temp`, `SpO2`, and `RR`.
- Do not include NEWS2 in the compact table. NEWS2 is a derived score without a
  corresponding sparkline in this comparison surface.
- Show only the latest visible subset of retained readings:
  `VISIBLE_TREND_SAMPLES = 4`.
- When fewer than four retained readings exist, show only the available
  readings.
- Display visible samples in chronological order from oldest to newest within
  the visible subset so the table direction matches the sparkline direction.
- Label the newest visible column explicitly.
- Label the panel as a retained-sample view, not as full history or timed
  history.
- Render RR values of `0` as `--` to preserve the existing "not measured"
  meaning.

### Data source and state rules

- Derive the compact table directly from `g_app.patient.readings` and
  `g_app.patient.reading_count`.
- Do not introduce a second retained-history cache or any persisted table state.
- The compact table, sparkline region, and lower history list must refresh from
  the same underlying patient record on every `update_dashboard()`.
- On login, logout, admit/refresh, clear-session, simulation disable, and
  automatic rollover reset, the compact table must clear or rebuild from the
  current patient record with no stale rows surviving the transition.

### Implementation note for recent-window extraction

The current `trend_extract_*()` helpers copy from the oldest retained reading
forward and are safe for the existing sparkline path because `MAX_READINGS`
matches the retained buffer size. They are not sufficient on their own for a
"latest 4 samples" table. A naive reuse with `max_out = 4` would return the
oldest four retained samples rather than the latest four.

Implementation should therefore add one tested, non-GUI helper in the trend
module to define the visible retained window for compact-table rendering. A
minimal acceptable shape is:

- `trend_recent_window_start(int count, int max_visible)`

with behavior equivalent to `max(0, count - max_visible)`.

The GUI can then format visible cells directly from
`g_app.patient.readings[start + i]`, preserving one authoritative source of
truth while keeping the subset selection testable outside Win32 paint code.

## Files expected to change

- `docs/history/specs/56-add-a-compact-tabular-trend-readout-beside-the-sparklines.md`
- `src/gui_main.c`
- `include/trend.h`
- `src/trend.c`
- `tests/unit/test_trend.cpp`
- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`
- `dvt/DVT_Protocol.md`

## Requirements and traceability impact

Recommended traceability approach:

- Keep existing user-need coverage under `UNS-009`, `UNS-010`, and `UNS-014`.
  No new UNS is required if the feature remains display-only and session-local.
- Add a new system requirement, tentatively `SYS-020`, for a compact retained
  trend review surface that is explicitly bounded to the current retained window
  and does not alter clinical calculations.
- Add a new GUI software requirement, tentatively `SWR-GUI-013`, for:
  row set, visible subset size, chronological ordering, newest-column marking,
  RR not-measured rendering, empty-state behavior, and reset behavior.
- Add a new trend helper requirement, tentatively `SWR-TRD-002`, for recent
  retained-window selection used by the compact table.
- Update the RTM and DVT mapping so the new helper has automated evidence and
  the new dashboard surface has manual GUI verification evidence.

## Medical-safety, security, and privacy impact

Medical-safety impact:

- Low direct impact because the change is presentation-only.
- Main hazard is interpretive: users may over-read a compact table as complete,
  time-based, or more authoritative than it is.
- Required controls are explicit retained-window labeling, explicit ordering,
  no timestamp claims, and visual secondary status relative to the current
  value tiles and alerts.

Security impact:

- None expected. No auth, session, network, or file-permission behavior changes.

Privacy impact:

- Low. No new data category or persistence path is introduced.
- The panel increases the density of values visible on screen, so it must stay
  local, session-scoped, and free of export/persistence expansion in this issue.

## AI/ML impact assessment

This change does not add, remove, modify, or depend on an AI-enabled device
software function.

- Model purpose: none
- Input data: none beyond existing deterministic vital-sign readings already in
  the patient record
- Output: none beyond a static UI rendering of existing values
- Human in the loop limits: unchanged
- Transparency needs: unchanged
- Dataset and bias considerations: none
- Monitoring expectations: unchanged
- PCCP impact: none

## Validation plan

Automated:

- Add unit tests for the recent-window helper covering:
  zero readings, one reading, fewer than visible max, exactly visible max, more
  than visible max, and full-buffer rollover counts.
- Keep existing trend, patient, and integration suites passing to show that the
  compact table work does not change clinical calculations.

Manual GUI verification:

- No patient admitted.
- Simulation disabled.
- One retained reading.
- Two to four retained readings.
- Full retained window of ten readings with the compact table still showing only
  the latest visible subset and explicitly identifying that scope.
- RR not measured (`0`) rendered as `--`.
- Admit/refresh resets the table to the new patient context.
- Clear-session removes stale rows.
- Logout/login removes stale rows.
- Automatic rollover reset in simulation mode removes stale pre-reset rows.
- Window resize around the current minimum width preserves readable tiles and
  does not overlap the compact table with other controls.

Build and regression commands:

- `cmake -S . -B build -G Ninja -DBUILD_TESTS=ON`
- `cmake --build build`
- `build/tests/test_unit.exe`
- `build/tests/test_integration.exe`

## Rollback or failure handling

- Fail closed in the UI. If there is no active patient or no visible retained
  sample window, render an empty-state message instead of stale or fabricated
  cells.
- If layout constraints cannot preserve readable labels and cells, use the
  stacked placement or omit the compact panel rather than clipping misleading
  values into the tile region.
- If implementation review finds that the panel causes overlap, stale-state
  risk, or misleading retained-window claims, revert only the compact-table
  change and related requirement updates. Existing sparkline and reading-history
  behavior must remain intact.
