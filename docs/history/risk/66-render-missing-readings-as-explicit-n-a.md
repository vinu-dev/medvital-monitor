# Risk Note: Issue #66

Date: 2026-05-06
Issue: `#66` - Feature: render missing readings as explicit N/A
Branch: `feature/66-render-missing-readings-as-explicit-n-a`

## proposed change

Approve a narrow display-policy change that renders intentionally missing
measurements as explicit `N/A` in the dashboard and patient summary instead of
showing `0`, `--`, or an omitted field. For this pilot, the safe MVP is limited
to absence states that are already defined by the product, especially
`respiration_rate == 0` meaning "not measured" and the existing device-mode
"no live data" state.

This issue must remain display-only. It must not change vital-sign
classification, aggregate alerting, NEWS2 scoring, alarm limits, data storage,
or any clinical threshold.

## medical-safety impact

The direct clinical logic impact should be none if the issue stays within the
display-only boundary. The medical-safety benefit is reduction of ambiguity:
showing a real-looking numeric zero or an unexplained blank can be misread as an
actual measured value, especially for respiration rate during review or handoff.

The main safety risk is scope drift. If design broadens this issue into a new
"missing" semantic for other vitals without an approved requirement-level
definition, the UI could hide truly abnormal or invalid data behind `N/A` and
weaken clinical review.

No AI/ML behavior is added, changed, removed, or depended on by this issue.

## security and privacy impact

No new authentication, authorization, persistence, export, network, or logging
path is expected. No patient-data scope expansion is proposed. Security impact
is low and privacy impact is none expected, provided the change remains local to
formatting and presentation code.

## affected requirements or "none"

No approved requirement currently states the exact text that must be used for an
intentionally missing in-session measurement. The nearest affected existing
requirements and evidence are:

- `SYS-011` patient status summary display
- `SYS-014` graphical vital signs dashboard
- `SYS-018` respiration-rate monitoring, including preserving the "not measured"
  state without spurious alert escalation
- `SYS-019` NEWS2 scoring must not be inflated by missing respiration rate
- `SWR-GUI-003` color-coded vital-sign display
- `SWR-PAT-006` patient summary display
- `SWR-VIT-008` respiration-rate check and `respiration_rate == 0` handling
- `SWR-NEW-001` NEWS2 handling when respiration rate is not measured
- `SWR-GUI-010` existing `N/A` display in device mode

If the design intends to introduce new "missing" semantics for any field beyond
already approved states, that is a separate requirements change and is not
covered by this note.

## intended use, user population, operating environment, and foreseeable misuse

Intended use remains bedside monitoring and review by trained clinical staff on
a Windows workstation. The relevant users are ward nurses, bedside clinicians,
and reviewers using the live dashboard and printed or console-style summary
output.

Foreseeable misuse includes:

- assuming `0` or blank means a valid measured value rather than an absent one
- assuming `N/A` means "clinically normal" rather than "not measured"
- expanding `N/A` rendering to fields whose missing-data semantics are not
  currently defined
- changing calculation behavior along with formatting under the false label of a
  display-only patch

## severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

- Severity: Moderate, because misreading a missing respiratory or optional value
  can contribute to delayed recognition, incorrect handoff interpretation, or
  mistaken trust in a nonexistent measurement.
- Probability: Occasional without a clear display control, because the current
  code path already uses `--` for missing RR in the live dashboard and omits RR
  from the summary.
- Initial risk: Medium.
- Risk controls:
  - Render approved missing states explicitly as `N/A` or an equivalent plain
    text label that does not look numeric.
  - Preserve all existing sentinel semantics and clinical calculations.
  - Do not reinterpret `0`, blank, or invalid values for other vitals as
    missing unless a requirement explicitly defines that state.
  - Keep alert badges and aggregate status derived from existing clinical logic,
    not from the display string.
  - Apply the rule consistently across the live tile and summary surfaces that
    already present the measurement.
- Verification method:
  - targeted unit tests for the formatting/helper path and summary output
  - regression tests proving RR=`0` still does not raise spurious alerts or
    NEWS2 points
  - manual GUI review with a sample where RR is intentionally not measured
  - diff review confirming no threshold, scoring, storage, or alarm workflow
    code changed
- Residual risk: Low if scope remains display-only and limited to already
  approved absence states.
- Residual-risk acceptability rationale: acceptable for this pilot because the
  change reduces ambiguity in clinician-facing presentation without changing the
  underlying clinical decision logic or data provenance.

## hazards and failure modes

- A clinician interprets `0` or blank as a true measurement and underestimates
  uncertainty in the latest reading set.
- A designer or implementer broadens the feature so that an invalid or critical
  value on another vital sign is masked as `N/A`.
- Dashboard and summary surfaces diverge, causing one view to show an explicit
  absence state while another omits the field or shows a numeric-looking token.
- Regression changes the RR sentinel handling and accidentally affects alerts,
  aggregate status, or NEWS2 scoring.
- Trend or history displays preserve raw zeros while the tile shows `N/A`,
  creating contradictory review evidence.

## existing controls

- `src/vitals.c` already treats `respiration_rate == 0` as "not measured" for
  aggregate alert-level calculation.
- `src/alerts.c` already suppresses RR alerts when `respiration_rate == 0`.
- `src/news2.c` already assigns zero NEWS2 RR points when respiration rate is
  not measured.
- `src/gui_main.c` already has explicit `N/A` rendering behavior for device mode
  and suppresses units when the rendered value is `N/A`.
- The issue body explicitly states that thresholds, NEWS2, alarm logic, and
  sensor integration are out of scope.

## required design controls

- Keep the MVP limited to established absence states already supported by code
  and requirements. The clearest in-scope case is RR not measured.
- Centralize missing-value formatting in a small helper or equally narrow
  presentation rule so dashboard and summary behavior stay consistent.
- Do not use the display rule to redefine storage semantics or to normalize
  invalid values into `N/A`.
- Leave trend extraction, alert generation, aggregate alerting, and NEWS2 logic
  unchanged unless a separate approved issue expands scope.
- If the team wants "other optional data" to support explicit missing rendering,
  define each field's missing-state semantics first and obtain requirements
  approval before implementation.

## validation expectations

- Verify the changed UI surfaces show explicit absence text for the approved
  missing state and do not append units to `N/A`.
- Verify patient summary output includes the affected measurement with an
  explicit absence marker instead of omitting it when that is the intended UX.
- Re-run the RR and NEWS2 regression checks so `respiration_rate == 0` still
  suppresses spurious alert escalation and extra NEWS2 score.
- Confirm that files outside presentation or summary formatting are unchanged,
  except for narrowly scoped tests if needed.
- Manually inspect the live dashboard with an RR-not-measured sample and confirm
  the badge/status colors still reflect existing clinical logic.

## residual risk for this pilot

Residual pilot risk is low. The feature is reasonable for design handoff
because it addresses a real human-factors ambiguity already visible in the code
paths, and it can be implemented without changing patient-facing clinical logic
if the boundary is respected.

## human owner decision needed, if any

No human clinical-threshold decision is needed for the narrow MVP.

A human requirements owner is needed only if the team wants to extend explicit
missing-value semantics beyond already approved states such as RR not measured
or device-mode no-data presentation.

## product hypothesis and intended user benefit

Hypothesis: clinicians and reviewers will make fewer interpretation mistakes if
an absent measurement is displayed as an explicit absence marker rather than a
numeric-looking placeholder or a missing line item.

Intended user benefit: faster and safer review of the latest reading set during
live monitoring, handoff, and summary review, especially when respiration rate
is optional or intentionally not captured for a cycle.

## source evidence quality

Evidence quality is moderate for a narrow product decision, not for a clinical
effect claim.

- Internal evidence is strong that the product already distinguishes "not
  measured" in logic: RR=`0` is excluded from aggregate alerting and NEWS2
  scoring, and device mode already renders `N/A` in the dashboard.
- External evidence is limited but credible for UX direction: the Philips
  EarlyVue VS30 IFU (Revision B, January 2024), cited in the issue and reviewed
  on 2026-05-06, distinguishes missing required measurements and separately
  labels manual respiration input. That supports the general need to make
  measurement absence explicit, but it does not justify copying Philips wording,
  layout, or workflow.
- No user study, clinical outcomes evidence, or multi-vendor comparison is
  provided, so this should remain an MVP display clarification rather than a
  broader workflow claim.

## MVP boundary that avoids copying proprietary UX

Use a plain repository-native absence indicator such as `N/A` in the affected
surfaces. Do not copy competitor pane labels, warning dialogs, menu structure,
icons, color semantics, or proprietary wording such as product-specific
advisory names.

The MVP should be a local formatting rule only, not a redesign of monitor
workflow or a clone of another vendor's review experience.

## clinical-safety boundary and claims that must not be made

Do not claim that this feature improves physiological measurement accuracy,
changes alert sensitivity, reduces alarms, improves NEWS2 validity, or
introduces support for new optional vital-sign fields.

The only defensible claim for this pilot is that the UI more explicitly shows an
already-known absence state to reduce presentation ambiguity.

## whether the candidate is safe to send to Architect

Yes, with constraints. This candidate is safe to send to Architect if the
design is explicitly limited to display-only handling of already-approved
absence states, preserves all existing RR and NEWS2 behavior, and does not
expand missing-data semantics to other vitals without a separate approved
requirements decision.
