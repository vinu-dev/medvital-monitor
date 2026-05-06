# Design Spec: Add baseline markers to vital trend sparklines

Issue: #51
Branch: `feature/51-add-baseline-markers-to-vital-trend-sparklines`
Spec path: `docs/history/specs/51-add-baseline-markers-to-vital-trend-sparklines.md`

## Problem

Issue `#51` asks for a baseline or target marker inside each vital trend
sparkline so a clinician can tell at a glance whether the current trace is
above or below a comparison line.

That request is clinically and semantically ambiguous in the current product:

- the dashboard has no approved patient-specific baseline concept
- the dashboard has no existing clinician-entered target workflow
- the sparkline renderer in `src/gui_main.c` currently draws only the extracted
  session history and auto-scales each tile to the history min/max
- the existing presentation already derives tile severity from fixed vital
  classification rules and derives sparkline data from `trend_extract_*()`

Without a precise reference definition, implementation would risk inventing a
new clinical meaning. The risk note at
`docs/history/risk/51-add-baseline-markers-to-vital-trend-sparklines.md`
explicitly disallows that. The design therefore has to define one approved
comparison source that is already part of the product's existing clinical
semantics and keep the change presentation-only.

## Goal

Add neutral reference markers to the existing five vital sparklines so the
dashboard shows deviation against an already approved comparison range, while
leaving capture, history storage, alert generation, NEWS2, and alarm behavior
unchanged.

## Non-goals

- No patient-derived baseline computed from prior readings.
- No new configurable target, no new settings, and no use of
  `g_app.alarm_limits` as the sparkline reference source in this issue.
- No change to `check_*()` classification behavior, alert generation,
  `overall_alert_level()`, NEWS2 scoring, or simulator/HAL timing.
- No new persistence, export, logging, analytics, or telemetry.
- No waveform work, no dashboard layout redesign, and no vendor-specific UI
  mimicry.
- No change to the current blood-pressure sparkline data source from systolic to
  a new combined systolic/diastolic visualization.

## Intended use impact, user population, operating environment, and foreseeable misuse

Intended use impact remains display-only. The feature adds a secondary visual
cue to the existing dashboard so trained users can orient more quickly to how
recent values sit relative to the product's already approved normal reference
range.

User population remains the current dashboard audience:

- bedside nurses
- ward clinicians
- intensivists
- supervised demo or pilot operators using the Win32 dashboard

Operating environment remains the current Windows dashboard path in
`src/gui_main.c`, using the in-memory `PatientRecord` history already populated
by simulation mode or the HAL-backed acquisition path.

Foreseeable misuse the implementation must resist:

- interpreting the marker as a patient-specific target or treatment goal
- interpreting the marker as a new alarm line or a configurable bedside limit
- assuming marker absence means normal rather than unavailable or not measured
- over-trusting the marker instead of the numeric value, severity tile color,
  alert banner, or alerts list
- reading the blood-pressure marker as covering diastolic behavior when the
  existing sparkline is systolic-only

## Current behavior

The current display and data path are already bounded:

- `paint_tiles()` in `src/gui_main.c` extracts per-parameter sparkline arrays
  with `trend_extract_hr()`, `trend_extract_sbp()`, `trend_extract_temp()`,
  `trend_extract_spo2()`, and `trend_extract_rr()`.
- `paint_tile()` renders the tile label, current value, one sparkline strip,
  and the bottom severity badge.
- `paint_sparkline()` auto-scales each sparkline against only the extracted
  series min/max and has no concept of an external comparison value.
- the blood-pressure tile uses systolic history only because the GUI already
  calls `trend_extract_sbp()`
- respiration rate shows `--` in the value field when the latest reading is
  not measured (`respiration_rate == 0`)
- tile severity colors still come from `check_heart_rate()`,
  `check_blood_pressure()`, `check_temperature()`, `check_spo2()`, and
  `check_respiration_rate()` in `src/vitals.c`

The approved comparison semantics that already exist in the code are the normal
bands embedded in those `check_*()` functions:

- heart rate: 60 to 100 bpm
- systolic blood pressure: 90 to 140 mmHg
- temperature: 36.1 to 37.2 C
- SpO2: 95% lower bound
- respiration rate: 12 to 20 br/min

Those values are already clinically meaningful to the current dashboard because
they anchor the tile's existing normal versus warning behavior. They are the
least risky comparison source for this issue.

## Proposed change

Implement this feature as a presentation-only overlay whose reference semantics
come from the existing vital normal-band thresholds already used by the current
dashboard logic.

### 1. Resolve "baseline" semantics to approved normal-range reference bounds

For this issue, "baseline marker" shall mean a neutral marker for the approved
normal display range already encoded in the existing `check_*()` logic.

It shall not mean:

- a patient-specific historical baseline
- a clinician-entered target
- a configurable alarm-limit value from `alarm_limits.cfg`
- a predictive or inferred safe zone

This choice keeps the visual cue aligned with the dashboard's current
classification semantics and avoids introducing a second, contradictory
reference model.

### 2. Expose read-only reference bounds from one authoritative source

Implementation should not duplicate threshold literals inside `gui_main.c`.

Instead, refactor the existing threshold values behind a small shared,
read-only interface so the renderer can ask for the approved reference bounds
without changing classification behavior. Acceptable implementation shapes:

- promote the existing threshold literals in `src/vitals.c` to named constants
  with corresponding read-only accessors in `include/vitals.h`
- or introduce a tiny shared helper/module that both classification logic and
  presentation code consume

The chosen approach must preserve the exact current output of the existing
`check_*()` functions. This is classification-adjacent work and must be handled
as a no-behavior-change refactor, not a clinical-threshold redesign.

### 3. Render one or two neutral reference markers per sparkline, truthfully by parameter

Parameter-specific rules:

- Heart Rate: render two markers at the lower and upper normal bounds
  (`60`, `100`)
- Blood Pressure tile: render two markers for the existing systolic sparkline
  only (`90`, `140`)
- Temperature: render two markers at `36.1` and `37.2`, using the same scaled
  integer convention already used by `trend_extract_temp()`
- SpO2: render one lower-bound marker at `95`
- Respiration Rate: render two markers at `12` and `20` only when the latest RR
  is actually measured

The blood-pressure rule is intentionally explicit: the existing tile already
uses a systolic-only sparkline, so the reference markers must match that same
subseries rather than imply a hidden diastolic model.

### 4. Extend sparkline scaling to include reference bounds

Because `paint_sparkline()` currently scales only against the extracted series,
reference markers would be misleading if they were projected with a different
scale or allowed to clip outside the strip.

The sparkline path should therefore compute one shared vertical scale per tile
using:

- the extracted sparkline values
- any reference bounds that will be drawn for that tile

That combined scale should then be used for:

- the polyline itself
- the one or two reference markers

This keeps the overlay geometrically truthful without changing the underlying
trend extraction data.

### 5. Keep the overlay visually secondary and explicitly non-alarm-colored

Visual rules:

- draw markers as thin neutral lines or ticks, not as a second red/amber/green
  severity system
- keep the current numeric value, tile border, and bottom severity badge as the
  dominant safety cues
- render markers inside the existing sparkline strip only
- if a marker layout would overlap or obscure the sparkline at the current tile
  size, prefer hiding the marker rather than degrading the primary value/badge
  display

The implementation should avoid words such as `Target` or `Goal` in the tile.
If any user-visible legend text is added, it should use neutral reference
language and remain subordinate to the primary tile content.

### 6. Tie visibility to the existing sparkline/live-data rules

Reference markers should be shown only when the corresponding sparkline context
is valid:

- simulation/live display path is active
- an active patient exists
- the current tile has a current measured value
- at least two sparkline samples exist, matching the current sparkline-render
  threshold

Markers must be hidden when:

- the tile shows `N/A` in device mode
- there is no active patient
- the current value is unavailable after admission/reset
- respiration rate is not measured in the latest sample (`respiration_rate == 0`)
- the tile is the NEWS2 tile

This issue should not broaden into a separate missing-data cleanup. In
particular, it should not redefine `trend_extract_rr()` semantics unless the
team separately approves that as a follow-on change.

### 7. Keep requirements scope in GUI behavior, not alarm-limit behavior

This issue should not redefine `SWR-ALM-001` because configurable alarm limits
are not the chosen reference source.

Recommended requirements approach:

- keep `SYS-014` as the parent dashboard requirement
- add a focused new GUI requirement, `SWR-GUI-013` or the next available GUI
  identifier, to define:
  - the five affected tile/sparkline surfaces
  - the per-parameter reference semantics listed above
  - the blood-pressure systolic-only limitation
  - the visibility suppression rules
  - the requirement that the overlay be visually secondary and must not alter
    severity colors or alert state
- keep `SWR-TRD-001` as the dependency for sparkline extraction only

## Files expected to change

Expected implementation files:

- `src/gui_main.c`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`

Likely implementation files if a shared threshold interface is added:

- `src/vitals.c`
- `include/vitals.h`

Likely automated-verification files:

- `tests/unit/test_vitals.cpp`
- `tests/unit/test_trend.cpp` if helper coverage is kept there
- or a new focused unit test file for sparkline-reference mapping/scale logic
- `tests/CMakeLists.txt` if a new unit-test translation unit is added

Files that should not change in this issue:

- `src/alerts.c`
- `src/news2.c`
- `src/patient.c`
- `src/sim_vitals.c`
- `src/alarm_limits.c`
- persistence/config paths and release workflows

## Requirements and traceability impact

Recommended requirement strategy:

- keep `SYS-014` unchanged as the governing system requirement unless the team
  wants the dashboard comparison overlay called out explicitly at system level
- add `SWR-GUI-013` for sparkline reference markers
- leave `SWR-TRD-001` responsible for the sparkline data extraction behavior
- leave `SWR-ALM-001` unchanged because this issue must not visualize or depend
  on configurable alarm-limit settings

Recommended `SWR-GUI-013` content:

- each of the five vital sparkline tiles may display one or two neutral
  reference markers derived from the approved normal range already used by the
  corresponding vital classification logic
- blood pressure uses the existing systolic sparkline and corresponding
  systolic reference bounds only
- SpO2 uses a single lower-bound marker
- respiration-rate markers are suppressed when the latest RR is not measured
- reference markers are supplementary only and shall not change tile severity
  colors, banner behavior, alert generation, or NEWS2 output

Traceability impact is expected in:

- forward traceability from the new GUI requirement to `src/gui_main.c`
- backward traceability to new automated evidence for threshold export and
  overlay-coordinate logic
- continued linkage to `SWR-TRD-001` for the existing sparkline extraction
  helpers

## Medical-safety, security, and privacy impact

Medical-safety impact is low but not zero because the change alters emphasis in
the clinician's glanceable display.

Primary safety risks:

- a user reads the marker as a patient-specific target rather than a normal
  reference bound
- a user assumes the marker changes alert behavior
- the systolic-only blood-pressure marker is misread as a full BP target
- overlay geometry is inaccurate because the reference bounds and sparkline are
  not scaled together

Primary controls in this design:

- define the marker semantics explicitly as existing normal-range references
- keep the overlay neutral and visually secondary
- use one authoritative threshold source shared with the current classification
  logic
- keep NEWS2, alert generation, and acquisition logic unchanged
- make the blood-pressure systolic-only scope explicit in requirements and test
  notes

Security impact is none expected. No authentication, authorization, networking,
or storage behavior changes.

Privacy impact is none expected. No new patient-data category, export path, or
telemetry path is introduced.

Because the feature touches clinical-display semantics adjacent to current vital
classification ranges, the implementation PR should explicitly state that the
threshold values are exported read-only and that `check_*()` behavior is
unchanged.

## AI/ML impact assessment

This change does not add, change, remove, or depend on an AI-enabled device
software function.

- Model purpose: none
- Input data: existing deterministic vital-sign history and existing approved
  normal-range bounds
- Output: passive sparkline reference markers only
- Human-in-the-loop limits: unchanged
- Transparency needs: the overlay must remain clearly supplementary and
  non-predictive
- Dataset and bias considerations: none beyond the existing deterministic
  bedside values already displayed
- Monitoring expectations: standard GUI and unit-test verification only
- PCCP impact: none

## Validation plan

Automated verification should cover both no-regression behavior and the new
overlay math.

Recommended unit-test additions:

- verify the exported reference bounds for HR, systolic BP, temperature, SpO2,
  and RR match the currently approved normal-band values
- verify existing `check_*()` behavior is unchanged after any threshold
  refactor
- verify the overlay-scale helper maps series values and reference bounds onto a
  shared y-axis without clipping or inverted ordering
- verify one-bound and two-bound marker cases
- verify suppression rules for no patient, device mode, unavailable current
  value, NEWS2, and RR-not-measured

Suggested commands:

```powershell
cmake --build build
build/tests/test_unit.exe
build/tests/test_integration.exe
```

Manual GUI verification:

- confirm each live vital tile shows its reference markers only when the
  sparkline itself is present
- confirm the reference markers stay behind the sparkline and do not overpower
  the numeric value or bottom severity badge
- confirm the blood-pressure markers track the systolic sparkline only
- confirm the SpO2 tile shows only one marker
- confirm no markers appear on NEWS2
- confirm RR markers disappear when RR is not measured
- confirm alert colors, alerts list, aggregate banner, and NEWS2 outputs are
  unchanged for the same simulation scenarios

## Rollback or failure handling

If implementation cannot expose the existing normal-band bounds without risking
behavior drift in `check_*()`, stop and return the issue for design review
rather than duplicating threshold literals in multiple places.

If the overlay cannot be made visually subordinate within the current tile
layout, hide or remove the overlay rather than weakening the readability of the
numeric value or severity badge.

Rollback is straightforward:

- revert the sparkline-overlay implementation commit(s)
- remove any read-only threshold-export helper added for the feature
- restore the existing sparkline-only dashboard behavior

If product ownership later wants configurable targets, patient-specific
baselines, or diastolic blood-pressure reference behavior, that should be a new
design issue with explicit clinical approval rather than an extension of this
MVP.
