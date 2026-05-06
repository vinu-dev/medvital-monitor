# Risk Note: Issue 74

Issue: `#74`
Branch: `feature/74-selectable-dashboard-layout-presets`

## proposed change

Add two operator-selectable dashboard layout presets, `Overview` and `Review`,
with one local toggle and a persisted default. Scope is limited to
presentation-layer arrangement of existing dashboard elements in
`src/gui_main.c`.

The proposed change does not alter vital-sign acquisition, threshold logic,
alert generation, NEWS2 calculation, authentication, or patient-record
behavior. It changes how existing information is arranged and identified on the
screen.

## product hypothesis and intended user benefit

The product hypothesis is that operators use the dashboard in at least two
distinct workflows:

- a broader bedside or monitoring workflow that benefits from the existing
  overview-oriented layout
- a denser review or handover workflow that benefits from reduced clutter and
  faster side-by-side comparison

If implemented carefully, a compact review preset could improve operator
ergonomics without changing the clinically validated monitoring core.

## source evidence quality

Source evidence is adequate for product-discovery context only, not for clinical
justification or UX copying. The cited Mindray public product page supports the
general claim that commercial patient monitors can offer configurable profiles
and screen-layout options for different use scenarios.

Evidence quality is:

- moderate for "this feature category exists in the market"
- low for "this exact interaction model should be copied"

Rationale:

- only one public vendor page is cited
- the source is product marketing, not a human-factors study, operator manual,
  or clinical-safety source
- the source is not evidence that a denser layout improves detection,
  comprehension, or patient outcomes

This is still enough to justify a narrow, non-copying MVP for design
exploration.

## medical-safety impact

Direct medical-safety impact is low because the issue is display-only and leaves
the IEC 62304-controlled clinical logic unchanged. Indirect medical-safety
impact is real and must be controlled because layout changes can alter what an
operator notices first, how quickly deterioration is recognized, and whether a
required tile or alert surface is visible during monitoring or handover.

The main safety risk is not wrong computation. The main safety risk is delayed
recognition caused by hidden, clipped, reordered, or de-emphasized information.

## security and privacy impact

No new network path, remote dependency, authentication path, or patient-data
sharing flow is needed for the MVP. Security and privacy impact should remain
low if persistence stays limited to a local UI preference.

Key boundaries:

- do not store patient-specific layout state
- do not add cloud-sync or shared-profile behavior
- do not write PHI into any new preset configuration artifact
- if persistence is added, keep it to non-clinical UI state only

## affected requirements or "none"

Existing approved requirements constrain this feature even though they do not
yet define layout presets explicitly:

- `UNS-010` consolidated status summary
- `UNS-014` graphical user interface
- `SYS-011` patient status summary display
- `SYS-014` graphical vital signs dashboard
- `SWR-GUI-003` color-coded tile display
- `SWR-GUI-004` patient data entry and dashboard workflow
- `SWR-GUI-010` persisted GUI operating mode behavior
- `SWR-GUI-012` localization and persisted UI selection behavior
- `SWR-VIT-008` because RESP RATE is a required displayed tile
- `SWR-NEW-001` because NEWS2 is a required displayed tile

New approved requirements will be needed before implementation for:

- preset selection behavior
- visibility invariants across presets
- persistence scope and default behavior

## intended use, user population, operating environment, and foreseeable misuse

Intended use remains unchanged: trained clinical staff use a Windows workstation
to monitor and review admitted-patient vital signs and alerts.

Relevant users:

- bedside clinicians
- ward nurses
- operators reviewing trends or handover context

Operating environment:

- local Win32 dashboard
- fixed-size or resized workstation windows
- live simulation mode and future device mode

Foreseeable misuse:

- the operator selects `Review` and assumes non-visible information is absent or
  normal
- the active preset is not obvious and the operator misreads the screen context
- a persisted compact preset surprises the next login session
- the design gradually expands into drag-and-drop or profile complexity without
  separate risk review

## severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

Severity:

- moderate to serious, because a poor layout could delay recognition of
  clinically important deterioration even while underlying alarm computation
  remains correct

Probability:

- low to medium without controls, because preset persistence, resize behavior,
  and visual hierarchy mistakes are plausible in a GUI-only change

Initial risk:

- medium

Risk controls:

- every preset must keep HEART RATE, BLOOD PRESSURE, TEMPERATURE, SpO2,
  RESP RATE, NEWS2, patient identity, aggregate status, alerts list, and
  history list visible at the same time on the supported window size
- the active preset must be continuously visible in the header and update
  immediately when toggled
- `Overview` should remain the first-run safe default unless a human owner
  explicitly accepts a different default behavior
- persisted preset state must be local UI preference only and must not become
  per-patient or cloud-synced state
- presets may change density, grouping, or order, but shall not suppress any
  vital tile, alert surface, NEWS2 surface, simulation/device indicator, or
  alarm color semantics
- if a supported viewport cannot fit mandatory content, the UI must degrade in
  a controlled way such as minimum window sizing or scrolling rather than silent
  clipping

Verification method:

- build and regression test per normal project workflow
- manual GUI review of both presets in no-patient, normal, warning, and
  critical states
- manual resize review to confirm no mandatory control or tile becomes clipped
- manual login/logout and app-restart review to confirm preset visibility and
  persistence behavior
- manual review of simulation/device mode banners and localization interactions
  if new text is added

Residual risk:

- low if the controls above are implemented as hard design invariants

Residual-risk acceptability rationale:

- acceptable for this pilot only if the feature remains presentation-only,
  preserves simultaneous visibility of mandatory clinical information, and does
  not introduce hidden state that changes monitoring context silently

## hazards and failure modes

- a preset hides, clips, or removes a required vital-sign tile
- NEWS2, alerts, or patient identity become less visible than in the current
  baseline
- the active preset is not obvious, so the operator assumes the wrong screen
  context
- the compact preset persists across sessions and surprises the next user
- resize or localization causes overlapping controls, truncated labels, or
  inaccessible actions
- future scope creep introduces editable layouts, shared profiles, or patient-
  specific layouts without new requirements and risk review

## existing controls

- `src/gui_main.c` currently renders the six-tile dashboard from a single
  `paint_tiles()` path
- `src/gui_main.c` already centralizes dashboard control reflow in
  `reposition_dash_controls()`
- alerting, NEWS2, and patient status are computed outside layout code and are
  not structurally dependent on tile placement
- the issue explicitly excludes threshold, alarm, NEWS2, and acquisition logic
  changes

## required design controls

- define non-negotiable visibility invariants before UI implementation starts
- treat preset choice as a presentation preference, not a clinical mode
- keep preset names generic and descriptive; do not copy competitor scenario
  names or proprietary profile taxonomy
- document which elements may move and which elements may never be hidden
- if persistence is implemented, define whether it is global or per-user and
  verify that behavior deliberately rather than inheriting it accidentally from
  another settings path
- keep drag-and-drop editing, custom user-authored layouts, per-patient
  profiles, and cloud-sync out of scope

## validation expectations

- verify both presets preserve all required tiles and lists on the supported
  window size
- verify the preset label is always visible and matches the actual active layout
- verify warning and critical color semantics remain unchanged in both presets
- verify logout/login and restart behavior for any persisted default
- verify no code change touches alarm logic, acquisition logic, or NEWS2 logic
- verify the final diff stays primarily within `src/gui_main.c`, related UI
  persistence code if needed, and new requirements/spec updates approved for the
  feature

## MVP boundary that avoids copying proprietary UX

Safe MVP boundary:

- exactly two local presets, `Overview` and `Review`
- one explicit toggle or selector
- optional persisted default
- no drag-and-drop editor
- no scenario library
- no named competitor workflow packs
- no attempt to reproduce a competitor's exact geometry, visual taxonomy, or
  profile system

## clinical-safety boundary and claims that must not be made

The feature must not be described as:

- improving clinical accuracy
- reducing alarm fatigue
- improving patient outcomes
- providing triage guidance
- changing the meaning or priority of any vital sign or NEWS2 output

The feature must remain a display-organization aid only.

## whether the candidate is safe to send to Architect

Yes. This candidate is safe to send to Architect as a bounded presentation-layer
feature, provided the design keeps all mandatory clinical information visible at
all times and treats preset state visibility and persistence as first-class
safety controls.

## residual risk for this pilot

Low, provided the implementation stays within the MVP boundary and the design
controls above are made explicit in requirements and verification planning
before code changes begin.

## human owner decision needed, if any

Human owner confirmation is still needed on two design points:

- whether remembered preset state is global or per-user; from a risk standpoint,
  per-patient state is not acceptable for this MVP
- whether `Overview` remains the forced safe default on first login and after
  logout, or whether remembered state is acceptable once the active preset is
  always obvious

These are design-acceptance decisions, not blockers to sending the issue to
Architect.
