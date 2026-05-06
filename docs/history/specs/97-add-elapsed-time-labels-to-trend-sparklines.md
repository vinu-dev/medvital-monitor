# Design Spec: Issue 97

Issue: `#97`  
Branch: `feature/97-add-elapsed-time-labels-to-trend-sparklines`  
Spec path: `docs/history/specs/97-add-elapsed-time-labels-to-trend-sparklines.md`  
Risk note: `docs/history/risk/97-add-elapsed-time-labels-to-trend-sparklines.md`

## Problem

The dashboard already renders per-parameter sparklines, but the current UI in
`src/gui_main.c` shows only shape. Clinicians can see whether a trend rises or
falls, but they cannot tell how recent each visible point is without manually
counting samples from left to right.

The obvious solution, clock-like elapsed-time labels, is not currently safe on
this codebase:

- `VitalSigns` in `include/vitals.h` stores physiologic values only; it has no
  timestamp or acquisition-source metadata.
- `trend_extract_*()` in `src/trend.c` returns ordered samples
  oldest-to-newest, but only as value arrays.
- `paint_sparkline()` in `src/gui_main.c` renders a bare polyline with no axis
  or recency markers.
- The app mixes two ingestion paths:
  - simulation samples added by the `WM_TIMER` path every 2 seconds
  - manual readings added through `do_add_reading()` at arbitrary times

Because session history can contain mixed-cadence samples, minute- or hour-like
labels would imply timing precision the current data model cannot support.

## Goal

Add a narrow, truthful recency cue to the existing sparkline area so clinicians
can judge oldest-versus-newest sample position without manual counting, while:

- keeping the current numeric value and alert badge as the primary surfaces
- avoiding any claim of real elapsed time that the data model cannot prove
- staying within the current bounded 10-reading session model
- avoiding new production persistence, networking, or clinical logic

## Non-goals

- Adding wall-clock timestamps, durable timing history, or cross-session trend
  retention.
- Reworking the dashboard into a full charting or bedside-monitor trend screen.
- Changing thresholds, alert generation, NEWS2 scoring, alarm limits, or any
  other clinical calculation.
- Making manual-entry history appear evenly timed or chart-grade.
- Adding exports, audit logs, EMR integration, or remote review.
- Creating a second implementation path for simulator-only elapsed-time labels
  in this issue.

## Intended Use Impact, User Population, Operating Environment, And Foreseeable Misuse

Intended use impact:

- The change adds a read-only recency aid to the existing trend sparkline.
- It improves within-session trend interpretation only; it does not change
  clinical decision logic.

User population:

- Trained clinical staff, internal testers, and reviewers using the local
  Windows workstation application.

Operating environment:

- The current Win32 dashboard, with bounded in-process patient history and both
  simulator-fed and manually entered readings.

Foreseeable misuse:

- Interpreting recency markers as actual elapsed minutes or hours.
- Assuming equal point spacing means equal acquisition intervals.
- Treating the sparkline as a complete chart rather than a bounded session aid.
- Over-valuing sparkline recency markers relative to the latest numeric value
  and current alert status.

## Current Behavior

- `PatientRecord` stores up to `MAX_READINGS` ordered `VitalSigns` samples and
  exposes them oldest-to-newest.
- `trend_extract_hr()`, `trend_extract_sbp()`, `trend_extract_temp()`,
  `trend_extract_spo2()`, and `trend_extract_rr()` copy those ordered samples
  into integer buffers for sparkline rendering.
- `paint_sparkline()` scales the values to a 12-pixel strip and draws only the
  line; it provides no axis, point marker, or recency label.
- `paint_tile()` reserves only a sparkline strip plus badge row at the bottom
  of each tile.
- In simulation mode, `WM_TIMER` appends one new sample every 2 seconds.
- Manual entry and canned scenarios append samples through the same
  `patient_add_reading()` history, but without any timing metadata.
- When the simulation path hits the 10-reading cap, the dashboard reinitializes
  the patient session before adding the next sample and records a reset notice
  for the history/event review lists.

## Proposed Change

1. Define the MVP semantics as a reading-order recency scale, not as real
   elapsed time.
2. Represent recency relative to the newest visible sample using an explicit
   reading-offset notation:
   - `R0` = latest reading
   - `R-1` = one reading earlier
   - `R-(n-1)` = oldest visible reading in the current sparkline
3. Extend the sparkline area to include a compact recency axis beneath the
   polyline, with point-aligned tick marks and at most three text labels:
   - left edge: oldest visible offset (`R-(n-1)`)
   - midpoint: optional midpoint offset when `n >= 5` and width permits
   - right edge: latest offset (`R0`)
4. Highlight the newest sparkline point visually so the axis anchor is obvious
   even when text labels are suppressed.
5. Keep label semantics identical for simulator, manual-entry, and mixed-mode
   histories. This avoids unsafe mode inference from a data model that does not
   currently record acquisition source or trustworthy time.
6. Reserve enough vertical space in `paint_tile()` for the recency axis by
   modestly rebalancing the lower tile layout, but do not expand the issue into
   a broader dashboard redesign.
7. Suppress text labels when they would overlap the value or badge region, or
   when fewer than two points exist. A truthful absence of labels is preferable
   to unreadable or misleading text.
8. Keep all recency markers read-only. They must not feed alerting, NEWS2, DVT
   calculations, persistence, or any other clinical or regulatory logic.
9. Update requirement wording away from unsupported "elapsed time" claims and
   toward explicit "recency markers based on reading order" for this MVP.
10. Treat actual minute/hour labels as a follow-on design only. That future work
    must first add validated per-reading timing metadata and define mixed-mode
    behavior before any UI copy changes.

## Files Expected To Change

Expected requirements and traceability files:

- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`

Expected implementation files:

- `src/gui_main.c`
- `src/trend.c`
- `include/trend.h`

Expected verification files:

- `tests/unit/test_trend.cpp`
- `dvt/DVT_Protocol.md`

Files expected not to change for this issue:

- `include/vitals.h`
- `include/patient.h`
- `src/patient.c`
- `src/alerts.c`
- `src/news2.c`
- `src/alarm_limits.c`
- authentication, persistence, and networking modules

## Requirements And Traceability Impact

Existing requirements directly impacted:

- `UNS-009` Vital Sign History
- `UNS-014` Graphical dashboard
- `SYS-009` Vital Sign Reading History
- `SYS-014` Graphical Vital Signs Dashboard
- `SWR-TRD-001` Trend Sparkline and Direction Detection
- `SWR-GUI-003` Colour-Coded Vital Signs Display

Derived requirement updates are expected for:

- explicit sparkline recency semantics based on reading order rather than
  elapsed minutes/hours
- bounded label density and suppression rules for readability
- clear identification of the newest visible sample
- preserving session-bounded behavior after existing history resets

Traceability notes:

- This issue should not introduce a new clinical algorithm or a new patient
  history model; it is a presentation refinement over the existing ordered
  `PatientRecord.readings[]` buffer.
- A new UNS entry is probably not required because the feature is a refinement
  of existing history and dashboard needs rather than a net-new intended use.
- If the team later pursues true elapsed-time labels, that follow-on must add
  new SYS/SWR traceability for time-source validity, mixed-mode handling, and
  any new persistence or acquisition metadata.

## Medical-Safety, Security, And Privacy Impact

Medical safety:

- No intended change to thresholds, alerts, NEWS2, alarm limits, or treatment
  guidance.
- Primary safety benefit: clinicians can orient themselves to newest-versus-
  oldest trend position more quickly.
- Primary safety risk: users may infer real elapsed time from the markers.
- Key control: the axis notation must stay explicitly ordinal (`R0`, `R-4`,
  etc.), never clock-like (`2 min`, `10 s`, `14:32`) in this issue.
- Secondary risk: label clutter could obscure current values or alert state.
  The layout must keep value text and severity badge visually primary.

Security:

- No new authentication, authorization, storage, or network path is added.

Privacy:

- No new patient-data category or retention scope is added; the feature reuses
  the same session-local readings already present in the dashboard.

## AI/ML Impact Assessment

This change does not add, change, remove, or depend on an AI-enabled device
software function.

- Model purpose: not applicable
- Input data: deterministic existing vital-sign readings only
- Output: deterministic recency markers derived from reading order only
- Human-in-the-loop limits: unchanged
- Transparency needs: the UI must clearly show that the markers are ordinal,
  not elapsed-time claims
- Dataset and bias considerations: not applicable
- Monitoring expectations: standard software verification and GUI review only
- PCCP impact: none

## Validation Plan

Implementation should add targeted automated coverage plus focused GUI review.

Automated validation scope:

- unit tests for any helper that derives recency labels or label positions from
  reading count
- boundary coverage for counts `0`, `1`, `2`, `5`, and `10`
- tests proving endpoint labels remain `R-(n-1)` on the left and `R0` on the
  right for oldest-to-newest input ordering
- tests for label-suppression behavior when data count is insufficient

Manual / GUI validation scope:

- confirm the sparkline still renders clearly with 2, 5, and 10 visible points
- confirm the latest numeric value and alert badge remain more prominent than
  the recency axis
- confirm manual-entry histories and mixed histories still show reading-order
  markers, not clock-like labels
- confirm session rollover/reset starts a fresh recency axis and does not imply
  continuity with cleared prior-session samples

Recommended validation commands:

```powershell
run_tests.bat
ctest --test-dir build --output-on-failure -R "Trend"
python dvt/run_dvt.py
```

Expected validation outcome:

- The implementation remains limited to sparkline presentation, trend-helper
  support, requirements, and verification assets.
- No tests or DVT evidence need to change around alert thresholds, NEWS2, or
  patient-storage semantics.
- Review evidence shows the recency axis is understandable, readable, and does
  not over-claim timing precision.

## Rollback Or Failure Handling

- If implementation pressure pushes the team toward true elapsed-time labels,
  stop and split that work into a follow-on issue for timing metadata design.
- If the recency axis cannot be made readable inside the current tile layout
  without obscuring primary status information, do not ship misleading clutter;
  revert to the pre-feature sparkline and keep the issue blocked for a larger
  UX change.
- If requirement wording cannot avoid implying real clock time, narrow the
  wording to reading-order recency only before implementation proceeds.
- Runtime rollback is straightforward because the intended change is additive
  and presentation-only: remove the recency-axis rendering and helper logic and
  return to the current bare sparkline behavior.
