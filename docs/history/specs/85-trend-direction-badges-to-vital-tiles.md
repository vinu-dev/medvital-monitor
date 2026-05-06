# Design Spec: Issue 85

Issue: `#85`  
Branch: `feature/85-trend-direction-badges-to-vital-tiles`  
Spec path: `docs/history/specs/85-trend-direction-badges-to-vital-tiles.md`

## Problem

The dashboard already draws a sparkline in each vital-sign tile, but the user
must still infer recent direction of change by visually reading the graph.
The repository already has a deterministic `trend_direction()` helper plus
per-parameter extraction helpers, yet that result is not surfaced in the GUI
as an explicit cue.

This leaves a gap between what the software already computes and what the
operator can understand at a glance:

- `src/gui_main.c` extracts trend series for heart rate, systolic blood
  pressure, temperature, SpO2, and respiration rate, but `paint_tile()` only
  renders the measured value, sparkline, and alert-severity badge.
- `src/trend.c` can already classify a series as `TREND_RISING`,
  `TREND_STABLE`, or `TREND_FALLING` with bounded static logic and unit tests.
- The blood-pressure sparkline currently uses systolic data only
  (`trend_extract_sbp()`), even though the visible tile value shows both
  systolic and diastolic pressure.
- Respiration rate uses `0` as a "not measured" sentinel, which is acceptable
  for storage but unsafe to present as if it were a real trend input.

## Goal

Add a narrow, display-only trend-direction cue to the five vital-sign tiles so
the operator can quickly see whether a parameter is rising, steady, or falling,
while keeping the existing sparkline and alert-severity surfaces as the primary
signals.

The design must:

- reuse the existing deterministic trend pipeline rather than inventing new
  clinical logic
- remain bounded to the current local session and current Win32 dashboard
- preserve existing alert thresholds, NEWS2 behavior, persistence scope, and
  authentication boundaries
- make the trend cue clearly directional only, not predictive, prescriptive, or
  a substitute for alert severity

## Non-goals

- Changing any vital-sign thresholds, alarm limits, NEWS2 scoring, alert
  generation, or aggregate status behavior.
- Adding treatment advice, escalation workflow, acknowledgement, multi-patient
  surveillance, retrospective review tooling, export, persistence, or network
  transport.
- Adding a trend cue to the NEWS2 tile, aggregate status banner, or any
  non-vital dashboard surface.
- Defining a new combined blood-pressure trend algorithm for this issue.
- Claiming that the cue indicates "better", "worse", or impending clinical
  deterioration.

## Intended Use Impact, User Population, Operating Environment, And Foreseeable Misuse

Intended use impact:

- The change adds a secondary review cue to the existing live dashboard.
- It does not change the primary monitoring workflow, alerting workflow, or
  patient-session model.

User population:

- Trained clinicians, internal testers, and reviewers using the local Windows
  desktop application.

Operating environment:

- The existing authenticated Win32 dashboard with bounded in-memory session
  history (`MAX_READINGS`), in simulation mode, manual-entry mode, or future
  HAL-backed live mode.

Foreseeable misuse:

- Treating "rising" as always worse or "falling" as always better.
- Assuming the blood-pressure cue represents both systolic and diastolic motion.
- Reading missing respiration data as a real falling or recovering trend.
- Treating the cue as an alarm, prediction, or recommendation.
- Over-interpreting one or two recent points as a meaningful direction signal.

## Current Behavior

- `paint_tiles()` extracts sparkline data from `g_app.patient.readings` using
  `trend_extract_hr()`, `trend_extract_sbp()`, `trend_extract_temp()`,
  `trend_extract_spo2()`, and `trend_extract_rr()`.
- `paint_sparkline()` renders the graph when at least two data points are
  available.
- `paint_tile()` renders one colored alert-severity badge per tile:
  `NORMAL`, `WARNING`, or `CRITICAL`.
- `trend_direction()` compares the mean of the first half of a value series to
  the mean of the second half using a 5 percent hysteresis band.
- The blood-pressure tile displays `systolic / diastolic`, but the current
  trend extraction path for that tile is systolic only.
- The respiration tile shows `--` when `respiration_rate == 0`, but the current
  extraction helper still copies that `0` into the sparkline buffer.
- The localization layer supports four languages, but no localized
  trend-direction strings exist today.

## Proposed Change

1. Add one secondary trend-direction cue to each of the five vital-sign tiles:
   heart rate, blood pressure, temperature, SpO2, and respiration rate.
2. Do not add a trend cue to the NEWS2 tile or the aggregate status banner in
   this issue.
3. Derive the cue from the same extracted value series already used by the
   tile sparkline. The GUI must not maintain a separate ad hoc trend ruleset.
4. Define four display states for the cue:
   - `Rising`
   - `Steady`
   - `Falling`
   - hidden because data is insufficient or unavailable
5. Require a minimum of four valid samples before any trend cue is shown.
   Rationale: `trend_direction()` compares two halves of a series; four points
   is the smallest defensible 2-vs-2 comparison and avoids presenting a
   directional claim from one or two points.
6. Apply parameter-specific validity rules:
   - Heart rate, temperature, and SpO2: all extracted points are valid samples.
   - Blood pressure: the MVP cue is systolic-only because the current data path
     is `trend_extract_sbp()`. The visible cue must say `SBP` or otherwise make
     the systolic-only scope explicit.
   - Respiration rate: exclude `0` sentinel values from trend eligibility. Hide
     the cue when the latest reading is not measured or when fewer than four
     non-zero RR samples exist in the current session window.
7. Keep the cue neutral in wording and styling:
   - text such as `Rising`, `Steady`, `Falling` is acceptable
   - optional neutral glyph accompaniment is acceptable
   - green, amber, and red remain reserved for alert severity only
   - the cue must not use wording such as `Improving`, `Worsening`, `Safe`, or
     `Danger`
8. Keep the existing alert-severity badge as the primary bottom-of-tile status
   element. If layout space is tight, split the bottom region into separate
   severity and trend sub-badges or slightly reduce sparkline height before
   reducing value readability.
9. Keep the cue read-only and display-only. It must not trigger alerts, modify
   thresholds, change persistence, or introduce any acknowledgement workflow.
10. Prefer implementing the cue semantics in a deterministic helper that is
    unit-testable outside the GUI event loop. The GUI should consume a simple
    display state rather than re-implementing minimum-sample and RR filtering
    rules inline.
11. Add localized strings for any user-visible trend words used in the badge.
    Preferred MVP behavior for unavailable data is to hide the cue rather than
    render a translated `No trend` placeholder.
12. If stakeholders later want a combined blood-pressure trend, that must be
    split into a follow-on issue with separate requirements and risk review.

## Files Expected To Change

Expected requirements and traceability files:

- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`

Expected implementation files:

- `include/trend.h`
- `src/trend.c`
- `include/localization.h`
- `src/localization.c`
- `src/gui_main.c`

Expected verification files:

- `tests/unit/test_trend.cpp`
- `tests/unit/test_localization.cpp`
- `dvt/DVT_Protocol.md`

Files expected not to change:

- `src/vitals.c`
- `src/alerts.c`
- `src/news2.c`
- `src/patient.c`
- `src/alarm_limits.c`
- authentication, persistence, or release pipeline files

## Requirements And Traceability Impact

- Preferred requirement update at system level:
  revise `SYS-014` so the graphical dashboard may present a secondary,
  direction-only trend cue for each vital tile when sufficient recent history
  exists, while preserving alert-severity colors as the primary safety signal.
- Existing software requirements likely to change:
  - `SWR-GUI-003` should define the neutral direction cue, its separation from
    the colored severity badge, and the requirement that the measured value and
    existing status remain primary.
  - `SWR-TRD-001` should define that the badge reuses the same extracted series
    as the sparkline, that the blood-pressure cue is systolic-only for the MVP,
    and that RR missing-data handling hides the cue instead of classifying a
    zero sentinel.
- Existing requirement parents are sufficient. No new clinical threshold,
  alerting, NEWS2, or persistence requirement is needed for this issue.
- `UNS-014` remains the primary user-facing parent; `UNS-009` is also relevant
  because the cue depends on session history rather than only the latest value.
- `requirements/TRACEABILITY.md` must map the revised dashboard/trend behavior
  to automated trend tests plus manual GUI evidence.
- Localization implementation will grow, but `SWR-GUI-012` does not need a new
  behavioral scope unless the team chooses to broaden localization acceptance
  criteria beyond string availability for the new badge text.

## Medical-Safety, Security, And Privacy Impact

Medical safety:

- No change to vital classification thresholds, alert generation, alarm limits,
  NEWS2 scoring, or aggregate severity logic.
- The main safety risk is interpretation: users may read direction as
  desirability or clinical urgency.
- Required design controls are:
  - minimum four-sample threshold before showing any cue
  - explicit systolic-only semantics for blood pressure
  - hidden RR cue when data is unavailable
  - neutral wording and neutral styling
  - preservation of the existing colored severity badge as the primary signal

Security:

- No new network, privilege, authentication, or authorization behavior.
- The cue reuses data already visible in the authenticated dashboard session.

Privacy:

- No new persistence, export, or external transmission path.
- The change stays within the current local session and existing patient-view
  boundary.

## AI/ML Impact Assessment

This change does not add, change, remove, or depend on an AI-enabled device
software function.

- Model purpose: not applicable
- Input data: deterministic local vital-sign history only
- Output: deterministic display-only direction cue
- Human-in-the-loop limits: unchanged
- Transparency needs: ordinary UI clarity only, not model explainability
- Dataset and bias considerations: not applicable
- Monitoring expectations: normal software verification and GUI review only
- PCCP impact: none

## Validation Plan

Implementation should add focused automated coverage for the badge semantics and
manual GUI evidence for the final presentation.

Automated validation scope:

- unit tests for the minimum-history rule:
  - fewer than four valid samples hides the cue
  - four or more valid samples can produce rising, steady, and falling states
- unit tests for blood-pressure semantics:
  - the display path uses the systolic series only for the MVP
- unit tests for respiration handling:
  - zero sentinel values do not count as valid RR trend samples
  - latest RR of `0` hides the cue even if older measured samples exist
- localization tests proving any new trend strings are available in all four
  supported languages

Manual GUI or DVT validation scope:

- known rising, falling, and steady scenarios for each supported vital tile
- blood-pressure tile presentation clearly indicates systolic-only direction
- respiration tile hides the cue when the displayed RR value is `--`
- alert-severity colors remain unchanged and are not reused for trend direction
- NEWS2 tile and aggregate banner remain unchanged
- tile layout remains readable at the current supported window size

Recommended validation commands:

```powershell
run_tests.bat
ctest --test-dir build --output-on-failure -R "Trend|Localization"
```

## Rollback Or Failure Handling

- If the cue cannot be added without crowding out the measured value, sparkline,
  or colored severity badge, keep the current sparkline-only design and split
  any broader tile redesign into a separate issue.
- If stakeholders do not accept systolic-only semantics for blood pressure, do
  not broaden this issue to invent a combined-BP trend. Split that work into a
  separate follow-on requirement and risk review.
- If RR missing-data handling cannot be made unambiguous in the implementation,
  hide the RR cue entirely rather than showing a misleading direction.
- If requirements wording starts implying prediction, deterioration detection,
  or clinical desirability, narrow it back to numeric direction only.
- Rollback is straightforward because the feature is additive and display-local:
  remove the trend cue helper, localization additions, and tile rendering
  changes, leaving the existing sparkline and severity badge behavior intact.
