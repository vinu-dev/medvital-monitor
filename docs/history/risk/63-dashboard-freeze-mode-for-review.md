# Risk Note: Issue #63

Date: 2026-05-06
Issue: `#63` - Feature: add a dashboard freeze mode for review
Branch: `feature/63-dashboard-freeze-mode-for-review`

## proposed change

Add an operator-invoked, read-only dashboard freeze state that holds a stable
visual snapshot long enough for handoff, screenshot capture, or review without
changing acquisition, thresholds, alert logic, NEWS2 logic, or stored patient
data.

The intended scope is presentation-only. The current dashboard behavior in
`src/gui_main.c` refreshes after each new reading and already has a separate
`sim_paused` state that stops simulation updates. A new freeze feature must
therefore be a distinct review state with distinct semantics, not a relabel of
the existing pause behavior.

## product hypothesis and intended user benefit

The product hypothesis is that operators reviewing a moving dashboard during
handoff or screenshot capture have avoidable cognitive load and transcription
friction. A short-lived freeze state could improve reviewability by preserving a
stable visual snapshot while discussion or capture occurs.

The intended benefit is operational clarity only. This feature does not justify
new diagnostic, alarm-management, or clinical-decision claims.

## source evidence quality

- Moderate for workflow inspiration, not for clinical proof.
- Draeger Vista 120 IFU (accessed 2026-05-06):
  `https://www.draeger.com/Content/Documents/Products/vista-ifu-2606251-en.pdf`
  describes a freeze function for waveform review and indicates frozen views
  should not be used for long durations. This is useful evidence that review
  freeze is a known workflow pattern and that time-bounding matters.
- Draeger Infinity MView product page (accessed 2026-05-06):
  `https://www.draeger.com/en_seeur/Products/Infinity-Mview-Bedside`
  describes review of historical data, alarm history, and trends while
  separating those review functions from primary alarm use. This supports a
  narrow review-only boundary.
- Both sources are manufacturer-controlled primary materials, but one is an IFU
  and one is a marketing page. They are enough to justify a bounded MVP
  hypothesis, not enough to justify copying proprietary UX or weakening live
  monitoring safeguards.

## MVP boundary that avoids copying proprietary UX

Implement only a generic dashboard-local review snapshot control and state
indicator. Do not copy competitor hardkey labels, menu hierarchy, layout,
waveform-browse gestures, full-disclosure tooling, alarm-history tooling, or
retrospective analytics workflows.

The MVP boundary should stay limited to:

- enter freeze
- show unmistakable frozen-state indication
- exit freeze manually or automatically

Out of scope:

- persistence
- export
- remote sharing
- central-station workflow
- retrospective trend browsing
- alarm history review

## clinical-safety boundary and claims that must not be made

This feature must not be described as continuous live monitoring, diagnostic
review, alarm acknowledgement, or a substitute for the primary monitoring view.
It may support a momentary review snapshot only.

Freeze must not suppress:

- acquisition
- patient-record writes
- alert generation
- NEWS2 calculation
- alarm routing

If the proposed design would alter any of those behaviors, the issue should be
re-scoped and re-reviewed rather than treated as a low-risk UI enhancement.

## medical-safety impact

The main medical-safety hazard is stale-display misuse: a clinician or operator
could interpret frozen values as current and delay recognition of patient
deterioration. That risk is foreseeable because the dashboard is normally
continuous and the existing product already uses explicit non-live states such
as `SIM PAUSED`.

This can still be acceptable for a pilot if the freeze state is unmistakable,
short-lived, reversible, and clearly separated from both live monitoring and
the existing simulation-pause behavior.

## security and privacy impact

No new authentication, authorization, network, storage, or patient-data flow is
needed if the feature remains local presentation state only.

The main privacy concern is operational rather than technical: screenshots of a
frozen dashboard may contain patient identifiers. The feature therefore should
not add any automated export or sharing path, and operators still need to
follow local screenshot and handoff discipline.

## affected requirements or "none"

- `UNS-010` Consolidated Status Summary
- `UNS-014` Graphical User Interface
- `UNS-015` Live Monitoring Feed
- `SYS-014` Graphical Vital Signs Dashboard, because it currently requires the
  dashboard to refresh automatically after each new reading is added
- likely new or revised SYS/SWR wording for an operator-invoked review/freeze
  state that preserves live acquisition and alarm behavior while temporarily
  holding the dashboard presentation
- adjacent software requirements likely to change or gain siblings:
  `SWR-GUI-003`, `SWR-GUI-004`, `SWR-GUI-010`, `SWR-GUI-011`

No approved requirement currently authorizes a deliberately stale dashboard
review state.

## intended use, user population, operating environment, and foreseeable misuse

- Intended use: brief review of a stable dashboard snapshot during handoff,
  screenshot capture, or non-diagnostic discussion.
- User population: trained, authenticated ward or ICU staff and internal review
  users already permitted to access the dashboard.
- Operating environment: the existing Windows bedside or review workstation UI.
- Foreseeable misuse: leaving freeze active during ongoing monitoring,
  mistaking freeze for live view, confusing freeze with `SIM PAUSED`, relying on
  a screenshot as current patient state, or allowing freeze to persist across a
  patient-context change.

## severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

- Severity: serious, because stale displayed vitals could contribute to delayed
  recognition of deterioration if interpreted as current.
- Probability without controls: occasional, because activation is deliberate but
  misuse during workload or handoff is reasonably foreseeable.
- Initial risk: medium-high for the pilot if the display can remain stale
  without strong visual and temporal controls.
- Risk controls:
  - show a full-width, high-contrast frozen banner or overlay containing the
    word `FROZEN`, the snapshot timestamp, and visible age
  - make freeze visually distinct from the existing `SIM PAUSED` state
  - keep freeze read-only and dashboard-local; do not allow it to mutate data
  - auto-exit on new critical deterioration, patient switch, logout, and a
    short maximum duration
  - preserve acquisition, patient-record writes, alert generation, and alarm
    routing while frozen, or explicitly limit the feature to environments where
    this guarantee can be made
  - document that freeze is review-only and not a diagnostic or alarm-management
    feature
- Verification method:
  - manual GUI review of enter/exit freeze, frozen-state visibility, distinct
    wording, and timeout behavior
  - negative test showing new critical state cannot remain hidden behind an
    indefinite frozen view
  - targeted regression check proving acquisition and alert generation continue
    as intended while the dashboard display is frozen, or explicit verification
    that the feature is limited to approved non-live contexts
  - requirements and traceability review confirming the new state is explicitly
    specified and bounded
- Residual risk: low-medium for the pilot if the controls above are implemented;
  medium and not acceptable if freeze can persist silently or indefinitely.
- Residual-risk acceptability rationale: acceptable only as a short-lived,
  unmistakably non-live review aid. It is not acceptable as a silent stale-data
  mode on a primary monitoring surface.

## hazards and failure modes

- Frozen dashboard is mistaken for current patient status.
- Freeze is confused with the existing simulation-pause behavior.
- A new warning or critical event occurs while the visual snapshot remains
  unchanged.
- A screenshot containing patient identifiers or stale values is used outside
  its intended review context.
- Freeze survives a patient switch, logout, or view transition and shows the
  wrong patient context.
- Operators continue manual-entry or scenario actions while looking at a frozen
  view, creating visible-versus-stored inconsistency.

## existing controls

- The issue body explicitly limits scope to display-only behavior and excludes
  thresholds, acquisition, alarms, NEWS2, persistence, export, and hardware
  integration.
- `src/gui_main.c` centralizes dashboard painting and update flow, which makes a
  presentation-only implementation feasible without changing core clinical
  logic.
- The current product already distinguishes live versus non-live simulation
  states in the header, so the UI has an established pattern for explicit state
  signalling.
- External source evidence supports the idea that freeze/review workflows should
  be bounded and clearly signalled rather than treated as primary-monitoring
  behavior.

## required design controls

- Add a dedicated frozen-state banner or overlay with timestamp and age.
- Keep the state read-only and local to the dashboard view.
- Do not reuse the existing `SIM PAUSED` label, color, or button semantics for
  freeze.
- Auto-exit freeze on new critical alarm, patient switch, logout, and a short
  maximum duration.
- Preserve acquisition, alert calculation, NEWS2 calculation, patient-record
  writes, and alarm outputs while frozen.
- Add or revise approved requirements and traceability before implementation.
- Keep the first version free of export, persistence, retrospective browsing,
  remote access, and copied competitor workflow details.
- If the team wants freeze in live/device mode without timeout or alarm-sensitive
  exit behavior, escalate for explicit human approval instead of treating it as
  routine UI scope.

## validation expectations

- Manual GUI walkthrough of enter freeze, exit freeze, timeout, and visual state
  distinctness relative to `SIM PAUSED`.
- Manual or automated proof that a new critical event or patient change forces
  exit or otherwise re-exposes live status in a way that cannot be missed.
- Confirmation that no persistence, export, sharing, or network behavior is
  introduced.
- Requirements and traceability review confirming the freeze state is approved
  as a bounded review workflow and does not silently violate the existing
  automatic-refresh requirement.
- Review of help text, labels, and UX wording to confirm the feature makes no
  diagnostic or primary-alarm-management claim.

## residual risk for this pilot

Residual risk is manageable for the pilot only if the feature remains a bounded,
short-lived review aid with explicit stale-data signalling and automatic return
to live behavior. Without those controls, the feature would create an
unacceptable stale-display risk on a clinical dashboard.

## human owner decision needed, if any

- Decide whether freeze is permitted in live/device mode or limited to
  simulation/demo/review contexts for the first release.
- Decide the maximum permitted freeze duration.
- Decide whether new warning states must also force exit, or whether only new
  critical states do.
- Decide whether screenshot use with real patient identifiers is permitted by
  operating procedure or should be limited to demo data.

## whether the candidate is safe to send to Architect

Yes, with mandatory guardrails.

This candidate is safe to send to Architect only if the design remains UI-only,
keeps the frozen state unmistakable and time-bounded, and does not suppress or
hide ongoing monitoring and alarm behavior. If the intended design requires an
indefinite stale view, weak state indication, or any change to acquisition or
alarm semantics, it should return for explicit human decision instead of moving
forward as a routine design item.
