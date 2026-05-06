# Risk Note: Issue #65

Date: 2026-05-06
Issue: `#65` - Feature: show current alert duration beside the status pill
Branch: `feature/65-show-current-alert-duration-beside-the-status-pill`

## proposed change

Add a display-only duration cue beside the existing aggregate status pill so the
dashboard can show how long the current abnormal aggregate state has been
active. The intended scope is limited to presentation of the existing
`patient_current_status()` result and its state-transition timing. The change
must not alter alert thresholds, aggregate severity rules, NEWS2 scoring,
audible behavior, acknowledgement workflow, or patient-record persistence.

## product hypothesis and intended user benefit

The product hypothesis is that operators can triage the current dashboard more
quickly if the aggregate status area answers two questions at once: severity
(`WARNING` or `CRITICAL`) and freshness (`for 00:40` versus `for 12:15`). The
expected benefit is faster scan interpretation during monitoring review and
handoff, especially when a persistent abnormal state is clinically different
from a newly emerged one.

This is a usability hypothesis, not a clinical-effectiveness claim. The issue
supports a bounded MVP because it reuses an already computed aggregate state and
adds no new physiological inference.

## source evidence quality

Source evidence quality is moderate for UX precedent and low for clinical
justification.

- The cited Philips Patient Information Center iX public IFU is a primary
  vendor source showing that review-oriented alarm-duration context exists in a
  commercial monitoring ecosystem. It is useful to justify that the concept is
  not novel or obviously out of family for monitoring UI.
- The cited source does not prove improved outcomes, reduced response time, or
  safe operator behavior for this repository's dashboard. It should not be used
  as clinical evidence.
- No comparative usability study, human-factors validation, or alarm-fatigue
  evidence was provided in the issue. Design should therefore stay conservative
  and avoid overstating benefit.

## medical-safety impact

The feature is not clinically silent even though it is display-only. A duration
badge can influence human urgency assessment, handoff prioritization, and
interpretation of whether a condition is new, persistent, or worsening.

The main safety risk is not threshold corruption; it is misinterpretation. If
the displayed duration is stale, fails to reset on an aggregate-state change,
continues during a paused feed, or visually dominates the existing severity
cue, an operator could under-triage a fresh critical change or over-trust the
age indicator as if it were a proven measure of physiological onset.

Because the existing dashboard already shows colour, current aggregate status,
raw vital values, and active alert text, the duration cue is secondary. That
keeps the overall safety impact bounded, but it still needs explicit design and
verification controls.

## security and privacy impact

No new patient-data category, credential flow, network path, or privilege
boundary is required for this feature if it remains local to `gui_main.c` and
uses existing in-memory patient state.

Privacy impact is none expected. Security impact is low and limited to display
integrity: the UI must not present misleading elapsed time because operators may
rely on it operationally. The feature should not create new logging, export,
persistence, or telemetry behavior without separate review.

## affected requirements or "none"

Likely affected existing requirements if the feature proceeds:

- `SYS-011` Patient Status Summary Display
- `SYS-014` Graphical Vital Signs Dashboard
- `SWR-GUI-003` Colour-Coded Vital Signs Display
- `SWR-PAT-004` Current Patient Status

No change should alter:

- `SYS-001` through `SYS-006` threshold and aggregate-alert logic
- `SYS-019` NEWS2 aggregate clinical risk scoring
- `SWR-VIT-*`, `SWR-NEW-001`, or `SWR-ALT-*` clinical classification behavior

The current approved requirements do not explicitly specify an alert-duration
display. Architect should either map this as a narrow extension to dashboard
display behavior or propose a tightly scoped requirement update without
rewriting clinical logic.

## intended use, user population, operating environment, and foreseeable misuse

- Intended use: give trained clinical users a concise age indicator for the
  currently active abnormal aggregate dashboard state.
- User population: trained clinicians or operators already using the dashboard
  for bedside-style monitoring review.
- Operating environment: Windows workstation dashboard in live simulation today
  and a future real-monitoring environment through the same aggregate-state
  display path.
- Foreseeable misuse: a user may treat the badge as time since physiological
  onset rather than time since the software entered the current aggregate state.
- Foreseeable misuse: a user may interpret the duration as acknowledgement,
  escalation, or muting state even though the product does not implement those
  workflows.
- Foreseeable misuse: if the timer keeps running during paused simulation,
  cleared session, patient refresh, or device mode, a user may assume the
  monitor continued actively observing the patient during that period.

## severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

- Severity: Moderate. A misleading duration could contribute to delayed or
  mis-prioritized response, but it does not by itself suppress the existing
  alert severity, raw values, or active-alert list.
- Probability: Low to medium before controls because elapsed-time features are
  prone to reset, rollover, and state-transition defects.
- Initial risk: Medium.
- Risk controls:
  - Derive the timer from aggregate-state transitions only, not from any single
    underlying vital sign.
  - Reset the timer whenever the aggregate level changes, the patient/session
    is cleared, the user logs out, or monitoring mode no longer represents
    active observation.
  - Show the badge only for abnormal states, not for `NORMAL`.
  - Keep the duration visually subordinate to the severity pill and current
    numeric values.
  - Label it as elapsed time in the current alert state, not as onset time or
    treatment timer.
- Verification method:
  - Transition tests or targeted verification for `NORMAL -> WARNING`,
    `WARNING -> CRITICAL`, `CRITICAL -> WARNING`, and abnormal `-> NORMAL`
    resets.
  - Manual GUI review confirming the badge never obscures the severity cue and
    behaves correctly during pause, resume, clear session, admit/refresh, and
    simulation-mode changes.
  - Diff review confirming no threshold or alerting logic changed outside the
    intended presentation/state-tracking scope.
- Residual risk: Low if the above controls are implemented and verified.
- Residual-risk acceptability rationale: acceptable for this pilot because the
  cue remains advisory, redundant with stronger existing alert indicators, and
  bounded away from thresholding or treatment logic.

## hazards and failure modes

- The timer does not reset when the aggregate level changes from `WARNING` to
  `CRITICAL`, causing a fresh critical escalation to look old and potentially
  less urgent.
- The timer resets on every new reading even while the aggregate level remains
  unchanged, making persistent deterioration appear fresh.
- The timer tracks one parameter rather than the aggregate status and diverges
  from the displayed severity pill.
- The timer continues through paused simulation, cleared session, logout, or
  device mode and implies monitoring continuity that did not exist.
- The badge wording or styling causes users to read it as alarm-acknowledgement
  age, escalation SLA, or clinically validated onset time.
- The implementation expands into acknowledgement, muting, history export, or
  retrospective alarm analytics without additional requirements and review.

## existing controls

- The current product already computes a single aggregate status from the
  latest reading via `patient_current_status()` and `overall_alert_level()`.
- The issue explicitly constrains scope to display-only behavior and excludes
  threshold changes, suppression, and acknowledgement workflow.
- Existing dashboard cues already include colour, status text, raw vital
  values, and active-alert messages, reducing dependence on the new duration
  badge as a sole signal.
- The current timer-driven dashboard update loop provides a natural place to
  evaluate aggregate-state transitions without adding network or persistence
  complexity.

## required design controls

- Define one clear semantics statement in design: the badge shows elapsed time
  since the software entered the current abnormal aggregate state.
- Track entered-state time in presentation state only; do not backfill from
  historical records or infer physiological onset.
- Reset or clear the badge on every aggregate-level transition, on return to
  `NORMAL`, on patient/session reset, on logout, and when live monitoring is
  not active.
- Keep the existing severity pill as the primary cue. The duration must not
  replace colour, level text, or the active-alert list.
- Use a simple bounded format such as `MM:SS` or `HH:MM:SS`; avoid ambiguous
  wording like "alarm time" unless requirements explicitly define it.
- Do not add acknowledgement count, alarm muting, trend interpretation, or
  escalation claims as part of this MVP.
- If design discovers the need for audit logging, persisted alarm history, or
  cross-session duration continuity, split that into a separate issue and risk
  review.

## MVP boundary that avoids copying proprietary UX

- Reuse only the general idea of showing elapsed abnormal-state context.
- Do not copy vendor terminology, layout, iconography, configuration concepts,
  or workflow terms from the Philips IFU.
- Keep the MVP local to the existing aggregate status area and current session.
- Exclude alarm-advisor logic, acknowledge counters, historical review screens,
  or configurable duration-trigger behavior.

## clinical-safety boundary and claims that must not be made

- Do not claim the badge measures time since clinical onset, deterioration
  onset, or treatment delay.
- Do not claim the feature improves outcomes, reduces alarm fatigue, or supports
  diagnosis without separate human-factors and clinical evidence.
- Do not alter or imply changes to alert thresholds, NEWS2, or escalation rules.
- Do not let the UI suggest that an old `CRITICAL` state is less urgent than a
  new one; the severity tier remains primary.

## validation expectations

- Verify the duration source of truth is the aggregate dashboard state, not an
  individual alert row or a single vital-sign threshold path.
- Exercise manual and automated transition coverage for steady abnormal state,
  escalation, de-escalation, return to normal, pause/resume, and patient reset.
- Confirm the badge is absent in `NORMAL` and does not persist into device mode
  or other non-monitoring states.
- Review the changed file list to ensure the patch stays within presentation,
  state-tracking, and narrowly related tests or documentation.
- Confirm requirements updates, if any, remain display-scope only and do not
  silently rewrite clinical behavior.

## residual risk for this pilot

Residual risk is low if the feature remains a bounded presentation-layer cue
with explicit reset semantics and validation of state transitions. Residual risk
increases to medium if the design leaves duration semantics ambiguous or allows
the timer to imply continuous observation when the feed is paused or reset.

## whether the candidate is safe to send to Architect

Yes, with the controls above. The issue is sufficiently bounded, the product
hypothesis is plausible, and the safety boundary is clear enough for Architect
to design a display-only solution without changing clinical classification
logic.

## human owner decision needed, if any

No blocking human decision is required to send this to Architect as long as the
scope stays display-only.

Human product review is still needed if the team wants any of the following:

- persisted alarm-duration history across sessions
- acknowledgement, mute, or escalation workflow
- claims about clinical benefit or response-time improvement
