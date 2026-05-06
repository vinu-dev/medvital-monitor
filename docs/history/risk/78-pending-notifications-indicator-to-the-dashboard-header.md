# Risk Note: Issue 78

Issue: `#78`  
Branch: `feature/78-pending-notifications-indicator-to-the-dashboard-header`

## proposed change

Add a compact, read-only dashboard-header indicator that shows whether one or
more non-alarm status messages are pending review in the current session.

The change should stay localized to the existing GUI presentation seams in
`src/gui_main.c`, reusing the current header paint path, status-banner path,
and session-scoped application state. The indicator should not acknowledge,
dismiss, escalate, silence, persist, or export anything. It is a secondary cue
that points the operator back to the existing status-message surface.

## product hypothesis and intended user benefit

The product hypothesis is credible: transient status messages are easier to miss
than persistent header cues during handoff or when the operator is focused on
the vital-sign tiles. A small header indicator can reduce search friction and
make the existing status-message path more visible without changing any
clinical-classification behavior.

Intended user benefit for this pilot:

- quicker recognition that a non-alarm status message exists
- less dependence on catching a transient banner state at exactly the right time
- no new workflow burden if the cue remains passive and session-scoped

## source evidence quality

Source evidence quality is moderate and sufficient for design discovery, not
for clinical justification.

- The Philips IntelliVue MX400/450/500/550 Instructions for Use are a primary
  manufacturer source and describe a notification symbol in the status area
  that opens a notifications window for active notifications:
  `https://www.documents.philips.com/assets/Instruction%20for%20Use/20260424/83cba6c0fe91400ea93ab436008655e7.pdf?feed=ifu_docs_feed`
- The Philips EarlyVue VS30 Instructions for Use are also a primary
  manufacturer source and describe a persistent main-screen information area
  with physiological alarm messaging, technical/information messaging, and user
  ID panes:
  `https://www.documents.philips.com/assets/Instruction%20for%20Use/20250314/ae9f51cf334740338e6eb2a00114ce4b.pdf?feed=ifu_docs_feed`

These sources support the narrow claim that bedside-monitor UIs commonly expose
persistent status/notification cues. They do not justify copying Philips
wording, iconography, layout, colors, interaction model, or proprietary
notification taxonomy. They are also not usability studies and do not, by
themselves, prove clinical benefit.

## MVP boundary that avoids copying proprietary UX

The safe MVP boundary is narrow:

- a neutral header badge with on/off state, or at most a bounded count of
  pending non-alarm status messages
- derived only from existing in-session status-message state already shown in
  the application
- read-only, with no new tray, no notifications list, no acknowledgment flow,
  no history view, and no persistence
- original wording, iconography, spacing, and visual treatment specific to this
  product

Out of scope for this issue:

- copying competitor notification symbols, labels, colors, placement, or
  interaction affordances
- creating a separate notifications window or queue browser
- exposing detailed message text in the header
- any alarm workflow, NEWS2, threshold, or clinical decision-support change

## clinical-safety boundary and claims that must not be made

This candidate is safe only if it remains explicitly outside the clinical alarm
path.

Claims that must not be made:

- that the header indicator is an alarm, warning, or triage signal
- that it changes, improves, or replaces alarm detection or alarm response
- that it provides diagnostic, treatment, or deterioration-assessment value by
  itself
- that absence of the indicator means absence of clinical risk

The indicator may only claim to surface the presence of already-generated
non-alarm status information inside the current UI session.

## medical-safety impact

Direct medical-safety impact is low because the proposed change does not alter
vital-sign acquisition, thresholding, alert generation, NEWS2 scoring,
authentication, or patient-record logic.

The meaningful safety risk is indirect: a poorly designed indicator could be
confused with an alarm or could become stale, causing operator distraction,
mis-prioritization, or misplaced trust in the header cue. Because the current
product already uses strong color semantics for alert severity and already has a
status banner, visual ambiguity is the primary hazard to control.

## security and privacy impact

Security impact is low if the indicator is only a bounded session-state cue.
No new network path, privilege boundary, or persisted data store is required.

Privacy impact is also low only if the header shows minimal state such as an
icon, neutral label, or small count. If the design surfaces detailed message
text, patient identifiers, or clinically sensitive status content in the always
visible header, shoulder-surfing and incidental disclosure risk increase. The
header indicator therefore should not expose PHI or detailed clinical content.

## affected requirements or "none"

No approved requirement text changes are in scope in this risk note itself, but
the candidate is not implementable cleanly without new or updated traceable GUI
requirements.

Existing foundations:

- `UNS-014` graphical dashboard clarity
- `SYS-014` graphical vital-signs dashboard
- `SWR-GUI-003` colour-coded dashboard display behavior
- `SWR-GUI-011` current status-banner behavior

Design should add or revise requirements to define at least:

- indicator purpose and message classes covered
- explicit non-alarm separation rules
- state source and clear/reset semantics
- privacy constraints on header content
- verification evidence for appearance, reset, and non-interference

## intended use, user population, operating environment, and foreseeable misuse

Intended use of the proposed feature:

- help a logged-in operator notice that non-alarm status information is pending
  in the current monitoring session

User population:

- trained clinical staff and authorized operators using the desktop monitor UI
- administrators or clinical users already authenticated into the session

Operating environment:

- Windows workstation dashboard in the pilot bedside-monitoring context
- local session with existing GUI header, tiles, and status-banner rendering

Foreseeable misuse:

- interpreting the badge as an alarm or warning severity indicator
- using the badge as the sole source of status awareness and ignoring the
  underlying banner/message surface
- expecting the badge to acknowledge, dismiss, or archive messages
- assuming the badge persists across logout, patient reset, or session changes
- overloading the badge with message text that is not appropriate for a
  persistent header surface

## severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

- Severity: minor to moderate if the cue is ambiguous, because confusion could
  distract from true alarms or delay review of operational status information;
  severe patient harm is not the direct expected effect of this UI-only change.
- Probability: medium without controls because the header already contains
  multiple badges and the status banner already uses severity colors.
- Initial risk: medium.
- Risk controls:
  - reserve alarm colors and alarm semantics for the existing alert path only
  - use neutral visual treatment and explicit non-alarm wording or iconography
  - keep the indicator read-only with no acknowledge/silence affordance
  - derive the indicator only from existing non-alarm status-message state
  - clear or recompute state on dismiss, supersede, logout, Clear Session,
    patient reset, and simulation/device-mode transitions
  - prevent PHI and detailed clinical text from appearing in the header cue
  - preserve the existing status banner as the primary message surface
- Verification method:
  - manual GUI smoke proving set, clear, and reset behavior
  - regression check that alarm coloring, NEWS2, and alert generation are
    unchanged
  - layout review at current dashboard widths to confirm no overlap with user,
    role, sim-state, or header controls
  - requirements and traceability update before implementation approval
- Residual risk: low if the controls above are implemented.
- Residual-risk acceptability rationale: the cue is secondary, passive, and
  non-clinical; the existing message surface remains in place; and no clinical
  algorithm, patient data flow, or alarm workflow changes when the feature is
  kept within the defined boundary.

## hazards and failure modes

- The indicator uses red, amber, pulsing, or alarm-like glyphs and is mistaken
  for an active physiological or technical alarm.
- The indicator remains set after the underlying status message is dismissed or
  superseded, creating stale unresolved-state bias.
- The indicator clears while a pending status message still exists, causing the
  operator to miss the message.
- The indicator persists across logout, patient change, or session reset and
  incorrectly carries forward old state.
- The header displays message details or patient-linked content that increase
  privacy exposure.
- The feature grows into a new acknowledgment or notification-management
  workflow without corresponding requirements and validation.
- The design copies competitor UX too closely, creating provenance and product
  distinctiveness concerns.

## existing controls

- `src/gui_main.c` already separates `paint_header()` from
  `paint_status_banner()`, which supports a bounded display-only change.
- Existing alarm semantics are already visually encoded in the tiles and status
  banner, giving the design a clear boundary it must not reuse for this cue.
- `g_app` already holds session-scoped UI state and is reset on logout/session
  transitions, which supports bounded non-persistent behavior.
- The issue explicitly scopes the feature as display-only and excludes alarm
  acknowledgement, persistence, export, and threshold changes.

## required design controls

- Define one approved message class boundary for the indicator before design:
  recommended scope is non-alarm status messages only.
- Prohibit alarm colors, alarm copy, alarm-count semantics, and alarm-like
  motion in the header indicator.
- Keep the indicator read-only. If a clickable surface is later proposed, stop
  and re-run risk assessment because the workflow has changed materially.
- Reset state deterministically on logout, Clear Session, patient replacement,
  message dismiss/supersede, and simulation/device-mode changes.
- Limit header content to neutral state or small count; do not display detailed
  message text or patient identifiers.
- Add traceable requirement text and explicit GUI verification evidence before
  implementation is accepted.
- Keep the implementation localized to existing GUI presentation/state seams;
  do not route through alert-generation, NEWS2, or other clinical logic.

## validation expectations

- Trigger a status message and confirm the header indicator appears while the
  underlying message remains pending.
- Dismiss or supersede the status message and confirm the indicator clears.
- Confirm the indicator is visually distinct from alarm states and does not use
  the tile/banner severity palette.
- Confirm logout, Clear Session, patient refresh, and simulation/device-mode
  transitions reset the indicator correctly.
- Confirm no patient identifiers or detailed message text are exposed in the
  header surface.
- Confirm existing build/test flow remains green and that no alert-generation or
  NEWS2 behavior changed.
- Confirm requirements and traceability updates exist before implementation is
  considered complete.

## residual risk for this pilot

Residual risk is low and acceptable for the pilot if the feature stays
read-only, session-scoped, non-alarm, and privacy-minimal. The remaining risk
is mainly operator confusion from poor visual design or stale-state handling,
not algorithmic clinical failure. That residual risk is acceptable for
architectural design work, but not for implementation without the controls
above.

## human owner decision needed, if any

Yes. A human product/design owner should approve:

- whether the indicator covers only non-alarm status messages, or a broader
  class such as prompts
- the final neutral label/icon treatment that makes the cue clearly non-alarm
- whether a count is allowed, or only a binary on/off state

## whether the candidate is safe to send to Architect

Yes. The candidate is safe to send to Architect as a bounded GUI feature if the
design stays within the read-only, non-alarm, non-persistent boundary and
introduces explicit requirements for alarm separation, reset semantics, and
privacy-minimal header content.
