# Design Spec: Issue 52

Issue: `#52`  
Branch: `feature/52-active-patient-identity-dashboard-header`  
Spec path: `docs/history/specs/52-active-patient-identity-dashboard-header.md`

## Problem

The dashboard already stores active-patient demographics in `g_app.patient.info`
and renders them in the lower patient bar, but the top header currently shows
only the app title, authenticated user, role badge, and simulation status.

That means the operator must look below the header to confirm which patient is
loaded. In device mode, clear-session state, and other transitions, the lower
patient bar also changes purpose, which increases the chance that patient
identity is less visible than the vitals and alert areas the user is acting on.

The current SWR set also does not define a persistent, read-only
header-patient-identity behavior, so implementation would otherwise depend on
implicit UI intent rather than explicit traceable requirements.

## Goal

Add a persistent, read-only active-patient identity card to the dashboard
header so an authenticated operator can confirm the active patient at a glance.

The MVP shall:

- show the active patient's existing name, ID, and age when
  `g_app.has_patient != 0`
- show a clear empty state when no patient is active
- render from the same in-memory patient/session state already used by the
  patient bar
- remain display-only with no effect on admission, monitoring, alerting,
  NEWS2, persistence, or patient-selection workflow

## Non-goals

- Adding patient search, switching, transport, verification, or editing
  controls to the header
- Adding new identifiers such as DOB, address, or any additional PHI beyond
  the existing stored name, ID, and age
- Changing `PatientRecord`, `patient_init()`, `patient_add_reading()`,
  thresholds, alerts, NEWS2 scoring, authentication rules, or persistence
- Claiming that the identity card verifies the patient or prevents
  wrong-patient events
- Redesigning the full header, changing CI/release artifacts, or modifying
  unrelated dashboard workflows

## Intended Use Impact, User Population, Operating Environment, And Foreseeable Misuse

Intended use impact:

- Improves situational awareness for the authenticated operator by keeping the
  active patient context visible near the top of the dashboard.
- Does not change care recommendations, alarm generation, or any clinical
  interpretation.

User population:

- Bedside clinicians using the monitoring dashboard
- Administrative users who can access the authenticated dashboard

Operating environment:

- Current Win32 desktop dashboard in the authenticated application window
- Simulation mode today, with existing device-mode states already represented
  in the same dashboard code path
- Minimum dashboard width currently constrained by `WM_GETMINMAXINFO`
  (`760 x 640`)

Foreseeable misuse:

- User treats the card as a formal patient-verification control rather than a
  context cue
- Header and patient bar diverge because they are formatted from different
  state or refreshed on different events
- The card remains visible after Clear Session, device-mode transitions, or
  logout
- The header card becomes clickable and expands the issue into patient
  switching or editing workflow

## Current Behavior

- `src/patient.c` and `include/patient.h` store the active patient's bounded
  demographics in `PatientInfo` and keep them in `g_app.patient`.
- `src/gui_main.c:292-327` paints the header with:
  - app title
  - authenticated user name
  - authenticated role badge
  - simulation status text when simulation is enabled
- `src/gui_main.c:333-360` paints the lower patient bar with:
  - a device-mode message when simulation is off
  - active-patient details, BMI, and reading count when `g_app.has_patient`
  - an empty waiting message when no patient is active in simulation mode
- `src/gui_main.c:764-823` updates patient state through Admit/Refresh,
  Add Reading, Clear Session, and demo-scenario actions.
- `src/gui_main.c:1687-1765` also changes patient state during dashboard
  creation, timer-driven simulation updates, and mode transitions.
- `update_dashboard()` invalidates the dashboard on patient-state changes, but
  there is no dedicated header identity element to verify that those transitions
  keep header and patient identity synchronized.
- The repo now has a localization layer (`include/localization.h`,
  `src/localization.c`), but the current patient-bar summary text is still
  composed from hard-coded English strings.

## Proposed Change

1. Add a compact, non-interactive active-patient identity card to
   `paint_header()` in `src/gui_main.c`.
2. Keep the card inside the existing header band rather than adding a new
   patient workflow region.
3. Populate the card only from the current dashboard session state already used
   by the patient bar:
   - `g_app.has_patient`
   - `g_app.patient.info.name`
   - `g_app.patient.info.id`
   - `g_app.patient.info.age`
4. Do not introduce a second cached patient-identity structure, a separate
   "header patient" flag, or any new persistence/logging path.
5. Define the card content by state:
   - no authenticated user: no dashboard header card is visible because the
     dashboard window is not available before login
   - authenticated session with no active patient: show a clear empty state
     such as `No active patient`
   - authenticated session with active patient: show patient name as the
     primary line and ID/age as compact secondary metadata
   - device mode and Clear Session: clear the card immediately so no stale
     identity remains visible
   - logout: destroy the dashboard and clear session state so no identity
     persists after returning to the login screen
6. Reuse a shared formatting/helper path for header and patient-bar identity
   content where practical so both views derive identity from the same source
   and transition rules.
7. Preserve existing controls and visibility priorities:
   - logout, pause, and settings buttons remain fully visible
   - authenticated user and role badge remain visible
   - simulation status remains visible when applicable
   - if horizontal space is tight, truncate the app title before allowing the
     patient card or safety-relevant controls to overlap
8. Keep the card read-only and visually distinct from command buttons so it is
   clearly an informational cue, not an action target.
9. Route any new visible header strings through the existing localization
   mechanism instead of adding new hard-coded English text.

Recommended visual shape:

- small header label such as `Active patient`
- patient name in the strongest text treatment inside the card
- compact metadata line such as `ID 1001 | Age 52`
- empty-state text that is visually quieter than an active patient name

## Files Expected To Change

Expected implementation files:

- `src/gui_main.c`
- `include/localization.h`
- `src/localization.c`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`

Files expected not to change for this issue:

- `src/patient.c`
- `include/patient.h`
- `src/vitals.c`
- `src/alerts.c`
- `src/news2.c`
- `src/gui_users.c`
- `src/gui_auth.c`
- CI workflows, release scripts, and persistence formats

## Requirements And Traceability Impact

Higher-level requirements already cover the bounded intent:

- `SYS-008` Patient Demographic Storage
- `SYS-011` Patient Status Summary Display
- `SYS-014` Graphical Vital Signs Dashboard

Authentication/session boundaries remain relevant through existing
`SYS-013` / `SWR-GUI-002`, because patient identity must not be shown before
authentication or after logout.

Implementation should add one explicit GUI requirement entry, expected to be
the next available GUI identifier (`SWR-GUI-014`), to define:

- header identity card presence in the authenticated dashboard
- displayed fields: name, ID, age
- empty-state behavior when no patient is active
- read-only/non-interactive scope
- synchronization expectations across Admit/Refresh, scenario changes,
  Add Reading, timer updates, Clear Session, mode changes, and logout

`requirements/TRACEABILITY.md` should then map the new SWR to:

- existing SYS links (`SYS-008`, `SYS-011`, `SYS-014`)
- implementation in `src/gui_main.c`
- manual GUI verification evidence for state transitions

`requirements/SYS.md` is not expected to change for this MVP unless
implementation-time review finds that the existing SYS wording is insufficient
to support the new SWR. If that happens, stop and get human review before
folding a broader SYS refactor into this issue.

## Medical-Safety, Security, And Privacy Impact

Medical safety:

- Positive intent: faster patient-context confirmation before acting on vitals,
  alerts, or trends.
- Main hazard: stale or divergent identity between header and patient bar could
  increase wrong-patient confusion.
- Required control: single-source rendering from current session state with
  explicit empty-state handling on every relevant transition.
- No clinical behavior change is allowed. Vital classification, NEWS2,
  alarming, and treatment guidance remain untouched.

Security:

- No new authentication, authorization, or persistence path is introduced.
- Patient identity remains visible only inside the authenticated dashboard.
- Logout must clear session state exactly as today, with no residual header
  identity.

Privacy:

- Scope stays limited to the already stored fields required for orientation:
  name, ID, and age.
- Do not add DOB, address, or any extra PHI.
- Do not write patient identity to config, logs, telemetry, or any new export
  surface.

## AI/ML Impact Assessment

This change does not add, change, remove, or depend on an AI-enabled device
software function.

- Model purpose: not applicable
- Input data: not applicable beyond existing non-AI patient demographics
- Output: not applicable
- Human-in-the-loop limits: not applicable
- Transparency needs: not applicable beyond standard UI clarity
- Dataset and bias considerations: not applicable
- Monitoring expectations: not applicable
- PCCP impact: none

## Validation Plan

Automated regression baseline:

```powershell
cmake --build build
build/tests/test_unit.exe
build/tests/test_integration.exe
```

Targeted manual GUI verification:

1. Log in and confirm the dashboard shows an empty patient card before any
   patient is active.
2. Admit patient A and confirm the header card and patient bar show the same
   name, ID, and age.
3. Add readings and allow timer-driven simulation updates; confirm the card
   remains stable and synchronized with the patient bar.
4. Run both demo scenarios and confirm the header updates to the correct
   patient each time.
5. Use Clear Session and confirm the header returns immediately to the empty
   state with no stale demographics.
6. Toggle simulation/device mode and confirm no patient identity remains
   visible when the session has been cleared.
7. Log out and confirm patient identity disappears with the dashboard.
8. If new strings are introduced, switch languages and confirm the header label
   and empty-state text follow the localization pattern and do not clip or
   overlap controls at minimum window width.
9. Confirm no new files, config values, logs, or network paths are introduced.

Recommended targeted repo checks during implementation:

```powershell
git diff --name-only
rg -n "SWR-GUI-014|active patient|No active patient" requirements src include
```

Expected validation outcome:

- changed files stay limited to the intended GUI/localization/requirements
  surface
- header and patient bar never disagree on active-patient identity
- no stale identity survives Clear Session, device mode, or logout
- no clinical logic or persistence behavior changes

## Rollback Or Failure Handling

- If the card cannot fit in the current header at minimum width without
  obscuring logout/settings/simulation controls or the role badge, stop and
  split the issue rather than silently redesigning the full header.
- If implementation requires new patient-selection workflow, new persistent
  session state, or additional identifiers, stop because that exceeds the MVP
  safety boundary.
- If requirement review shows the current SYS layer is too weak to support the
  new GUI requirement, pause and get human review before expanding scope into
  a broader requirements rewrite.
- Rollback is straightforward: revert the header-card, localization, and
  requirement-document changes together so the dashboard returns to its prior
  patient-bar-only identity behavior.
