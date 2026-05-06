# Risk Note: Issue #64

Date: 2026-05-06
Issue: `#64` - Feature: add an alarm-audio status badge
Branch: `feature/64-alarm-audio-status-badge`

## proposed change

Add a small, persistent dashboard badge that reports the current local
alarm-audio presentation state as an informational UI element. The intended MVP
is read-only operator feedback only. It must not change alarm thresholds, alert
classification, escalation, alarm routing, or clinician workflow. The current
GUI already uses header/status badges for role and simulation state, so this
candidate fits the existing presentation pattern.

## product hypothesis and intended user benefit

The product hypothesis is reasonable: bedside users benefit from immediate
confirmation of whether this workstation is expected to emit local alarm audio.
That can reduce uncertainty during handoff, troubleshooting, or simulation
review, especially when the dashboard already encodes multiple states visually.

The intended user benefit is situational awareness only. This feature does not
improve diagnosis, treatment selection, or patient prioritization by itself.

## source evidence quality

Evidence quality is moderate for product discovery and low for clinical claims.

- Repo evidence is strong that this product already depends on dense visual
  status signaling in the GUI (`README.md`, `src/gui_main.c`, `SYS-014`,
  `SWR-GUI-003`, `SWR-GUI-010`).
- The issue cites a Philips IntelliVue MX750 product page, accessed
  2026-05-06, which shows alarm-management and remote-alarm presentation are
  real market concerns, but it is vendor marketing material rather than
  independent clinical or usability evidence:
  `https://www.usa.philips.com/healthcare/product/HC866471/intellivue-mx750-bedside-patient-monitor`
- That source is sufficient to justify "this is a standard monitor concern"
  but not sufficient to justify copying wording, layout, iconography, or
  workflow from a competitor.

## MVP boundary that avoids copying proprietary UX

The MVP should be a generic text badge within the existing header/status visual
language of this application. It should not copy competitor placement,
terminology, icons, color semantics, or remote-display workflow.

The badge should report only states that this software can determine from an
authoritative local source of truth. If the system only knows "local audio
enabled" versus "local audio silenced", the MVP should stop there. It should
not imply routed, transferred, acknowledged, or guaranteed alarm delivery
unless those states exist explicitly in the product architecture.

## medical-safety impact

Direct functional impact is low if the change remains informational and
read-only. Indirect safety impact is not zero: an inaccurate, stale, or
ambiguous audio-state badge could create false reassurance during an active
alarm condition and contribute to delayed clinician response.

This candidate is acceptable for design only if the badge is clearly secondary
to the existing clinical alert indicators and fails safe when the true audio
state is unknown.

## clinical-safety boundary and claims that must not be made

The design must not:

- claim that no alarm condition exists when audio is silenced
- claim that alarms are being heard elsewhere unless remote routing is a real,
  verified product state
- claim compliance with IEC 60601-1-8 alarm-delivery behavior on the strength
  of a UI badge alone
- alter alarm thresholds, alert generation, NEWS2 behavior, or escalation logic
- become the primary alarm indication instead of the existing clinical tiles,
  alerts, and status banner

## security and privacy impact

No new patient-data flow is needed for this feature. If implemented as a local
read-only state indicator, privacy impact is none expected.

Security impact is low, with two constraints:

- do not expose new network, remote-control, or persistence paths just to drive
  the badge
- do not allow the badge itself to change alarm state; display only

## affected requirements or "none"

No currently approved requirement explicitly covers an alarm-audio-state badge.
Adjacent requirements exist, but they are not sufficient authorization on their
own:

- `UNS-014` graphical dashboard
- `SYS-014` graphical vital signs dashboard
- `SWR-GUI-003` colour-coded dashboard display
- `SWR-GUI-010` simulation/status indicator behavior

Before implementation, the team should add a new requirement that defines:

- the authoritative source of alarm-audio state
- allowed displayed states and wording
- fail-safe behavior when state is unavailable or indeterminate
- verification expectations for state transitions

## intended use, user population, operating environment, and foreseeable misuse

- Intended use: help trained clinical users understand whether this workstation
  is expected to emit local alarm audio while monitoring a patient.
- User population: trained bedside clinicians and administrators using the
  Windows dashboard in the pilot environment.
- Operating environment: local patient-monitoring workstation GUI with live
  simulation today and future real-hardware integration via the HAL.
- Foreseeable misuse: users may interpret "Silenced" as "no active alarm",
  interpret local silence as remote alarm coverage, or trust a stale badge after
  a mode switch, pause/resume event, or audio-state fault.

## severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

- Severity: potentially serious if a wrong or misleading badge contributes to a
  delayed response to a clinically significant alarm.
- Probability: low to medium before controls, because presentation-state drift
  and ambiguous wording are plausible UI failure modes.
- Initial risk: medium for a medical-monitor UI if the badge is added without a
  precise state model.
- Risk controls:
  - bind the badge only to an authoritative alarm-audio state source
  - make the badge read-only and visually distinct from patient-severity colors
  - use explicit wording such as local audio state, not icon-only semantics
  - show `Unknown` or `Unavailable` rather than guessing when state cannot be
    determined
  - keep existing clinical alert indicators primary
  - do not overload simulation pause, device mode, or alarm mute into one
    ambiguous label
- Verification method:
  - requirement review for state definitions and wording
  - targeted unit tests if state-mapping logic is introduced outside paint code
  - manual GUI verification of every supported state transition
  - regression check that thresholds, alert logic, and mute behavior do not
    change
- Residual risk: low if the feature is constrained to verified read-only state
  presentation with explicit unknown-state handling.
- Residual-risk acceptability rationale: acceptable for this pilot because the
  feature is non-controlling, adds operator clarity, and can be bounded so that
  uncertainty defaults to non-assertion rather than false reassurance.

## hazards and failure modes

- Badge shows `Audible` while local audio is actually silenced or unavailable.
- Badge shows `Silenced` and a user infers there is no active alarm condition.
- Badge conflates local silence with remote routing or central-station coverage.
- Badge state is stale after login, logout, simulation toggle, pause/resume, or
  alarm configuration changes.
- Badge uses the same red/amber/green meaning as patient severity and is read
  as a clinical status signal instead of an audio-state signal.
- Badge becomes clickable or otherwise changes alarm behavior, expanding scope
  beyond the issue.

## existing controls

- The issue explicitly limits scope to informational-only behavior and excludes
  alarm-threshold and workflow changes.
- Existing alert tiles, active-alert generation, and overall status display
  remain the primary clinical indicators.
- The architecture separates presentation from safety-relevant domain logic,
  which reduces the chance that a display-only change must alter core alert
  algorithms.
- The current GUI already has established header/status presentation patterns
  for role and simulation state, reducing the need for novel interaction
  behavior.

## required design controls

- Define the alarm-audio state model before UI design. Do not infer semantics
  from competitor marketing language.
- Add a separate requirement and traceability entry for this feature rather than
  reusing alarm-threshold or alert-severity requirements.
- Keep the control display-only. Any future mute/silence action must be handled
  as a separate, higher-scrutiny change.
- Use neutral styling that distinguishes audio-state semantics from patient-risk
  severity.
- Include an explicit `Unknown` or equivalent fail-safe state when the source of
  truth is unavailable.
- If the product may later support routed or transferred alarm audio, split
  that into separate states and requirements instead of overloading `Silenced`.

## validation expectations

- Confirm the changed file list stays limited to UI/status-state plumbing,
  specifications, and tests needed for the badge.
- Verify each supported badge state manually in the GUI, including startup,
  login, logout, simulation enabled/disabled, and any silence-state transition.
- Verify that active clinical alerts remain visible and unchanged regardless of
  the badge state.
- Verify that the badge never asserts a specific state when the source data is
  missing, stale, or unsupported.
- Add or update traceability so the feature is covered by explicit requirements
  before implementation is approved.

## residual risk for this pilot

Low, provided the feature remains a read-only local state indicator with an
explicit unknown-state path and no alarm-control behavior. Residual risk rises
to medium if the team tries to compress routed, silenced, unavailable, and
normal-audio conditions into an ambiguous two-state label.

## whether the candidate is safe to send to Architect

Yes. This candidate is safe to send to Architect if the design stays within the
informational MVP boundary above and adds explicit requirements for the state
model, wording, and fail-safe behavior before implementation.

## human owner decision needed, if any

The product owner should confirm one decision before design finalization:

- whether this pilot truly supports only two local states (`Audible`,
  `Silenced`) or whether the product intent requires a distinct third state
  such as `Remote`, `Unavailable`, or `Unknown`

My recommendation is to require an explicit unknown/unavailable state even if
the visible MVP remains otherwise two-state, because that is the safest way to
avoid false reassurance from missing or stale state.
