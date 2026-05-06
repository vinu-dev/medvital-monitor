# Risk Note: Issue #48

Date: 2026-05-06
Issue: `#48` - Feature: show live trend direction badges on vital tiles
Branch: `feature/48-show-live-trend-direction-badges-on-vital-tiles`

## proposed change

Add a compact, display-only trend badge to the existing live vital tiles for
heart rate, blood pressure, temperature, SpO2, and respiration rate. The badge
should reuse the existing session-scoped trend extraction and
`trend_direction()` helper already present in `src/trend.c`, and it should be
hidden when the current value is unavailable or the history is below an
explicitly approved minimum sample count.

The change should remain presentation-layer only:

- no acquisition or HAL behavior change
- no threshold, alerting, or NEWS2 scoring change
- no new persistence, export, or telemetry path
- no AI/ML model added, changed, or depended on
- no change to the NEWS2 tile unless separately approved

## product hypothesis and intended user benefit

The product hypothesis is credible and narrow: an operator can understand tile
direction faster if the existing sparkline is paired with a simple rising,
falling, or stable cue. The intended user benefit is faster at-a-glance review
during demo, handoff, and deterioration-observation scenarios without requiring
the user to parse the full sparkline every time.

## source evidence quality

Source evidence quality is low-to-moderate for product discovery and low for
medical-safety claims.

- The Philips Horizon Trends marketing page shows that commercial monitors use
  at-a-glance trend presentation and describes short-window trend indicators and
  graphical trend displays.
- The linked Philips IFU is vendor-authored product documentation for a Philips
  monitoring system, but it is not independent clinical evidence and is not a
  validated usability study for this repository's UI.
- The supplied sources establish that trend cues are a common product pattern.
  They do not establish that this specific badge design improves outcomes,
  reduces response time, or is clinically validated for this product.

This is enough to justify a non-copying MVP exploration, but not enough to make
clinical-effectiveness, safety-performance, or regulatory-equivalence claims.

## MVP boundary that avoids copying proprietary UX

- Use a generic badge derived from the repository's existing
  `trend_direction()` output.
- Do not copy Philips deviation bars, split-screen layout, configurable trend
  periods, baseline-target visuals, or proprietary arrow styling.
- Keep the feature session-scoped and bounded by the existing `MAX_READINGS`
  history.
- Do not add a new trends screen, export flow, or clinician-configurable trend
  window in this issue.
- Keep the NEWS2 tile unchanged unless a separate requirement is approved.

## clinical-safety boundary and claims that must not be made

- The badge is supplementary context only. It is not an alarm, diagnosis,
  treatment recommendation, or predictor of deterioration.
- The badge must not be described as clinically validated or as improving
  detection accuracy without separate human-factors and validation evidence.
- The badge must not override or visually compete with the primary safety cues:
  numeric value, tile severity color, alert banner, and alerts list.
- Absence of a badge must mean trend data is unavailable or insufficient, not
  "the patient is stable," unless that behavior is explicitly approved and
  explained in requirements.

## medical-safety impact

Medical-safety impact is indirect but non-zero because the feature changes the
presentation of clinically relevant data on the live monitoring dashboard.

The main hazard is interpretation bias: an incorrect or overstated badge could
cause an operator to underweight the absolute severity of the current value or
to infer a meaningful direction from too little history. That could delay
attention to a worsening parameter even though the underlying thresholds and
alerts have not changed.

Overall medical-safety impact is low-to-medium if the feature stays
display-only, uses the existing deterministic trend helper, and enforces clear
suppression rules for missing or insufficient data. It becomes materially
higher if the badge is allowed to act like an alarming or predictive cue.

## security and privacy impact

No new patient-data class, access-control path, persistence location, or
external interface is needed for the intended MVP. Security and privacy impact
is therefore none expected if implementation remains in `src/gui_main.c` and
reuses existing in-memory patient history only.

Security/privacy risk increases only if the design expands into logging,
telemetry, export, or remote transmission of trend annotations. That expansion
should remain out of scope for this issue.

## affected requirements or "none"

- `UNS-009` Vital Sign History
- `UNS-010` Consolidated Status Summary
- `UNS-014` Graphical User Interface
- `SYS-009` Vital Sign Reading History
- `SYS-014` Graphical Vital Signs Dashboard
- `SWR-GUI-003` Colour-Coded Vital Signs Display
- `SWR-TRD-001` Trend Sparkline and Direction Detection
- likely a new or revised GUI-level requirement is needed for badge states,
  suppression rules, and explicit exclusion of NEWS2
- `SWR-NEW-001` / `SYS-019` should remain unchanged because the issue body
  explicitly keeps the NEWS2 tile out of scope

## intended use, user population, operating environment, and foreseeable misuse

Intended use: provide a secondary visual cue about recent direction of already
displayed vital signs inside the current live dashboard.

User population: trained clinical staff or pilot users viewing the dashboard on
a Windows workstation in the bedside, ward, handoff, or demo context described
by the repository documents.

Operating environment: the same session-scoped, bounded-history monitoring view
already used by the Win32 dashboard, including simulation mode and future HAL
back-ends that feed the same `VitalSigns` structure.

Foreseeable misuse:

- treating a trend badge as equivalent to alarm severity
- assuming "stable" means clinically safe or no action needed
- assuming no badge means stable instead of "not enough data"
- inferring a validated deterioration prediction from a small visual cue
- expecting NEWS2 or aggregate scores to carry the same trend meaning

## severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

Severity: Moderate. A misleading cue could contribute to delayed or
mis-prioritized attention to a worsening vital sign, but it remains secondary to
existing numeric and alerting displays.

Probability: Low to medium without controls because small-sample and
missing-value cases are common in session-scoped history. Low with controls
because the helper is deterministic and already unit-tested.

Initial risk: Medium.

Risk controls:

- define explicit badge states (`rising`, `falling`, `stable`, `hidden`)
- hide the badge until an approved minimum history count is met
- hide the badge when the current value is unavailable, in device mode, after a
  cleared session, or when the parameter is not measured
- derive the badge only from the existing `trend_direction()` helper and the
  same extracted history used for the sparkline
- keep the badge visually subordinate to numeric values and severity colors
- keep the NEWS2 tile unchanged
- document that badge direction is not equivalent to alarm severity or clinical
  recommendation

Verification method:

- targeted unit coverage for badge-state mapping and suppression conditions
- regression checks that alert thresholds, alert generation, NEWS2 scoring, and
  HAL behavior are unchanged
- GUI verification that the badge and sparkline agree for the same history and
  that no badge appears on the NEWS2 tile
- visual review of warning and critical tiles to confirm the badge does not
  obscure or outrank the existing primary safety cues

Residual risk: Low if all listed controls are applied.

Residual-risk acceptability rationale: acceptable for this pilot only if the
feature remains a small supplementary cue and does not create new clinical
claims. Primary validated safety signals remain the current numeric value,
severity color mapping, alert banner, and alert logic.

## hazards and failure modes

- A badge is shown from too little history and is interpreted as a meaningful
  clinical trend.
- A badge is shown when the current parameter is unavailable (`N/A`, no patient,
  device mode, session cleared, or respiration rate not measured).
- The badge direction disagrees with the sparkline because a different history
  path, scaling rule, or parameter subset is used.
- A badge is added to the NEWS2 tile, implying approved trend semantics for an
  aggregate score that currently has no such requirement.
- Badge styling becomes more visually salient than the current value or the
  warning/critical state.
- Users interpret "stable" as "normal" or "no action required."

## existing controls

- `src/trend.c` already implements deterministic `trend_direction()` behavior
  with hysteresis and fixed-memory extract helpers.
- `tests/unit/test_trend.cpp` already verifies rising, falling, stable, and
  extraction behavior for the current helper.
- `src/gui_main.c` already paints sparklines from the same bounded patient
  history, and primary tile severity remains tied to validated alert
  classification functions.
- The issue body explicitly keeps acquisition, thresholds, alerting, and NEWS2
  behavior out of scope.
- The repository already bounds history with `MAX_READINGS`, reducing state
  complexity and limiting stale-history exposure.

## required design controls

- Add or revise a GUI requirement so badge presence, meaning, and suppression
  rules are explicit and testable.
- Keep implementation in the presentation layer and reuse the existing
  `trend_direction()` helper rather than adding a second trend heuristic.
- Ensure the badge uses the same per-parameter history as the sparkline so both
  cues remain internally consistent.
- Do not show a badge for unavailable data, insufficient history, or aggregate
  scores such as NEWS2.
- Keep badge visual hierarchy secondary to the numeric value and severity state.
- If color is used, do not overload alert-severity colors with trend meaning in
  a conflicting way.
- Treat configurable windows, baseline comparisons, long-horizon trend views,
  and any clinical-performance claims as separate future work.

## validation expectations

- Add tests for badge rendering state from known rising, falling, and stable
  histories already represented in `tests/unit/test_trend.cpp`.
- Add tests or targeted review evidence for hidden-badge conditions:
  insufficient history, missing current value, device mode, and `respiration_rate == 0`.
- Confirm no badge is rendered for the NEWS2 tile.
- Confirm the change does not modify threshold classification, alert
  generation, NEWS2 calculation, or persistence behavior.
- Perform GUI verification on normal, warning, and critical examples to ensure
  the badge does not reduce legibility of existing primary data.

## residual risk for this pilot

Residual pilot risk is low if the feature remains a constrained display-only
cue with the controls above. Residual risk becomes medium if the team broadens
the feature into a stronger clinical signal without separate validation.

## human owner decision needed, if any

No blocking human-owner decision is needed before Architect work if the issue
remains within the display-only MVP boundary above.

Human-owner approval is required before implementation if any of the following
are proposed:

- badge behavior on the NEWS2 tile or any other aggregate score
- configurable time windows or baseline/deviation views
- use of badge state as an alarm/escalation signal
- clinical or performance claims beyond "secondary visual cue"

## whether the candidate is safe to send to Architect

Yes. This candidate is safe to send to Architect if the design keeps the badge
supplementary, deterministic, hidden on missing/insufficient data, and clearly
separated from alarm severity and NEWS2 semantics.
