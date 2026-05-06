# Risk Note: Issue #51

Date: 2026-05-06
Issue: `#51` - Feature: add baseline markers to vital trend sparklines
Branch: `feature/51-add-baseline-markers-to-vital-trend-sparklines`

## proposed change

Add a visual reference overlay to each vital-sign sparkline so a clinician can
see whether the current trace is above or below a clearly defined comparison
value at a glance.

This item is safe to advance only as a presentation-layer change. It must not
change measurement capture, history storage, trend extraction, alert
classification, NEWS2 scoring, or alarm-limit behavior. The design must use an
already approved data source already present in application state; it must not
invent a new patient-specific baseline in this issue.

## product hypothesis and intended user benefit

The product hypothesis is that a lightweight comparison cue inside the existing
sparkline area will reduce the time needed for a trained clinician to orient to
deviation during dashboard review.

The intended benefit is faster visual interpretation of already displayed data,
not new clinical decision support and not a replacement for the current
numeric values, status tiles, or alert banner.

## source evidence quality

The issue cites one public Philips Horizon Trends product page as source
evidence. That source is acceptable as product-discovery evidence that
deviation-to-reference visual patterns exist in patient-monitoring interfaces.

Evidence quality is limited:

- it is single-vendor marketing material
- it is not neutral usability research
- it is not clinical validation evidence
- it is not sufficient support for outcome claims or close UX copying

It should be used only to justify a narrow workflow pattern, not to justify
clinical benefit claims.

## MVP boundary that avoids copying proprietary UX

The MVP should remain a generic thin reference marker or bounded reference cue
inside the existing tile sparkline strip. It should not reproduce Philips
split-screen layouts, branded terminology, multi-horizon trend controls,
proprietary visual hierarchy, or other vendor-specific presentation patterns.

The repo already has its own tile layout, colors, and sparkline renderer. The
design should stay within that existing visual language.

## clinical-safety boundary and claims that must not be made

The marker must not be presented as:

- a diagnosis aid
- a treatment recommendation
- a clinician-approved target unless that target already exists in approved app state
- a new alarm threshold
- a deterioration prediction

The change must not claim improved outcomes, reduced alarm fatigue, or better
clinical detection without separate human review and evidence.

## whether the candidate is safe to send to Architect

Yes, with constraints.

It is safe to send to Architect if the design stays presentation-only and the
spec defines the exact reference semantics before implementation. If the design
needs a new patient-specific baseline, a new configurable target, or any new
clinical claim, it must come back for human product/clinical approval instead
of proceeding under this issue.

## medical-safety impact

Direct medical-safety impact is low but not zero. The feature changes emphasis
and glance interpretation, not the underlying measured values or alert logic.

The main safety risk is semantic confusion: a clinician may misread the marker
as a treatment target, alarm boundary, or personalized baseline when it is
actually something else. That confusion could bias attention even though the
numeric values and alert state remain unchanged.

## security and privacy impact

No new patient-data category, authentication path, role behavior, export path,
or external transmission is needed for the proposed scope.

Privacy impact is none expected if the implementation reuses in-memory values
only and does not add logging or persistence. Security impact is low. Scope
must not expand into new configurable target storage under this issue.

## affected requirements or "none"

- `SYS-014` Graphical Vital Signs Dashboard
- `SWR-GUI-003` Colour-Coded Vital Signs Display
- `SWR-TRD-001` Trend Sparkline and Direction Detection
- `SWR-ALM-001` only if the design explicitly reuses existing
  `g_app.alarm_limits` values as the visible reference source

No change is justified to alert-generation logic, NEWS2 logic, authentication,
patient-record persistence, or HAL behavior.

## intended use, user population, operating environment, and foreseeable misuse

Intended use:

- support trained clinicians reviewing the active patient's trend visuals on the
  existing dashboard

User population:

- bedside nurses
- ward clinicians
- intensivists
- admin or clinical users who already operate the dashboard

Operating environment:

- Windows workstation running the current dashboard in clinical demonstration or
  pilot monitoring workflow

Foreseeable misuse:

- treating the marker as a patient-specific safe target when it is not
- treating the marker as an alarm threshold when the actual alert state is unchanged
- assuming no marker means normal
- over-trusting marker position instead of reading the numeric value and badge
- showing one unlabeled marker for a parameter that really has two relevant bounds

## severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

Severity:

- low to medium, because misinterpretation could delay or distort clinician
  attention even without changing the underlying measurement or alert engine

Probability:

- medium without controls, because the issue text allows "baseline or target"
  wording while the current product has no single approved baseline concept

Initial risk:

- medium

Risk controls:

- keep the change visual-only; do not alter `check_*()`,
  `overall_alert_level()`, `generate_alerts()`, `news2_calculate()`,
  acquisition timing, or history storage
- define the exact reference source per parameter in the design spec before
  implementation; do not infer a new baseline from patient history
- do not add a new patient-specific target or configurable baseline in this issue
- if existing alarm-limit values are reused, label them explicitly as alarm or
  reference bounds and handle two-sided parameters truthfully
- keep current numeric values, badges, and alert colors visually dominant over
  the reference overlay
- suppress the overlay when no valid reference source exists

Verification method:

- targeted unit tests for any helper that maps the approved reference source to
  overlay coordinates, including "no reference" and two-bound handling paths
- manual GUI review in simulation and manual-entry flows to confirm the overlay
  appears correctly and does not change alert state, NEWS2 output, or device/sim behavior
- changed-file review to confirm the patch stays in presentation code plus any
  narrowly scoped helper/tests

Residual risk:

- low if the above controls are followed

Residual-risk acceptability rationale:

- acceptable for this pilot because the feature is bounded to passive visual
  emphasis, existing approved numeric and alert outputs remain primary, and any
  design that introduces new clinical semantics is explicitly routed back to a
  human owner

## hazards and failure modes

- the marker is interpreted as a clinician-approved target or alarm threshold
- a two-sided physiological range is collapsed into one ambiguous marker
- marker scaling exaggerates or understates the apparent deviation
- the overlay obscures the sparkline, badge, or current value
- stale or missing reference data is rendered as if valid
- future contributors extend the feature into clinical-decision logic without
  separate approval

## existing controls

- the issue body explicitly constrains scope to presentation-only change
- current dashboard status colors still come from approved validation logic
- current sparkline rendering is derived from existing patient-history values only
- current application state already contains configured alarm-limit data if the
  design chooses to reuse that approved source
- no new storage is required because history is already bounded by `MAX_READINGS`

## required design controls

- the architect must define the exact meaning of the reference cue for each
  parameter before implementation starts
- if truthful semantics require two bounds for a parameter, show that
  explicitly or exclude that parameter from the first MVP rather than inventing
  one hidden target
- terminology must avoid implying diagnosis, treatment recommendation, or
  patient-specific optimization
- the overlay must be visually secondary to the measured value and alert badge
- no new settings, persistence, exports, or analytics are in scope
- PR text and test notes must state that clinical logic is unchanged

## validation expectations

- confirm the chosen reference source already exists in approved app state and
  does not require new persistence or new user input
- confirm diffs do not change `src/vitals.c`, `src/alerts.c`, `src/news2.c`,
  `src/patient.c`, `src/sim_vitals.c`, requirements files, or release artifacts
  unless a separate approval is obtained
- run the issue's suggested build/test commands
- perform manual GUI smoke review for normal, warning, critical, no-patient,
  simulation-off, and RR-not-measured conditions
- explicitly confirm that alert levels, NEWS2 score, and banner behavior are unchanged

## residual risk for this pilot

Low after the above controls.

The remaining risk is user misreading a passive visual cue. That residual risk
is acceptable for this pilot only if the feature remains subordinate to the
approved numeric and alert outputs and does not create a new baseline concept.

## human owner decision needed, if any

Human owner decision is needed if the team wants any of the following:

- a patient-specific baseline derived from prior readings
- a new user-configurable clinical target not already represented in current app state
- one unlabeled marker for a parameter that requires two clinically meaningful bounds
- claims of improved clinical performance or safety outcome
