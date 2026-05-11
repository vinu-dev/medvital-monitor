# Risk Note: Issue #52

Date: 2026-05-06
Issue: `#52` - Feature: show active patient identity in the dashboard header

## proposed change

Add a persistent, read-only active-patient identity card to the dashboard
header so an authenticated operator can confirm the currently loaded patient
without relying on the lower patient bar alone.

The approved MVP scope is limited to displaying already-stored demographics
from the active in-memory patient record. It must not change admission logic,
patient selection workflow, session persistence, alerting logic, thresholds,
NEWS2 scoring, or any clinical interpretation.

## medical-safety impact

This is a display-only UI change, but it sits on the patient-identification
path and therefore has real safety significance.

Potential safety benefit:

- Better visibility of the active patient may reduce wrong-patient ambiguity
  when a user admits, switches, or revisits a patient while responding to
  monitor data.

Potential safety harm if designed poorly:

- A stale or inconsistent header identity could mislead a user into acting on
  the wrong patient's vitals or alerts.
- Duplicating demographics in both the header and patient bar creates a
  consistency hazard if the two views do not update from the same source at the
  same time.

The change is safe to advance only if design keeps the feature read-only,
single-source, and clearly bounded as situational awareness support rather than
patient-verification automation.

## security and privacy impact

Security impact is low if the feature only re-renders demographics already
visible to an authenticated session.

Privacy impact is low to moderate:

- The issue does not introduce a new patient-data store, export path, or API.
- It does increase the prominence of patient demographics on screen, so the MVP
  should remain limited to the current fields already stored for the active
  record and must not add extra identifiers such as date of birth, address, or
  other unnecessary PHI.
- The identity card must disappear on logout and must not write patient
  identity to logs, config, crash dumps, screenshots, or telemetry.

## affected requirements or none

- `SYS-008` Patient Demographic Storage
- `SYS-011` Patient Status Summary Display
- `SYS-014` Graphical Vital Signs Dashboard
- `SYS-013` User Authentication Enforcement, because patient identity must not
  be shown before authentication or after logout
- `SWR-PAT-001` Patient Initialisation
- `SWR-PAT-006` Patient Summary Display
- `SWR-GUI-002` Session Management, because the header already displays the
  authenticated user name
- `SWR-GUI-004` Patient Data Entry via GUI

Architect/design should expect to add or refine an explicit GUI requirement for
header-patient identity behavior, because the current SWR set does not define a
persistent patient identity card in the header.

## intended use, user population, operating environment, and foreseeable misuse

Intended use:

- Help an authenticated operator keep the active patient context visible while
  using the monitoring dashboard.

User population:

- Bedside clinicians using the monitoring dashboard
- Ward administrators who can also access the authenticated dashboard

Operating environment:

- Windows desktop GUI in the pilot monitor application
- Simulation mode today, with future hardware-backed monitoring mode already
  represented in the UI

Foreseeable misuse:

- User assumes the header card is authoritative even if it is stale after
  Admit/Refresh, demo-scenario loading, Clear Session, logout, or mode changes
- User treats the card as a patient-verification control rather than a visual
  reminder
- Design adds clickable editing or switching behavior into the header card,
  expanding the safety scope
- Design adds extra identifiers or persistent display behavior that increase
  privacy exposure without clear need

## severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

- Severity: Major, because an incorrect or stale patient identity display could
  contribute to wrong-patient interpretation of vitals or alerts.
- Probability: Occasional before controls, because the UI already has multiple
  patient-state transitions and currently renders patient identity in a separate
  patient bar.
- Initial risk: Medium.
- Risk controls:
  - Render the header identity from the same active patient object
    (`g_app.patient`) and state flag (`g_app.has_patient`) already used by
    `paint_patient_bar()`.
  - Keep the card read-only. No search, selection, editing, or workflow control
    is allowed in the header MVP.
  - Define explicit empty-state, device-mode, clear-session, and logout
    behavior so no stale identity remains visible.
  - Refresh the header on every event that updates the patient bar today:
    admit/refresh, scenario load, added reading, clear session, mode toggles,
    and logout.
  - Limit content to current stored demographics needed for orientation, with
    no new identifiers or derived clinical claims.
  - State in design and test artifacts that the card supports context
    awareness only and does not replace formal patient identification checks.
- Verification method:
  - Manual GUI smoke test across at least two different patient records, plus
    logout, Clear Session, simulation pause/resume, device mode, and scenario
    transitions.
  - Confirm that header and patient bar show the same patient identity after
    every relevant transition.
  - Confirm that no new persistence, log, telemetry, or network path is added.
  - If new static strings are introduced, verify they follow the existing
    localization pattern.
- Residual risk: Low if the above controls are implemented.
- Residual-risk acceptability rationale: Acceptable for this pilot because the
  feature is display-only, reuses existing stored demographics, and can reduce
  context ambiguity without altering clinical logic, provided the implementation
  prevents stale or divergent identity displays.

## hazards and failure modes

- Header card shows a previous patient after a switch, scenario load, or Clear
  Session.
- Header card updates while the patient bar does not, or vice versa.
- Header remains visible after logout or when no patient is active.
- Header adds a second interaction path for patient switching or editing,
  increasing wrong-patient workflow risk.
- Header over-emphasizes a single identifier and may encourage users to skip
  established multi-identifier verification practices.
- Device mode or empty-state copy implies a real patient is active when one is
  not.

## existing controls

- `patient_init()` zero-fills the patient record and writes the bounded
  demographic fields defined by `SYS-008`.
- `paint_patient_bar()` already renders active-patient demographics, a device
  mode message, or an empty waiting state from the current in-memory session.
- The dashboard is created only after successful authentication, and logout
  clears session data before returning to the login screen.
- The issue explicitly constrains scope to a display-only change with no impact
  on thresholds, session state, or clinical classification.
- Public product-discovery evidence is directionally supportive: Philips
  IntelliVue MX550 marketing emphasizes seeing patient information at a glance,
  but it is marketing collateral rather than independent human-factors
  validation.
- General patient-safety guidance remains relevant: Joint Commission guidance
  continues to treat reliable patient identification as a safety-critical
  process and expects use of person-specific identifiers matched to the
  intended patient rather than convenience cues alone.

## required design controls

- Specify one source of truth for header identity content and update timing.
- Keep the header identity card read-only and non-interactive in the MVP.
- Define exact behavior for:
  - no authenticated user
  - authenticated session with no active patient
  - active patient in simulation mode
  - device mode with no live patient context
  - logout and Clear Session
- Preserve or improve visibility without obscuring existing alert, role, or
  simulation-status elements in the header.
- Avoid copying proprietary vendor layout, wording, or visual trade dress from
  competitor products; the MVP should solve the user problem with the repo's
  own UI language.
- Document in design artifacts that the feature does not verify identity,
  perform matching, or change care workflow.

## validation expectations

- Review the issue scope against `SYS-008`, `SYS-011`, `SYS-014`,
  `SWR-GUI-002`, and `SWR-GUI-004` before design finalization.
- Manually verify the current UI states in `src/gui_main.c` that already affect
  patient identity visibility: patient bar, header, device mode, clear session,
  scenarios, and logout.
- During implementation, run the standard build and existing automated tests
  named in the issue, then add a manual GUI checklist specifically for
  identity-state transitions.
- Confirm the design does not introduce patient search, patient switching,
  persistence across launches, new identifiers, or any clinical claim about
  preventing wrong-patient events.

## residual risk for this pilot

Residual risk is low if the design remains a display-only prominence
improvement and implementation uses the same session-state source already used
for the patient bar.

Residual risk becomes medium if the change duplicates state, introduces new
interaction affordances, or leaves identity visible when session context is no
longer valid.

## product hypothesis and intended user benefit

Hypothesis:

- A more prominent active-patient identity display will reduce orientation time
  and wrong-patient ambiguity during dashboard use.

Intended user benefit:

- Faster confirmation of patient context before acting on vitals, alerts, or
  trend information.

Repo evidence supports the feasibility of a bounded MVP because the application
already stores the required demographic fields and already renders patient
identity in the patient bar.

## source evidence quality

Source evidence is sufficient for bounded product discovery, but not strong
enough to justify copying a vendor UX or making safety-effectiveness claims.

- Philips IntelliVue MX550 product page:
  `https://www.usa.philips.com/healthcare/product/HC866066/intellivue-mx550-portablebedside-patient-monitor`
  This is an official vendor source and it supports the general idea that
  patient information should be easy to see quickly. It is still marketing
  collateral, not a detailed usability specification or comparative study.
- Joint Commission patient-identifier FAQ:
  `https://www.jointcommission.org/en-us/knowledge-library/support-center/standards-interpretation/standards-faqs/000001545`
  This is stronger safety-context evidence. It supports the boundary that
  visible identity cues help context, but safe care still depends on reliable,
  person-specific identifiers and defined organizational process.

Overall evidence quality: Moderate for approving design exploration of a
display-only context cue; insufficient for broader workflow, verification, or
claim expansion.

## MVP boundary that avoids copying proprietary UX

- Show only the active patient's existing name, ID, and age, plus a clear empty
  state when no patient is active.
- Reuse repo-native typography, layout language, and color system rather than
  imitating Philips screen composition or visual branding.
- Do not add tabs, search, patient lists, confirmation dialogs, selection
  workflow, transport workflow, or cross-screen identity synchronization beyond
  the current dashboard session.

## clinical-safety boundary and claims that must not be made

- The feature must not claim to verify patient identity, prevent wrong-patient
  events, or replace established two-identifier clinical practice.
- The feature must not change alert thresholds, NEWS2 score behavior, patient
  status classification, or any clinical recommendation.
- The feature must not introduce AI, inference, reconciliation, or any
  algorithmic patient-matching behavior.

## whether the candidate is safe to send to Architect

Yes, with constraints.

This candidate is safe to send to Architect because the issue provides a
bounded display-only MVP, identifies the intended patient-safety benefit, names
the affected requirement area, and includes enough source evidence to justify a
non-copying design exploration.

Architect should treat single-source rendering, explicit state transitions, and
non-interactive scope as mandatory controls, not optional polish.

## human owner decision needed, if any

No immediate human owner decision is needed to start design for the bounded
display-only MVP.

Human product/safety review is required before any future expansion that adds
new identifiers, persistent patient context across launches, patient-switching
workflow, verification claims, or any change to clinical decision pathways.
